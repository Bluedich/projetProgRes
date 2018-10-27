#ifndef _LINES_H_
#define _LINES_H_

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define BUFFER_SIZE 1024
#define MAX_NICK_SIZE 64

typedef enum {NONE, RECONNECT, CLOSE} CMD;

typedef enum {MSG, QUIT, NICK, WHO, WHOIS, MSGW, MSGALL, CREATE, LEAVE, JOIN} S_CMD;

void error(const char *msg);

int readline(int fd, char * buffer, int maxlen);

int writeline(int fd_rcv,int fd_exp, char * buffer, int maxlen);

int separate( char buffer_out[], char buffer_in[]);

int get_name_in_command( char  buffer_out[], char  buffer_in[]);

#endif
