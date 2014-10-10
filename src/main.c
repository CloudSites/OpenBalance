#include "main.h"

int main(int argc, char *argv[])
{
	if(!parse_cli_arguments(argc, argv))
		return 1;

	log_message(LOG_INFO, "Starting OpenBalance v%s ...\n", VERSION);
	
	if(!init_event_system())
	{
		log_message(LOG_ERROR, "Failed to start event system\n");
		return 1;
	}
	
	startup_modules(module_list);
	
	event_process();
	
	config_cleanup();
	free_pool();
	cleanup_event_system();
	
	return 0;
}
