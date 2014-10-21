#include "event.h"


int init_event_system(uv_loop_t **event_loop)
{
	uv_loop_t *new_loop;
	new_loop = malloc(sizeof(*new_loop));
	if(!new_loop)
	{
		log_message(LOG_ERROR, "Could not allocate memory for event loop");
		return 0;
	}

	if(uv_loop_init(new_loop) < 0)
	{
		log_message(LOG_ERROR, "Could not initialize event loop");
		free(new_loop);
		return 0;
	}

	(*event_loop) = new_loop;
	return 1;
}


void cleanup_event_system(uv_loop_t *event_loop)
{
	uv_loop_close(event_loop);
	free(event_loop);
}


void event_process(uv_loop_t *event_loop)
{
	uv_run(event_loop, UV_RUN_DEFAULT);
}
