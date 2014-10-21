#include "main.h"
#include <unistd.h>


int main(int argc, char *argv[])
{
	uv_loop_t *master_event_loop;

	if(!parse_cli_arguments(argc, argv))
		return 1;

	log_message(LOG_INFO, "Starting OpenBalance v%s ...\n", VERSION);

	if(!start_master())
		return 1;


	if(!start_workers())
		return 1;

	wait_for_workers();

	if(!init_event_system(&master_event_loop))
	{
		log_message(LOG_ERROR, "Failed to start event system\n");
		return 1;
	}

	startup_modules(module_list, master_event_loop);

	event_process(master_event_loop);;

	config_cleanup();
	free_pool();
	cleanup_event_system(master_event_loop);

	signal_worker_shutdown();
	shutdown_master();

	return 0;
}
