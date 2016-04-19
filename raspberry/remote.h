#ifndef _REMOTE_H_
#define _REMOTE_H_

#include <stdint.h>

#ifndef TXPIN
#define TXPIN 2
#endif

#ifndef LEDPIN
#define LEDPIN 17
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define OFF 0
#define ON  1


int remote_init(void);
void remote_check_action(void);
void remote_execute_once(uint8_t *sockets, uint8_t mode);

#endif
