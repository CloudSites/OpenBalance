#ifndef __TCP_PROXY_H__
#define __TCP_PROXY_H__

#include "connection.h"
#include "module.h"
#include "config.h"
#include "memory.h"


// Module definition
#define tcp_proxy {"tcp_proxy", \
                     tcp_proxy_configure, \
                     tcp_proxy_startup, \
                     tcp_proxy_cleanup}

typedef struct tcp_proxy_config tcp_proxy_config;
typedef struct upstream_connection upstream_connection;
typedef struct proxy_client proxy_client;
typedef struct accept_callback accept_callback;

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
	accept_callback *accept_cb;
	struct sockaddr_in *upstream_addr;
};


// Module hooks
handler_response tcp_proxy_configure(json_t* config, void **conf_struct);
handler_response tcp_proxy_startup(void *config, uv_loop_t *master_loop);
handler_response tcp_proxy_cleanup(void *config);

// Event Handlers
void tcp_proxy_new_client(proxy_client *new, uv_stream_t *listener);
void tcp_proxy_new_upstream(uv_connect_t* conn, int status);

void tcp_proxy_client_read(uv_stream_t *inbound, ssize_t readlen,
                           const uv_buf_t *buffer);
void tcp_proxy_upstream_read(uv_stream_t *inbound, ssize_t readlen,
                             const uv_buf_t *buffer);
void free_handle_and_client(uv_handle_t *handle);

#endif
