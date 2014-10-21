#include "module.h"

ob_module *module_list;

int startup_modules(ob_module *startup_list)
{
	ob_module *module;
	handler_response ret;
	module = startup_list;

	while(module)
	{
		ret = module->startup(module->config, module);
		if(ret != MOD_OK)
		{
			log_message(LOG_ERROR, "%s failed to start up\n", module->name);
			return 0;
		}
		module = module->previous;
	}
	return 1;
}
