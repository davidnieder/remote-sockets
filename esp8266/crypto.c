#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "osapi.h"
#include "crypto.h"
#include "defines.h"
#include "config.h"


/* module functions */
static uint8_t verify_client_hmac(uint8_t *message, size_t message_length,
		uint8_t *message_hmac);
static uint8_t decrypt_client_message(uint8_t *message, size_t message_length,
		uint8_t *dest, size_t dest_length);
static void encrypt_server_message(uint8_t *message, uint8_t *dest);
static void add_padding_ascii(uint8_t *buf, size_t len);
static uint8_t check_padding_ascii(uint8_t *buf);
static void remove_padding_ascii(uint8_t *buf);
static void hex_to_binary(uint8_t *str, size_t str_size, uint8_t *dest);

/* module globals */
static uint8_t server_aes_key[32];
static uint8_t server_hmac_key[32];
static uint8_t client_aes_key[32];
static uint8_t client_hmac_key[32];


void
crypto_keys_init()
{
	size_t ss_len = strlen(SERVER_SECRET);
	size_t cs_len = strlen(CLIENT_SECRET);

	HMAC_SHA256(SERVER_SECRET, ss_len, "sak", 3, server_aes_key);
	HMAC_SHA256(SERVER_SECRET, ss_len, "shk", 3, server_hmac_key);
	HMAC_SHA256(CLIENT_SECRET, cs_len, "cak", 3, client_aes_key);
	HMAC_SHA256(CLIENT_SECRET, cs_len, "chk", 3, client_hmac_key);
}

uint8_t
verify_and_decrypt_client_message(uint8_t *packet, uint8_t *decrypted_message)
{
	uint8_t aes_block[AES_PACKET_SIZE];
	uint8_t hmac_block[HMAC_SIZE];
	uint8_t clear_text[AES_BLOCK_SIZE];


	hex_to_binary(packet, 2*AES_PACKET_SIZE, aes_block);
	hex_to_binary(&packet[2*AES_PACKET_SIZE], 2*HMAC_SIZE, hmac_block);

	if (verify_client_hmac(aes_block, AES_PACKET_SIZE, hmac_block) != OK)	{
		return HMAC_FAILURE;
	}

	if (decrypt_client_message(aes_block, AES_PACKET_SIZE, clear_text, AES_BLOCK_SIZE) != OK)	{
		return DEC_FAILURE;
	}

	memcpy(decrypted_message, clear_text, AES_BLOCK_SIZE);
	return OK;
}

uint8_t
encrypt_and_hmac_server_message(uint8_t *message, size_t message_size,
		uint8_t *dest)
{
	//uint8_t tmp[CRYPTO_PACKET_SIZE];

	if (message_size >= AES_BLOCK_SIZE)	{
		return FAILURE;
	}

	add_padding_ascii(message, message_size);
	//encrypt_server_message(message, tmp);
	//memcpy(dest, tmp, AES_PACKET_SIZE);
	//HMAC_SHA256(tmp, AES_PACKET_SIZE, server_hmac_key, SHA256_SIZE, &dest[AES_PACKET_SIZE]);
	encrypt_server_message(message, dest);
	HMAC_SHA256(dest, AES_PACKET_SIZE, server_hmac_key, SHA256_SIZE, &dest[AES_PACKET_SIZE]);

	return OK;
}

static uint8_t
verify_client_hmac(uint8_t *message, size_t message_length,
		uint8_t *message_hmac)
{
	uint8_t hmac_digest[SHA256_SIZE];
	HMAC_SHA256(message, message_length, client_hmac_key, SHA256_SIZE, hmac_digest);

	for (uint8_t i=0; i<SHA256_SIZE; i++)	{
		if (hmac_digest[i] != message_hmac[i])
			return HMAC_FAILURE;
	}
	return OK;
}

static uint8_t
decrypt_client_message(uint8_t *message, size_t message_length, uint8_t *dest,
		size_t dest_length)
{
	uint8_t iv[AES_IV_SIZE];
	uint8_t aes_block[AES_BLOCK_SIZE];
	uint8_t padding_byte;

	if (message_length > CRYPTO_PACKET_SIZE || dest_length < AES_BLOCK_SIZE)	{
		return FAILURE;
	}

	memcpy(iv, message, AES_IV_SIZE);
	memcpy(aes_block, &message[AES_IV_SIZE], AES_BLOCK_SIZE);

	AES_CTX ctx;
	AES_set_key(&ctx, client_aes_key, iv, AES_MODE_256);
	AES_convert_key(&ctx);
	AES_cbc_decrypt(&ctx, aes_block, dest, AES_BLOCK_SIZE);

	if (check_padding_ascii(dest) != OK)	{
		return PADDING_FAILURE;
	}
	remove_padding_ascii(dest);

	return OK;
}

static void
encrypt_server_message(uint8_t *padded_message,	uint8_t *dest)
{
	uint8_t iv[AES_IV_SIZE];
	AES_CTX ctx;

	os_get_random(iv, AES_IV_SIZE);
	memcpy(dest, iv, AES_IV_SIZE);

	AES_set_key(&ctx, server_aes_key, iv, AES_MODE_256);
	AES_cbc_encrypt(&ctx, padded_message, &dest[AES_IV_SIZE], AES_BLOCK_SIZE);
}



static void
add_padding_ascii(uint8_t *buf, size_t len)
{
	uint8_t p_value = AES_BLOCK_SIZE - len;
	uint8_t tmp[2];
	sprintf((char *) tmp, "%x", p_value);

	for (uint8_t i=len; i<AES_BLOCK_SIZE; i++)	{
		buf[i] = tmp[0];
	}
}

static uint8_t
check_padding_ascii(uint8_t *buf)
{
	uint8_t tmp[2];
	tmp[0] = buf[AES_BLOCK_SIZE-1];
	tmp[1] = '\0';
	uint8_t p_value = (uint8_t) strtoul((char*) tmp, NULL, 16);

	for (uint8_t i=AES_BLOCK_SIZE-p_value; i<AES_BLOCK_SIZE; i++)	{
		if (buf[i] != buf[AES_BLOCK_SIZE-1])
			return FAILURE;
	}
	return OK;
}

static void
remove_padding_ascii(uint8_t *buf)
{
	uint8_t tmp[2];
	tmp[0] = buf[AES_BLOCK_SIZE-1];
	tmp[1] = '\0';
	uint8_t p_value = (uint8_t) strtoul((char*) tmp, NULL, 16);

	buf[AES_BLOCK_SIZE-p_value] = '\0';
}

/* transform hexadecimal str to binary data */
static void
hex_to_binary(uint8_t *str, size_t str_size, uint8_t *dest)
{
	uint8_t tmp[2];
	for	(uint8_t i=0; i<str_size; i+=2)	{
		memcpy(tmp, &str[i], 2);
		dest[i/2] = (uint8_t) strtoul(tmp, NULL, 16);
	}
}
