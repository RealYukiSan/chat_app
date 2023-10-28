#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>

#ifndef __packed
#define __packed __attribute__((__packed__))
#endif

#define MAX_SIZE_MSG 4096
#define HEADER_SIZE 4
#define IP4_IDENTITY_SIZE INET_ADDRSTRLEN + sizeof(":65535")

enum {
	CL_PKT_MSG = 10,

	SR_PKT_MSG_ID = 20,
	SR_PKT_JOIN = 21,
	SR_PKT_LEAVE = 22,
};

struct packet_msg_leave {
	char	identity[IP4_IDENTITY_SIZE];
} __packed;

struct packet_msg_join {
	char	identity[IP4_IDENTITY_SIZE];
} __packed;

struct packet_msg {
	uint16_t	len;
	char		data[];
} __packed;

struct packet_msg_id {
	char			identity[IP4_IDENTITY_SIZE];
	struct packet_msg 	msg;
} __packed;

struct packet {
	uint8_t	 	type;
	uint8_t	 	__pad;
	uint16_t	len;
	union {
		/* broadcast data from server to clients */
		struct packet_msg_id	msg_id;
		/* send data from client to server */
		struct packet_msg 	msg;
		char			__raw_buf[4096 + MAX_SIZE_MSG];
	};
};

#endif