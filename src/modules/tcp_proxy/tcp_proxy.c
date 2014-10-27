#include "tcp_proxy.h"


handler_response tcp_proxy_configure(json_t* config, void **conf_struct)
{
	tcp_proxy_config *module_config;

	// Error for non-object
	if(!json_is_object(config))
	{
		log_message(LOG_ERROR,
		            "tcp_proxy configuration needs to be a JSON object\n");
		return MOD_ERROR;
	}

	// Allocate and start filling out configuration structure
	module_config = malloc(sizeof(*module_config));

	module_config->pool = NULL;
	module_config->accept_cb = malloc(sizeof(*module_config->accept_cb));
	module_config->accept_cb->callback = tcp_proxy_new_client;
	module_config->listener = calloc(1, sizeof(*module_config->listener));

	module_config->listen_addr = get_config_string(config, "listen_addr",
	                                               "tcp://localhost:8080",
	                                               300);
	if(!module_config->listen_addr)
		return MOD_ERROR;

	module_config->upstream_host = get_config_string(config, "upstream_host",
	                                               "127.0.0.1", 256);
	if(!module_config->upstream_host)
		return MOD_ERROR;

	module_config->upstream_port = get_config_int(config, "upstream_port",
	                                              6379);
	if(module_config->upstream_port < 1
	   || module_config->upstream_port >= 65535)
		return MOD_ERROR;

	module_config->backlog_size = get_config_int(config, "backlog_size", 128);
	if(module_config->upstream_port < 1)
		return MOD_ERROR;

	module_config->connection_pooling = get_config_boolean(config,
	                                                      "connection_pooling",
	                                                       0);

	// Set configuration structure
	(*conf_struct) = module_config;

	return MOD_OK;
}


handler_response tcp_proxy_startup(void *config, uv_loop_t *master_loop)
{
	int ret;
	tcp_proxy_config *cfg = config;
	accept_callback *accept_cb;
	resolve_callback *resolve_cb;
	bind_and_listen_data *bnl_data;

	log_message(LOG_INFO, "tcp proxy starting!\n");

	// Resolve upstream address
	cfg->upstream_sockaddr = malloc(sizeof(*cfg->upstream_sockaddr));
	ret = uv_ip4_addr(cfg->upstream_host, cfg->upstream_port,
	                  cfg->upstream_sockaddr);
	if(ret)
	{
		log_message(LOG_ERROR, "Upstream socket resolution error: %s\n",
		            uv_err_name(ret));
		return MOD_ERROR;
	}

	// Setup address resolution callback
	resolve_cb = malloc(sizeof(*resolve_cb));
	resolve_cb->callback = bind_on_and_listen;

	// Setup bind and listen parameters
	bnl_data = malloc(sizeof(*bnl_data));
	bnl_data->backlog_size = cfg->backlog_size;
	bnl_data->listener = cfg->listener;
	resolve_cb->data = bnl_data;

	// Setup connection acceptor callback
	accept_cb = cfg->accept_cb;//malloc(sizeof(*accept_cb));
	accept_cb->data = cfg;
	bnl_data->accept_cb = accept_cb;

	// Put it all in motion
	resolve_address(master_loop, cfg->listen_addr, resolve_cb);

	return MOD_OK;
}


handler_response tcp_proxy_cleanup(void *config)
{
	tcp_proxy_config *module_config = config;
	uv_loop_t *main_loop = module_config->listener->loop;

	log_message(LOG_INFO, "Cleaning up tcp_proxy\n");

	// Free all upstream connections
	if(module_config->pool)
		free_conn_pool(module_config->pool);

	// Free handles from configuration structure
	free(module_config->listen_addr);
	free(module_config->upstream_host);
	uv_close((uv_handle_t*)module_config->listener, NULL);

	// Drain the loop
	while(uv_run(main_loop, UV_RUN_NOWAIT));

	// Free remainder of configuration structure
	free(module_config->upstream_sockaddr);
	free(module_config->listener);
	if(module_config->accept_cb)
		free(module_config->accept_cb);
	free(module_config);
	return MOD_OK;
}


void tcp_proxy_new_client(proxy_client *new, uv_stream_t *listener)
{
	int ret;
	tcp_proxy_config *config = new->data;

	// Recycle open upstream connection if available
	if((new->upstream = upstream_from_pool(&config->pool)))
	{
		// Set read event handlers
		new->downstream->data = new;
		ret = uv_read_start((uv_stream_t*) new->downstream,
		                    alloc_from_pool, tcp_proxy_client_read);
		if(ret)
		{
			log_message(LOG_ERROR, "Failed to start client read handling\n");
			return;
		}

		new->upstream->data = new;
		ret = uv_read_start((uv_stream_t*) new->upstream,
		                    alloc_from_pool, tcp_proxy_upstream_read);
		if(ret)
		{
			log_message(LOG_ERROR, "Failed to start upstream read handling\n");
			return;
		}
	}
	else
	{
		// Start a new upstream connection if one was not available in pool
		new->connection = malloc(sizeof(*new->connection));
		new->upstream = malloc(sizeof(*new->upstream));
		ret = uv_tcp_init(listener->loop, new->upstream);
		if(ret)
		{
			log_message(LOG_ERROR, "Error initializing upstream tcp socket\n");
			return;
		}

		new->connection->data = new;
		new->upstream->data = new;

		ret = uv_tcp_connect(new->connection, new->upstream,
		                     (struct sockaddr *)config->upstream_sockaddr,
		                     tcp_proxy_new_upstream);
		if(ret)
		{
			log_message(LOG_ERROR, "Upstream tcp connection error\n");
			return;
		}
	}
}


void tcp_proxy_new_upstream(uv_connect_t* conn, int status)
{
	int ret;
	proxy_client *client = conn->data;
	free(client->connection);

	log_message(LOG_INFO, "New upstream\n");

	if(status)
	{
		log_message(LOG_ERROR, "Failed handling new upstream connection\n");
		// Accept and shut down the connection
		uv_close((uv_handle_t*)client->downstream, free_handle);
		// Cleanup the rest of the connection
		uv_close((uv_handle_t*)client->upstream, free_handle);
		free(client);
		return;
	}

	// Set read event handlers
	ret = uv_read_start((uv_stream_t*) client->downstream,
	                    alloc_from_pool, tcp_proxy_client_read);
	if(ret)
	{
		log_message(LOG_ERROR, "Failed to start client read handling\n");
		return;
	}

	ret = uv_read_start((uv_stream_t*) client->upstream,
	                    alloc_from_pool, tcp_proxy_upstream_read);
	if(ret)
	{
		log_message(LOG_ERROR, "Failed to start upstream read handling\n");
		return;
	}
}


void tcp_proxy_client_read(uv_stream_t *inbound, ssize_t readlen,
                             const uv_buf_t *buffer)
{
	uv_buf_t *response;
	uv_write_t *req;
	proxy_client *client = inbound->data;
	tcp_proxy_config *config = client->data;

	log_message(LOG_DEBUG, "Client read event\n");

	if(readlen < 0)
	{
		log_message(LOG_INFO, "Client disconnected\n");

		// Free associated buffer and handle
		if(buffer->base)
		{
			return_alloc_to_pool(buffer->base);
		}
		uv_close((uv_handle_t*)inbound, free_handle_and_client);

		// Return connection to pool or close based on settings
		if(config->connection_pooling)
		{
			return_upstream_connection(client->upstream,
			                           &(config->pool));
			client->downstream = NULL;
		}
		else
			uv_close((uv_handle_t*)client->upstream, free_handle_and_client);

		//uv_stop(inbound->loop); // FOR TESTING
		return;
	}
	else if(readlen == 0)
	{
		free(buffer->base);
		return;
	}
	else
	{
		response = malloc(sizeof(*response));
		response->base = buffer->base;
		response->len = readlen;

		req = calloc(1, sizeof(*req));
		req->data = response;
		// Send to associated upstream connection
		uv_write(req, (uv_stream_t *)client->upstream, response, 1,
		         free_request);
	}
}


void tcp_proxy_upstream_read(uv_stream_t *outbound, ssize_t readlen,
                             const uv_buf_t *buffer)
{
	uv_buf_t *response;
	uv_write_t *req;
	proxy_client *client = outbound->data;
	tcp_proxy_config *config = client->data;

	if(readlen < 0)
	{
		log_message(LOG_DEBUG, "Upstream disconnected\n");
		// Close client and upstream handles and free structures
		uv_close((uv_handle_t*)outbound, free_handle);
		if(client->downstream)
			uv_close((uv_handle_t*)client->downstream, free_handle);
		else
			upstream_disconnected(&(config->pool),
			                      (uv_tcp_t*) outbound);
		free(buffer->base);
		free(client);
		return;
	}

	response = malloc(sizeof(*response));
	response->base = buffer->base;
	response->len = readlen;

	req = calloc(1, sizeof(*req));
	req->data = response;
	// Send to upstream connection
	uv_write(req, (uv_stream_t *)client->downstream, response, 1,
	         free_request);
}

void free_handle_and_client(uv_handle_t *handle)
{
	proxy_client *client = handle->data;
	free(handle);
	free(client);
}
