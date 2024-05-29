#include <ctype.h>
#include <enet/enet.h>
#include <stdio.h>
#include <string.h>
#include <sys/poll.h>
#include <unistd.h>

struct pollfd fds[2];
ENetEvent event;
ENetAddress server;
ENetHost *client;
ENetPeer *peer;
char messages[255];

int main(void)
{
	enet_initialize();

	client = enet_host_create(NULL, 1, 2, 0, 0);
	// client->checksum = enet_crc32;
	// client->usingNewPacket = 1;
	// enet_host_compress_with_range_coder(client);

	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;

	fds[1].fd = client->socket;
	fds[1].events = POLLIN;

	puts("connecting to server...");
	server.port = 6969;
	enet_address_set_host(&server, "localhost");
	peer = enet_host_connect(client, &server, 2, 0);
	if (peer == NULL) {
		puts("No available peers for initiating an ENet connection.");
		return -1;
	}

	enet_host_service(client, &event, 5);

	int ret;
	printf("Enter message: ");
	fflush(stdout);
	while (1) {
		ret = poll(fds, 2, -1);
		if (ret < 0) {
			perror("poll");
			return -1;
		}

		if (fds[0].revents & POLLIN) {
			printf("Enter message: ");
			fflush(stdout);
			fgets(messages, sizeof(messages), stdin);
			if (!messages[0])
				continue;

			// remove trailing newline.
			messages[strlen(messages) - 1] = '\0';
			ENetPacket *packet = enet_packet_create(messages, strlen(messages) + 1, ENET_PACKET_FLAG_RELIABLE);
			enet_peer_send(peer, 0, packet);
		} else if(fds[1].revents & POLLIN) {
			if (enet_host_service(client, &event, 5) > 0) {
				switch (event.type) {
					case ENET_EVENT_TYPE_CONNECT:
						puts("connected");
						break;
					case ENET_EVENT_TYPE_RECEIVE:
						printf("\rpacket received: %s\n", event.packet->data);
						printf("Enter message: ");
						fflush(stdout);
						break;
					case ENET_EVENT_TYPE_DISCONNECT:
						puts("disconnected");
						break;
				}
			}
		}
	}

	// for (;;) {
	// 	if (enet_host_service(client, &event, 5) > 0) {
	// 		switch (event.type) {
	// 			case ENET_EVENT_TYPE_CONNECT:
	// 				puts("connected");
	// 				break;
	// 			case ENET_EVENT_TYPE_RECEIVE:
	// 				puts("packet received:");
	// 				for (int i = 0; i < event.packet->dataLength; i++) {
	// 				    char byte = event.packet->data[i];
	// 				    if (isalnum(byte))
	// 					putchar(byte);
	// 				    else
	// 					printf(" %02hhX ", byte);
	// 				}
	// 				break;
	// 			case ENET_EVENT_TYPE_DISCONNECT:
	// 				puts("disconnected");
	// 				break;
	// 		}
	// 	}
	// }

	return 0;
}
