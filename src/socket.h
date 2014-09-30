#ifndef __SOCKET_H__
#define __SOCKET_H__

/*
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
*/

#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "logging.h"
#include "event.h"

typedef struct ob_socket_address ob_socket_address;
typedef struct ob_listen_socket ob_listen_socket;
typedef struct ob_client_socket ob_client_socket;

typedef enum
{
	TCP_SOCKET = 0,
} socket_type;


typedef enum
{
	IPV4_ADDR = 0,
	IPV6_ADDR
} addr_type;

struct ob_socket_address
{
	addr_type type;
	char *string;
	char *node;
	char *service;
	struct addrinfo *addrinfo;
};

struct ob_listen_socket
{
	socket_type type;
	ob_socket_address *socket_addresses;
	evutil_socket_t listener;
};

struct ob_client_socket
{
	socket_type type;
	ob_socket_address *socket_addresses;
	struct bufferevent *client_buffer;
};


ob_socket_address* get_socket_addresses(int type, char *node, char *service);
void free_socket_addresses(ob_socket_address *target);

ob_listen_socket* new_listen_socket(int type, char *node, char *service);
ob_client_socket* new_client_socket(int type, char *node, char *service);
ob_client_socket* new_client_socket_from_addr(ob_socket_address *addresses);
void free_ob_listen_socket(ob_listen_socket *target);
void free_ob_client_socket(ob_client_socket *target);

#endif
