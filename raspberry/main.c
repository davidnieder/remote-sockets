#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "main.h"
#include "net.h"
#include "crypto.h"
#include "remote.h"


/* module function prototypes */
static void remove_on_exit(char *file);
static int parse_args(int argc, char *argv[], config_t *config);
static int find_config_file(char *buf, ssize_t size);
static int get_config_from_file(char *file, config_t *config);
static void print_config(config_t *config);
static void print_help(char *name);


/*
 * "daemonize" the process
 */
int
daemonize(config_t *config)
{
	struct passwd *pwd;
	struct group *grp;

    /* fork and exit parent */
    pid_t pid = fork();
    if (pid == -1)  {
        return -1;
    }
    else if (pid > 0)   {
        /* parent */
        exit(EXIT_SUCCESS);
    }

    /* new session */
    if (setsid() == -1) {
        return -1;
    }

    /* ignore SIGHUP */
    if (register_signal(SIGHUP, SIG_IGN) == -1) {
        return -1;
    }

    /* catch SIGTERM */
    if (register_signal(SIGTERM, handle_sigterm) == -1) {
        return -1;
    }

    chdir("/");
    umask(0);

	/* get userid from name */
	errno = 0;
	pwd = getpwnam(config->user_name);
	if (pwd == NULL)	{
		if (errno == 0)	{
			fprintf(stderr, "getpwnam(): user not found\n");
		} else {
			perror("getpwname()");
		}
		return -1;
	}
	/* get groupid from name */
	errno = 0;
	grp = getgrnam(config->group_name);
	if (grp == NULL)	{
		if (errno == 0)	{
			fprintf(stderr, "getgrnam(): group not found\n");
		} else {
			perror("getgrname()");
		}
		return -1;
	}

	/* set user and group id */
	setgid(grp->gr_gid);
	setuid(pwd->pw_uid);

	puts("daemon started, loosing control terminal");
	syslog(LOG_NOTICE, "daemon started, pid %i", getpid());

    /* close stdin, out, err */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    return 0;
}

/*
 * register a signal handler
 */
int
register_signal(int signal, void (*signal_handler)(int))
{
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = signal_handler;
    sa.sa_flags = SA_RESTART;
    if (sigaction(signal, &sa, NULL) == -1) {
        return -1;
    }
    return 0;
}

void
handle_sigterm(int signal)
{
    syslog(LOG_NOTICE, "received SIGTERM, shutting down");
	terminate_process(0, NULL);
	exit(EXIT_SUCCESS);
}

void
terminate_process(int s, void *arg)
{
	server_shut_down();
	remove_on_exit("");
}

static void
remove_on_exit(char *file)
{
	/* remember file on first call */
	static char f[256] = "";
	if (strlen(f) == 0)	{
		strncpy(f, file, 256);
		return;
	}

	/* remove on second call */
	if (unlink(f) == -1)	{
		syslog(LOG_ERR, "could not remove pidfile");
	}
}

/*
 * parse commandline args using 'getopt_long()'
 */
static int
parse_args(int argc, char *argv[], config_t *config)
{
	int opt, port;
	struct option long_options[] = {
		{ "port",			required_argument,	NULL,	'p' },
		{ "address",		required_argument,	NULL,	'a' },
		{ "pid-file",		required_argument,	NULL,	'f' },
		{ "server-secret",	required_argument,	NULL,	's' },
		{ "client-secret",	required_argument,	NULL,	'c' },
		{ "user-name",		required_argument,	NULL,	'u'	},
		{ "group-name",		required_argument,	NULL,	'g' },
		{ "print-config",	no_argument,		NULL,	'd' },
		{ "help",			no_argument,		NULL,	'h' },
		{ 0		,			0,					0,		0	}
	};

	while ((opt=getopt_long(argc, argv, "p:a:s:c:u:g:dh", long_options, NULL)) != -1)	{
		switch (opt)	{
		case 'p':
			port = atoi(optarg);
			if (port > 0 && port < 0xffff)	{
				config->port = port;
			} else	{
				fprintf(stderr, "argument error: port\n");
				return 1;
			}
			break;
		case 'a':
			strncpy(config->address, optarg, 255);
			config->address[255] = '\0';
			break;
		case 'f':
			strncpy(config->pidfile, optarg, 255);
			config->pidfile[255] = '\0';
			break;
		case 's':
			strncpy(config->server_secret, optarg, 255);
			config->server_secret[255] = '\0';
			break;
		case 'c':
			strncpy(config->client_secret, optarg, 255);
			config->client_secret[255] = '\0';
			break;
		case 'u':
			strncpy(config->user_name, optarg, 31);
			config->user_name[31] = '\0';
			break;
		case 'g':
			strncpy(config->group_name, optarg, 31);
			config->group_name[31] = '\0';
			break;
		case 'd':
			config->print_config = 1;
			break;
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
			break;
		}
	}
	return 0;
}

/*
 * stores /path/to/CONFIGFILE in 'buf'
 * does not check if CONFIGFILE exists
 */
static int
find_config_file(char *buf, ssize_t size)
{
    int ret; char *ret_p;

    /* get path of executable */
    ret = readlink("/proc/self/exe", buf, size);
    if (ret == -1 || ret == size)  {
        return -1;
    }
    buf[ret] = '\0';

    /* get directory portion of path */
    ret_p = strrchr(buf, '/');
    if (ret_p == NULL)   {
        return -1;
    }
    *ret_p = '\0';

    if (strlen(buf)+strlen(CONFIGFILE) >= size-2)  {
        return -1;
    }

    /* append config file name */
    strcat(buf, "/");
    strcat(buf, CONFIGFILE);

    return 0;
}

/* TODO replace with better, robust config parser */
static int
get_config_from_file(char *filename, config_t *config)
{
	FILE *file;
	char line[100];
	char key[100], value[100];

	file = fopen(filename, "r");
	if (file == NULL)	{
		perror("fopen()");
		return 1;
	}

	while (fgets(line, 100, file) != NULL)	{
		if (line[0] != '#' && line[0] != '\n')	{
			sscanf(line, "%s %s\n", key, value);


			if (strcmp(key, "port") == 0)	{
				long port = atoi(value);
				if (port > 0 && port < 0xffff)
					config->port = port;
			} else if (strcmp(key, "address") == 0)	{
				strncpy(config->address, value, 255);
			} else if (strcmp(key, "pidfile") == 0)	{
				strncpy(config->pidfile, value, 255);
			} else if (strcmp(key, "server-secret") == 0)	{
				strncpy(config->server_secret, value, 255);
			} else if (strcmp(key, "client-secret") == 0)	{
				strncpy(config->client_secret, value, 255);
			} else if (strcmp(key, "user-name") == 0)	{
				strncpy(config->user_name, value, 31);
			} else if (strcmp(key, "group-name") == 0)	{
				strncpy(config->group_name, value, 31);
			} else {
				printf("unkown configuration key: %s\n", key);
			}
		}
	}

	fclose(file);
	return 0;
}

static void
print_config(config_t *config)
{
	printf("port:          %i\n", config->port);
	printf("address:       %s\n", config->address);
	printf("pid-file:      %s\n", config->pidfile);
	printf("server-secret: %s\n", "***");
	printf("client-secret: %s\n", "***");
	printf("user-name:     %s\n", config->user_name);
	printf("group-name:    %s\n", config->group_name);
}

static void
print_help(char *name)
{
	printf("Usage: %s [options]\n", name);
	printf("Options:\n");
	printf("-p, --port\t\tBind server to this port\n");
	printf("-a, --address\t\tBind server to this address\n");
	printf("-f, --pid-file\t\tLocation of process pid file\n");
	printf("-s, --server-secret\tUse this key for encryption and verification\n");
	printf("-k, --client-secret\tUse this key for decryption and verification\n");
	printf("-u, --user-name\t\tSwitch to this user after initialization\n");
	printf("-g, --group-name\tSwitch to this group after initialization\n");
	printf("-d, --print-config\tPrint parsed config to std out\n");
	printf("-h, --help\t\tPrint help message and exit\n");
}

int
main(int argc, char *argv[])
{
	int ret, pid, pidfile;
	char pid_string[11], config_path[256];
    config_t config;

    /* get config file path */
    ret = find_config_file(config_path, sizeof(config_path));
    if (ret != 0)   {
        fprintf(stderr, "failed to locate config file: %s\n", CONFIGFILE);
        exit(EXIT_FAILURE);
    }

	/* get config from file */
	ret = get_config_from_file(config_path, &config);
	if (ret != 0)	{
		fprintf(stderr, "failed to load config from file: %s\n", config_path);
        exit(EXIT_FAILURE);
	}

	/* get config from command line */
	ret = parse_args(argc, argv, &config);
	if (ret != 0)	{
		fprintf(stderr, "error: parse_args()\n");
		exit(EXIT_FAILURE);
	}

	/* print config? */
	if (config.print_config)	{
		print_config(&config);
		exit(EXIT_SUCCESS);
	}

	/* check pid file existence */
	ret = open(config.pidfile, O_WRONLY);
	if (ret != -1)	{
		fprintf(stderr, "pid file exists, server already running?\n");
		exit(EXIT_FAILURE);
	}
	close(ret);

	/* init raspberry pi gpio */
	ret = remote_init();
	if (ret != 0)	{
		fprintf(stderr, "error: remote_init()\n");
		exit(EXIT_FAILURE);
	}

	/* start server */
	ret = server_start(config.address, config.port);
	if (ret != 0)	{
		fprintf(stderr, "error: server_start()\n");
		exit(EXIT_FAILURE);
	}

	/* daemonize */
	ret = daemonize(&config);
	if (ret != 0)	{
		fprintf(stderr, "error: daemonize()\n");
		exit(EXIT_FAILURE);
	}

	/* create pidfile */
	pidfile = open(config.pidfile, O_WRONLY | O_CREAT | O_TRUNC);
	if (pidfile == -1)	{
		syslog(LOG_ERR, "could not create pidfile: %s", strerror(errno));
	}

	/* write pidfile */
	pid = getpid();
	sprintf(pid_string, "%i", pid);
	ret = write(pidfile, pid_string, strlen(pid_string));
	if (ret == -1)	{
		syslog(LOG_ERR, "could not write pidfile: %s", strerror(errno));
	}
	close(pidfile);

	/* remove pidfile when terminating */
	remove_on_exit(config.pidfile);

	/* generate keys from shared secrets */
	crypto_keys_init(config.server_secret, config.client_secret);

	while (1)	{
		/* wait for connections, blocking */
		server_accept();
		/* send commands over 433MHz */
		remote_check_action();
	}

	/* shouldn't reach this */
	exit(EXIT_FAILURE);
}
