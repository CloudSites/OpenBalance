#ifndef __MASTER_H__
#define __MASTER_H__

#include "event.h"
#include "worker.h"


int start_master(void);
void wait_for_workers(void);
void signal_worker_shutdown(void);
void shutdown_master(void);

#endif
