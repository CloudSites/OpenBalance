#ifndef __WORKER_H__
#define __WORKER_H__

#include "event.h"
#include "config.h"


typedef struct worker_list worker_list;

struct worker_list
{
	uv_thread_t thread;
	void *socket;
	char address[20];
	worker_list *next;
};

worker_list *workers;


int start_workers(void);
void worker_thread(void *arg);
worker_list* get_worker(void);

#endif
