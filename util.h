#ifndef _UTIL_h
#define _UTIL_h

#define SIZE_MSG ((1 << 16) - 1)
#define PORT 8001

#ifndef __packed
#define __packed __attribute__((__packed__))
#endif

struct data {
        uint16_t len;
        char msg[];
} __packed;

int get_input_and_send(int fd, struct data *d);
int receive_data(int fd, struct data *d, const char *type);

#endif