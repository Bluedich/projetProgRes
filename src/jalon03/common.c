#include "common.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int readline(int fd, char * buffer, int maxlen){
  assert(buffer);
  memset(buffer, 0, maxlen);
  int size_read = read(fd, buffer, maxlen);
  if(size_read==-1){
    error("ERROR reading line");
  }
  return size_read;
}

int writeline(int fd, char * buffer, int maxlen){
  assert(buffer);
  int to_send = strlen(buffer);
  int sent;
  int i=0;
  while(to_send>0 || i>1000){ //try maximum of 1000 times
    sent=write(fd, buffer, to_send);
    if (sent==-1)
      error("ERROR writing line");
    to_send-=sent;
    i++;
  }
  //if(to_send)
    //printf("> Only managed to send %d out of %d bytes of the message.\n", ((int) strlen(buffer))-to_send, (int) strlen(buffer));
  //else
    //printf("> All %d bytes of message succesfully sent.\n", (int) strlen(buffer));
}