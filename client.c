#include <errno.h>
#include <stdlib.h>

#ifndef _WIN32
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#define STDIN_FILENO __fileno(stdin)
#endif

#include <stdio.h>
#include <unistd.h>

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

	#ifdef _WIN32
	u_long mode = 1;
	ioctlsocket(fd, FIONBIO, &mode);
	#endif

	memset(&addr, 0, sizeof(addr));
	inet_pton(AF_INET, server_addr, &addr.sin_addr);
	addr.sin_port = htons(SERVER_PORT);
	addr.sin_family = AF_INET;

	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		#ifndef _WIN32
		perror("connect");
		close(fd);
		#else
		if (WSAGetLastError() == WSAEWOULDBLOCK)
			return fd;
		// possible error: https://learn.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
		printf("connect: %d\n", WSAGetLastError());
		closesocket(fd);
		#endif

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
		#ifndef __WIN32
		perror("send");
		#else
		printf("send: %d\n", WSAGetLastError());
		#endif
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
		#ifdef _WIN32
		system("cls");
		#else
		printf("\ec");
		#endif
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
	unsigned int flag;

	#ifndef _WIN32
	flag = MSG_DONTWAIT;
	#else
	flag = 0;
	#endif

	buf = (char *)&cl_ctx->pkt + cl_ctx->recv_len;
	ret = recv(cl_ctx->tcp_fd, buf, sizeof(cl_ctx->pkt) - cl_ctx->recv_len, flag);
	if (ret < 0) {
		#ifdef _WIN32
		if (WSAGetLastError() == WSAEWOULDBLOCK)
			return 0;
		DEBUG_PRINT("error code: %d\n", WSAGetLastError());
		#else
		if (errno == EAGAIN || errno == EINTR)
			return 0;

		perror("recv");
		#endif
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

#ifndef __WIN32
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
#else
static int handle_events(struct client_ctx *cl_ctx, int nr_ready)
{
	if (nr_ready == WAIT_OBJECT_0 + 1) {
		if (handle_user_input(cl_ctx) < 0)
			return -1;
	}

	if (nr_ready == WAIT_OBJECT_0) {
		if (handle_server(cl_ctx) < 0)
			return -1;
	}

	return 0;
}
#endif

static void start_event_loop(struct client_ctx *cl_ctx)
{
	int nr_ready;
	DEBUG_PRINT("Program ID: %d\n", getpid());

	cl_ctx->need_reload_prompt = true;
	while (1) {
		if (cl_ctx->need_reload_prompt) {
			printf(PLACE_HOLDER);
			fflush(stdout);
			cl_ctx->need_reload_prompt = false;
		}

		#ifndef __WIN32
		nr_ready = poll(cl_ctx->fds, 2, -1);
		#else
		HANDLE h[2];
		WSAEVENT recvEvent;

		recvEvent = WSACreateEvent();
		WSAEventSelect(cl_ctx->fds[0].fd, recvEvent, FD_READ);

		h[0] = recvEvent;
		h[1] = GetStdHandle(STD_INPUT_HANDLE);
		nr_ready = WaitForMultipleObjects(2, h, FALSE, INFINITE);
		#endif
		DEBUG_PRINT("nr_ready = %d\n", nr_ready);

		if (nr_ready < 0) {
			if (errno == EINTR)
				continue;

			perror("poll");
			break;
		}

		if (
			#ifdef __WIN32
			handle_events(cl_ctx, nr_ready)
			#else
			handle_events(cl_ctx)
			#endif
			< 0)
			break;
	}
}

static int initialize_ctx(struct client_ctx *cl_ctx)
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

	cl_ctx->tcp_fd = connect_server();
	if (cl_ctx->tcp_fd < 0)
		return -1;

	printf("Successfuly connected to %s:%d\n", SERVER_ADDR, SERVER_PORT);

	#ifdef DEBUG
	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);
	getsockname(cl_ctx->tcp_fd, (struct sockaddr *)&client_addr, &len);
	DEBUG_PRINT("Client port: %d\n", ntohs(client_addr.sin_port));
	#endif

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
	#ifdef _WIN32
	closesocket(cl_ctx.tcp_fd);
	WSACleanup();
	#else
	close(cl_ctx.tcp_fd);
	#endif

	return 0;
}
