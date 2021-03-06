#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "protocol.h"
#include "defines.h"
#include "crypto.h"
#include "remote.h"


/* declaration module functions */
static uint8_t packet_sanity_check(uint8_t *packet, size_t packet_size);
static void make_response(uint8_t type, uint8_t *buf);
static void make_network_packet(uint8_t *message, uint8_t *network_packet);
static uint8_t parse_command(uint8_t *command);
static uint8_t parse_set_command(uint8_t *set_command);


uint8_t packet_out[NETWORK_PACKET_SIZE+1];

uint8_t *packet_process(uint8_t *packet_in)
{
	uint8_t ret;
	uint8_t message[MESSAGE_SIZE];

	/* simple surface check of incoming data */
	ret = packet_sanity_check(packet_in, NETWORK_PACKET_SIZE);
	if (ret != OK)	{
		make_response(NACK, packet_out);
		return packet_out;
	}

	/* verify hmac and decrypt data */
	ret = verify_and_decrypt_client_message(packet_in, message);
	if (ret == HMAC_FAILURE)	{
		make_response(HMAC_FAILURE, packet_out);
		return packet_out;
	}
	else if (ret == DEC_FAILURE)	{
		make_response(DEC_FAILURE, packet_out);
		return packet_out;
	}

	/* parse received, decrypted command */
	ret = parse_command(message);
	make_response(ret, packet_out);
	return packet_out;
}

static uint8_t packet_sanity_check(uint8_t *packet, size_t packet_size)	{
	/* last two bytes must encode '\n' (0x0a) */
	if (!(packet[packet_size-2] == '0' && packet[packet_size-1] == 'a'))	{
		return FAILURE;
	}

	/* packet must only contain [0-9][a-f] */
	for(int i; i<packet_size; i++)	{
		if (!((packet[i] >= '0' && packet[i] <= '9') ||
			  (packet[i] >= 'a' && packet[i] <= 'f')))	{
			return FAILURE;
		}
	}
	return OK;
}

static void make_response(uint8_t type, uint8_t *buf)
{
	uint8_t clear_text[MESSAGE_SIZE];
	uint8_t cipher_text[CRYPTO_PACKET_SIZE];

	switch (type)	{
	case ACK:
		strcpy((char *) clear_text, "ack");
		break;
	case NACK:
		strcpy((char *) clear_text, "nack");
		break;
	case HMAC_FAILURE:
		strcpy((char *) clear_text, "nack:hmac");
		break;
	case DEC_FAILURE:
		strcpy((char *) clear_text, "nack:decrypt");
		break;
	case ACK_SET:
		strcpy((char *) clear_text, "ack:set");
		break;
	case NACK_SET:
		strcpy((char *) clear_text, "nack:set");
		break;
	case ACK_PING:
		strcpy((char *) clear_text, "ack:ping");
		break;
	case NACK_CMD:
		strcpy((char *) clear_text, "nack:command");
		break;
	}

	encrypt_and_hmac_server_message(clear_text, strlen((char*) clear_text), cipher_text);
	make_network_packet(cipher_text, buf);
}

/* trusts that:
 * - message contains 'CRYPTO_PACKET_SIZE' bytes
 * - network_packet is at lest NETWORK_PACKET_SIZE bytes wide
 */
static void make_network_packet(uint8_t *message, uint8_t *network_packet)
{
	for (uint8_t i=0; i<CRYPTO_PACKET_SIZE; i++)	{
		sprintf((char *) &network_packet[i*2], "%02x", (int) message[i]);
	}
	network_packet[NETWORK_PACKET_SIZE-2] = '0';
	network_packet[NETWORK_PACKET_SIZE-1] = 'a';
}

uint8_t parse_command(uint8_t *command)
{
	if (strncmp((char *) command, "set", 3) == 0)	{
		return parse_set_command(command);
	}
	else if (strncmp((char *) command, "ping", 4) == 0)	{
		return ACK_PING;
	}

	return NACK_CMD;
}

uint8_t parse_set_command(uint8_t *set_command)
{
	char *command, *ports, *action;
	char *delemiter = ":", *delemiter_ports = ",";
	uint8_t port_list[4];
	char *port_string;

	command = strtok((char *) set_command, delemiter);
	ports = strtok(NULL, delemiter);
	action = strtok(NULL, delemiter);

	/* parse packet */
	if (command == NULL || ports == NULL || action == NULL)	{
		return NACK_SET;
	}

	/* parse "command" */
	if (strcmp((char *) command, "set") != 0)	{
		return NACK_SET;
	}

	/* parse "ports" */
	/* initialize port_list array */
	for (int i=0; i<4; i++)	{
		port_list[i] = 0;
	}
	/* parse first, mandatory port number */
	port_list[0] = atoi(strtok(ports, delemiter_ports));
	if (port_list[0] < 1 || port_list[0] > 4)	{
		return NACK_SET;
	}

	/* parse more, optional port numbers */
	for (int i=1; i<4; i++)	{
		port_string = strtok(NULL, delemiter_ports);
		if (port_string == NULL) break;

		port_list[i] = atoi(port_string);
		if (port_list[i] < 1 || port_list[i] > 4)	{
			return NACK_SET;
		}
	}

	/* parse "action" */
	if (strcmp(action, "on") != 0 && strcmp(action, "off") != 0)	{
		return NACK_SET;
	}

	/* execute command */
	if (strcmp(action, "on") == 0)	{
		remote_execute_once(port_list, ON);
	} else {
		remote_execute_once(port_list, OFF);
	}

	return ACK_SET;
}

