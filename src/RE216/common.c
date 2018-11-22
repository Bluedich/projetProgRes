#include "common.h"

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void init_serv_addr(int port, struct sockaddr_in * s_addr){
  s_addr->sin_family = AF_INET;
  s_addr->sin_port = htons(port);
  s_addr->sin_addr.s_addr = INADDR_ANY;
}

void init_serv_addr6(int port, struct sockaddr_in6 * s_addr){
  s_addr->sin6_family = AF_INET6;
  s_addr->sin6_port = htons(port);
  s_addr->sin6_addr = in6addr_any;
  // s_addr->sin6_scope_id = 0;
}

void do_bind6(int sock, struct sockaddr_in6 * s_addr){
  assert(s_addr);
  if (bind(sock, (const struct sockaddr *) s_addr, sizeof(*s_addr)) == -1)
    error("ERROR binding");
}

int do_socket6_s(){
  int fd = socket(AF_INET6, SOCK_STREAM, 0);
  if(fd == -1)
    error("ERROR creating socket");
  int yes = 1;
  // set socket option, to prevent "already in use" issue when rebooting the server right on
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
      error("ERROR setting socket options");
  return fd;
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

void do_bind(int sock, struct sockaddr_in * s_addr){
  assert(s_addr);
  if (bind(sock, (const struct sockaddr *) s_addr, sizeof(*s_addr)) == -1)
    error("ERROR binding");
}

int do_accept(int sock, struct sockaddr * c_addr, int * c_addrlen){
  int c_sock = accept(sock, c_addr, c_addrlen);
  if(c_sock == -1)
    error("ERROR accepting");
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
  if (strlen(group)>0){ //user is speaking in a group, whispering or broadcasting
    sprintf(temp, BOLDBLUE "<%s><%s>" RESET " %s", nick, group, buffer);
}
  if ( (strlen(nick)>0) && (strlen(group)==0) ){
    if(strncmp(nick, "Server", 6)==0) sprintf(temp, BOLDBLACK"<%s> %s" RESET, nick, buffer);
    else sprintf(temp, BOLDBLUE "<%s>" RESET " %s", nick, buffer);
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
