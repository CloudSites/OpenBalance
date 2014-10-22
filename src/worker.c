#include "worker.h"


int start_workers(void)
{
	worker_list *first, *i;

	// Start up workers
	first = workers;
	i = first;
	do
	{
		uv_thread_create(&(i->thread), worker_thread, i);
		i = i->next;
	}
	while(i != first);

	return 1;
}


void worker_thread(void *arg)
{
	int running = 1;
	worker_event event;
	master_event online_notice;
	worker_list *this = arg;
	void *master_socket = zmq_socket(zeromq_context, ZMQ_PAIR);

	// Send master worker_online notification
	zmq_connect(master_socket, this->address);
	online_notice.data = (void*)this->address;
	zmq_send(master_socket, &online_notice, sizeof(online_notice), 0);

	// Worker process loop
	while(running)
	{
		// Get an event from the master
		zmq_recv(master_socket, &event, sizeof(event), 0);
		// Execute event by type
		switch(event.type)
		{
			case WORKER_SHUTDOWN:
				running = 0;
				break;

			default:
				log_message(LOG_ERROR, "Worker got unhandled task type\n");
				running = 0;
				break;
		}
	}

	zmq_close(this->socket);
	zmq_close(master_socket);
}

worker_list* get_worker(void)
{
	worker_list *worker;

	// Return current worker from list, rotate to next
	worker = workers;
	workers = workers->next;

	return worker;
}
