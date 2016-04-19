#ifndef NET_H
#define NET_H

#include <arpa/inet.h>

typedef int socket_t;
typedef struct	{
	socket_t socket;
	struct sockaddr_in6 addr;
} connection_t;

int server_start(char *addr, int port);
void server_accept(void);
void server_shut_down(void);

#endif
