#ifndef _LINES_H_
#define _LINES_H_

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define BUFFER_SIZE 1024
#define MAX_NICK_SIZE 64
#define MAX_CL 20
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define RESET   "\033[0m"

typedef enum {NONE, RECONNECT, CLOSE, FTREQ, FTREQP_C, FTREQN_C, USERNAME, NEWPROMPT} CMD; //commands received by client

typedef enum {MSG, QUIT, NICK, MSGW, MSGALL, CREATE, LEAVE, JOIN, WHO, WHOIS, WHOINGROUP,GROUP, FTREQP_S, FTREQN_S, FTSUCCESS, SEND} S_CMD; //commands received by server

void error(const char *msg);

int readline(int fd, char * buffer, int maxlen);

int writeline(int fd_rcv, char * nick, char * group, char * buffer, int maxlen);

int separate(char buffer[]); //removes start of buffer until next ' ' in buffer

int get_arg_in_command(char buffer_out[], char buffer_in[]); //returns start of buffer until next ' ' or '\0' or '\n'

int get_next_arg(char buffer_in[], char buffer_out[]);

#endif
