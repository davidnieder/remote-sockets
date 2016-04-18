#ifndef _DEFINES_H_
#define _DEFINES_H_

#include "crypto.h"

#define MESSAGE_SIZE		AES_BLOCK_SIZE
#define CRYPTO_PACKET_SIZE	(AES_PACKET_SIZE+HMAC_SIZE)
#define NETWORK_PACKET_SIZE	(CRYPTO_PACKET_SIZE)*2+2


/* magic numbers */
#define OK		0
#define FAILURE	1

#define READ_WAIT		10
#define READ_COMPLETE	11

#define ACK		12
#define NACK	13

#define HMAC_FAILURE	20
#define DEC_FAILURE		21
#define PADDING_FAILURE	22

#define ACK_SET		30
#define NACK_SET	31
#define ACK_PING	32
#define NACK_CMD	33

#endif
