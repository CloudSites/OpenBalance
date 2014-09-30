#ifndef __REDIS_PROXY_H__
#define __REDIS_PROXY_H__

#include "module.h"
#include "config.h"

#define MODULE_NAME "redis_proxy"

#define redis_proxy {MODULE_NAME, \
                     redis_proxy_configure, \
                     redis_proxy_startup, \
                     redis_proxy_cleanup}

typedef struct redis_proxy_config redis_proxy_config;
typedef struct redis_proxy_client redis_proxy_client;
typedef struct memory_allocation memory_allocation;

struct memory_allocation
{
	void *allocation;
	struct memory_pool_entry *previous;
}

struct redis_proxy_config
{
	char *listen_host;
	char *listen_port;
	char *upstream_host;
	char *upstream_port;
	uv_tcp_t *listener;
	struct sockaddr_in *upstream_addr;
};

struct redis_proxy_client
{
	redis_proxy_config *config;
	uv_stream_t *server;
	uv_connect_t *connection;
	uv_tcp_t *upstream;
	uv_tcp_t *downstream;
};

// Module function prototypes
handler_response redis_proxy_configure(json_t* config, void **conf_struct);
handler_response redis_proxy_startup(void *config, ob_module *module);
handler_response redis_proxy_cleanup(void *config);

void redis_proxy_new_client(uv_stream_t *server, int status);
void redis_proxy_new_upstream(uv_connect_t* conn, int status);
void redis_proxy_free_handle(uv_handle_t *handle);
void redis_proxy_read_alloc(uv_handle_t *handle, size_t suggested_size,
                            uv_buf_t *buffer);
void redis_proxy_client_read(uv_stream_t *inbound, ssize_t readlen,
                             const uv_buf_t *buffer);
void redis_proxy_upstream_read(uv_stream_t *inbound, ssize_t readlen,
                               const uv_buf_t *buffer);
void redis_proxy_free_request(uv_write_t *req, int status);
#endif
