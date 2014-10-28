#ifndef __CONNECTION_H__
#define __CONNECTION_H__
#include <stdlib.h>
#include "uv.h"
#include "logging.h"
#include "worker.h"
#include "memory.h"


typedef struct accept_callback accept_callback;
typedef struct resolve_callback resolve_callback;
typedef struct upstream_connection upstream_connection;
typedef struct proxy_client proxy_client;
typedef struct bind_and_listen_data bind_and_listen_data;
typedef struct proxy_config proxy_config;

struct accept_callback
{
	void (*callback)(proxy_client*, uv_stream_t *);
	void *data;
};

struct resolve_callback
{
	char *service;
	char *node;
	void (*callback)(uv_getaddrinfo_t *, struct addrinfo *);
	void *data;
	uv_loop_t *loop;
};

struct upstream_connection
{
	uv_tcp_t *stream;
	void *previous;
};

struct proxy_config
{
	struct sockaddr_in *upstream_sockaddr;
};

struct proxy_client
{
	void *data;
	uv_stream_t *server;
	uv_connect_t *connection;
	uv_tcp_t *upstream;
	uv_tcp_t *downstream;
	proxy_config *proxy_settings;
};

struct bind_and_listen_data
{
	uv_tcp_t *listener;
	int backlog_size;
	accept_callback* accept_cb;
};


// Connection pooling related functions
void return_upstream_connection(uv_tcp_t* connection,
                                upstream_connection **pool);
uv_tcp_t* upstream_from_pool(upstream_connection **pool);
void upstream_disconnected(upstream_connection **pool, uv_tcp_t* connection);
void free_conn_pool(upstream_connection *pool);
void free_handle(uv_handle_t *handle);
void free_handle_and_client(uv_handle_t *handle);


// Address resolution related functions
int resolve_address(uv_loop_t *loop, char *address,
                    resolve_callback *callback);
void resolve_address_cb(uv_getaddrinfo_t *req, int status,
                        struct addrinfo *res);
void bind_on_and_listen(uv_getaddrinfo_t *req, struct addrinfo *res);

// Proxy client related functions
void proxy_accept_client(uv_stream_t *server, int status);
void proxy_new_client(proxy_client *new, uv_stream_t *listener);
void proxy_new_upstream(uv_connect_t* conn, int status);

void proxy_client_read(uv_stream_t *inbound, ssize_t readlen,
                       const uv_buf_t *buffer);
void proxy_upstream_read(uv_stream_t *inbound, ssize_t readlen,
                         const uv_buf_t *buffer);

#endif
