#ifndef __REDIS_PROXY_H__
#define __REDIS_PROXY_H__

#include "connection.h"
#include "module.h"
#include "config.h"
#include "memory.h"
#include "buffer_chain.h"


// Module definition
#define redis_proxy {"redis_proxy", \
                     redis_proxy_configure, \
                     redis_proxy_startup, \
                     redis_proxy_cleanup}

typedef struct redis_proxy_config redis_proxy_config;
typedef struct redis_request redis_request;

typedef enum
{
	SERIALIZED_REQUEST = 0,
	INLINE_REQUEST
} redis_request_type;

typedef enum
{
	REDIS_UNSET = -1,
	REDIS_PING = 0,
	REDIS_GET,
	REDIS_SET,
	REDIS_COMMAND_LEN
} redis_command;

typedef enum
{
	READ_ARG_SIZE = 0,
	READ_ARG
} redis_read_state;

struct redis_proxy_config
{
	char *listen_addr;
	char *upstream_host;
	int upstream_port;
	int backlog_size;
	accept_callback *accept_cb;
	proxy_config *proxy_settings;
};

struct redis_request
{
	redis_request_type type;
	buffer_chain *buffer;
	uv_stream_t *client;
	uv_stream_t *server;
	redis_command command;
	int arg_count;
	redis_read_state read_state;
/*	int links_in_buffer_chain;
	int arg_count;
	int *arg_sizes;
	int cur_arg;*/
};


// Module hooks
handler_response redis_proxy_configure(json_t* config, void **conf_struct);
handler_response redis_proxy_startup(void *config, uv_loop_t *master_loop);
handler_response redis_proxy_cleanup(void *config);

// Client read hooks
void read_request(uv_stream_t *client, uv_stream_t *server, char *buffer,
                  ssize_t len);

// Helpers
void parse_request(redis_request *request);

#endif
