#ifndef __WORKER_H__
#define __WORKER_H__

#include "event.h"
#include "config.h"


typedef struct thread_list thread_list;

struct thread_list
{
	uv_thread_t thread;
	char addr[20];
	thread_list *previous;
};


thread_list *worker_threads;


int start_workers(void);
void worker_thread(void *arg);

#endif
