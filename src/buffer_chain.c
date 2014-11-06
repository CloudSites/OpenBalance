#include "buffer_chain.h"


int bc_memcmp(void *ptr, buffer_chain *buffer, ssize_t size)
{
	int ret_val;
	ssize_t i = 0, buffer_i = buffer->offset;
	buffer_chain *cur_link = buffer;
	char *data = ptr;

	while(i < size)
	{
		ret_val = data[i] - cur_link->buffer[buffer_i];
		if(ret_val)
			return (ret_val > 0) ? 1 : -1;
		i++;

		if(buffer_i == cur_link->len - 1)
		{
			// If more to compare, but buffer is exhausted
			if(!cur_link->next && i < size)
				return 1;
			else
			{
				buffer_i = 0;
				cur_link = cur_link->next;
			}
		}
		else
		{
			buffer_i++;
		}
	}
	return 0;
}


int bc_strcmp(char *string1, buffer_chain *string2)
{
	return bc_strncmp(string1, string2, strlen(string1));
}


int bc_strcasecmp(char *string1, buffer_chain *string2)
{
	return bc_strncasecmp(string1, string2, strlen(string1));
}


int bc_strncmp(char *string1, buffer_chain *string2, ssize_t size)
{
	return bc_memcmp(string1, string2, size);
}


int bc_strncasecmp(char *string1, buffer_chain *string2, ssize_t size)
{
	int ret_val;
	ssize_t i = 0, buffer_i = string2->offset;
	char compare_1, compare_2;
	buffer_chain *cur_link = string2;

	while(i < size)
	{
		compare_1 = string1[i];
		compare_2 = cur_link->buffer[buffer_i];

		if(compare_1 >= 'A' && compare_1 <= 'Z')
			compare_1 += 0x20;

		if(compare_2 >= 'A' && compare_2 <= 'Z')
			compare_2 += 0x20;

		ret_val = compare_1 - compare_2;
		if(ret_val)
			return (ret_val > 0) ? 1 : -1;
		i++;

		if(buffer_i == cur_link->len - 1)
		{
			if(!cur_link->next && i < size)
				return 1;
			else
			{
				buffer_i = 0;
				cur_link = cur_link->next;
			}
		}
		else
		{
			buffer_i++;
		}
	}
	return 0;
}


buffer_chain* bc_memchr(buffer_chain *haystack, char needle)
{
	buffer_chain *ret_val;
	buffer_chain *cur_link = haystack;
	ssize_t i = haystack->offset;

	while(i < cur_link->len || cur_link->next)
	{
		if(cur_link->buffer[i] == needle)
		{
			ret_val = malloc(sizeof(*ret_val));
			memcpy(ret_val, cur_link, sizeof(*ret_val));
			ret_val->offset = i;
			return ret_val;
		}

		if(i == cur_link->len - 1)
		{
			if(cur_link->next)
			{
				i = 0;
				cur_link = cur_link->next;
			}
			else
			{
				return NULL;
			}
		}
		else
		{
			i++;
		}
	}

	return NULL;
}
