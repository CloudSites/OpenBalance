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
		snprintf(new->addr, 19, "inproc://worker%d", iterator);
		new->socket = zmq_socket(zeromq_context, ZMQ_PAIR);
		zmq_bind(new->socket, new->addr);
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
	for(i = first->next; i != first; i = i->next)
	{
		zmq_recv(i->socket, &online_notice, sizeof(online_notice), 0);
	}
	zmq_recv(first->socket, &online_notice, sizeof(online_notice), 0);
}

void signal_worker_shutdown(void)
{
	worker_event shutdown;
	worker_list *first, *i;
	thread_list *worker_thread, *prev;

	shutdown.type = WORKER_SHUTDOWN;

	first = workers;
	for(i = first->next; i != first; i = i->next)
	{
		zmq_send(i->socket, &shutdown, sizeof(shutdown), 0);
	}
	zmq_send(first->socket, &shutdown, sizeof(shutdown), 0);

	for(worker_thread = worker_threads; worker_thread;
	    worker_thread = prev)
	{
		prev = worker_thread->previous;
		uv_thread_join(&(worker_thread->thread));
		free(worker_thread);
	}
}


void shutdown_master(void)
{
	worker_list *next, *i;

	i = workers->next;
	while(i != workers)
	{
		next = i->next;
		zmq_close(i->socket);
		free(i);
		i = next;
	}
	zmq_close(workers->socket);
	free(workers);
	zmq_ctx_destroy(zeromq_context);
}
