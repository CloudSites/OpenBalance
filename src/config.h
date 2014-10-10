#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <jansson.h>

#include "logging.h"
#include "modules.h"

#define DEFAULT_CONFIG_FILE_PATH "/etc/openbalance/openbalance.conf"

int parse_cli_arguments(int argc, char *argv[]);
void config_cleanup(void);
char* get_config_string(json_t* json, char *key, char* def_value,
                        size_t maxlen);
int get_config_int(json_t* json, char *key, int def_value);
int get_config_boolean(json_t* json, char *key, int def_value);

#endif
