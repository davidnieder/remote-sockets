#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <syslog.h>

#include "remote.h"

#define GPIO_BASE (0x20000000 + 0x200000) /* memory addr gpio controller */
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define GPIO_SET *(gpio+7)  /* sets   bits which are 1 ignores bits which are 0 */
#define GPIO_CLR *(gpio+10) /* clears bits which are 1 ignores bits which are 0 */

#define SEND_PERIOD 300	/* send codes for 300 usecs */
#define SEND_REPEAT	10	/* repeat sending 10 times */

static volatile unsigned *gpio;
static const unsigned int codes[4][2] = {
	{ 136320, 136316 },
	{ 137292, 137288 },
	{ 137616, 137612 },
	{ 137724, 137720 }
};

static struct {
	uint8_t execute;
	uint8_t mode;
	uint8_t sockets[4];
} action;

static void send_code(unsigned long code);


/*
 * initialize the gpio transmit pin
 */
int remote_init()
{
	int fd;
	void *gpio_map;

	if ((fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
		perror("open()");
		return 1;
	}

	/* map gpio memory page */
	gpio_map = mmap(NULL, getpagesize(), PROT_WRITE, MAP_SHARED, fd, GPIO_BASE);
	close(fd);

	if (gpio_map == MAP_FAILED) {
		perror("mmap()");
		return 1;
	}

	gpio = (volatile unsigned *)gpio_map;

	/* set TXPIN as output */
	INP_GPIO(TXPIN);
	OUT_GPIO(TXPIN);

    /* set LEDPIN as output */
    INP_GPIO(LEDPIN);
    OUT_GPIO(LEDPIN);

	action.execute = FALSE;

	return 0;
}

void
remote_check_action()
{
	if (action.execute == FALSE)	{
		return;
	}
	action.execute = FALSE;

	for (uint8_t i=0; i<4; i++)	{
		if (action.sockets[i] == 0) break;
        GPIO_SET = 1<<LEDPIN;        
		send_code(codes[action.sockets[i]-1][action.mode]);
        GPIO_CLR = 1<<LEDPIN;

		syslog(LOG_INFO, "turning %s socket %i", action.mode ? "on" : "off", action.sockets[i]);
	}
}

void
remote_execute_once(uint8_t *sockets, uint8_t mode)
{
	if (mode == ON || mode == OFF)	{
		action.mode = mode;
		action.execute = TRUE;
		memcpy(action.sockets, sockets, 4);
	}
}

/*
 * send a code with the 433MHz transmitter
 *
 * implementation originally from the "RemoteSwitch" Arduino library
 * https://bitbucket.org/fuzzillogic/433mhzforarduino/
 */
static void
send_code(unsigned long code)   {
	//code &= 0xfffff; /* Truncate to 20 bit */

	unsigned long data_base4 = 0;
	for (uint8_t i=0; i<12; i++) {
		data_base4 <<= 2;
		data_base4 |= (code%3);
		code /= 3;
	}

    for (int j=0; j<SEND_REPEAT; j++)   {
		/* sent one telegram */
		code = data_base4;
		for (uint8_t i=0; i<12; i++) {
			switch (code & 0x03) {
				case 0:
					GPIO_SET = 1<<TXPIN;
					usleep(SEND_PERIOD);
					GPIO_CLR = 1<<TXPIN;
					usleep(SEND_PERIOD*3);
					GPIO_SET = 1<<TXPIN;
					usleep(SEND_PERIOD);
					GPIO_CLR = 1<<TXPIN;
					usleep(SEND_PERIOD*3);
					break;
				case 1:
					GPIO_SET = 1<<TXPIN;
					usleep(SEND_PERIOD*3);
					GPIO_CLR = 1<<TXPIN;
					usleep(SEND_PERIOD);
					GPIO_SET = 1<<TXPIN;
					usleep(SEND_PERIOD*3);
					GPIO_CLR = 1<<TXPIN;
					usleep(SEND_PERIOD);
					break;
				case 2: /* KA: X or float */
					GPIO_SET = 1<<TXPIN;
					usleep(SEND_PERIOD);
					GPIO_CLR = 1<<TXPIN;
					usleep(SEND_PERIOD*3);
					GPIO_SET = 1<<TXPIN;
					usleep(SEND_PERIOD*3);
					GPIO_CLR = 1<<TXPIN;
					usleep(SEND_PERIOD);
					break;
			}
			/* next trit */
			code>>=2;
		}

		/* send termination/synchronization-signal. total length: 32 periods */
		GPIO_SET = 1<<TXPIN;
		usleep(SEND_PERIOD);
		GPIO_CLR = 1<<TXPIN;
		usleep(SEND_PERIOD*31);
	}
}
