#ifndef UTIL_H
#define UTIL_H

#ifndef SERVER_PORT
#define SERVER_PORT 8787
#endif

#ifndef SERVER_ADDR
#define SERVER_ADDR "127.0.0.1"
#endif

#ifdef DEBUG
#define DEBUG_PRINT(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG_PRINT(...) { }
#endif

extern const char *server_addr;

#endif
