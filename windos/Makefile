CFLAGS := -Wall -Wextra -O2 -ggdb3
LDFLAGS := -lws2_32

ifdef SERVER_ADDR
CFLAGS += -DSERVER_ADDR=\"$(SERVER_ADDR)\"
endif

ifdef SERVER_PORT
CFLAGS += -DSERVER_PORT=$(SERVER_PORT)
endif

HDR = packet.h util.h

all: client server

util.o: util.c util.h
client.o: client.c $(HDR)
server.o: server.c $(HDR)

client: util.o client.o
	$(CC) util.o client.o -o $@ $(LDFLAGS)
server: util.o server.o
	$(CC) util.o server.o -o $@ $(LDFLAGS)

clean:
	rm -vf client server *.o *.db

.PHONY: all clean
