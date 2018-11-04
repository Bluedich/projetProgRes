#include "common.h"
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>

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
  printf("> Connected to host.\n");
}

void handle_client_message(int sock, char * buffer, int * cont){
  assert(buffer);
  if( strncmp(buffer,"/quit",5)==0 && strlen(buffer)==6){
    *cont=0; //stop client
  }
  writeline(sock,"","", buffer, BUFFER_SIZE);
}

CMD handle_server_response(int sock, char * buffer){
  assert(buffer);
  int size_read = readline(sock, buffer, BUFFER_SIZE);
  if(size_read==0){
    printf("Server has closed connection\n");
    while(1){
      printf("> Try to reconnect to the server ? (y/n)\n");
      readline(0,buffer,BUFFER_SIZE);
      if(strncmp(buffer,"y",1)==0){
        return RECONNECT;
      }
      if(strncmp(buffer,"n",1)==0){
        return CLOSE;
      }
    }
  }
  else{
    printf(" %s", buffer);
    return NONE;
  }
}

int main(int argc,char** argv) {

    if (argc != 3)
    {
        printf("usage: RE216_CLIENT hostname port\n");
        return 1;
    }

    struct addrinfo* res;
    //get address info from the server
    get_addr_info(argv[1], argv[2], &res);

    //get the socket
    int sock = do_socket();
    //connect to remote socket
    do_connect(sock, res->ai_addr, res->ai_addrlen);

    char buffer[BUFFER_SIZE];
    int cont=1;
    CMD cmd;
    struct pollfd fds[2];

    fds[0].fd = 0;
    fds[0].events = POLLIN;

    fds[1].fd = sock;
    fds[1].events = POLLIN;

    printf("> ");   // je suis pas sure que ce soit pas de la merde ça
    fflush(stdout); //to make sure above printf is displayed

    do{
      assert(fds);
      if(poll(fds, 2, -1)==-1)
        error("ERROR polling");

      if(fds[0].revents & POLLIN){
        //get user input
        readline(0, buffer, BUFFER_SIZE);
        //send message to the server
        handle_client_message(sock, buffer, &cont);
      }

      if(fds[1].revents & POLLIN){
        //receive message from server
        cmd = handle_server_response(sock, buffer);

      //apply commands from server or user if needed
      switch(cmd){
        case RECONNECT:
          close(sock);
          sock = do_socket();
          do_connect(sock, res->ai_addr, res->ai_addrlen);
          break;
        case CLOSE:
          freeaddrinfo(res); //no longer needed
          close(sock);
          cont=0;
          break;
        }
      }
      printf("> ");   // C'est un peu de la merde ça
      fflush(stdout); //to make sure above printf is displayed
    }while(cont);
    printf("Stopping client\n");
    return 0;
}
