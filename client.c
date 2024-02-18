#include <errno.h>
#include <stdlib.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>

#include "util.h"
#include "packet.h"

#define PLACE_HOLDER "ME > "
struct client_ctx {
	/* tcp sockstream FD */
	int 		tcp_fd;
	/* contain `STDIN_FILENO` and sockstream FD */
	struct pollfd 	fds[2];
	/* packet used as protocol to communicate in tcp network */
	struct packet	pkt;
	/* keep track the packet */
	size_t		recv_len;
	/* contain the data received from fgets func */
	char		msg[MAX_SIZE_MSG];
	/* implment DRY principle with this member */
	bool		need_reload_prompt;
};

static int connect_server(void)
{
	int fd;
	struct sockaddr_in addr;
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	inet_pton(AF_INET, server_addr, &addr.sin_addr);
	addr.sin_port = htons(SERVER_PORT);
	addr.sin_family = AF_INET;
	
	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("connect");
		close(fd);
		return -1;
	}

	return fd;
}

/**
 * https://github.com/Reyuki-san/chat_app/commit/4fa068256a113295338e340a1b87a0d24cb8111b?diff=split
 * simulate short recv to make sure the server can handle that
 * see the below links for further explanation:
 * https://github.com/Reyuki-san/chat_app/commit/14b61d5e062dffcfab8400f0932b32be0d0a2591
 * https://t.me/GNUWeeb/849129
 * https://t.me/GNUWeeb/852296
*/
static int send_message(struct client_ctx *cl_ctx, size_t len)
{
	struct packet *pkt;
	size_t pkt_len;

	pkt = malloc(sizeof(*pkt));
	if (!pkt) {
		perror("malloc");
		return -1;
	}

	pkt_len = prep_pkt_msg(pkt, cl_ctx->msg, len);
	if (send(cl_ctx->tcp_fd, pkt, pkt_len, 0) < 0) {
		perror("send");
		free(pkt);
		return -1;
	}

	free(pkt);
	return 0;
}

static int process_user_input(struct client_ctx *cl_ctx, size_t len)
{
	if (!strcmp(cl_ctx->msg, "exit"))
		return -1;

	if (!strcmp(cl_ctx->msg, "clear")) {
		printf("\ec");
		fflush(stdout);
		return 0;
	}

	return send_message(cl_ctx, len);
}

static int handle_user_input(struct client_ctx *cl_ctx)
{
	size_t len;

	if (!fgets(cl_ctx->msg, sizeof(cl_ctx->msg), stdin))
		return -1;

	len = strlen(cl_ctx->msg);

	if (len == 1) {
		cl_ctx->need_reload_prompt = true;
		return 0;
	}

	if (cl_ctx->msg[len - 1] == '\n') {
		cl_ctx->msg[len - 1] = '\0';
		len--;
	}

	if (process_user_input(cl_ctx, len + 1) < 0)
		return -1;

	cl_ctx->need_reload_prompt = true;
	return 0;
}

static int process_srv_packet(struct client_ctx *cl_ctx)
{
	size_t expected_len;
	
try_again:
	if (cl_ctx->recv_len < HEADER_SIZE)
		return 0;

	expected_len = ntohs(cl_ctx->pkt.len) + HEADER_SIZE;

	if (cl_ctx->recv_len < expected_len)
		return 0;

	switch (cl_ctx->pkt.type) {
	case SR_PKT_LEAVE:
		printf("\r%s left the server\n", cl_ctx->pkt.event.identity);
		break;
	case SR_PKT_JOIN:
		printf("\r%s joined the server\n", cl_ctx->pkt.event.identity);
		break;
	case SR_PKT_MSG_ID:
		printf("\r%s > %s\n", cl_ctx->pkt.msg_id.identity, cl_ctx->pkt.msg_id.msg.data);
		break;

	default:
		printf("Invalid packet %hhu\n", cl_ctx->pkt.type);
		return -1;
	}

	cl_ctx->recv_len -= expected_len;
	if (cl_ctx->recv_len > 0) {
		char *dest = (char *)&cl_ctx->pkt;
		char *src = dest + expected_len;
		/* you can reproduce buffer overflow on the memmove with this commit: https://github.com/Reyuki-san/chat_app/blob/7b409a8326c6d784e2fec290173d7d0562bef0f5/client.c#L160 */
		memmove(dest, src, cl_ctx->recv_len);
		goto try_again;
	}

	return 0;
}

static int handle_server(struct client_ctx *cl_ctx)
{
	char *buf;
	ssize_t ret;

	buf = (char *)&cl_ctx->pkt + cl_ctx->recv_len;
	ret = recv(cl_ctx->tcp_fd, buf, sizeof(cl_ctx->pkt) - cl_ctx->recv_len, MSG_DONTWAIT);
	if (ret < 0) {
		if (errno == EAGAIN || errno == EINTR)
			return 0;

		perror("recv");
		return -1;
	}

	if (ret == 0) {
		printf("\nServer disconnected\n");
		return -1;
	}
	
	cl_ctx->recv_len += ret;
	if (process_srv_packet(cl_ctx) < 0)
		return -1;

	cl_ctx->need_reload_prompt = true;

	return 0;
}

static int handle_events(struct client_ctx *cl_ctx)
{
	if (cl_ctx->fds[0].revents & POLLIN) {
		if (handle_server(cl_ctx) < 0)
			return -1;
	}

	if (cl_ctx->fds[1].revents & POLLIN) {
		if (handle_user_input(cl_ctx) < 0)
			return -1;
	}

	return 0;
}

static void start_event_loop(struct client_ctx *cl_ctx)
{
	int nr_ready;

	cl_ctx->need_reload_prompt = true;
	while (1) {
		if (cl_ctx->need_reload_prompt) {
			printf(PLACE_HOLDER);
			fflush(stdout);
			cl_ctx->need_reload_prompt = false;
		}

		nr_ready = poll(cl_ctx->fds, 2, -1);
		if (nr_ready < 0) {
			if (errno == EINTR)
				continue;

			perror("poll");
			break;
		}

		if (handle_events(cl_ctx) < 0)
			break;
	}
}

static int initialize_ctx(struct client_ctx *cl_ctx)
{
	cl_ctx->tcp_fd = connect_server();
	if (cl_ctx->tcp_fd < 0)
		return -1;

	cl_ctx->fds[0].fd = cl_ctx->tcp_fd;
	cl_ctx->fds[0].events = POLLIN;

	cl_ctx->fds[1].fd = STDIN_FILENO;
	cl_ctx->fds[1].events = POLLIN;

	cl_ctx->recv_len = 0;
	memset(&cl_ctx->pkt, 0, sizeof(cl_ctx->pkt));

	return 0;
}

int main(void)
{
	struct client_ctx cl_ctx;
	if (initialize_ctx(&cl_ctx) < 0)
		return -1;

	start_event_loop(&cl_ctx);
	close(cl_ctx.tcp_fd);

	return 0;
}
