#include "master.h"


int start_master(void)
{
	int iterator;
	worker_list *new;

	// Initialize 0mq and workers list
	zeromq_context = zmq_ctx_new();
	workers = NULL;

	for(iterator = 0; iterator < global_config.worker_threads; iterator++)
	{
		new = calloc(1, sizeof(*new));
		snprintf(new->address, 19, "inproc://worker%d", iterator);
		new->socket = zmq_socket(zeromq_context, ZMQ_PAIR);
		zmq_bind(new->socket, new->address);
		// Add to circular list
		if(!workers)
		{
			new->next = new;
			workers = new;
		}
		else
		{
			new->next = workers->next;
			workers->next = new;
			workers = new;
		}
	}

	return 1;
}

void wait_for_workers(void)
{
	master_event online_notice;
	worker_list *first, *i;

	first = workers;
	i = first;
	do
	{
		zmq_recv(i->socket, &online_notice, sizeof(online_notice), 0);
		i = i->next;
	}
	while(i != first);
}

void signal_worker_shutdown(void)
{
	worker_event shutdown;
	worker_list *first, *i, *temp;

	shutdown.type = WORKER_SHUTDOWN;

	first = workers;
	i = first;
	do
	{
		zmq_send(i->socket, &shutdown, sizeof(shutdown), 0);
		uv_thread_join(&(i->thread));
		temp = i->next;
		free(i);
		i = temp;
	}
	while(i != first);
}


void shutdown_master(void)
{
	zmq_ctx_destroy(zeromq_context);
}
