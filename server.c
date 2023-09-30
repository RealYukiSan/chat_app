#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "util.h"

static const char bind_addr[] = "127.0.0.1";
static const uint16_t bind_port = PORT;

static int create_socket(void)
{
        int ret;
        // handle tcp socket file descriptor
        int fd;
        struct sockaddr_in addr;
        // see the list of protocols on netinet/in.h if you wants to specify the protocol instead of 0.
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
                return -1;
        }
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        ret = inet_pton(AF_INET, bind_addr, &addr.sin_addr);
        if (ret < 1) {
                perror("inet_pton: Invalid bind address");
                close(fd);
                return -1;
        };

        addr.sin_port = htons(bind_port);
        ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
        if (ret < 0) {
                perror("bind: port already used");
                close(fd);
                return -1;
        }

        ret = listen(fd, 10);
        if (ret < 0) {
                perror("listen");
                close(fd);
                return -1;
        }

        printf("Successfuly listen on port: %d\n", bind_port);
        
        return fd;
}

static int listening(int fd)
{
        int ret;
        struct sockaddr_in client_addr;
        struct data *data_client;
        int client_fd;
        socklen_t client_len;
        char client_ip[INET_ADDRSTRLEN];
        uint16_t client_port;

        client_len = sizeof(client_addr);
        printf("Waiting for connection...\n");
        client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
                perror("accept");
                return -1;
        }

        data_client = malloc(sizeof(*data_client) + SIZE_MSG);
        if (!data_client) {
                perror("malloc");
                close(client_fd);
                return -1;
        }
        

        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        client_port = ntohs(client_addr.sin_port);
        printf("New client connected with\nip: %s\nport: %hu\n", client_ip, client_port);

        while (1) {
                printf("Waiting for the client to send a message\n");
                ret = receive_data(client_fd, data_client, "Client");
                if (ret == -2) {
			printf("Client disconnected\n");
			break;
		} else if (ret == -1) {
			perror("recv");
			break;
		}

                if (get_input_and_send(client_fd, data_client))
                        break;
        }
        
        close(client_fd);
        free(data_client);
        return 0;
}

static void start_event_loop(int fd)
{
        int ret;
        while (1) {
                ret = listening(fd);
                if (ret < 0) 
                        break;
                
        }
        
}

int main(void)
{
        int tcp_fd;
        tcp_fd = create_socket();
        if (tcp_fd < 0) 
                return 1;
        
        start_event_loop(tcp_fd);
        return 0;
}
