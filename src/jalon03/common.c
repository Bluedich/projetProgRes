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

int separate( char  buffer[]){
  assert(buffer);
  char * txt=malloc(sizeof(char)*BUFFER_SIZE);
  strcpy(txt,buffer);

  int i=0;
  while(strncmp(txt+i," ",1)!=0){ //aller jusqu'au premeir espace
      i++;
  }
  i++;
  int j=i;
  while(strncmp(txt+j,"\0",1)!=0){
    buffer[j-i]=txt[j];
    j++;
  }
  buffer[j-i]=txt[j];
  }

int get_name_in_command( char  buffer_out[], char  buffer_in[]){
  assert(buffer_in);
  assert(buffer_out);

  int i=0;
  while(strncmp(buffer_in+i," ",1)){ //
      buffer_out[i]=buffer_in[i];
      i++;
  }
  strncpy(buffer_out+i,"\0",1);
}

int writeline(int fd_rcv,char nick[],char group[], char * buffer, int maxlen){
  assert(buffer);
  char * temp = malloc(sizeof(char)*BUFFER_SIZE);
  //strcpy(temp,buffer);
  memset(temp, 0, BUFFER_SIZE);
  assert(nick);
  int i=0;
  int to_send=0;
  int sent =0;

  sprintf(temp,"%s",buffer);
  if (strlen(group)>0){
    sprintf(temp,"[%s]> [%s]: %s",group, nick, buffer);
}
  if ( (strlen(nick)>0) && (strlen(group)==0) ){
    sprintf(temp,"[%s] : %s",nick,buffer);
  }

  to_send = strlen(temp);
  while(to_send>0 || i<1000){ //try maximum of 1000 times // la j'ai changé, avant la condtion était i>1000
    sent=write(fd_rcv, temp, to_send);
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
