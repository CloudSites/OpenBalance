#include "connectionpool.h"

void return_upstream_connection(uv_tcp_t* stream,
                                upstream_connection **pool)
{
	upstream_connection *new = malloc(sizeof(*new));

	// Add to the front of the list
	new->previous = *pool;
	new->stream = stream;
	*pool = new;
}


uv_tcp_t* upstream_from_pool(upstream_connection **pool)
{
	uv_tcp_t *ret;
	
	if(!(*pool))
		return NULL;
	else
	{
		ret = (*pool)->stream;
		*pool = (*pool)->previous;
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

void upstream_disconnected(upstream_connection **pool, uv_tcp_t* connection)
{
	upstream_connection *previous = NULL;
	upstream_connection *i = *pool;
	
	while(i)
	{
		if(i->stream == connection)
		{
			if(previous)
			{
				// If we're removing a list element not at the top, remove
				//  this element and set the one before to point where this one
				//  used to
				previous->previous = i->previous;
				free(i);
			}
			else
			{
				// If this is at the top of the queue we just pop it off moving
				//  the next one up to the top
				*pool = i->previous;
				free(i);
			}
			return;
		}
		previous = i;
		i = i->previous;
	}
}
