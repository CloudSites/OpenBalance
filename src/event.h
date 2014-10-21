#ifndef __EVENT_H__
#define __EVENT_H__

#include <stdlib.h>
#include "logging.h"
#include "uv.h"
#include "zmq.h"


// Worker event types
enum
{
	WORKER_SHUTDOWN = 0
};

// Master event types
enum
{
	WORKER_DIED = -1,
	WORKER_ONLINE
};


typedef struct master_event master_event;
typedef struct worker_event worker_event;

struct worker_event
{
	int type;
	void *data;
};

struct master_event
{
	int type;
	void *data;
};


void *zeromq_context;


int init_event_system(uv_loop_t **event_loop);
void cleanup_event_system(uv_loop_t *event_loop);
void event_process(uv_loop_t *event_loop);

#endif
