#ifndef _LINES_H_
#define _LINES_H_

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>


#define BUFFER_SIZE 1024
#define MAX_NICK_SIZE 64
#define MAX_CL 20
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define RED         "\033[31m"
#define RESET   "\033[0m"

typedef enum {NONE, RECONNECT, CLOSE, FTREQ, USERNAME, NEWPROMPT, INFO_CONN} CMD; //commands received by client

typedef enum {MSG, QUIT, NICK, MSGW, MSGALL, CREATE, LEAVE, JOIN, WHO, WHOIS, WHOINGROUP,GROUP, FTREQP, FTREQN, FTSUCCESS, SEND, CONN_INFO} S_CMD; //commands received by server

void error(const char *msg);

int do_accept(int sock, struct sockaddr * c_addr, int * c_addrlen);

void init_serv_addr(int port, struct sockaddr_in * s_addr);

void init_serv_addr6(int port, struct sockaddr_in6 * s_addr);

void do_bind(int sock, struct sockaddr_in * s_addr);

void do_bind6(int sock, struct sockaddr_in6 * s_addr);

int readline(int fd, char * buffer, int maxlen);

int writeline(int fd_rcv, char * nick, char * group, char * buffer, int maxlen);

int separate(char buffer[]); //removes start of buffer until next ' ' in buffer

int get_arg_in_command(char buffer_out[], char buffer_in[]); //returns start of buffer until next ' ' or '\0' or '\n'

int get_next_arg(char buffer_in[], char buffer_out[]); //returns next arg in buffer_out and removes start of buffer from buffer_in


#endif
