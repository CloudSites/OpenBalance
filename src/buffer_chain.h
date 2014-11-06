#ifndef __BUFFER_CHAIN_H__
#define __BUFFER_CHAIN_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef enum
{
	STATIC = 0, // Do not free
	POOLED,     // Free to memory pooler
	UNPOOLED    // Just free() it
} buffer_free_type;

typedef struct buffer_chain buffer_chain;

struct buffer_chain
{
	char *buffer;
	ssize_t len;
	ssize_t offset;
	buffer_chain *next;
	buffer_free_type free_type;
};


int bc_memcmp(void *ptr, buffer_chain *buffer, size_t size);
int bc_strcmp(char *string1, buffer_chain *string2);
int bc_strcasecmp(char *string1, buffer_chain *string2);
int bc_strncmp(char *string1, buffer_chain *string2, size_t size);
int bc_strncasecmp(char *string1, buffer_chain *string2, size_t size);
buffer_chain* bc_memchr(buffer_chain *haystack, char needle);
void* bc_memcpy(void *dest, buffer_chain *src, size_t len);
char* bc_getdelim(buffer_chain *buffer, char delim, size_t *len);

#endif
