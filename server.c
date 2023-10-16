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
#include "packet.h"

#define NR_CLIENT 2

struct client_state {
	int 			fd;
	struct sockaddr_in 	addr;
	size_t			recv_len;
	struct packet 		pkt;
};

struct server_ctx {
	int 			tcp_fd;
	struct pollfd		*fds;
	struct client_state	*clients;
};

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

	if (listen(fd, NR_CLIENT) < 0) {
		perror("listen");
		close(fd);
		return -1;
	}

	printf("Server running on %s:%d...\n", server_addr, SERVER_PORT);	
	return fd;
}

static void start_event_loop(struct server_ctx *srv_ctx)
{
	int ret;
	struct client_state cl_state;
	socklen_t addr_len = sizeof(cl_state.addr);
	char buf[5];

	while (1) {
		if (poll(srv_ctx->fds, NR_CLIENT, -1) < 0) {
			perror("poll");
			break;
		}

		if (srv_ctx->fds[0].revents & POLLIN) {
			cl_state.fd = accept(srv_ctx->tcp_fd, (struct sockaddr *)&cl_state.addr, &addr_len);
			if (cl_state.fd < 0) {
				perror("accept");
				break;
			}

			srv_ctx->fds[1].fd = cl_state.fd;
			printf("New client connected\n");
		}

		if (srv_ctx->fds[1].revents & POLLIN) {
			ret = recv(srv_ctx->fds[1].fd, buf, 5, 0);
			if (ret < 0) {
				perror("recv");
				break;
			}

			if (ret == 0) {
				printf("Client disconnected\n");
				close(srv_ctx->fds[1].fd);
				continue;
			}

			printf("%s", buf);
			fflush(stdout);
		}
	}
}

static int initialize_ctx(struct server_ctx *srv_ctx)
{
	srv_ctx->tcp_fd = create_server();
	if (srv_ctx->tcp_fd < 0)
		return -1;

	srv_ctx->fds = calloc(NR_CLIENT + 1, sizeof(*srv_ctx->fds));
	if (!srv_ctx->fds) {
		perror("calloc");
		return -1;
	}
	
	srv_ctx->fds[0].fd = srv_ctx->tcp_fd;
	srv_ctx->fds[0].events = POLLIN;
	srv_ctx->fds[0].revents = 0;

	for (size_t i = 1; i <= NR_CLIENT; i++) {
		srv_ctx->fds[i].events = POLLIN;
		srv_ctx->fds[i].revents = 0;
		srv_ctx->fds[i].fd = -1;
	}

	srv_ctx->clients = calloc(NR_CLIENT, sizeof(*srv_ctx->clients));
	if (!srv_ctx->clients) {
		perror("malloc");
		return -1;
	}

	return 0;	
}

static void clean_ctx(struct server_ctx *srv_ctx)
{
	free(srv_ctx->fds);
	free(srv_ctx->clients);
	close(srv_ctx->tcp_fd);
}

int main(void)
{
	struct server_ctx srv_ctx;

	if (initialize_ctx(&srv_ctx) < 0) {
		return -1;
	}
	
	start_event_loop(&srv_ctx);
	clean_ctx(&srv_ctx);

	return 0;
}