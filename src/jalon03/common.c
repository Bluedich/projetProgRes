#include "common.h"

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

int separate( char  buffer_out[], char  buffer_in[]){
  assert(buffer_in);
  assert(buffer_out);

  int i=0;
  while(strncmp(buffer_in+i," ",1)!=0){ //aller jusqu'au premeir espace
      i++;
  }
  i++;
  int j=i;
  while(strncmp(buffer_in+i,"\0",1)!=0){
    buffer_out[j-i]=buffer_in[j];
    j++;
  }
  buffer_out[j-i]=buffer_in[j];
  j++;
  buffer_out[j-i]=buffer_in[j];

  }

int get_name_in_command( char  buffer_out[], char  buffer_in[]){
  assert(buffer_in);
  assert(buffer_out);

  int i=0;
  while(strncmp(buffer_in+i," ",1)){ //aller jusqu'au premeir espace
      buffer_out[i]=buffer_in[i];
      i++;
  }
  strncpy(buffer_in+i,"\0",1);
}


int writeline(int fd_rcv,int fd_exp, char * buffer, int maxlen){
  assert(buffer);
  int to_send = strlen(buffer);
  int sent;
  int i=0;
  if(fd_rcv==fd_exp){
    while(to_send>0 || i>1000){ //try maximum of 1000 times
      sent=write(fd_rcv, buffer, to_send);
      if (sent==-1)
        error("ERROR writing line");
      to_send-=sent;
      i++;
    }
  }
  else{
    //strcat(buffer,"c'est un peu la merde frère, en vrai c'est pas le client qui te parler\n");
    while(to_send>0 || i>1000){ //try maximum of 1000 times
      sent=write(fd_rcv, buffer, to_send);
      if (sent==-1)
        error("ERROR writing line");
      to_send-=sent;
      i++;
    }
  }
  //if(to_send)
    //printf("> Only managed to send %d out of %d bytes of the message.\n", ((int) strlen(buffer))-to_send, (int) strlen(buffer));
  //else
    //printf("> All %d bytes of message succesfully sent.\n", (int) strlen(buffer));
}
