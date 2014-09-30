#ifndef __EVENT_H__
#define __EVENT_H__

#include <stdlib.h>
#include "logging.h"
#include "uv.h"

uv_loop_t *event_loop;

int init_event_system(void);
void cleanup_event_system(void);
void event_process(void);

#endif
