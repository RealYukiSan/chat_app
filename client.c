#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "util.h"

static const char server_addr[] = "127.0.0.1";
static const uint16_t server_port = PORT;

static int connect_server(void)
{
	struct sockaddr_in addr;
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	inet_pton(AF_INET, server_addr, &addr.sin_addr);
	addr.sin_port = htons(server_port);
	printf("Connecting to %s:%hu...\n", server_addr, server_port);
	
	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("connect");
		close(fd);
		return -1;
	}	
	
	printf("Successfuly connected\n");
	return fd;
}

static void open_chat(int fd)
{
	int ret;
	struct data *d;
	d = malloc(sizeof(*d) + SIZE_MSG);
	if (!d) {
		perror("malloc");
		return;
	}

	while (1) {
		ret = get_input_and_send(fd, d);
		
		if (ret < 0) 
			break;
		
		printf("Waiting for the server to respond...\n");
		ret = receive_data(fd, d, "Server");
		if (ret == -2) {
			printf("Server disconnected\n");
			break;
		} else if (ret == -1) {
			perror("recv");
			break;
		}

		
	}
	free(d);	
}

int main(void)
{
	int tcp_fd;
	tcp_fd = connect_server();

	if (tcp_fd < 0) {
		return 1;
	}

	open_chat(tcp_fd);

	close(tcp_fd);
	return 0;
}