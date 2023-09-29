# SPDX-License-Identifier: GPL-2.0-only

CFLAGS = -Wall -Wextra -O2

all: client server

client.o: client.c util.h
server.o: server.c util.h
util.o: util.c util.h

OBJ = util.o

client: $(OBJ) client.o
server: $(OBJ) server.o

clean:
	rm -vf client server *.o

.PHONY: all clean
