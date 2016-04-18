#include <Arduino.h>
#include <string.h>

#include "remote_driver.h"
#include "config.h"

static void remote_set(uint8_t socket, uint8_t mode);
static void send_code(unsigned long code);

static const unsigned int codes[SOCKETS][2] = {
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


void
remote_init(void)
{
	pinMode(TX_PIN, OUTPUT);
	pinMode(LED_PIN, OUTPUT);

	digitalWrite(TX_PIN, LOW);
	digitalWrite(LED_PIN, LOW);

	action.execute = false;
}

void
remote_check_action()
{
	if (action.execute == false)	{
		return;
	}
	action.execute = false;

	for (uint8_t i=0; i<4; i++)	{
		if (action.sockets[i] == 0) break;
		remote_set(action.sockets[i], action.mode);
	}
}

void
remote_execute_once(uint8_t *sockets, uint8_t mode)
{
	action.mode = mode;
	action.execute = true;
	memcpy(action.sockets, sockets, 4);
}

static void
remote_set(uint8_t socket, uint8_t mode)
{
	digitalWrite(LED_PIN, HIGH);
	if (mode == ON)	{
		send_code(codes[socket-1][1]);
	} else {
		send_code(codes[socket-1][0]);
	}
	digitalWrite(LED_PIN, LOW);
}

/*
 * implementation originally from the "RemoteSwitch" Arduino library
 * https://bitbucket.org/fuzzillogic/433mhzforarduino/
 */
static void
send_code(unsigned long code)	{
  code &= 0xfffff; /* truncate to 20 bit */
  /* convert the base3-code to base4, to avoid lengthy calculations when
   * transmitting.. messes op timings.
   * also note this swaps endianess in the process. the msb must be transmitted
   * first, but is converted to lsb here. this is easier when actually
   * transmitting later on.
   */
  unsigned long data_base4 = 0;
  for (byte i=0; i<12; i++) {
    data_base4 <<= 2;
    data_base4 |= (code%3);
    code /= 3;
  }

  for (int j=0; j<SEND_REPEAT; j++) {
    /* sent one telegram */
    /* recycle code as working var to save memory */
    code = data_base4;
    for (uint8_t i=0; i<12; i++)	{
      switch (code & 0x03)	{
        case 0:
          digitalWrite(TX_PIN, HIGH);
          delayMicroseconds(SEND_PERIOD);
          digitalWrite(TX_PIN, LOW);
          delayMicroseconds(SEND_PERIOD*3);
          digitalWrite(TX_PIN, HIGH);
          delayMicroseconds(SEND_PERIOD);
          digitalWrite(TX_PIN, LOW);
          delayMicroseconds(SEND_PERIOD*3);
          break;
        case 1:
          digitalWrite(TX_PIN, HIGH);
          delayMicroseconds(SEND_PERIOD*3);
          digitalWrite(TX_PIN, LOW);
          delayMicroseconds(SEND_PERIOD);
          digitalWrite(TX_PIN, HIGH);
          delayMicroseconds(SEND_PERIOD*3);
          digitalWrite(TX_PIN, LOW);
          delayMicroseconds(SEND_PERIOD);
          break;
        case 2: // KA: X or float
          digitalWrite(TX_PIN, HIGH);
          delayMicroseconds(SEND_PERIOD);
          digitalWrite(TX_PIN, LOW);
          delayMicroseconds(SEND_PERIOD*3);
          digitalWrite(TX_PIN, HIGH);
          delayMicroseconds(SEND_PERIOD*3);
          digitalWrite(TX_PIN, LOW);
          delayMicroseconds(SEND_PERIOD);
          break;
      }
      /* next trit */
      code >>= 2;
    }
	/* send termination/synchronization-signal. total length: 32 periods */
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(SEND_PERIOD);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(SEND_PERIOD*31);
  }
}
