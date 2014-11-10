#include "redis_proxy.h"


char *redis_command_strings[] = {"PING\r\n", "SET", "GET"};


handler_response redis_proxy_configure(json_t* config, void **conf_struct)
{
	redis_proxy_config *module_config;

	// Error for non-object
	if(!json_is_object(config))
	{
		log_message(LOG_ERROR,
		            "redis_proxy configuration needs to be a JSON object\n");
		return MOD_ERROR;
	}

	// Allocate and start filling out configuration structure
	module_config = malloc(sizeof(*module_config));

	module_config->accept_cb = malloc(sizeof(*module_config->accept_cb));
	module_config->accept_cb->callback = proxy_new_client;
	module_config->proxy_settings = malloc(sizeof(*module_config->proxy_settings));
	module_config->proxy_settings->client_read_event = read_request;
	module_config->proxy_settings->upstream_read_event = proxy_stream_relay;

	module_config->listen_addr = get_config_string(config, "listen_addr",
	                                               "tcp://127.0.0.1:6380",
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

	// Set configuration structure
	(*conf_struct) = module_config;

	return MOD_OK;
}


handler_response redis_proxy_startup(void *config, uv_loop_t *master_loop)
{
	int ret;
	redis_proxy_config *cfg = config;
	proxy_config *proxy_cfg = cfg->proxy_settings;
	accept_callback *accept_cb;
	resolve_callback *resolve_cb;
	bind_and_listen_data *bnl_data;

	log_message(LOG_INFO, "redis_proxy starting!\n");

	// Resolve upstream address
	proxy_cfg->loop = master_loop;
	proxy_cfg->upstream_sockaddr = malloc(sizeof(*proxy_cfg->upstream_sockaddr));
	ret = uv_ip4_addr(cfg->upstream_host, cfg->upstream_port,
	                  proxy_cfg->upstream_sockaddr);
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
	resolve_cb->data = bnl_data;

	// Setup connection acceptor callback
	accept_cb = cfg->accept_cb;
	accept_cb->config = proxy_cfg;
	bnl_data->accept_cb = accept_cb;

	// Put it all in motion
	resolve_address(master_loop, cfg->listen_addr, resolve_cb);

	return MOD_OK;
}


handler_response redis_proxy_cleanup(void *config)
{
	redis_proxy_config *module_config = config;
	uv_loop_t *main_loop = module_config->proxy_settings->loop;

	log_message(LOG_INFO, "Cleaning up redis_proxy\n");

	// Free handles from configuration structure
	free(module_config->listen_addr);
	free(module_config->upstream_host);
	uv_close((uv_handle_t*)module_config->proxy_settings->listener, NULL);

	// Drain the loop
	while(uv_run(main_loop, UV_RUN_NOWAIT));

	// Free remainder of configuration structure
	free(module_config->proxy_settings->upstream_sockaddr);
	free(module_config->proxy_settings->listener);
	free(module_config->proxy_settings);
	if(module_config->accept_cb)
		free(module_config->accept_cb);
	free(module_config);
	return MOD_OK;
}


void free_redis_request(uv_write_t *req, int status)
{
	uv_buf_t *ptr = req->data;

	// Save the read buffer for reuse, free the rest
	return_alloc_to_pool(ptr->base);
	free(ptr);
	free(req);
}


void read_request(uv_stream_t *client, uv_stream_t *server, char *buffer,
                  ssize_t len)
{
	proxy_client *pxy_client = client->data;
	redis_request *client_request = pxy_client->data;
	char *carraige_return;
	buffer_chain *last_buffer;

	if(client_request)
	{
		if(client_request->type == INLINE_REQUEST)
		{
			last_buffer = client_request->buffer;

			while(last_buffer->next)
			{
				last_buffer = last_buffer->next;
			}

			// If our last input was a carraige return, and we have a newline
			if(last_buffer->buffer[last_buffer->len - 1] == '\r'
			   && buffer[0] == '\n')
			{
				last_buffer->next = malloc(sizeof(*last_buffer));
				last_buffer->next->next = NULL;
				last_buffer->next->len = len;

				// If this is all we have we have a complete request
				if(len == 1)
				{
					last_buffer->next->free_type = POOLED;
					last_buffer->next->buffer = buffer;
					parse_request(client_request);
					return;
				}
				// or else we have extra data too
				else
				{
					last_buffer->next->free_type = STATIC;
					last_buffer->next->buffer = "\n";
					parse_request(client_request);
					printf("STILL MORE UNHANDLED SHTUFF!\n");
				}
				
			}

			carraige_return = memchr(buffer, '\r', len);
			// If still incomplete
			if(!carraige_return || carraige_return == buffer + len - 1)
			{
				last_buffer->next = malloc(sizeof(*last_buffer));
				last_buffer->next->buffer = buffer;
				last_buffer->next->len = len;
				last_buffer->next->next = NULL;
				last_buffer->next->free_type = POOLED;
				printf("still incomplete buffer\n");
				return;
			}

			if(carraige_return[1] == '\n')
			{
				// If the remainder of this command exists within this buffer
				if(carraige_return == buffer + len - 2)
				{
					printf("yay! end of a command\n");
					last_buffer->next = malloc(sizeof(*client_request->buffer));
					last_buffer->next->buffer = buffer;
					last_buffer->next->len = len;
					last_buffer->next->next = NULL;
					last_buffer->next->free_type = POOLED;
					parse_request(client_request);
					return;
				}
				else
				{
					printf("got the end of a command, more in the buffer\n");
					return;
				}
				
			}
		}
		else
		{
			printf("continue reading serial...\n");
		}
	}
	else
	{
		client_request = malloc(sizeof(*client_request));
		pxy_client->data = client_request;
		client_request->links_in_buffer_chain = 1;
		client_request->command = REDIS_UNSET;
		client_request->client = client;
		client_request->server = server;

		if(buffer[0] == '*')
		{
			client_request->type = SERIALIZED_REQUEST;
		}
		else
		{
			client_request->type = INLINE_REQUEST;
		}
		
		if(client_request->type == INLINE_REQUEST)
		{
			carraige_return = memchr(buffer, '\r', len);
			if(!carraige_return || carraige_return == buffer + len - 1)
			{
				client_request->buffer = malloc(sizeof(*client_request->buffer));
				client_request->buffer->buffer = buffer;
				client_request->buffer->len = len;
				client_request->buffer->next = NULL;
				client_request->buffer->free_type = POOLED;
				printf("incomplete inline buffer\n");
				return;
			}

			if(carraige_return[1] == '\n')
			{
				// If the full command exists within this one buffer
				if(carraige_return == buffer + len - 2)
				{
					printf("yay! 1 solid command\n");
					client_request->buffer = malloc(sizeof(*client_request->buffer));
					client_request->buffer->buffer = buffer;
					client_request->buffer->offset = 0;
					client_request->buffer->len = len;
					client_request->buffer->next = NULL;
					client_request->buffer->free_type = POOLED;
					parse_request(client_request);
					return;
				}
				else
				{
					printf("got a full command, with more in the buffer\n");
					return;
				}
				
			}
			
			printf("cr not followed by lf\n");
		}
		else
		{
			printf("no support for cereal :(\n");
		}
	}
}


void parse_request(redis_request *request)
{
	int command, buffer_count, buf_i;
	size_t len;
	char *command_string;
	buffer_chain *i;
	uv_buf_t *req_buffer;
	uv_write_t *req;

	if(request->type == INLINE_REQUEST)
	{
		for(command = REDIS_PING; command < REDIS_COMMAND_LEN; command++)
		{
			if(!bc_strncasecmp(redis_command_strings[command], request->buffer,
			                  strlen(redis_command_strings[command])))
			{
				request->command = command;
				break;
			}
		}

		if(request->command != REDIS_UNSET)
		{
			printf("%s type request!\n", redis_command_strings[request->command]);
			buffer_count = 1;
			i = request->buffer;
			while(i->next)
			{
				buffer_count++;
				i = i->next;
			}

			req_buffer = malloc(sizeof(*req_buffer) * buffer_count);
			i = request->buffer;
			buf_i = 0;
			while(i)
			{
				req_buffer[buf_i].base = i->buffer;
				req_buffer[buf_i].len = i->len;
				i = i->next;
			}
			req = calloc(1, sizeof(*req));
			req->data = req_buffer;

			// Send to buffer from inbound handle to outbound handle
			uv_write(req, request->server, req_buffer, buffer_count, NULL);
		}
		else
		{
			command_string = bc_getdelim(request->buffer, ' ', &len);
			if(!command_string)
				command_string = bc_getdelim(request->buffer, '\r', &len);
			command_string[--len] = '\0';

			req_buffer = malloc(sizeof(*req_buffer) * 3);
			req_buffer[0].base = "-ERR unknown command '";
			req_buffer[0].len = 22;
			req_buffer[1].base = command_string;
			req_buffer[1].len = len;
			req_buffer[2].base = "'\r\n";
			req_buffer[2].len = 3;

			req = calloc(1, sizeof(*req));
			req->data = req_buffer;

			// Send to buffer from inbound handle to outbound handle
			uv_write(req, request->client, req_buffer, 3, NULL);
		}
	}
	else
	{
		printf("no parsing support for serialized requests yet :(\n");
	}
}
