#include "simple_proxy.h"


handler_response simple_proxy_configure(json_t* config, void **conf_struct)
{
	simple_proxy_config *module_config;

	// Error for non-object
	if(!json_is_object(config))
	{
		log_message(LOG_ERROR,
		            "simple_proxy configuration needs to be a JSON object\n");
		return MOD_ERROR;
	}

	// Allocate and start filling out configuration structure
	module_config = malloc(sizeof(*module_config));

	module_config->accept_cb = malloc(sizeof(*module_config->accept_cb));
	module_config->accept_cb->callback = proxy_new_client;
	module_config->proxy_settings = malloc(sizeof(*module_config->proxy_settings));
	module_config->proxy_settings->client_read_event = proxy_stream_relay;
	module_config->proxy_settings->upstream_read_event = proxy_stream_relay;

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

	// Set configuration structure
	(*conf_struct) = module_config;

	return MOD_OK;
}


handler_response simple_proxy_startup(void *config, uv_loop_t *master_loop)
{
	int ret;
	simple_proxy_config *cfg = config;
	proxy_config *proxy_cfg = cfg->proxy_settings;
	accept_callback *accept_cb;
	resolve_callback *resolve_cb;
	bind_and_listen_data *bnl_data;

	log_message(LOG_INFO, "simple_proxy starting!\n");

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


handler_response simple_proxy_cleanup(void *config)
{
	simple_proxy_config *module_config = config;
	uv_loop_t *main_loop = module_config->proxy_settings->loop;

	log_message(LOG_INFO, "Cleaning up simple_proxy\n");

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
