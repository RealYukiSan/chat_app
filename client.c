#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#include "util.h"

struct client_ctx {
	int 		tcp_fd;
	struct pollfd 	*fds;
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

static start_event_loop(struct client_ctx *cl_ctx)
{
	/**
	 * TODO:
	 * 	sebelum mengirim:
	 * 		- membaca pesan dari input terminal melalui stdin
	 * 		- mempersiapkan paket yang akan dikirim
	 * 	mengirim pesan ke server
	 * 	menerima pesan dari server
	 * 	setelah menerima:
	 * 		- mengekstra pesan dengan cara yang benar
	 * 		- menampilkan pesan yang telah terekstrak
	*/
}

int main(void)
{
	struct client_ctx cl_ctx;

	cl_ctx.tcp_fd = connect_server();
	if (cl_ctx.tcp_fd < 0)
		return -1;
	
	return 0;
}