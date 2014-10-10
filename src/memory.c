#include "memory.h"

memory_allocation *free_memory_list = NULL;

void alloc_from_pool(uv_handle_t *handle, size_t suggested_size,
                     uv_buf_t *buffer)
{
	memory_allocation *ptr;
	if(free_memory_list)
	{
		buffer->base = free_memory_list->allocation;
		ptr = free_memory_list->previous;
		free(free_memory_list);
		free_memory_list = ptr;
	}
	else
	{
		buffer->base = malloc(suggested_size);
	}
	buffer->len = suggested_size;
}


void return_alloc_to_pool(void *allocation)
{
	memory_allocation *new;
	
	// Allocate memory tracking linked list element
	new = malloc(sizeof(*new));
	
	// Add to the front of the list
	new->previous = free_memory_list;
	new->allocation = allocation;
	free_memory_list = new;
}

void free_pool(void)
{
	memory_allocation *ptr;
	while(free_memory_list)
	{
		ptr = free_memory_list->previous;
		free(free_memory_list->allocation);
		free(free_memory_list);
		free_memory_list = ptr;
	}
}



void free_request(uv_write_t *req, int status)
{
	uv_buf_t *ptr = req->data;
	
	// Save the read buffer for reuse, free the rest
	return_alloc_to_pool(ptr->base);
	free(ptr);
	free(req);
}
