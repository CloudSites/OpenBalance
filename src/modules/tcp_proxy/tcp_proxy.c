#include "tcp_proxy.h"

memory_allocation *free_memory_list;
upstream_connection *free_connection_list;


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
	
	module_config->listener = calloc(1, sizeof(*module_config->listener));
	
	module_config->listen_host = get_config_string(config, "listen_host",
	                                               "127.0.0.1", 256);
	if(!module_config->listen_host)
		return MOD_ERROR;
	
	module_config->listen_port = get_config_int(config, "listen_port", 6380);
	if(!module_config->listen_port)
		return MOD_ERROR;
	
	module_config->upstream_host = get_config_string(config, "upstream_host",
	                                               "127.0.0.1", 256);
	if(!module_config->upstream_host)
		return MOD_ERROR;
	
	module_config->upstream_port = get_config_int(config, "upstream_port",
	                                              6379);
	if(module_config->upstream_port < 1
	   || module_config->upstream_port > 65535)
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


handler_response tcp_proxy_startup(void *config, ob_module *module)
{
	int ret;
	tcp_proxy_config *cfg = config;
	struct sockaddr_in bind_addr;
	
	log_message(LOG_INFO, "tcp proxy starting!\n");
	// Initialize free lists
	free_memory_list = NULL;
	free_connection_list = NULL;
	
	// Resolve listen address
	ret = uv_ip4_addr(cfg->listen_host, cfg->listen_port, &bind_addr);
	if(ret)
	{
		log_message(LOG_ERROR, "Listen socket resolution error: %s\n",
		            uv_err_name(ret));
		return MOD_ERROR;
	}
	
	// Resolve upstream address
	cfg->upstream_addr = malloc(sizeof(*cfg->upstream_addr));
	ret = uv_ip4_addr(cfg->upstream_host, cfg->upstream_port,
	                  cfg->upstream_addr);
	if(ret)
	{
		log_message(LOG_ERROR, "Upstream socket resolution error: %s\n",
		            uv_err_name(ret));
		return MOD_ERROR;
	}
	
	// Initialize listening tcp socket
	ret = uv_tcp_init(event_loop, cfg->listener);
	if(ret)
	{
		log_message(LOG_ERROR, "Failed to initialize tcp socket: %s\n",
		            uv_err_name(ret));
		return MOD_ERROR;
	}
	cfg->listener->data = cfg;
	
	// Bind address
	ret = uv_tcp_bind(cfg->listener, (struct sockaddr*)&bind_addr, 0);
	if(ret)
	{
		log_message(LOG_ERROR, "Failed to bind on %s:%d: %s\n",
		            cfg->listen_host, cfg->listen_port,uv_err_name(ret));
		return MOD_ERROR;
	}
	
	// Begin listening
	ret = uv_listen((uv_stream_t*) cfg->listener, cfg->backlog_size,
	                tcp_proxy_new_client);
	if(ret)
	{
		log_message(LOG_ERROR, "Listen error: %s\n", uv_err_name(ret));
		return MOD_ERROR;
	}
	
	return MOD_OK;
}


handler_response tcp_proxy_cleanup(void *config)
{
	memory_allocation *ptr;
	upstream_connection *conn;
	tcp_proxy_config *module_config = config;
	uv_loop_t *main_loop = module_config->listener->loop;
	
	log_message(LOG_INFO, "Cleaning up tcp_proxy\n");
	
	// Free all memory allocations and upstream connections
	while(free_memory_list)
	{
		ptr = free_memory_list->previous;
		free(free_memory_list->allocation);
		free(free_memory_list);
		free_memory_list = ptr;
	}
	while(free_connection_list)
	{
		conn = free_connection_list->previous;
		uv_close((uv_handle_t*)free_connection_list->stream,
		         tcp_proxy_free_handle);
		free(free_connection_list);
		free_connection_list = conn;
	}
	// Free handles from configuration structure
	free(module_config->listen_host);
	free(module_config->upstream_host);
	uv_close((uv_handle_t*)module_config->listener, NULL);
	
	// Drain the loop
	while(uv_run(main_loop, UV_RUN_NOWAIT));
	
	// Free remainder of configuration structure
	free(module_config->upstream_addr);
	free(module_config->listener);
	free(module_config);
	return MOD_OK;
}


void tcp_proxy_new_client(uv_stream_t *listener, int status)
{
	int ret;
	tcp_proxy_client *new;
	
	log_message(LOG_INFO, "New client connection\n");
	
	if(status)
	{
		log_message(LOG_ERROR, "Error accepting new client\n");
		return;
	}
	
	// Allocate our client structure
	new = malloc(sizeof(*new));
	// Set reference to config structure
	new->config = listener->data;
	// Set reference to listener stream
	new->server = listener;
	
	// Initialize and accept connection
	new->downstream = malloc(sizeof(*new->downstream));
	ret = uv_tcp_init(new->server->loop, new->downstream);
	if(ret)
	{
		log_message(LOG_ERROR, "Failed initializing new client socket\n");
		return;
	}
	new->downstream->data = new;
	ret = uv_accept(new->server, (uv_stream_t*)new->downstream);
	if(ret)
	{
		log_message(LOG_ERROR, "Failed to accept new client socket\n");
		return;
	}
	
	// Recycle open connection if available
	if(free_connection_list)
	{
		new->upstream = free_connection_list->stream;
		free_connection_list = free_connection_list->previous;
		new->upstream->data = new;
		ret = uv_read_start((uv_stream_t*) new->downstream,
		                    tcp_proxy_read_alloc, tcp_proxy_client_read);
		if(ret)
		{
			log_message(LOG_ERROR, "Failed to start client read handling\n");
			return;
		}
	
		ret = uv_read_start((uv_stream_t*) new->upstream,
		                    tcp_proxy_read_alloc, tcp_proxy_upstream_read);
		if(ret)
		{
			log_message(LOG_ERROR, "Failed to start upstream read handling\n");
			return;
		}
	}
	else
	{
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
		                     (struct sockaddr *)new->config->upstream_addr,
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
	tcp_proxy_client *client = conn->data;
	free(client->connection);
	
	log_message(LOG_INFO, "New upstream\n");
	
	if(status)
	{
		log_message(LOG_ERROR, "Failed handling new upstream connection\n");
		// Accept and shut down the connection
		uv_accept(client->server, (uv_stream_t*)client->downstream);
		uv_close((uv_handle_t*)client->downstream, tcp_proxy_free_handle);
		// Cleanup the rest of the connection
		uv_close((uv_handle_t*)client->upstream, tcp_proxy_free_handle);
		free(client);
		return;
	}
	
	ret = uv_read_start((uv_stream_t*) client->downstream,
	                    tcp_proxy_read_alloc, tcp_proxy_client_read);
	if(ret)
	{
		log_message(LOG_ERROR, "Failed to start client read handling\n");
		return;
	}
	
	ret = uv_read_start((uv_stream_t*) client->upstream,
	                    tcp_proxy_read_alloc, tcp_proxy_upstream_read);
	if(ret)
	{
		log_message(LOG_ERROR, "Failed to start upstream read handling\n");
		return;
	}
}


void tcp_proxy_free_handle(uv_handle_t *handle)
{
	free(handle);
}


void tcp_proxy_read_alloc(uv_handle_t *handle, size_t suggested_size,
                            uv_buf_t *buffer)
{
	memory_allocation *ptr;
	if(free_memory_list)
	{
		buffer->base = free_memory_list->allocation;
		ptr = free_memory_list->previous;
		free(free_memory_list);
		free_memory_list = ptr;
	}
	else
	{
		buffer->base = malloc(suggested_size);
	}
	buffer->len = suggested_size;
}


void tcp_proxy_client_read(uv_stream_t *inbound, ssize_t readlen,
                             const uv_buf_t *buffer)
{
	uv_buf_t *response;
	uv_write_t *req;
	tcp_proxy_client *client = inbound->data;
	
	log_message(LOG_DEBUG, "Client read event\n");
	
	if(readlen < 0)
	{
		if(buffer->base)
		{
			tcp_proxy_return_allocation(buffer->base);
		}
		uv_close((uv_handle_t*)inbound, tcp_proxy_free_handle);
		log_message(LOG_INFO, "Client disconnected\n");
		tcp_proxy_return_upstream_connection(client->upstream);
		//uv_close((uv_handle_t*)client->upstream, tcp_proxy_free_handle);
		free(client);
		//uv_stop(inbound->loop);
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
		// Send to upstream connection
		uv_write(req, (uv_stream_t *)client->upstream, response, 1,
		         tcp_proxy_free_request);
	}
}


void tcp_proxy_free_request(uv_write_t *req, int status)
{
	uv_buf_t *ptr = req->data;
	
	// Save the read buffer for reuse, free the rest
	tcp_proxy_return_allocation(ptr->base);
	free(ptr);
	free(req);
}


void tcp_proxy_upstream_read(uv_stream_t *inbound, ssize_t readlen,
                             const uv_buf_t *buffer)
{
	uv_buf_t *response;
	uv_write_t *req;
	tcp_proxy_client *client = inbound->data;
	
	if(readlen < 0)
	{
		log_message(LOG_DEBUG, "Upstream disconnected\n");
		uv_close((uv_handle_t*)inbound, tcp_proxy_free_handle);
		uv_close((uv_handle_t*)client->downstream, tcp_proxy_free_handle);
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
	         tcp_proxy_free_request);
}

void tcp_proxy_return_allocation(void *allocation)
{
	memory_allocation *new;
	
	// Allocate memory tracking linked list element
	new = malloc(sizeof(*new));
	
	// Add to the front of the list
	new->previous = free_memory_list;
	new->allocation = allocation;
	free_memory_list = new;
}

void tcp_proxy_return_upstream_connection(uv_tcp_t* stream)
{
	upstream_connection *new;
	tcp_proxy_client *client = stream->data;
	
	if(client->config->connection_pooling)
	{
		// Allocate upstream linked list element
		new = malloc(sizeof(*new));
	
		// Add to the front of the list
		new->previous = free_connection_list;
		new->stream = stream;
		free_connection_list = new;
	}
	else
	{
		uv_close((uv_handle_t*)client->upstream, tcp_proxy_free_handle);
	}
}
