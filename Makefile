# SPDX-License-Identifier: GPL-2.0-only

# https://t.me/GNUWeeb/854603 and https://gcc.gnu.org/legacy-ml/gcc-help/2009-02/msg00130.html
CFLAGS = -Wall -Wextra -O2 -ggdb3
# CFLAGS += -DSERVER_ADDR "0.0.0.0"

all: client server

# It seems object files are only needed if the header also has the c file, like util.h and util.c
# but for now, only use HDR instead of OBJ
HDR = util.h packet.h

client.o: client.c $(HDR)
server.o: server.c $(HDR)

client: client.o
server: server.o

clean:
	rm -vf client server *.o *.db

.PHONY: all clean
