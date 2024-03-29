#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stddef.h>
#include <errno.h>

#ifndef _WIN32
#include <sys/socket.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "packet.h"

#define NR_CLIENT 5

static const char db_file_name[] = "chat_history.db";

struct client_state {
	int 			fd;
	struct sockaddr_in 	addr;
	struct packet 		pkt;
	size_t			recv_len;
};

struct server_ctx {
	int 			tcp_fd;
	FILE			*db;
	struct pollfd		*fds;
	struct client_state	*clients;
};

static int create_server(void)
{
	int fd;
	struct sockaddr_in addr;
	int type;

	#ifndef _WIN32
	type = SOCK_STREAM | SOCK_NONBLOCK;
	#else
	type = SOCK_STREAM;
	#endif

	fd = socket(AF_INET, type, 0);
	if (fd < 0) {
		perror("socket");
		return -1;
	}

	#ifdef _WIN32
	u_long mode = 1;
	ioctlsocket(fd, FIONBIO, &mode);
	#endif

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &(const char){1}, sizeof(int));

	memset(&addr, 0, sizeof(addr));
	inet_pton(AF_INET, server_addr, &addr.sin_addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		#ifdef _WIN32
		closesocket(fd);
		#else
		close(fd);
		#endif
		return -1;
	}

	if (listen(fd, NR_CLIENT) < 0) {
		perror("listen");
		#ifdef _WIN32
		closesocket(fd);
		#else
		close(fd);
		#endif
		return -1;
	}

	printf("Server running on %s:%d...\n", server_addr, SERVER_PORT);	
	return fd;
}

static void close_cl(struct server_ctx *srv_ctx, size_t idx)
{
	#ifdef _WIN32
	closesocket(srv_ctx->fds[idx + 1].fd);
	#else
	close(srv_ctx->fds[idx + 1].fd);
	#endif
	srv_ctx->fds[idx + 1].fd = -1;
	srv_ctx->clients[idx].fd = -1;
}

static int broadcast_join(struct server_ctx *srv_ctx, uint32_t idx, const char *addr_str)
{
	struct packet *pkt;
	size_t pkt_len;

	pkt = malloc(sizeof(*pkt));
	if (!pkt) {
		perror("malloc");
		return -1;
	}

	pkt_len = prep_pkt_msg_join(pkt, addr_str);
	for (uint32_t i = 0; i < NR_CLIENT; i++) {
		if (idx == i || srv_ctx->clients[i].fd < 0)
			continue;

		if (send(srv_ctx->clients[i].fd, pkt, pkt_len, 0) < 0) {
			puts("[broadcast_join]");
			#ifndef __WIN32
			perror("send");
			#else
			printf("send: %d\n", WSAGetLastError());
			#endif
			close_cl(srv_ctx, idx);
		}
	}

	free(pkt);

	return 0;
}

static const char *stringify_ipv4(struct sockaddr_in *addr)
{
	static char buf[IP4_IDENTITY_SIZE];
	inet_ntop(AF_INET, &addr->sin_addr, buf, INET_ADDRSTRLEN);
	sprintf(buf + strlen(buf), ":%hu", ntohs(addr->sin_port));
	return buf;
}

static void abort_db_corruption(size_t len, size_t exp_len, const char *desc)
{
	printf("The database is corrupted! %zu != %zu (%s)\n", len, exp_len, desc);
	abort();
}

static int sync_history(struct server_ctx *srv_ctx, int cs_fd)
{
	/**
	 * read file contents
	 * broadcast it to the active client
	*/

	const size_t content_len = offsetof(struct packet_msg_id, msg.data);
	struct packet *pkt;
	pkt = malloc(sizeof(*pkt));
	if (!pkt) {
		perror("malloc");
		return -1;
	}

	rewind(srv_ctx->db);
	while (1) {
		size_t len;
		size_t msg_len;

		len = fread(&pkt->msg_id, 1, content_len, srv_ctx->db);
		if (!len)
			break;
		
		/* cannot read the header, therefore indicating database corruption */
		if (len != content_len)
			abort_db_corruption(len, content_len, "fread");

		msg_len = ntohs(pkt->msg_id.msg.len);
		if (msg_len > MAX_SIZE_MSG)
			abort_db_corruption(len, content_len, "msg len too large");

		len = fread(&pkt->msg_id.msg.data, 1, msg_len, srv_ctx->db);
		if (len != msg_len)
			abort_db_corruption(len, content_len, "mismatch msg len");

		pkt->len = htons(msg_len + sizeof(pkt->msg_id));
		pkt->type = SR_PKT_MSG_ID;
		if (send(cs_fd, pkt, HEADER_SIZE + msg_len + sizeof(pkt->msg_id), 0) < 0) {
			puts("[sync_history]");
			#ifndef __WIN32
			perror("send");
			#else
			printf("send: %d\n", WSAGetLastError());
			#endif
			free(pkt);
			return -1;
		}
	}

	free(pkt);
	return 0;
}

static int plug_client(int fd, struct sockaddr_in addr, struct server_ctx *srv_ctx)
{
	struct client_state *cs = NULL;
	const char *addr_str;
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

	addr_str = stringify_ipv4(&addr);
	printf("New client connected from %s\n", addr_str);

	if (broadcast_join(srv_ctx, i, addr_str) < 0)
		return -1;

	if (sync_history(srv_ctx, cs->fd) < 0)
		close_cl(srv_ctx, i);

	return 0;
}

static int accept_new_connection(struct server_ctx *srv_ctx)
{
	struct client_state cl_state;
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(cl_state.addr);
	int fd;

	fd = accept(srv_ctx->tcp_fd, (struct sockaddr *)&addr, &addr_len);
	DEBUG_PRINT("fd on accept: %d\n", fd);
	#ifdef _WIN32
	u_long mode = 1;
	ioctlsocket(fd, FIONBIO, &mode);
	#endif

	if (fd < 0) {
		int ret = errno;
		if (ret == EINTR || ret == EAGAIN)
			return 0;

		#ifdef __WIN32
		if (WSAGetLastError() == WSAEWOULDBLOCK)
			return 0;
		#endif

		perror("accept");
		return -1;
	}

	if (plug_client(fd, addr, srv_ctx) < 0) {
		printf("Client slot is full, dropping a new connection...\n");
		
		#ifdef _WIN32
		closesocket(fd);
		#else
		close(fd);
		#endif
	}

	return 0;
}

/* store it, just store the following information: msg, msg len, identity */
static int store_msg(struct packet *pkt, FILE *fd)
{
	size_t write_len;

	write_len = ntohs(pkt->msg_id.msg.len) + sizeof(pkt->msg_id);

	clearerr(fd);
	fseek(fd, 0, SEEK_END);
	fwrite(&pkt->msg_id, write_len, 1, fd);
	if (ferror(fd)) {
		perror("fwrite");
		return -1;
	}

	fflush(fd);
	return 0;
}

static int broadcast_msg(struct packet *pkt, struct server_ctx *srv_ctx, struct client_state *cs, size_t pkt_len)
{
	int ret;

	for (size_t i = 0; i < NR_CLIENT; i++) {
		if (cs->fd == srv_ctx->clients[i].fd || srv_ctx->clients[i].fd < 0)
			continue;

		if (send(srv_ctx->clients[i].fd, pkt, pkt_len, 0) < 0) {
			puts("[broadcast_msg]");
			#ifndef __WIN32
			perror("send");
			#else
			printf("send: %d\n", WSAGetLastError());
			#endif
			return -1;
		}
	}

	ret = store_msg(pkt, srv_ctx->db);

	return ret;
}

static int handle_cl_pkt_msg(struct client_state *cs, struct server_ctx *srv_ctx)
{
	size_t msg_len_he;
	size_t pkt_len;
	struct packet *pkt;
	int ret;

	msg_len_he = ntohs(cs->pkt.msg.len);

	if (msg_len_he > MAX_SIZE_MSG)
		return -1;

	if (cs->pkt.msg.data[msg_len_he - 1] != '\0')
		return -1;

	pkt = malloc(sizeof(*pkt));
	if (!pkt) {
		perror("malloc");
		return -1;
	}

	pkt_len = prep_pkt_msg_id(pkt, stringify_ipv4(&cs->addr), cs->pkt.msg.data, msg_len_he);

	ret = broadcast_msg(pkt, srv_ctx, cs, pkt_len);
	printf("New message from %s = %s\n", pkt->msg_id.identity, pkt->msg_id.msg.data);
	free(pkt);

	return ret;
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
		if (handle_cl_pkt_msg(cs, srv_ctx) < 0)
			return -1;
		break;

	default:
		printf("Invalid packet\n");
		return -1;
	}

	cs->recv_len -= expected_len;
	if (cs->recv_len > 0) {
		/**
		 * the terms packed: just sent occupied size to the server
		 * and unpacked is the opposite: adding redundant padding
		 * the condition will not be met until short recv occurs
		 * and also either short or multiple *unpacked* packet will met this condition but result in an Invalid packet 
		 * actually invalid packet (like just send partial data, not included message) but simulated in short recv will work just fine, but will be buggy
		 * multiple packet that are packed will not going to this condition, it will just work as expected, no buf overf, no vuln.
		*/
		char *dest = (char *)&cs->pkt;
		char *src = dest + expected_len;
		memmove(dest, src, cs->recv_len);
		goto try_again;
	}

	return 0;
}

static int broadcast_leave(struct server_ctx *srv_ctx, struct client_state *cs)
{
	struct packet *pkt;

	pkt = malloc(sizeof(*pkt));
	if (!pkt) {
		perror("malloc");
		return -1;
	}

	prep_pkt_msg_left(pkt, stringify_ipv4(&cs->addr));
	for (size_t i = 0; i < NR_CLIENT; i++) {
		if (cs == &srv_ctx->clients[i] || srv_ctx->clients[i].fd < 0)
			continue;

		if (send(srv_ctx->clients[i].fd, pkt, HEADER_SIZE + IP4_IDENTITY_SIZE, 0) < 0) {
			puts("[broadcast_leave]");
			#ifndef __WIN32
			perror("send");
			#else
			printf("send: %d\n", WSAGetLastError());
			#endif
			close_cl(srv_ctx, i);
		}
	}

	free(pkt);

	return 0;
}

static int handle_event(struct server_ctx *srv_ctx, size_t id_client)
{
	ssize_t ret;
	struct client_state *cs = &srv_ctx->clients[id_client];
	/* raw buffer of packet struct */
	char *buf;
	unsigned int flag;

	#ifndef _WIN32
	flag = MSG_DONTWAIT;
	#else
	flag = 0;
	#endif

	buf = (char *)&cs->pkt + cs->recv_len;
	DEBUG_PRINT("fd on recv: %d\n", cs->fd);
	ret = recv(cs->fd, buf, sizeof(cs->pkt) - cs->recv_len, flag);
	if (ret < 0) {
		#ifdef __WIN32
		printf("recv: %d\n", WSAGetLastError());
		#endif
		if (ret == EAGAIN || ret == EINTR)
			return 0;

		/* the client will be disconnected ungracefully */
		perror("recv");
		return -1;
	}

	if (ret == 0) {
		// TODO: find out why the event on disconnect not dispatched
		// it seems the WSAPoll function cannot recognize disconnected client and won't be able to notify us
		// https://github.com/pocoproject/poco/issues/3248
		// SOLVED: ret == 0 on windows moved to if condition POLLHUP
		printf("Client disconnected\n");
		broadcast_leave(srv_ctx, cs);

		return -1;
	}

	cs->recv_len += ret;
	if (process_cl_pkt(cs, srv_ctx) < 0)
		return -1;

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

		#ifdef _WIN32
		if (srv_ctx->fds[i].revents & POLLHUP) {
			printf("Client disconnected\n");
			broadcast_leave(srv_ctx, &srv_ctx->clients[i - 1]);
			close_cl(srv_ctx, i - 1);
			nr_event--;			
		}
		#endif

		if (srv_ctx->fds[i].revents & POLLIN) {
			if (handle_event(srv_ctx, i - 1) < 0)
				close_cl(srv_ctx, i - 1);
			nr_event--;
		}
	}

	return 0;
}

static void start_event_loop(struct server_ctx *srv_ctx)
{
	int ret;
	DEBUG_PRINT("PID: %d\n", getpid());

	while (1) {
		#ifndef __WIN32
		ret = poll(srv_ctx->fds, NR_CLIENT + 1, -1);
		#else
		ret = WSAPoll(srv_ctx->fds, NR_CLIENT + 1, INFINITE);
		#endif

		if (ret < 0) {
			if (errno == EINTR)
				continue;

			perror("poll");
			break;
		}

		if (handle_events(srv_ctx, ret) < 0)
			break;
	}
}

static int initialize_ctx(struct server_ctx *srv_ctx)
{
	#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		printf("Could not find a usable version of Winsock.dll\n");
		WSACleanup();
		return 1;
	} else
		printf("The Winsock 2.2 dll was found okay\n");
	#endif

	srv_ctx->tcp_fd = create_server();
	if (srv_ctx->tcp_fd < 0)
		return -1;

	srv_ctx->fds = calloc(NR_CLIENT + 1, sizeof(*srv_ctx->fds));
	if (!srv_ctx->fds) {
		perror("calloc");
		#ifdef _WIN32
		closesocket(srv_ctx->tcp_fd);
		#else
		close(srv_ctx->tcp_fd);
		#endif
		return -1;
	}

	srv_ctx->clients = calloc(NR_CLIENT, sizeof(*srv_ctx->clients));
	if (!srv_ctx->clients) {
		perror("calloc");
		free(srv_ctx->fds);
		#ifdef _WIN32
		closesocket(srv_ctx->tcp_fd);
		#else
		close(srv_ctx->tcp_fd);
		#endif
		return -1;
	}

	srv_ctx->fds[0].fd = srv_ctx->tcp_fd;
	srv_ctx->fds[0].events = POLLIN;
	srv_ctx->fds[0].revents = 0;

	for (size_t i = 1; i <= NR_CLIENT; i++)
		srv_ctx->fds[i].fd = -1;

	for (size_t i = 0; i < NR_CLIENT; i++)
		srv_ctx->clients[i].fd = -1;

	srv_ctx->db = fopen(db_file_name, "r+");
	/* if the file didn't exist, then create one */
	if (!srv_ctx->db) {
		srv_ctx->db = fopen(db_file_name, "w+");
		/* if something still goes wrong, raise/throw an error */
		if (!srv_ctx->db) {
			perror("fopen");
			#ifdef _WIN32
			closesocket(srv_ctx->tcp_fd);
			#else
			close(srv_ctx->tcp_fd);
			#endif
			free(srv_ctx->fds);
			free(srv_ctx->clients);
			return -1;
		}
		printf("Create new database file...\n");
	} else {
		printf("Load the database...\n");
	}

	return 0;
}

static void clean_ctx(struct server_ctx *srv_ctx)
{
	#ifdef _WIN32
	WSACleanup();
	#endif
	free(srv_ctx->fds);
	free(srv_ctx->clients);
	fclose(srv_ctx->db);
	#ifdef _WIN32
	closesocket(srv_ctx->tcp_fd);
	#else
	close(srv_ctx->tcp_fd);
	#endif
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