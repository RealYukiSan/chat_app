# SPDX-License-Identifier: GPL-2.0-only

# https://t.me/GNUWeeb/854603 and https://gcc.gnu.org/legacy-ml/gcc-help/2009-02/msg00130.html
CFLAGS := -Wall -Wextra -O2 -ggdb3

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
server: util.o server.o

clean:
	rm -vf client server *.o *.db

.PHONY: all clean
