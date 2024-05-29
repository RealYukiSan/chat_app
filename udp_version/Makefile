# Define variables
ENET_DIR = ./thirdparty/enet
ENET_SRC = $(wildcard $(ENET_DIR)/*.c)
ENET_OBJS = $(ENET_SRC:.c=.o)
ENET_LIB = $(ENET_DIR)/libenet.a

# Targets
all: server client

# Compile ENet source files into object files
$(ENET_OBJS): $(ENET_SRC)
	cd $(ENET_DIR) && gcc -c *.c

# Create the ENet static library
$(ENET_LIB): $(ENET_OBJS)
	ar rcs $@ $(ENET_OBJS)

# Compile the server
server: server.c $(ENET_LIB)
	gcc server.c $(ENET_LIB) -o server

# Compile the client
client: client.c $(ENET_LIB)
	gcc client.c $(ENET_LIB) -o client

# Clean up object files and binaries
clean:
	rm -vf $(ENET_DIR)/{*.o,*.a} $(ENET_LIB) server client

.PHONY: all clean

