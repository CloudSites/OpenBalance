#include "socket.h"

ob_socket_address* get_socket_addresses(int type, char *node, char *service)
{
	//ob_socket *new;
	int ret, count;
	size_t len;
	struct addrinfo hints;
	struct addrinfo *servinfo, *i;
	struct sockaddr_in *ipv4;
	struct sockaddr_in6 *ipv6;
	ob_socket_address *return_array, *iter;

	if(type != TCP_SOCKET)
	{
		printf("non-TCP sockets not yet supported :(\n");
		return NULL;
	}
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	log_message(LOG_DEBUG, "Getting address info for %s:%s\n", node, service);
	ret = getaddrinfo(node, service, &hints, &servinfo);
	if(ret == EAI_NONAME)
	{
		log_message(LOG_ERROR, "Could not resolve interface for '%s'\n",
		            node);
		return NULL;
	}
	else if(ret == EAI_SERVICE)
	{
		log_message(LOG_ERROR, "Could not resolve service type for '%s:%s'\n",
		            node, service);
		return NULL;
	}
	else if(ret != 0)
	{
		log_message(LOG_ERROR, "Address resolution failure: %s\n",
		            gai_strerror(ret));
		return NULL;
	}
	
	// Count resolved addresses
	count = 0;
	for(i = servinfo; i; i = i->ai_next)
	{
		count++;
	}
	
	return_array = malloc(sizeof(*return_array) * (count+1));
	return_array->addrinfo = servinfo;
	
	// Iterate over resolved addresses
	iter = return_array;
	for(i = servinfo; i; i = i->ai_next)
	{
		// Copy node and service
		len = strlen(node);
		iter->node = calloc(len + 1, sizeof(char));
		strncpy(iter->node, node, len);
		len = strlen(service);
		iter->service = calloc(len + 1, sizeof(char));
		strncpy(iter->service, service, len);
		
		if(i->ai_family == AF_INET)
		{
			ipv4 = (struct sockaddr_in *)i->ai_addr;
			iter->type = IPV4_ADDR;
			iter->string = malloc(INET_ADDRSTRLEN);
			inet_ntop(i->ai_family, &(ipv4->sin_addr), iter->string, INET_ADDRSTRLEN);
			log_message(LOG_INFO, "%s resolved to %s (IPv4)\n", node,
			            iter->string);
		}
		else
		{
			ipv6 = (struct sockaddr_in6 *)i->ai_addr;
			iter->type = IPV6_ADDR;
			iter->string = malloc(INET6_ADDRSTRLEN);
			inet_ntop(i->ai_family, &(ipv6->sin6_addr),
			          iter->string, INET6_ADDRSTRLEN);
			log_message(LOG_INFO, "%s resolved to %s (IPv6)\n", node,
			            iter->string);
		}
		iter++;
	}
	memset(&(return_array[count]), 0, sizeof(*return_array));
	
	return return_array;
}


void free_socket_addresses(ob_socket_address *target)
{
	ob_socket_address *i;
	for(i = target; i->string; i++)
	{
		free(i->string);
		free(i->node);
		free(i->service);
	}
	freeaddrinfo(target->addrinfo);
	free(target);
}


ob_listen_socket* new_listen_socket(int type, char *node, char *service)
{
	ob_socket_address *addresses;
	ob_listen_socket *new;
	
	// Right now we only handle TCP
	if(type != TCP_SOCKET)
	{
		printf("non-TCP sockets not yet supported :(\n");
		return NULL;
	}
	
	
	// Resolve node and service
	addresses = get_socket_addresses(type, node, service);
	if(!addresses)
	{
		log_message(LOG_ERROR,
		            "Could not create listen socket due to address resolution "
		            "failure\n");
		return NULL;
	}
	
	// Allocate ob_struct
	new = malloc(sizeof(*new));
	if(!new)
	{
		log_message(LOG_EMERGENCY, "Could not allocate for ob_socket\n");
		free_socket_addresses(addresses);
		return NULL;
	}
	
	// Start filling out structure
	new->socket_addresses = addresses;
	new->type = type;
	
	// Build socket
	new->listener = socket(addresses->addrinfo->ai_family,
	                       addresses->addrinfo->ai_socktype, 0);
	if(new->listener == -1)
	{
		log_message(LOG_EMERGENCY, "Could not create socket: %s\n",
		            strerror(errno));
		free(new);
		free_socket_addresses(addresses);
		return NULL;
	}
	
	evutil_make_socket_nonblocking(new->listener);
	
	if(evutil_make_listen_socket_reuseable(new->listener) == -1)
	{
		log_message(LOG_EMERGENCY, "Could not set socket as reusable\n");
		evutil_closesocket(new->listener);
		free(new);
		free_socket_addresses(addresses);
		return NULL;
	}
	
	// Bind socket
	if(bind(new->listener,
	        (struct sockaddr*)new->socket_addresses->addrinfo->ai_addr,
	        new->socket_addresses->addrinfo->ai_addrlen))
	{
		log_message(LOG_ERROR, "Could not bind socket on %s:%s - %s\n", node,
		            service, strerror(errno));
		evutil_closesocket(new->listener);
		free(new);
		free_socket_addresses(addresses);
		return NULL;
	}
	log_message(LOG_INFO, "Bound socket on %s:%s\n", node, service);
	
	// Start listen on socket
	if(listen(new->listener, 20) < 0)
	{
		log_message(LOG_ERROR, "Could not listen on socket %s:%s - %s\n", node,
		            service, strerror(errno));
		evutil_closesocket(new->listener);
		free(new);
		free_socket_addresses(addresses);
		return NULL;
	}
	log_message(LOG_INFO, "Now listening on %s:%s\n", node, service);
	
	return new;
}

ob_client_socket* new_client_socket(int type, char *node, char *service)
{
	ob_socket_address *addresses;
	ob_client_socket *new;
	
	// Right now we only handle TCP
	if(type != TCP_SOCKET)
	{
		printf("non-TCP sockets not yet supported :(\n");
		return NULL;
	}
	
	
	// Resolve node and service
	addresses = get_socket_addresses(type, node, service);
	if(!addresses)
	{
		log_message(LOG_ERROR,
		            "Could not create listen socket due to address resolution "
		            "failure\n");
		return NULL;
	}
	
	// Allocate ob_struct
	new = malloc(sizeof(*new));
	if(!new)
	{
		log_message(LOG_EMERGENCY, "Could not allocate for ob_socket\n");
		free_socket_addresses(addresses);
		return NULL;
	}
	
	// Start filling out structure
	new->socket_addresses = addresses;
	new->type = type;
	
	// Build client buffer
	new->client_buffer = new_socket_buffer(-1);
	
	if(bufferevent_socket_connect(new->client_buffer,
	                (struct sockaddr*)new->socket_addresses->addrinfo->ai_addr,
	                          new->socket_addresses->addrinfo->ai_addrlen) < 0)
	{
		log_message(LOG_ERROR, "Could not connect to %s:%s - %s\n",
		            addresses[0].node, addresses[0].service, strerror(errno));
		bufferevent_free(new->client_buffer);
		free_socket_addresses(addresses);
		return NULL;
	}
	
	return new;
}

ob_client_socket* new_client_socket_from_addr(ob_socket_address *addresses)
{
	ob_client_socket *new;
	
	// Allocate ob_struct
	new = malloc(sizeof(*new));
	if(!new)
	{
		log_message(LOG_EMERGENCY, "Could not allocate for ob_socket\n");
		return NULL;
	}
	
	// Start filling out structure
	new->socket_addresses = addresses;
	new->type = addresses[0].type;
	
	// Build client buffer
	new->client_buffer = new_socket_buffer(-1);
	
	if(bufferevent_socket_connect(new->client_buffer,
	                (struct sockaddr*)new->socket_addresses->addrinfo->ai_addr,
	                          new->socket_addresses->addrinfo->ai_addrlen) < 0)
	{
		log_message(LOG_ERROR, "Could not connect to %s:%s - %s\n",
		            addresses[0].node, addresses[0].service, strerror(errno));
		bufferevent_free(new->client_buffer);
		free_socket_addresses(addresses);
		return NULL;
	}
	
	return new;
}

void free_ob_listen_socket(ob_listen_socket *target)
{
	if(target->listener)
		evutil_closesocket(target->listener);
	if(target->socket_addresses)
		free_socket_addresses(target->socket_addresses);
	free(target);
}


void free_ob_client_socket(ob_client_socket *target)
{
	if(target->client_buffer)
		bufferevent_free(target->client_buffer);
	free(target);
}
