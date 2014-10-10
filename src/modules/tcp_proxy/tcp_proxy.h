#ifndef __TCP_PROXY_H__
#define __TCP_PROXY_H__

#include "module.h"
#include "config.h"
#include "memory.h"
#include "connectionpool.h"

// Module definition
#define tcp_proxy {"tcp_proxy", \
                     tcp_proxy_configure, \
                     tcp_proxy_startup, \
                     tcp_proxy_cleanup}

typedef struct tcp_proxy_config tcp_proxy_config;
typedef struct tcp_proxy_client tcp_proxy_client;

struct tcp_proxy_config
{
	char *listen_host;
	int listen_port;
	char *upstream_host;
	int upstream_port;
	int backlog_size;
	int connection_pooling;
	uv_tcp_t *listener;
	upstream_connection *pool;
	struct sockaddr_in *upstream_addr;
};


struct tcp_proxy_client
{
	tcp_proxy_config *config;
	uv_stream_t *server;
	uv_connect_t *connection;
	uv_tcp_t *upstream;
	uv_tcp_t *downstream;
};


// Module hooks
handler_response tcp_proxy_configure(json_t* config, void **conf_struct);
handler_response tcp_proxy_startup(void *config, ob_module *module);
handler_response tcp_proxy_cleanup(void *config);

// Event Handlers
void tcp_proxy_new_client(uv_stream_t *server, int status);
void tcp_proxy_new_upstream(uv_connect_t* conn, int status);

void tcp_proxy_client_read(uv_stream_t *inbound, ssize_t readlen,
                           const uv_buf_t *buffer);
void tcp_proxy_upstream_read(uv_stream_t *inbound, ssize_t readlen,
                             const uv_buf_t *buffer);

#endif
