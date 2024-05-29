#include <enet/enet.h>
#include <stdio.h>

ENetEvent event;
ENetAddress address;
ENetHost *server;
static const char data[] = "acked.";
static char ipstring[INET_ADDRSTRLEN];

int main(void)
{
	enet_initialize();

	address.host = ENET_HOST_ANY;
	address.port = 6969;
	server = enet_host_create(&address, 1, 2, 0, 0);
	// server->checksum = enet_crc32;
	// enet_host_compress_with_range_coder(server);

	puts("STARTING EVENT LOOP...");
	while (1) {
		if (enet_host_service(server, &event, 5) <= 0)
			continue;
		ENetPeer *peer = event.peer;
		switch (event.type) {
			case ENET_EVENT_TYPE_CONNECT:
				inet_ntop(AF_INET, &event.peer->address.host, ipstring, INET_ADDRSTRLEN);
				printf("A new client connected from %s:%u.\n", ipstring, event.peer->address.port);
				break;
			case ENET_EVENT_TYPE_RECEIVE:
				printf("packet received: %s\n", event.packet->data);
				ENetPacket *packet = enet_packet_create(data, sizeof(data) + 1, ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, packet);
				break;
			case ENET_EVENT_TYPE_DISCONNECT:
				puts("disconnected");
				break;
		}
	}

	return 0;
}
