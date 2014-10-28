#ifndef __SIMPLE_PROXY_H__
#define __SIMPLE_PROXY_H__

#include "connection.h"
#include "module.h"
#include "config.h"
#include "memory.h"


// Module definition
#define simple_proxy {"simple_proxy", \
                     simple_proxy_configure, \
                     simple_proxy_startup, \
                     simple_proxy_cleanup}

typedef struct simple_proxy_config simple_proxy_config;

struct simple_proxy_config
{
	char *listen_addr;
	char *upstream_host;
	int upstream_port;
	int backlog_size;
	uv_tcp_t *listener;
	accept_callback *accept_cb;
	proxy_config *proxy_settings;
};


// Module hooks
handler_response simple_proxy_configure(json_t* config, void **conf_struct);
handler_response simple_proxy_startup(void *config, uv_loop_t *master_loop);
handler_response simple_proxy_cleanup(void *config);

#endif
