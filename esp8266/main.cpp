#include <stdio.h>
#include <ESP8266WiFi.h>

extern "C"	{
	#include "main.h"
	#include "config.h"
	#include "crypto.h"
	#include "protocol.h"
	#include "defines.h"
	#include "remote_driver.h"
}



WiFiServer server(LISTEN_PORT);
WiFiClient client;

void
setup()
{
	remote_init();

	Serial.begin(BAUD_RATE);
	Serial.println("ESP8266 started");

	Serial.print("Connecting to ");
	Serial.println(WIFI_SSID);

	/* connect to network */
	WiFi.mode(WIFI_STA);
	WiFi.begin(WIFI_SSID, WIFI_PWD);

	uint8_t i = 0;
	while (WiFi.status() != WL_CONNECTED && i++ < 20)	{
		Serial.print(".");
		delay(500);
	}

	if (WiFi.status() != WL_CONNECTED)	{
		Serial.print("Could not connect to WiFi: ");
		Serial.println(WIFI_SSID);
		Serial.println("Sleeping");

		/* TODO go to actual power down mode */
		while (1)
			delay(500);
	}

	/* network connection established */
	Serial.print("Connected to Network. Got address: ");
	Serial.println(WiFi.localIP());

	/* initialize crypto keys */
	crypto_keys_init();

	/* start tcp server */
	server.begin();
	server.setNoDelay(1);

	Serial.print("Server started. Listening on port ");
	Serial.println(LISTEN_PORT);
}

void
loop()
{
	/* check if network connection was lost */
	if (WiFi.status() != WL_CONNECTED)	{
		Serial.println("Lost network connection! Restarting...");
		ESP.restart();
	}

	/* check tcp server for new connection */
	if (server.hasClient())	{
		if (!client)	{
			client = server.available();

			Serial.print("Accepted connection from ");
			Serial.println(client.remoteIP());

			Serial.print("waiting for ");
			Serial.print(NETWORK_PACKET_SIZE);
			Serial.println(" bytes");
		} else {
			server.available().stop();
			Serial.println("Had to reject connection");
		}
	}

	/* client closed connection? */
	if (client && !client.connected())	{
		client.stop();
		packet_read_reset();

		Serial.println("Connection closed by peer");
	}

	/* client has new data? */
	if (client.available())	{
		while (client.available())	{
			int8_t ret;
			ret = packet_read_byte(client.read());

			if (ret == READ_WAIT)	{
				continue;
			}
			if (ret == READ_COMPLETE)	{
				uint8_t *response;
				response = packet_process();

				client.write(&response[0], NETWORK_PACKET_SIZE);
			}

			client.flush();
			client.stop();

			Serial.println("Closed connection");
		}
	}

	remote_check_action();
}

void
uart_write(int8_t *bytes)	{
	Serial.print((char *) bytes);
}
