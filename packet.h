#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>

#ifndef __packed
#define __packed __attribute__((__packed__))
#endif

enum {
	CL_PKT_MSG = 10,
	CL_PKT_JOIN = 11,

	SR_PKT_MSG = 20,
	SR_PKT_JOIN = 21,
};

struct packet_msg {
	uint16_t	len;
	char		msg[];
} __packed;


struct packet {
	uint8_t	 	type;
	uint8_t	 	__pad;
	uint16_t	len;
	union {
		struct packet_msg	msg;
		char			__raw_buf[4096];
	};	
};

#endif