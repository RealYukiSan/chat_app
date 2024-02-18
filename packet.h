#ifndef PACKET_H
#define PACKET_H

#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>

#ifndef __packed
#define __packed __attribute__((__packed__))
#endif

#define MAX_SIZE_MSG 4096
#define HEADER_SIZE 4
#define IP4_IDENTITY_SIZE INET_ADDRSTRLEN + sizeof(":65535")

enum {
	/* sent packet from client to server */
	CL_PKT_MSG = 10,

	/* broadcast packet from server to active client */
	SR_PKT_MSG_ID = 20,
	/* send event info from server to client */
	SR_PKT_JOIN = 21,
	SR_PKT_LEAVE = 22,
};

struct packet_msg {
	uint16_t	len;
	char		data[];
} __packed;

/* Why does the struct order is matter here? see https://t.me/GNUWeeb/854569 for further explanation */
struct packet_msg_id {
	char			identity[IP4_IDENTITY_SIZE];
	struct packet_msg 	msg;
} __packed;

struct packet_msg_event {
	char			identity[IP4_IDENTITY_SIZE];
} __packed;

struct packet {
	uint8_t	 	type;
	uint8_t	 	__pad;
	/**
	 * one question: is this member really needed? I mean, its value can be replicated by sizeof(struct packet) + msg len
	 * answer: ofc if you use sizeof(struct packet) the union size will be picked; in this case, the larger member is __raw_buf
	 * so it is not efficient since the flexible or dynamic array isn't used anymore.
	*/
	uint16_t	len;
	union {
		/* broadcast data from server to clients */
		struct packet_msg_id	msg_id;
		/* send data from client to server */
		struct packet_msg 	msg;
		/* join and leave notifier */
		struct packet_msg_event	event;
		char			__raw_buf[4096 + MAX_SIZE_MSG];
	};
};

static inline size_t prep_pkt_msg_id(struct packet *pkt, const char *id, const char *msg, size_t len)
{
	size_t body_len;

	body_len = sizeof(pkt->msg_id) + len;
	pkt->type = SR_PKT_MSG_ID;
	pkt->len = htons(body_len);
	pkt->msg_id.msg.len = htons(len);
	strncpy(pkt->msg_id.identity, id, sizeof(pkt->msg_id.identity));
	strncpy(pkt->msg_id.msg.data, msg, len);

	return HEADER_SIZE + body_len;
}

static inline size_t prep_pkt_msg(struct packet *pkt, const char *msg, size_t len)
{
	/* Make it dynamic/flexible, Just send occupied size */
	size_t body_len;

	body_len = sizeof(pkt->msg) + len;
	pkt->type = CL_PKT_MSG;
	pkt->len = htons(body_len);
	pkt->msg.len = htons(len);
	memcpy(pkt->msg.data, msg, len);

	return HEADER_SIZE + body_len;
}

static inline size_t prep_pkt_msg_join(struct packet *pkt, const char *id)
{
	pkt->type = SR_PKT_JOIN;
	pkt->len = htons(sizeof(pkt->event));
	strncpy(pkt->event.identity, id, sizeof(pkt->event.identity));

	return HEADER_SIZE + sizeof(pkt->event);
}

static inline size_t prep_pkt_msg_left(struct packet *pkt, const char *id)
{
	pkt->type = SR_PKT_LEAVE;
	pkt->len = htons(sizeof(pkt->event));
	strncpy(pkt->event.identity, id, sizeof(pkt->event.identity));

	return HEADER_SIZE + sizeof(pkt->event);
}

#endif
