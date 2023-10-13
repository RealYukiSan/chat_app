#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#include "util.h"

static int create_server()
{
	int fd;
	struct sockaddr_in addr;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket");
		return -1;
	}

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
	setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int));

	memset(&addr, 0, sizeof(addr));
	inet_pton(AF_INET, server_addr, &addr.sin_addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		close(fd);
		return -1;
	}

	if (listen(fd, 2) < 0) {
		perror("listen");
		close(fd);
		return -1;
	}

	printf("Server running on %s:%d...\n", server_addr, SERVER_PORT);	
	return fd;
}

static void start_event_loop(int fd)
{
	struct sockaddr_in addr;
	int client_fd;
	char buf[5];
	socklen_t addr_len = sizeof(addr);

	while (1) {
		client_fd = accept(fd, (struct sockaddr *)&addr, &addr_len);

		if (client_fd < 0) {
			perror("accept");
			break;
		}

		printf("Client connected\n");

		if (recv(client_fd, buf, 5, 0) < 0) {
			perror("recv");
			break;
		}

		printf("%s", buf);
		fflush(stdout);

		/**
		 * since the current communication was simplex, it was made into a single-session
		 * so the next incoming request from the client will be handled by the server.
		*/
		close(client_fd);
	}
}

int main(void)
{
	int socket_fd;
	socket_fd = create_server();
	if (socket_fd < 0)
		return -1;

	start_event_loop(socket_fd);
	close(socket_fd);

	return 0;
}