#ifndef __MASTER_H__
#define __MASTER_H__

#include "event.h"
#include "worker.h"


typedef struct worker_list worker_list;

struct worker_list
{
	void *socket;
	char addr[20];
	worker_list *next;
};


worker_list *workers;


int start_master(void);
void wait_for_workers(void);
void signal_worker_shutdown(void);
void shutdown_master(void);

#endif
