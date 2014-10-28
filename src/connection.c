#include "connection.h"

void return_upstream_connection(uv_tcp_t* stream,
                                upstream_connection **pool)
{
	upstream_connection *new = malloc(sizeof(*new));

	// Add to the front of the list
	new->previous = *pool;
	new->stream = stream;
	*pool = new;
}


uv_tcp_t* upstream_from_pool(upstream_connection **pool)
{
	uv_tcp_t *ret;

	if(!(*pool))
		return NULL;
	else
	{
		ret = (*pool)->stream;
		*pool = (*pool)->previous;
		return ret;
	}
}


void free_conn_pool(upstream_connection *pool)
{
	upstream_connection *conn;
	while(pool)
	{
		conn = pool->previous;
		uv_close((uv_handle_t*)pool->stream, free_handle);
		free(pool);
		pool = conn;
	}
}


void free_handle(uv_handle_t *handle)
{
	free(handle);
}

void upstream_disconnected(upstream_connection **pool, uv_tcp_t* connection)
{
	upstream_connection *previous = NULL;
	upstream_connection *i = *pool;

	while(i)
	{
		if(i->stream == connection)
		{
			if(previous)
			{
				// If we're removing a list element not at the top, remove
				//  this element and set the one before to point where this one
				//  used to
				previous->previous = i->previous;
				free(i);
			}
			else
			{
				// If this is at the top of the queue we just pop it off moving
				//  the next one up to the top
				*pool = i->previous;
				free(i);
			}
			return;
		}
		previous = i;
		i = i->previous;
	}
}


int resolve_address(uv_loop_t *loop, char *address, resolve_callback *callback)
{
	int nodelen, servicelen;
	char *node_start, *node_end, *service_start;
	uv_getaddrinfo_t *lookup_req;

	if(!strncasecmp("tcp://", address, 6))
	{
		// Search for port separator ':'
		node_start = address + 6;
		node_end = strchr(node_start, ':');
		if(!node_end)
			return 0;

		// Determine length of node name and allocate
		nodelen = node_end - node_start;
		callback->node = malloc(nodelen + 1);
		memcpy(callback->node, node_start, nodelen);
		callback->node[nodelen] = '\0';

		// Determine length of service name and allocate
		service_start = node_end + 1;
		servicelen = strlen(service_start);
		callback->service = malloc(servicelen + 1);
		memcpy(callback->service, service_start, servicelen);
		callback->service[servicelen] = '\0';

		// Build the libuv getaddrinfo request
		lookup_req = malloc(sizeof(*lookup_req));
		lookup_req->data = callback;
		uv_getaddrinfo(loop, lookup_req, resolve_address_cb, callback->node,
		               callback->service, NULL);
	}
	else if(!strncasecmp("unix://", address, 7))
	{
		node_start = address + 7;
		nodelen = strlen(node_start);
		// TODO actual unix socket support
		callback->node = malloc(nodelen + 1);
		memcpy(callback->node, node_start, nodelen);
		callback->node[nodelen] = '\0';

		callback->callback(callback, loop, NULL);
		free(callback->node);
		free(callback);
	}
	else
	{
		log_message(LOG_ERROR, "Unknown socket type for address %s\n",
		            address);
		return 0;
	}
	return 1;
}


void resolve_address_cb(uv_getaddrinfo_t *req, int status,
                        struct addrinfo *res)
{
	resolve_callback *callback = req->data;

	if(status < 0)
	{
		log_message(LOG_ERROR, "Address resolution error: %s\n",
		            uv_err_name(status));
		return;
	}

	callback->callback(callback, req->loop, res);
	free(callback->service);
	free(callback->node);
	free(callback);
	free(req);
	uv_freeaddrinfo(res);
}


void bind_on_and_listen(resolve_callback *callback, uv_loop_t *loop,
                        struct addrinfo *res)
{
	int ret;
	bind_and_listen_data *data = callback->data;
	proxy_config *proxy_cfg = data->accept_cb->config;

	// If res is not NULL this is a TCP socket
	if(res)
	{
		// Initialize listening tcp socket
		proxy_cfg->listener = calloc(1, sizeof(uv_tcp_t));
		ret = uv_tcp_init(loop, (uv_tcp_t*)proxy_cfg->listener);
		if(ret)
		{
			log_message(LOG_ERROR, "Failed to initialize tcp socket: %s\n",
				        uv_err_name(ret));
			return;
		}

		// Bind address
		ret = uv_tcp_bind((uv_tcp_t*)proxy_cfg->listener, res->ai_addr, 0);
		if(ret)
		{
			log_message(LOG_ERROR, "Failed to bind: %s\n", uv_err_name(ret));
			return;
		}
	}
	else
	{
		// Initialize listening UNIX/PIPE otherwise
		proxy_cfg->listener = calloc(1, sizeof(uv_pipe_t));
		ret = uv_pipe_init(loop, (uv_pipe_t*)proxy_cfg->listener, 1);
		if(ret)
		{
			log_message(LOG_ERROR, "Failed to initialize tcp socket: %s\n",
				        uv_err_name(ret));
			return;
		}

		// Bind address
		ret = uv_pipe_bind((uv_pipe_t*)proxy_cfg->listener, callback->node);
		if(ret)
		{
			log_message(LOG_ERROR, "Failed to bind: %s\n", uv_err_name(ret));
			return;
		}
	}

	// Begin listening
	proxy_cfg->listener->data = data->accept_cb;
	ret = uv_listen(proxy_cfg->listener, data->backlog_size,
	                proxy_accept_client);
	if(ret)
	{
		log_message(LOG_ERROR, "Listen error: %s\n", uv_err_name(ret));
		return;
	}
	free(data);
}


void proxy_accept_client(uv_stream_t *listener, int status)
{
	int ret;
	proxy_client *new;
	accept_callback *callback;

	log_message(LOG_INFO, "New client connection\n");

	if(status)
	{
		log_message(LOG_ERROR, "Error accepting new client\n");
		return;
	}

	// Allocate our client structure
	new = malloc(sizeof(*new));

	// Set reference to listener stream, config and data pointer
	new->server = listener;
	callback = listener->data;
	new->proxy_settings = callback->config;

	// Initialize downstream connection
	new->downstream = malloc(sizeof(*new->downstream));
	ret = uv_tcp_init(listener->loop, new->downstream);
	if(ret)
	{
		log_message(LOG_ERROR, "Failed initializing new client socket\n");
		return;
	}

	// Accept connection
	new->downstream->data = new;
	ret = uv_accept(new->server, (uv_stream_t*)new->downstream);
	if(ret)
	{
		log_message(LOG_ERROR, "Failed to accept new client socket\n");
		return;
	}

	// Run callback
	callback->callback(new, listener);
}


void proxy_new_client(proxy_client *new, uv_stream_t *listener)
{
	int ret;
	proxy_config *config = new->proxy_settings;

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
	                     proxy_new_upstream);
	if(ret)
	{
		log_message(LOG_ERROR, "Upstream tcp connection error\n");
		return;
	}
}


void proxy_new_upstream(uv_connect_t* conn, int status)
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
	                    alloc_from_pool, proxy_client_read);
	if(ret)
	{
		log_message(LOG_ERROR, "Failed to start client read handling\n");
		return;
	}

	ret = uv_read_start((uv_stream_t*) client->upstream,
	                    alloc_from_pool, proxy_upstream_read);
	if(ret)
	{
		log_message(LOG_ERROR, "Failed to start upstream read handling\n");
		return;
	}
}


void proxy_client_read(uv_stream_t *inbound, ssize_t readlen,
                       const uv_buf_t *buffer)
{
	proxy_client *client = inbound->data;
	proxy_config *config = client->proxy_settings;

	log_message(LOG_DEBUG, "Client read event\n");

	if(readlen < 0)
	{
		log_message(LOG_INFO, "Client disconnected\n");

		// Free associated buffer and handle
		if(buffer->base)
		{
			return_alloc_to_pool(buffer->base);
		}
		uv_close((uv_handle_t*)inbound, free_handle);
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
		// Normal, good read event, pass it down to the associated hook
		config->client_read_event(inbound, (uv_stream_t*)client->upstream,
		                          buffer->base, readlen);
	}
}

void proxy_stream_relay(uv_stream_t *inbound, uv_stream_t *outbound,
                        char *buffer, ssize_t buflen)
{
	uv_buf_t *response;
	uv_write_t *req;

	response = malloc(sizeof(*response));
	response->base = buffer;
	response->len = buflen;

	req = calloc(1, sizeof(*req));
	req->data = response;

	// Send to buffer from inbound handle to outbound handle
	uv_write(req, outbound, response, 1, free_request);
}


void proxy_upstream_read(uv_stream_t *outbound, ssize_t readlen,
                         const uv_buf_t *buffer)
{
	uv_buf_t *response;
	uv_write_t *req;
	proxy_client *client = outbound->data;

	if(readlen < 0)
	{
		log_message(LOG_DEBUG, "Upstream disconnected\n");
		// Close client and upstream handles and free structures
		uv_close((uv_handle_t*)outbound, free_handle);
		if(client->downstream)
			uv_close((uv_handle_t*)client->downstream, free_handle);
		else
			upstream_disconnected(NULL,//&(config->pool),
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
