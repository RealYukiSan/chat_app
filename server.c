#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <errno.h>
#include "util.h"

#define MAX_CLIENT 10

static const char bind_addr[] = "127.0.0.1";
static const uint16_t bind_port = PORT;
struct client_state {
	struct data		*d;
	struct sockaddr_in	addr;
	int			fd;
};
struct ctx_server {
	int main_fd;
	struct pollfd fds[MAX_CLIENT + 1];
	struct client_state clients[MAX_CLIENT];
};

static int create_server(void) {
	struct sockaddr_in addr;
	int fd;
	int ret;

	fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (fd < 0) {
		perror("socket");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	ret = inet_pton(AF_INET, bind_addr, &addr.sin_addr);
	if (ret <= 0) {
		if (ret == 0)
			fprintf(stderr, "Not in presentation format");
		else
			perror("inet_pton");
		close(fd);
		return -1;
	}

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
	setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int));

	addr.sin_port = htons(bind_port);
	ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		perror("bind");
		close(fd);
		return -1;
	}

	ret = listen(fd, 10);
	if (ret < 0) {
		perror("listen");
		close(fd);
		return -1;
	}

	printf("Server listen on %s:%hu\n", bind_addr, bind_port);
	return fd;
}

static int plug_client(struct ctx_server *ctx, int cl_fd, struct sockaddr_in *cl_addr)
{
	struct client_state *cl_st = NULL;
	int i;

	for (i = 0; i < MAX_CLIENT; i++) {
		if (ctx->clients[i].fd < 0) {
			cl_st = &ctx->clients[i];
			break;
		}
	}

	if (!cl_st) {
		return -EAGAIN;
	}

	cl_st->fd = cl_fd;
	cl_st->addr = *cl_addr;
	ctx->fds[i + 1].fd = cl_fd;
	ctx->fds[i + 1].events = POLLIN;

	return 0;
}

static int accept_connection(struct ctx_server *ctx)
{
	char addrstr[INET_ADDRSTRLEN];
	uint16_t port;
	struct sockaddr_in client_addr;
	socklen_t addr_len;
	int cl_fd;
	addr_len = sizeof(client_addr);
	cl_fd = accept(ctx->main_fd, (struct sockaddr *)&client_addr, &addr_len);
	if (cl_fd < 0) {
		int err = errno;
		if (err == EAGAIN || err == EINTR) 
			return 0;
		
		perror("accept");
		return -1;
	}

	port = ntohs(client_addr.sin_port);
	inet_ntop(AF_INET, &client_addr.sin_addr, addrstr, sizeof(addrstr));
	printf("new connected client: %s:%hu\n", addrstr, port);
	if (plug_client(ctx, cl_fd, &client_addr) < 0) {
		printf("The client slot is full! dropping new client\n");
		close(cl_fd);
	}
	
	return 0;
}

static int broadcast(struct ctx_server *ctx, int id, uint16_t len)
{
	struct client_state *from = &ctx->clients[id];
	struct client_state *cl;
	struct data_srv *d;
	size_t send_len;

	send_len = sizeof(*d) + len;
	d = malloc(send_len);
	if (!d) {
		perror("malloc");
		return -1;
	}
	
	inet_ntop(AF_INET, &from->addr.sin_addr, d->sender, sizeof(d->sender));
	sprintf(d->sender + strlen(d->sender), ":%hu", ntohs(from->addr.sin_port));
	memcpy(&d->data, from->d, sizeof(*from->d) + len);

	send_len = sizeof(*d) + len;
	for (size_t i = 0; i < MAX_CLIENT; i++) {
		cl = &ctx->clients[i];
		if (cl->fd < 0 || cl == from)
			continue;
		
		if (send(cl->fd, d, send_len, 0) < 0)
			close_client(cl, ctx, i);
	}
	
	free(d);
	return 0;
}

static int process_client_data(struct ctx_server *ctx, struct client_state *cs, int i)
{
	char client_addr[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &cs->addr.sin_addr, client_addr, sizeof(client_addr));
	uint16_t len = ntohs(cs->d->len);
	cs->d->msg[len - 1] = '\0';
	printf("From %s: %s\n", client_addr, cs->d->msg);
	
	return broadcast(ctx, i, len);
}

static void close_client(struct client_state *cs, struct ctx_server *ctx, int i)
{
	close(cs->fd);
	ctx->fds[i + 1].fd = -1;
	ctx->fds[i + 1].events = 0;
	ctx->fds[i + 1].revents = 0;
	ctx->clients[i].fd = -1;
}

static int handle_client_event(struct ctx_server *ctx, int i)
{
	struct client_state *cs = &ctx->clients[i];
	int ret;
	ret = recv(cs->fd, cs->d, sizeof(*cs->d) + SIZE_MSG, 0);
	if (ret < 0) {
		perror("recv");
		close_client(cs, ctx, i);
		return -1;
	}

	if (ret == 0) {
		printf("Client disconnected\n");
		close_client(cs, ctx, i);
		return 0;
	}

	return process_client_data(ctx, cs, i);
}

static void event_loop(int fd)
{
	struct ctx_server ctx;
	memset(&ctx, 0, sizeof(ctx));

	for (size_t i = 0; i < MAX_CLIENT; i++)
		ctx.clients[i].fd = -1;
	
	for (size_t i = 0; i < MAX_CLIENT; i++) {
		struct data *d;
		d = malloc(sizeof(*d) + SIZE_MSG);
		if (!d)
			goto out_free;
		ctx.clients[i].d = d;
	}

	ctx.main_fd = fd;
	ctx.fds[0].fd = fd;
	ctx.fds[0].events = POLLIN;
	ctx.fds[0].revents = 0;
	for (size_t i = 1; i <= MAX_CLIENT; i++) {
		ctx.fds[i].fd = -1;
		ctx.fds[i].events = 0;
		ctx.fds[i].revents = 0;
	}

	while (1) {
		int nr_ready;
		nr_ready = poll(ctx.fds, MAX_CLIENT + 1, -1);
		if (nr_ready < 0) {
			perror("poll");
			break;
		}

		if (ctx.fds[0].revents & POLLIN) {
			if (accept_connection(&ctx) < 0) {
				break;
			}
			nr_ready--;
		}

		if (nr_ready == 0)
			continue;

		for (size_t i = 1; i <= MAX_CLIENT; i++) {
			if (ctx.fds[i].revents & POLLIN) {
				if (handle_client_event(&ctx, i - 1) < 0)
					goto out_free;
				nr_ready--;
			}

			if (nr_ready == 0)
				break;
		}
	
	}
	 
	out_free:
		for (size_t i = 0; i < MAX_CLIENT; i++) {
			if (ctx.clients[i].d)
				free(ctx.clients[i].d);
			if (ctx.clients[i].fd > 0)
				ctx.clients[i].fd = -1;
		}
}

int main(void)
{
	int tcp_fd;
	tcp_fd = create_server();
	if (tcp_fd < 0)
		return -1;
	
	event_loop(tcp_fd);
	close(tcp_fd);
	return 0;
}
