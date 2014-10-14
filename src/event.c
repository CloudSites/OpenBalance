#include "event.h"



int init_event_system(void)
{
	event_loop = malloc(sizeof *event_loop);
	if (!event_loop) {
	    log_message(LOG_ERROR, "Could not allocate memory for event loop");
	    return 0;
	}

        if (uv_loop_init(event_loop) < 0) {
	    log_message(LOG_ERROR, "Could not initialize event loop");
	    free(event_loop);
	    return 0;
        }

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
