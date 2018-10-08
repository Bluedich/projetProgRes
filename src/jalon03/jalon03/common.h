#ifndef _LINES_H_
#define _LINES_H_

#define BUFFER_SIZE 1024

typedef enum {NONE, RECONNECT, CLOSE} CMD;

void error(const char *msg);

int readline(int fd, char * buffer, int maxlen);

int writeline(int fd, char * buffer, int maxlen);

#endif
