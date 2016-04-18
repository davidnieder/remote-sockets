#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include <inttypes.h>

uint8_t packet_read_byte(uint8_t);
void packet_read_reset(void);
uint8_t *packet_process(void);

#endif
