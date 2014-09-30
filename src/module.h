#ifndef __MODULE_H__
#define __MODULE_H__

#include <string.h>
#include "jansson.h"
#include "logging.h"
#include "event.h"

#define foreach_available_module(varname) for(varname = available_modules; varname->name; varname++)

typedef enum
{
	MOD_ERROR = -1,
	MOD_OK,
} handler_response;

typedef struct _ob_module
{
	struct _ob_module *previous;
	char *name;
	void *config;
	handler_response(*configure)(json_t*, void **);
	handler_response(*startup)(void *, struct _ob_module*);
	handler_response(*cleanup)(void *);
} ob_module;

typedef struct
{
	char *name;
	handler_response(*configure)(json_t*, void **);
	handler_response(*startup)(void *, ob_module*);
	handler_response(*cleanup)(void *);
} ob_module_structure;

extern ob_module *module_list;

int startup_modules(ob_module *startup_list);

#endif
