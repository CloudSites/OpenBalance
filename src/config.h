#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <jansson.h>
#include "logging.h"
#include "module.h"


#define DEFAULT_CONFIG_FILE_PATH "/etc/openbalance/openbalance.conf"


typedef struct openbalance_config openbalance_config;

struct openbalance_config
{
	int worker_threads;
};

openbalance_config global_config;

int parse_cli_arguments(int argc, char *argv[]);
void config_cleanup(void);
char* get_config_string(json_t* json, char *key, char* def_value,
                        size_t maxlen);
int get_config_int(json_t* json, char *key, int def_value);
int get_config_boolean(json_t* json, char *key, int def_value);
int load_module(ob_module_structure *module, json_t *config);

#endif
