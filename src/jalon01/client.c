#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define BUFFER_SIZE 280

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void get_addr_info(const char* addr, const char* port, struct addrinfo** res){
  assert(res);
  int status;
  struct addrinfo hints;

  memset(&hints,0,sizeof(hints));

  hints.ai_family=AF_INET;
  hints.ai_socktype=SOCK_STREAM;

  status = getaddrinfo(addr,port,&hints,res);
  if(status!=0){
    printf("getaddrinfo: returns %d aka %s\n", status, gai_strerror(status)); //fonction qui renvoie un message en rapport avec l'erreur détectée
    exit(1);
  }
}

int do_socket() {
    int sockfd;
    int yes = 1;
    //create the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    //check for socket validity
    if(sockfd==-1){
      error("ERROR creating socket");
    }
    printf("> Socket created.\n");
    // set socket option, to prevent "already in use" issue when rebooting the server right on
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        error("ERROR setting socket options");

    return sockfd;
}

void do_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  assert(addr);
  int res = connect(sockfd, addr, addrlen);
  if (res != 0) {
    error("ERROR connecting");
  }
  //printf("> Connected to host.\n");   // no longer needed, un message du server fait la bienvenu
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

int sendline(int fd, char * buffer, int maxlen){
  assert(buffer);
  int to_send = strlen(buffer);
  int sent;
  int i=0;
  while(to_send>0 || i>1000){ //try maximum of 1000 times
    sent=write(fd, buffer, to_send);
    if (sent==-1)
      error("ERROR sending line");
    to_send-=sent;
    i++;
  }
  if(to_send)
    printf("> Only managed to send %d out of %d bytes of the message.\n", ((int) strlen(buffer))-to_send, (int) strlen(buffer));
  else
    printf("> All %d bytes of message succesfully sent.\n", (int) strlen(buffer));
}

void handle_client_message(int sock, char * buffer, int * cont){
  assert(buffer);
  if(strncmp(buffer,"/quit",5)==0){
    *cont=0; //stop client
  }
  sendline(sock, buffer, BUFFER_SIZE);
}

void handle_server_response(int sock, char * buffer){
  assert(buffer);
  memset(buffer,0, BUFFER_SIZE);
  int size_read = readline(sock, buffer, BUFFER_SIZE);
  //printf("> Read %d bytes from server\n",size_read);  // test
  printf("> [Server] : %s", buffer);
}

int main(int argc,char** argv) {

    if (argc != 3)
    {
        fprintf(stderr,"usage: RE216_CLIENT hostname port\n");
        return 1;
    }

    struct addrinfo* res;
    //get address info from the server
    get_addr_info(argv[1], argv[2], &res);

    //get the socket
    int sock = do_socket();
    //connect to remote socket
    do_connect(sock, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res); //no longer needed


    char buffer[BUFFER_SIZE];
    int cont=1;

    handle_server_response(sock, buffer); // message de bienvenu du server

    do{
      //get user input
      printf("> ");
      fflush(stdout); //to make sure above printf is displayed
      readline(0, buffer, sizeof(buffer));
      //send message to the server
      handle_client_message(sock, buffer, &cont);

      //receive message from server
      handle_server_response(sock, buffer);
    }while(cont);

    printf("> Stopping client\n");
    return 0;
}
