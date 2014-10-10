#include "connectionpool.h"


void return_upstream_connection(uv_tcp_t* stream,
                                upstream_connection *pool)
{
	upstream_connection *new = malloc(sizeof(*new));

	// Add to the front of the list
	new->previous = pool;
	new->stream = stream;
	pool = new;
}


uv_tcp_t* upstream_from_pool(upstream_connection *pool)
{
	uv_tcp_t *ret;
	
	if(!pool)
		return NULL;
	else
	{
		ret = pool->stream;
		pool = pool->previous;
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
