#ifndef __MODULE_H__
#define __MODULE_H__

#include <string.h>
#include "jansson.h"
#include "logging.h"
#include "event.h"

#define foreach_available_module(varname) for(varname = available_modules; varname->name; varname++)

typedef struct ob_module ob_module;

typedef enum
{
	MOD_ERROR = -1,
	MOD_OK,
} handler_response;

struct ob_module
{
	ob_module *previous;
	char *name;
	void *config;
	handler_response(*configure)(json_t*, void **);
	handler_response(*startup)(void *, uv_loop_t *);
	handler_response(*cleanup)(void *);
};

typedef struct
{
	char *name;
	handler_response(*configure)(json_t*, void **);
	handler_response(*startup)(void *, uv_loop_t *);
	handler_response(*cleanup)(void *);
} ob_module_structure;

extern ob_module *module_list;

int startup_modules(ob_module *startup_list, uv_loop_t *master_loop);

#endif
