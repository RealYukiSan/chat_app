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

all: create_dir $(DESTDIR)/client $(DESTDIR)/server

HDR = ../util.h ../packet.h

create_dir:
	mkdir -p $(DESTDIR)

$(DESTDIR)/client:
	$(CC) -o $(DESTDIR)/client ../client.c

$(DESTDIR)/server:
	$(CC) -o $(DESTDIR)/server ../server.c

clean:
	rm -vfr $(DESTDIR)

.PHONY: all clean
