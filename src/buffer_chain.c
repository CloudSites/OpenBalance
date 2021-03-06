#include "buffer_chain.h"


int bc_memcmp(void *ptr, buffer_chain *buffer, size_t size)
{
	int ret_val;
	size_t i = 0, buffer_i = buffer->offset;
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


int bc_strncmp(char *string1, buffer_chain *string2, size_t size)
{
	return bc_memcmp(string1, string2, size);
}


int bc_strncasecmp(char *string1, buffer_chain *string2, size_t size)
{
	int ret_val;
	size_t i = 0, buffer_i = string2->offset;
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
	size_t i = haystack->offset;

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


buffer_chain* bc_memstr(buffer_chain *haystack, char *needle)
{
	return bc_memmem(haystack, needle, strlen(needle));
}


buffer_chain* bc_memmem(buffer_chain *haystack, char *needle, size_t len)
{
	size_t cur_offset, cur_needle = 0;
	int last_needle = len - 1;
	buffer_chain *ret_val, *tmp, scratch;

	ret_val = bc_memchr(haystack, needle[cur_needle]);
	// If first needle found
	if(ret_val)
	{
		cur_offset = ret_val->offset;
	}

	tmp = ret_val;
	while(tmp)
	{
		// If character mismatch, reset current needle and continue checking
		if(tmp->buffer[cur_offset] != needle[cur_needle])
		{
			cur_needle = 0;
			memcpy(&scratch, ret_val, sizeof(*ret_val));
			scratch.offset++;
			free(ret_val);
			ret_val = bc_memchr(&scratch, needle[cur_needle]);
			tmp = ret_val;
		}
		else
		{
			// Detect match completion or increment needle
			if(cur_needle == last_needle)
			{
				return ret_val;
			}
			else
			{
				cur_needle++;
			}

			// Detect end of current chain
			if(cur_offset == tmp->len - 1)
			{
				// If end of haystack
				if(!tmp->next)
				{
					return NULL;
				}
				else
				{
					// Use next chain
					tmp = tmp->next;
					cur_offset = tmp->offset;
				}
			}
			else
			{
				cur_offset++;
			}
		}
	}

	return NULL;
}


void* bc_memcpy(void *dest, buffer_chain *src, size_t len)
{
	buffer_chain *cur_link = src;
	size_t buf_i = src->offset;
	size_t index = 0;
	char *destination = dest;

	while(index < len && (buf_i < cur_link->len || cur_link->next))
	{
		destination[index] = cur_link->buffer[buf_i];
		index++;

		if(buf_i == cur_link->len - 1)
		{
			if(cur_link->next)
			{
				buf_i = 0;
				cur_link = cur_link->next;
			}
			else
			{
				return dest;
			}
		}
		else
		{
			buf_i++;
		}
	}
	
	return dest;
}


char* bc_getdelim(buffer_chain *buffer, char delim, size_t *len)
{
	buffer_chain *delimiter, *i;
	size_t length = buffer->offset * -1;
	char *ret_val;

	delimiter = bc_memchr(buffer, delim);
	if(!delimiter)
	{
		*len = 0;
		return NULL;
	}

	i = buffer;
	while(i->buffer != delimiter->buffer)
	{
		length += i->len;
		i = i->next;
	}

	length += delimiter->len - (delimiter->len - delimiter->offset) + 1;

	ret_val = malloc(length);
	bc_memcpy(ret_val, buffer, length);
	*len = length;
	free(delimiter);

	return ret_val;
}


char* bc_getstrdelim(buffer_chain *buffer, char *delim, size_t *len)
{
	buffer_chain *delimiter, *i;
	size_t length = buffer->offset * -1;
	char *ret_val;

	delimiter = bc_memstr(buffer, delim);
	if(!delimiter)
	{
		*len = 0;
		return NULL;
	}

	i = buffer;
	while(i->buffer != delimiter->buffer)
	{
		length += i->len;
		i = i->next;
	}

	length += delimiter->len - (delimiter->len - delimiter->offset) + 1;

	ret_val = malloc(length);
	bc_memcpy(ret_val, buffer, length);
	*len = length;

	return ret_val;
}


char* bc_getline(buffer_chain *buffer, size_t *len)
{
	return bc_getdelim(buffer, '\n', len);
}


size_t bc_strlen(buffer_chain *buffer)
{
	size_t len = 0;

	do
	{
		len += buffer->len - buffer->offset;
		buffer = buffer->next;
	}
	while(buffer);

	return len;
}
