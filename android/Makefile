NDK	:= /home/reyuki/Android/Sdk/ndk/26.2.11394342
ARCH	:= arm
API_LEVEL := 32

ifeq ($(ARCH),arm)
TRIPLET := armv7a-linux-androideabi
endif

TOPDIR	:= $(shell pwd)
DESTDIR	:= $(TOPDIR)/$(TRIPLET)/out
CHAT_APP_SRC := ..

TOOLCHAIN := $(NDK)/toolchains/llvm/prebuilt/linux-x86_64

CFLAGS	:= -Wall -Wextra -O2 -ggdb3
CC	:= $(TOOLCHAIN)/bin/$(TRIPLET)$(API_LEVEL)-clang

ifdef SERVER_ADDR
CFLAGS += -DSERVER_ADDR=\"$(SERVER_ADDR)\"
endif

ifdef SERVER_PORT
CFLAGS += -DSERVER_PORT=$(SERVER_PORT)
endif

ifdef DEBUG
CFLAGS += -DDEBUG
endif

all: create_dir $(DESTDIR)/client $(DESTDIR)/server

create_dir:
	@mkdir -p $(DESTDIR)

$(DESTDIR)/client.o: ../client.c ../packet.h
$(DESTDIR)/server.o: ../server.c ../packet.h
$(DESTDIR)/util.o: ../util.c ../util.h

$(DESTDIR)/client: util.o client.o
	$(CC) $^ -o $@

$(DESTDIR)/server: util.o server.o
	$(CC) $^ -o $@

clean:
	rm -vfr $(DESTDIR)

.PHONY: all clean
