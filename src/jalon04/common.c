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

int separate(char buffer[]){
  assert(buffer);
  int i=0;
  while(buffer[i]!=' '){ //go to first blank space
      if(buffer[i]=='\n' && buffer[i]=='\0'){
        memset(buffer, '\0', BUFFER_SIZE);
        return 0;
      }
      i++;
  }
  i++; //to skip blank space
  int j=i;
  while(buffer[j]!='\n' && buffer[j]!='\0'){
    buffer[j-i]=buffer[j];
    j++;
  }
  buffer[j-i]='\0';
  return 0;
}

int get_arg_in_command(char buffer_in[], char buffer_out[]){
  assert(buffer_in);
  assert(buffer_out);
  memset(buffer_out, 0, BUFFER_SIZE);

  int i=0;
  while(buffer_in[i]!=' ' && buffer_in[i]!='\n' && buffer_in[i]!='\0'){ //go to first blank space
      buffer_out[i]=buffer_in[i];
      i++;
  }
  buffer_out[i]='\0';
  return 0;
}

int get_next_arg(char buffer_in[], char buffer_out[]){
  get_arg_in_command(buffer_in, buffer_out);
  separate(buffer_in);
  return 0;
}

int writeline(int fd_rcv, char nick[], char group[], char * buffer, int maxlen){
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
    sprintf(temp,BOLDBLUE "\n<%s><%s>" RESET " %s",group, nick, buffer);
}
  if ( (strlen(nick)>0) && (strlen(group)==0) ){
    if(strncmp(nick, "Server", 6)==0) sprintf(temp,BOLDBLACK"<%s>" RESET " %s",nick,buffer);
    else sprintf(temp,BOLDBLUE"\n<%s>" RESET " %s",nick,buffer);
  }

  to_send = strlen(temp);
  while(to_send>0 && i<1000){ //try maximum of 1000 times
    sent=write(fd_rcv, temp, to_send);
    if (sent==-1) error("ERROR writing line");
    to_send-=sent;
    i++;
  }

  //if(to_send)
    //printf("> Only managed to send %d out of %d bytes of the message.\n", ((int) strlen(buffer))-to_send, (int) strlen(buffer));
  //else
    //printf("> All %d bytes of message succesfully sent.\n", (int) strlen(buffer));
}

// int main(){
//   char buffer[BUFFER_SIZE];
//   char buffer_out[BUFFER_SIZE];
//   strcpy(buffer, "/ftreq nomFichier nomEnvoyeur\n");
//   printf("%s %s\n", buffer, buffer_out);
//   get_next_arg(buffer, buffer_out);
//   printf("%s %s\n", buffer, buffer_out);
//   get_next_arg(buffer, buffer_out);
//   printf("%s %s\n", buffer, buffer_out);
//   get_next_arg(buffer, buffer_out);
//   printf("%s %s\n", buffer, buffer_out);
//   get_next_arg(buffer, buffer_out);
// }
