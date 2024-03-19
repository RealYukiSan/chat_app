#include <stdlib.h>
#include <sys/types.h>
#include <uv.h>
#include <uv/unix.h>

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
	uv_tcp_t	tcp_fd;
	/* contain `STDIN_FILENO` and sockstream FD */
	struct pollfd 	fds[2];
	/* packet used as protocol to communicate in tcp network */
	const struct packet *pkt;
	/* keep track the packet */
	size_t		recv_len;
	/* contain the data received from fgets func */
	char		msg[MAX_SIZE_MSG];
	/* implment DRY principle with this member */
	bool		need_reload_prompt;
};

struct client_ctx cl_ctx;
uv_fs_t stdin_watcher;
uv_loop_t *loop;

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;


void free_write_req(uv_write_t *req) {
    write_req_t *wr = (write_req_t*) req;
    free(wr->buf.base);
    free(wr);
}

void on_write(uv_write_t *req, int status) {
	if (status)
		fprintf(stderr, "Write error %s\n", uv_strerror(status));
	free_write_req(req);
}

static void print_label(void)
{
	printf(PLACE_HOLDER);
	fflush(stdout);
}

static int send_message(void) {
	struct packet pkt;
	size_t pkt_len;
	write_req_t *req;

	pkt_len = prep_pkt_msg(pkt, cl_ctx.msg, stdin_watcher.result);
	req = (write_req_t*)malloc(sizeof(write_req_t));
	req->buf = uv_buf_init((char *)&pkt, pkt_len);
	uv_write((uv_write_t *)req, (uv_stream_t *)&cl_ctx.tcp_fd, &req->buf, 1, on_write);

	return 0;
}

static int process_user_input(void)
{
	if (!strcmp(cl_ctx.msg, "exit"))
		return -1;

	if (!strcmp(cl_ctx.msg, "clear")) {
		#ifdef _WIN32
		system("cls");
		#else
		printf("\ec");
		#endif
		fflush(stdout);
		return 0;
	}

	return send_message();
}

static void on_type(uv_fs_t *req)
{
	if (stdin_watcher.result < 1) {
		print_label();
		return;
	}

	cl_ctx.msg[stdin_watcher.result] = '\0';

	process_user_input();
	print_label();
	uv_buf_t buf = uv_buf_init(cl_ctx.msg, MAX_SIZE_MSG);
	uv_fs_read(loop, &stdin_watcher, 0, &buf, 1, -1, on_type);
}

static void process_srv_packet(uv_stream_t *srv, ssize_t nread, const uv_buf_t *buf)
{
	cl_ctx.pkt = (const struct packet *)buf->base;
	size_t expected_len;
	
	if (nread < HEADER_SIZE)
		return;

	expected_len = ntohs(cl_ctx.pkt->len) + HEADER_SIZE;

	if (cl_ctx.recv_len < expected_len)
		return;

	switch (cl_ctx.pkt->type) {
	case SR_PKT_LEAVE:
		printf("\r%s left the server\n", cl_ctx.pkt->event.identity);
		break;
	case SR_PKT_JOIN:
		printf("\r%s joined the server\n", cl_ctx.pkt->event.identity);
		break;
	case SR_PKT_MSG_ID:
		printf("\r%s > %s\n", cl_ctx.pkt->msg_id.identity, cl_ctx.pkt->msg_id.msg.data);
		break;

	default:
		printf("Invalid packet %hhu\n", cl_ctx.pkt->type);
	}

	free((void *)cl_ctx.pkt);
	free(srv);
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
  buf->base = malloc(suggested_size);
  buf->len = suggested_size;
}

static void on_connect(uv_connect_t *req, int status)
{
	if (status < 0) {
		fprintf(stderr, "connect failed error %s\n", uv_err_name(status));
		free(req);
		return;
	}

	uv_read_start((uv_stream_t *)req->handle, alloc_buffer, process_srv_packet);
	print_label();

	return;
}

int main(void)
{
	loop = uv_default_loop();
	uv_buf_t buf = uv_buf_init(cl_ctx.msg, MAX_SIZE_MSG);
	uv_connect_t *connect_req = (uv_connect_t *)malloc(sizeof(uv_connect_t));
	struct sockaddr_in addr;

	uv_fs_read(loop, &stdin_watcher, 0, &buf, 1, -1, on_type);
	uv_tcp_init(loop, &cl_ctx.tcp_fd);
	uv_ip4_addr(SERVER_ADDR, SERVER_PORT, &addr);
	uv_tcp_connect(connect_req, &cl_ctx.tcp_fd, (const struct sockaddr *)&addr, on_connect);
	print_label();

	return 0;
}
