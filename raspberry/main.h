#ifndef MAIN_H
#define MAIN_H

#ifndef CONFIGFILE
#define CONFIGFILE "config"
#endif

typedef struct  {
    int port;
	char address[256];
	char pidfile[256];
	char configfile[256];
	char server_secret[256];
	char client_secret[256];
	char user_name[32];
	char group_name[32];

	char print_config;
} config_t;

int daemonize(config_t *config);
int register_signal(int signal, void (*signal_handler)(int));
void handle_sigterm(int signal);
void terminate_process(int, void*);

#endif
