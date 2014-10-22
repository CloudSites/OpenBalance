#ifndef __CONNECTION_H__
#define __CONNECTION_H__
#include <stdlib.h>
#include "uv.h"
#include "logging.h"
#include "worker.h"


typedef struct accept_callback accept_callback;
typedef struct upstream_connection upstream_connection;
typedef struct proxy_client proxy_client;

struct accept_callback
{
	void (*callback)(proxy_client*, uv_stream_t *);
	void *data;
};

struct upstream_connection
{
	uv_tcp_t *stream;
	void *previous;
};

struct proxy_client
{
	void *data;
	uv_stream_t *server;
	uv_connect_t *connection;
	uv_tcp_t *upstream;
	uv_tcp_t *downstream;
};


// Connection pooling related functions
void return_upstream_connection(uv_tcp_t* connection,
                                upstream_connection **pool);
uv_tcp_t* upstream_from_pool(upstream_connection **pool);
void upstream_disconnected(upstream_connection **pool, uv_tcp_t* connection);
void free_conn_pool(upstream_connection *pool);
void free_handle(uv_handle_t *handle);


// Proxy client related functions
void proxy_new_client(uv_stream_t *server, int status);

#endif
