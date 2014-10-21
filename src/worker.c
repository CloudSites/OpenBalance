#include "worker.h"


int start_workers(void)
{
	int iterator;
	thread_list *new_worker;

	worker_threads = NULL;

	// Spin up workers
	for(iterator = global_config.worker_threads; iterator; iterator--)
	{
		new_worker = malloc(sizeof(*new_worker));
		new_worker->previous = worker_threads;
		worker_threads = new_worker;
		snprintf(new_worker->addr, 19, "inproc://worker%d", iterator - 1);
		uv_thread_create(&(new_worker->thread), worker_thread, new_worker);
	}

	return 1;
}


void worker_thread(void *arg)
{
	int running = 1;
	void *master_socket = zmq_socket(zeromq_context, ZMQ_PAIR);
	worker_event event;
	master_event online_notice;
	thread_list *this = arg;

	zmq_connect(master_socket, this->addr);
	online_notice.data = (void*)this->addr;
	zmq_send(master_socket, &online_notice, sizeof(online_notice), 0);
	while(running)
	{
		zmq_recv(master_socket, &event, sizeof(event), 0);
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
	
	zmq_close(master_socket);
}


