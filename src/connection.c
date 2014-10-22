#include "connection.h"

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


void proxy_new_client(uv_stream_t *listener, int status)
{
	int ret;
	proxy_client *new;
	accept_callback *callback;

	callback = listener->data;
	listener->data = callback->data;

	log_message(LOG_INFO, "New client connection\n");

	if(status)
	{
		log_message(LOG_ERROR, "Error accepting new client\n");
		return;
	}

	// Allocate our client structure
	new = malloc(sizeof(*new));

	// Set reference to listener stream and data pointer
	new->server = listener;
	new->data = callback->data;

	// Initialize downstream connection
	new->downstream = malloc(sizeof(*new->downstream));
	ret = uv_tcp_init(listener->loop, new->downstream);
	if(ret)
	{
		log_message(LOG_ERROR, "Failed initializing new client socket\n");
		return;
	}

	// Accept connection
	new->downstream->data = new;
	ret = uv_accept(new->server, (uv_stream_t*)new->downstream);
	if(ret)
	{
		log_message(LOG_ERROR, "Failed to accept new client socket\n");
		return;
	}

	// Run callback
	callback->callback(new, listener);
}
