#ifndef _REMOTE_DRIVER_H_
#define _REMOTE_DRIVER_H_

#define ON 0
#define OFF 1

#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

#define SEND_PERIOD 300	/* send codes for 300 usecs */
#define SEND_REPEAT	10	/* repeat sending 10 times */


void remote_init(void);
void remote_check_action(void);
void remote_execute_once(uint8_t *sockets, uint8_t mode);

#endif
