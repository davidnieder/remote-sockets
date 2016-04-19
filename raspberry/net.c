#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "net.h"
#include "defines.h"
#include "protocol.h"


static connection_t server, client;


int
server_start(char *addr, int port)
{
	int ret;
	socket_t server_socket;
    struct sockaddr_in6 server_addr;

	/* create socket */
	server_socket = socket(AF_INET6, SOCK_STREAM, 0);
	if (server_socket == -1)	{
		perror("socket()");
		return 1;
	}

	/* set SO_REUSEADDR */
	ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 },
			sizeof(int));
	if (ret != 0)	{
		perror("setsockopt()");
		close(server_socket);
		return 1;
	}

	/* set sockaddr_in server_addr */
    memset(&server_addr, 0, sizeof(struct sockaddr_in6));
    server_addr.sin6_family = AF_INET6;
	server_addr.sin6_addr = in6addr_any;
    server_addr.sin6_port = htons(port);
	server_addr.sin6_flowinfo = 0;

	/* bind socket */
	ret = bind(server_socket, (struct sockaddr *) &server_addr,
			sizeof(struct sockaddr_in6));
	if (ret == -1)	{
		perror("bind()");
		close(server_socket);
		return 1;
	}

	/* listen() */
	ret = listen(server_socket, 5);
	if (ret == -1)	{
		perror("listen()");
		close(server_socket);
		return 1;
	}

	server.socket = server_socket;
	server.addr = server_addr;

	char buf[INET6_ADDRSTRLEN];
	printf("server started, listening on %s:%i\n",
			inet_ntop(AF_INET6, &(server.addr.sin6_addr), buf, INET6_ADDRSTRLEN),
			//inet_ntoa(server.addr.sin_addr),
			ntohs(server.addr.sin6_port));
	return 0;
}

void
server_accept()
{
	char client_msg[NETWORK_PACKET_SIZE+1], *response;
	int ret;

	client.socket = accept(server.socket, (struct sockaddr *) &client.addr,
						&(socklen_t) {sizeof(struct sockaddr_in)});
	if (client.socket == -1)	{
		perror("accept()");
		close(client.socket);
		return;
	}

	ret = recv(client.socket, client_msg, NETWORK_PACKET_SIZE, MSG_WAITALL);
	if (ret == NETWORK_PACKET_SIZE)	{
		response = (char*) packet_process((uint8_t*) client_msg);
		send(client.socket, response, NETWORK_PACKET_SIZE, 0);
	}

	close(client.socket);
}

void
server_shut_down()
{
	close(client.socket);
	close(server.socket);
}
