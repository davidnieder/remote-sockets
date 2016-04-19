# vim:set tabstop=8 softtabstop=8 shiftwidth=8 noexpandtab
#

# application name
TARGET = remote-sockets
BUILDOUT = build
MAKEPATH := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
CONFIGFILE = config
SERVICEFILE = $(TARGET).service

# application default configuration
DEFAULT_PORT = 32768
DEFAULT_ADDR = "::"
DEFAULT_PIDFILE = "/tmp/remote-sockets.pid"
DEFAULT_SERVERSECRET = "abcdefghijklmnopqrstuvwxyz123456"
DEFAULT_CLIENTSECRET = "654321zyxwvutsrqponmlkjihgfedcba"
DEFAULT_USERNAME = "david"
DEFAULT_GROUPNAME = "users"


# src files
#SRC = main.c crypto.c axtls_crypto.c
SRC = *.c

# c language standard
CSTANDARD = gnu99

# c compiler flags
CFLAGS += -Wall
CFLAGS += -Wpedantic
#CFLAGS += -O2
CFLAGS += -std=$(CSTANDARD)

# programs and commands
CC = gcc
REMOVE = rm -rf
MKDIR = mkdir -p
WRITECONFIG = echo >>$(BUILDOUT)/$(CONFIGFILE)
WRITESERVICE = echo >>$(BUILDOUT)/$(SERVICEFILE)
TOUCH = touch
CHMOD = chmod
ECHO = echo


.PHONY: all config systemd
all: clean build config systemd

# build executable
build:
	$(MKDIR) $(BUILDOUT)
	$(CC) $(CFLAGS) -o $(BUILDOUT)/$(TARGET) $(SRC)

# write config file
config:
	$(MKDIR) $(BUILDOUT)
	$(REMOVE) $(BUILDOUT)/$(CONFIGFILE)
	$(TOUCH) $(BUILDOUT)/$(CONFIGFILE)
	$(CHMOD) 600 $(BUILDOUT)/$(CONFIGFILE)
	@$(WRITECONFIG) "#"
	@$(WRITECONFIG) "# remote-socket server configuration file"
	@$(WRITECONFIG) "#"
	@$(WRITECONFIG) ""
	@$(WRITECONFIG) "# address and port the server will listen on"
	@$(WRITECONFIG) "address" $(DEFAULT_ADDR)
	@$(WRITECONFIG) "port   " $(DEFAULT_PORT)
	@$(WRITECONFIG) ""
	@$(WRITECONFIG) "# user, group the process should run under"
	@$(WRITECONFIG) "user-name " $(DEFAULT_USERNAME)
	@$(WRITECONFIG) "group-name" $(DEFAULT_GROUPNAME)
	@$(WRITECONFIG) ""
	@$(WRITECONFIG) "# pre-shared keys"
	@$(WRITECONFIG) "server-secret" $(DEFAULT_SERVERSECRET)
	@$(WRITECONFIG) "client-secret" $(DEFAULT_CLIENTSECRET)
	@$(WRITECONFIG) ""
	@$(WRITECONFIG) "# location to write pid file"
	@$(WRITECONFIG) "pidfile" $(DEFAULT_PIDFILE)
	@$(WRITECONFIG) ""

# write systemd service file
systemd:
	$(MKDIR) $(BUILDOUT)
	$(REMOVE) $(BUILDOUT)/$(SERVICEFILE)
	$(TOUCH) $(BUILDOUT)/$(SERVICEFILE)
	@$(WRITESERVICE) "[Unit]"
	@$(WRITESERVICE) "Description="$(TARGET)
	@$(WRITESERVICE) "After=network.target"
	@$(WRITESERVICE) ""
	@$(WRITESERVICE) "[Service]"
	@$(WRITESERVICE) "ExecStart="$(MAKEPATH)/$(BUILDOUT)/$(TARGET)
	@$(WRITESERVICE) "Type=forking"
	@$(WRITESERVICE) "PIDFile=$(DEFAULT_PIDFILE)"
	@$(WRITESERVICE) "KillMode=process"
	@$(WRITESERVICE) "KillSignal=SIGTERM"
	@$(WRITESERVICE) ""
	@$(WRITESERVICE) "[Install]"
	@$(WRITESERVICE) "WantedBy=multi-user.target"


# remove created files
clean:
	$(REMOVE) $(BUILDOUT)

