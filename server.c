#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
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
	struct packet 		pkt;
	size_t			recv_len;
};

struct server_ctx {
	int 			tcp_fd;
	struct pollfd		*fds;
	struct client_state	*clients;
};

static int create_server(void)
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

static int plug_client(int fd, struct sockaddr_in addr, struct server_ctx *srv_ctx)
{
	struct client_state *cs = NULL;
	char addr_str[INET_ADDRSTRLEN];
	uint16_t port;
	uint32_t i;

	for (i = 0; i < NR_CLIENT; i++) {
		if (srv_ctx->clients[i].fd < 0) {
			cs = &srv_ctx->clients[i];
			break;
		}
	}

	if (!cs)
		return -1;

	cs->fd = fd;
	cs->addr = addr;
	srv_ctx->fds[i + 1].fd = fd;
	srv_ctx->fds[i + 1].events = POLLIN;

	port = ntohs(addr.sin_port);
	inet_ntop(AF_INET, &addr.sin_addr, addr_str, INET_ADDRSTRLEN);
	printf("New client connected from %s:%d\n", addr_str, port);

	return 0;
}

static int accept_new_connection(struct server_ctx *srv_ctx)
{
	struct client_state cl_state;
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(cl_state.addr);
	int fd;

	fd = accept(srv_ctx->tcp_fd, (struct sockaddr *)&addr, &addr_len);

	if (fd < 0) {
		int ret = errno;
		if (ret == EINTR || ret == EAGAIN)
			return 0;

		perror("accept");
		return -1;
	}

	if (plug_client(fd, addr, srv_ctx) < 0) {
		printf("Client slot is full, dropping a new connection...\n");
		close(fd);
	}

	return 0;
}

static const char *stringify_ipv4(struct sockaddr_in *addr)
{
	static char buf[IP4_IDENTITY_SIZE];
	inet_ntop(AF_INET, &addr->sin_addr, buf, INET_ADDRSTRLEN);
	sprintf(buf + strlen(buf), ":%hu", ntohs(addr->sin_port));
	return buf;
}

static int broadcast_msg(struct client_state *cs, struct server_ctx *srv_ctx)
{
	struct packet *pkt;
	struct packet_msg_id *msg_id;
	size_t body_len;
	size_t msg_len;

	pkt = malloc(sizeof(*pkt));
	if (!pkt) {
		perror("malloc");
		return -1;
	}

	msg_id = &pkt->msg_id;
	msg_len = ntohs(cs->pkt.msg.len);
	body_len = sizeof(*msg_id) + msg_len;
	pkt->type = SR_PKT_MSG_ID;
	pkt->len = htons(body_len);
	msg_id->msg.len = cs->pkt.msg.len;
	memcpy(&msg_id->msg.data, &cs->pkt.msg.data, msg_len);
	memcpy(&msg_id->identity, stringify_ipv4(&cs->addr), IP4_IDENTITY_SIZE);

	for (size_t i = 0; i < NR_CLIENT; i++) {
		if (cs->fd == srv_ctx->clients[i].fd || srv_ctx->clients[i].fd < 0)
			continue;
		
		if (send(srv_ctx->clients[i].fd, pkt, HEADER_SIZE + body_len, 0) < 0) {
			perror("send");
			free(pkt);
			return -1;
		}
	}

	free(pkt);

	return 0;
}

static int handle_cl_pkt_msg(struct client_state *cs, struct server_ctx *srv_ctx)
{
	printf("New message from %s = %s\n", stringify_ipv4(&cs->addr), cs->pkt.msg.data);

	/**
	 * TODO:
	 * 	setelah mentransform dan menampilkan pesannya:
	 * 		- melakukan broadcast ke client yang terhubung
	 * 		- menyimpan pesan kedalam record yang ada di db history
	*/
	if (broadcast_msg(cs, srv_ctx) < 0)
		return -1;

	return 0;
}

static void close_cl(struct server_ctx *srv_ctx, int idx)
{
	close(srv_ctx->fds[idx + 1].fd);
	srv_ctx->fds[idx + 1].fd = -1;
	srv_ctx->clients[idx].fd = -1;
}

static int process_cl_pkt(struct client_state *cs, struct server_ctx *srv_ctx)
{
	size_t expected_len;

try_again:
	if (cs->recv_len < HEADER_SIZE)
		return 0;

	expected_len = HEADER_SIZE + ntohs(cs->pkt.len);
	if (cs->recv_len < expected_len)
		return 0;

	switch (cs->pkt.type) {
	case CL_PKT_MSG:
		handle_cl_pkt_msg(cs, srv_ctx);
		break;

	default:
		printf("Invalid packet\n");
		return -1;
	}

	cs->recv_len -= expected_len;
	if (cs->recv_len > 0) {
		char *dest = (char *)&cs->pkt;
		char *src = dest + expected_len;
		memmove(dest, src, cs->recv_len);
		goto try_again;
	}

	return 0;
}

static int handle_event(struct server_ctx *srv_ctx, int id_client)
{
	ssize_t ret;
	struct client_state *cs = &srv_ctx->clients[id_client];
	/* raw buffer of packet struct */
	char *buf;

	buf = (char *)&cs->pkt + cs->recv_len;
	ret = recv(cs->fd, buf, sizeof(cs->pkt), MSG_DONTWAIT);
	if (ret < 0) {
		if (ret == EAGAIN || ret == EINTR)
			return 0;

		perror("recv");
		return -1;
	}

	if (ret == 0) {
		printf("Client disconnected\n");
		close_cl(srv_ctx, id_client);
		return 0;
	}

	cs->recv_len += ret;
	if (process_cl_pkt(cs, srv_ctx) < 0)
		close_cl(srv_ctx, id_client);

	return 0;
}

static int handle_events(struct server_ctx *srv_ctx, int nr_event)
{
	if (srv_ctx->fds[0].revents & POLLIN) {
		if (accept_new_connection(srv_ctx) < 0)
			return -1;

		nr_event--;
	}

	for (size_t i = 1; i <= NR_CLIENT; i++) {
		if (nr_event == 0)
			break;

		if (srv_ctx->fds[i].revents & POLLIN) {
			if (handle_event(srv_ctx, i - 1) < 0)
				return -1;
			nr_event--;
		}
	}

	return 0;
}

static void start_event_loop(struct server_ctx *srv_ctx)
{
	int ret;

	while (1) {
		ret = poll(srv_ctx->fds, NR_CLIENT + 1, -1);

		if (ret < 0) {
			perror("poll");
			break;
		}

		if (handle_events(srv_ctx, ret) < 0)
			break;
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
		close(srv_ctx->tcp_fd);
		return -1;
	}

	srv_ctx->clients = calloc(NR_CLIENT, sizeof(*srv_ctx->clients));
	if (!srv_ctx->clients) {
		perror("calloc");
		free(srv_ctx->fds);
		close(srv_ctx->tcp_fd);
		return -1;
	}

	srv_ctx->fds[0].fd = srv_ctx->tcp_fd;
	srv_ctx->fds[0].events = POLLIN;
	srv_ctx->fds[0].revents = 0;

	for (size_t i = 1; i <= NR_CLIENT; i++)
		srv_ctx->fds[i].fd = -1;

	for (size_t i = 0; i < NR_CLIENT; i++)
		srv_ctx->clients[i].fd = -1;

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

	if (initialize_ctx(&srv_ctx) < 0)
		return -1;
	
	start_event_loop(&srv_ctx);
	clean_ctx(&srv_ctx);

	return 0;
}