#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/socket.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"

static int create_server()
{
	int fd;
	struct sockaddr_in addr;

	fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
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
	struct pollfd *fds;
	int client_fd = -1;
	int ret;
	char buf[5];
	socklen_t addr_len = sizeof(addr);

	fds = calloc(2, sizeof(*fds));
	if (!fds) {
		perror("calloc");
		return;
	}

	fds[0].fd = fd;
	fds[0].events = POLLIN;

	fds[1].fd = client_fd;
	fds[1].events = POLLIN;

	while (1) {
		if (poll(fds, 2, -1) < 0) {
			perror("poll");
			break;
		}

		if (fds[0].revents & POLLIN) {
			client_fd = accept(fd, (struct sockaddr *)&addr, &addr_len);
			if (client_fd < 0) {
				perror("accept");
				break;
			}

			fds[1].fd = client_fd;
			printf("New client connected\n");
		}

		if (fds[1].revents & POLLIN) {
			ret = recv(fds[1].fd, buf, 5, 0);
			if (ret < 0) {
				perror("recv");
				break;
			}

			if (ret == 0) {
				printf("Client disconnected\n");
				close(fds[1].fd);
				continue;
			}

			printf("%s", buf);
			fflush(stdout);
		}
	}

	close(client_fd);
	free(fds);
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