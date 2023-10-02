#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include "util.h"

static const char server_addr[] = "127.0.0.1";
static const uint16_t server_port = PORT;

#define PLACEHOLDER "> "

static int connect_srv(void)
{
	int fd;
	struct sockaddr_in addr;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(server_port);
	inet_pton(AF_INET, server_addr, &addr.sin_addr);

	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("connect");
		close(fd);
		return -1;
	}

	printf("Connected XD\n");
	return fd;
}

static int process_msg(int fd, struct data *d)
{
	if (!strcmp(d->msg, "clear")) {
		printf("\ec");
		printf(PLACEHOLDER);
		fflush(stdout);
		return 0;
	}

	if (!strcmp(d->msg, "exit"))
		return -1;

	size_t len = d->len;
	d->len = htons(d->len);
	if (send(fd, d, sizeof(*d) + len, 0) < 0) {
		perror("send");
		return -1;
	}
	
	printf(PLACEHOLDER);
	fflush(stdout);
	return 0;
}

static int get_input(struct data *d)
{
	size_t len;

	if (!fgets(d->msg, SIZE_MSG, stdin)) {
		printf("EOF!\n");
		return -1;
	}

	len = strlen(d->msg);
	if (d->msg[len -1] == '\n') {
		d->msg[len -1] = '\0';
		len--;
	}

	d->len = len + 1;
	return 0;
}

static int receive_msg(int fd)
{
	struct data_srv *d;
	d = malloc(sizeof(*d) + SIZE_MSG);
	if (!d) {
		perror("malloc");
		return -1;
	}

	if (recv(fd, d, sizeof(*d) + SIZE_MSG, 0) < 0) {
		perror("recv");
		free(d);
		return -1;
	}

	d->data.len = ntohs(d->data.len);
	d->data.msg[d->data.len - 1] = '\0';
	printf("\rSender: %s\nMessage: %s\n", d->sender, d->data.msg);

	printf(PLACEHOLDER);
	fflush(stdout);
	return 0;
}

static void event_loop(int fd)
{
	int ret;
	struct data *d;
	struct pollfd fds[2];

	d = malloc(sizeof(*d) + SIZE_MSG);
	if (!d) {
		perror("malloc");
		return;
	}

	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;
	fds[0].revents = 0;
	fds[1].fd = fd;
	fds[1].events = POLLIN;
	fds[1].revents = 0;

	printf(PLACEHOLDER);
	fflush(stdout);
	while (1) {
		ret = poll(fds, 2, -1);

		if (ret < 0) {
			perror("poll");
			break;
		}

		if (fds[0].revents & POLLIN) {
			if (get_input(d) < 0)
				break;
			if (d->len == 1) {
				printf(PLACEHOLDER);
				fflush(stdout);
				continue;
			}

			if (process_msg(fd, d) < 0)
				break;
		}

		if (fds[1].revents & POLLIN) {
			receive_msg(fd);
		}
	}

	free(d);
}

int main(void)
{
	int tcp_fd = connect_srv();
	if (tcp_fd < 0)
		return -1;
	
	event_loop(tcp_fd);
	close(tcp_fd);
        return 0;
}