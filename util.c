#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include "util.h"

static void interpret_message(struct data *d, const char *type)
{
	d->len = htons(d->len);
	d->msg[d->len - 1] = '\0';
	printf("From %s: %s\n", type, d->msg);
}

int receive_data(int fd, struct data *d, const char *type)
{
	int ret;

	ret = recv(fd, d, sizeof(*d) + SIZE_MSG - 1, 0);
	if (ret == -1)
		return -1;

	if (ret == 0)
		return -2;

	interpret_message(d, type);
	return 0;
}

static int send_data_to_dst(int fd, struct data *d)
{
	uint16_t len_host = d->len;

	d->len = htons(len_host);
	if (send(fd, d, sizeof(*d) + len_host, 0) < 0) {
		perror("send");
		return -1;
	}
	return 0;
}

int get_input_and_send(int fd, struct data *d)
{
	size_t len;
	printf("Enter the message: ");
	if (!fgets(d->msg, SIZE_MSG, stdin)) {
		perror("EOF!\n");
		return -1;
	}

	len = strlen(d->msg);
	if (d->msg[len - 1] == '\n') {
		d->msg[len] = '\0';
		len--;
	}

	if (len == 0) 
		return 0;

	if (!strcmp(d->msg, "exit"))
		return -1;
		
	d->len = len + 1;
	return send_data_to_dst(fd, d);     
}
