#ifndef _UTIL_h
#define _UTIL_h

#define SIZE_MSG ((1 << 16) - 1)
#define PORT 8000

struct data {
        uint16_t len;
        char msg[];
};

int get_input_and_send(int fd, struct data *d);
int receive_data(int fd, struct data *d, const char *type);

#endif