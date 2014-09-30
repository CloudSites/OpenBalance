#include "event.h"



int init_event_system(void)
{
	event_loop = uv_loop_new();
	return 1;
}


void cleanup_event_system(void)
{
	log_message(LOG_DEBUG, "cleaning up event system!\n");
	uv_loop_close(event_loop);
	free(event_loop);
}


void event_process(void)
{
	log_message(LOG_DEBUG, "processing events!\n");
	uv_run(event_loop, UV_RUN_DEFAULT);
}
