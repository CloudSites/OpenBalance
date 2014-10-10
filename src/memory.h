#ifndef __MEMORY_H__
#define __MEMORY_H__
#include <stdlib.h>
#include "uv.h"

typedef struct memory_allocation memory_allocation;

struct memory_allocation
{
	void *allocation;
	void *previous;
};


void alloc_from_pool(uv_handle_t *handle, size_t suggested_size,
                     uv_buf_t *buffer);
void return_alloc_to_pool(void *allocation);
void free_pool(void);
void free_request(uv_write_t *req, int status);

#endif
