#include <stdio.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>

#include "util.h"
#include "packet.h"

#define PLACE_HOLDER "ME> "
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

static int connect_server()
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

static int handle_input(struct client_ctx *cl_ctx)
{
	/**
	 * TODO:
	 * 	sebelum mengirim:
	 * 		- membaca pesan dari input terminal melalui stdin
	 * 		- mempersiapkan paket yang akan dikirim
	 * 	mengirim pesan ke server
	*/

	if (!fgets(cl_ctx->msg, sizeof(cl_ctx->msg), stdin))
		return -1;

	printf("%s", cl_ctx->msg);
	cl_ctx->need_reload_prompt = true;

	return 0;
}

static int handle_events(struct client_ctx *cl_ctx)
{
	if (cl_ctx->fds[0].revents & POLLIN) {
		/**
		 * TODO:
		 * 	setelah menerima:
		 * 		- transform format/mengekstra pesan dengan cara yang benar
		 * 		- menampilkan pesan yang telah terekstrak
		*/
	}

	if (cl_ctx->fds[1].revents & POLLIN) {
		handle_input(cl_ctx);
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
			perror("poll");
			break;
		}

		handle_events(cl_ctx);
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

	return 0;
}

int main(void)
{
	struct client_ctx cl_ctx;
	initialize_ctx(&cl_ctx);
	start_event_loop(&cl_ctx);
	
	return 0;
}