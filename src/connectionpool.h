#ifndef __CONNECTIONPOOL_H__
#define __CONNECTIONPOOL_H__
#include <stdlib.h>
#include "uv.h"

typedef struct upstream_connection upstream_connection;


struct upstream_connection
{
	uv_tcp_t *stream;
	void *previous;
};


void return_upstream_connection(uv_tcp_t* connection,
                                upstream_connection **pool);
uv_tcp_t* upstream_from_pool(upstream_connection **pool);
void upstream_disconnected(upstream_connection **pool, uv_tcp_t* connection);
void free_conn_pool(upstream_connection *pool);
void free_handle(uv_handle_t *handle);

#endif
