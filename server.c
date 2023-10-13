#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#include "util.h"

static int create_server()
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
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		close(fd);
		return -1;
	}

	if (listen(fd, 10) < 0) {
		perror("listen");
		close(fd);
		return -1;
	}

	printf("Server running on %s:%d...\n", server_addr, SERVER_PORT);	
	return fd;
}

static void start_event_loop(int fd)
{
	void *tmp;
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);

	while (1) {
		accept(fd, (struct sockaddr *)&addr, &addr_len);
		
		printf("Client connected\n");
	}
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