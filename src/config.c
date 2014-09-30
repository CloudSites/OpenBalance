#include "config.h"

void help_option(void);
void invalid_option(void);
int log_level(char *level);
int config_file(char *filepath);


// Handle the command line arguments
int parse_cli_arguments(int argc, char *argv[])
{
	int opt_char;

	// Squelch getopt's error output, we handle that ourselves!
	opterr = 0;

	// Iterate over options and handle them as they come
	while((opt_char = getopt(argc, argv, "hl:c:")) != -1)
	{
		switch(opt_char)
		{
			case 'h':
				// Help option
				help_option();
				return 0;
			case 'l':
				// Logging level option
				if(!log_level(optarg))
				{
					log_message(LOG_ERROR, "%s is not a valid log level\n",
					            optarg);
					return 0;
				}
				break;
			case 'c':
				// Config file option
				if(!config_file(optarg))
				{
					log_message(LOG_ERROR,
					            "%s is not a valid configuration file\n",
					            optarg);
					return 0;
				}
				break;
			case '?':
				// Unhandled option
				invalid_option();
				return 0;
		}
	}

	return 1;
}

// Print help dialog
void help_option(void)
{
	printf("    OpenBalance Usage\n"
	       " -h       | Display this help prompt\n"
	       " -l LEVEL | Set logging level [EMERGENCY, ALERT, ERROR, WARNING, "
	         "INFO, DEBUG]\n"
	       " -c FILE  | Use configuration file provided\n");
}


// Handle invalid options
void invalid_option()
{
	if(optopt == 'l')
	{
		log_message(LOG_ERROR, "-l requires a logging level as an argument\n");
	}
	else if(optopt == 'c')
	{
		log_message(LOG_ERROR, "-c requires a config file as an argument\n");
	}
	else
	{
		log_message(LOG_ERROR, "Unknown option '%c' given.\n", optopt);
	}

	help_option();
}


// Handle log level option
int log_level(char *level)
{
	int level_iter;
	for(level_iter = LOG_EMERGENCY; level_iter <= LOG_DEBUG; level_iter++)
	{
		if(!strcasecmp(log_level_strings[level_iter], level))
		{
			set_logging_level(level_iter);
			return 1;
		}
	}
	return 0;
}


// Handle config file option
int config_file(char *filepath)
{
	json_t *file_root;
	json_error_t error;
	const char *key;
	json_t *value;
	ob_module_structure *i;
	ob_module *new;
	int match;
	
	// Parse JSON
	file_root = json_load_file(filepath, JSON_REJECT_DUPLICATES, &error);
	if(!file_root)
	{
		if(error.line == -1)
		{
			// General file handling error
			log_message(LOG_ERROR,
			            "Failed to parse '%s' as JSON: %s\n",
			            filepath, error.text);
		}
		else
		{
			// Error within config file
			log_message(LOG_ERROR,
			            "Failed to parse '%s' as JSON: %s on line %d\n",
			            filepath, error.text, error.line);
		}
		return 0;
	}
	else if(!json_is_object(file_root))
	{
		log_message(LOG_ERROR,
		            "Failed to handle config JSON from '%s': Root structure is"
		            " not a JSON object.\n", filepath);
		json_decref(file_root);
		return 0;
	}
	
	// Handle per-module configs
	json_object_foreach(file_root, key, value)
	{
		match = 0;
		foreach_available_module(i)
		{
			if(!strcmp(i->name, key))
			{
				match = 1;
				// Allocate config structure for this module and add to list
				new = malloc(sizeof(*new));
				if(!new)
				{
					log_message(LOG_ERROR,
					            "Failed to allocate module structure\n");
					json_decref(file_root);
					return 0;
				}
				if(!module_list)
				{
					new->previous = NULL;
				}
				else
				{
					new->previous = module_list;
				}
				module_list = new;
				// Copy name and hooks over
				new->name = i->name;
				new->configure = i->configure;
				new->startup = i->startup;
				new->cleanup = i->cleanup;
				
				// Run configuration hook
				if(new->configure(value, &(new->config)) != MOD_OK)
				{
					log_message(LOG_ERROR,
					            "Configuration for '%s' module failed\n", key);
					json_decref(file_root);
					return 0;
				}
			}
		}
		
		if(!match)
		{
			log_message(LOG_ERROR,
				        "Unhandled key '%s' in JSON root object from '%s'\n",
				        key, filepath);
			json_decref(file_root);
			return 0;
		}
	}
	
	json_decref(file_root);
	return 1;
}

void config_cleanup(void)
{
	ob_module *previous;
	while(module_list)
	{
		previous = module_list->previous;
		if(module_list->config)
		{
			if(module_list->cleanup(module_list->config) != MOD_OK)
			{
				log_message(LOG_WARNING, "%s module failed config clean up\n",
				            module_list->name);
			}
		}
		free(module_list);
		module_list = previous;
	}
}

char* get_config_string(json_t* json, char *key, char* def_value,
                        size_t maxlen)
{
	json_t *value;
	size_t len;
	char *ret;
	
	value = json_object_get(json, key);
	if(!value)
	{
		log_message(LOG_INFO,
		            "No '%s' specified, setting to '%s'\n", key,
		            def_value);
		len = strlen(def_value);
		ret = calloc(1, len + 1);
		strncpy(ret, def_value, len);
	}
	else if(!json_is_string(value))
	{
		log_message(LOG_ERROR,
		            "Value 'listen_host' requires a string\n");
		return NULL;
	}
	else
	{
		const char *string = json_string_value(value);
		size_t len;
		len = strnlen(string, maxlen);
		ret = calloc(1, len + 1);
		strncpy(ret, string, len);
	}
	return ret;
}
