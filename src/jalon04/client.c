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
#include <arpa/inet.h>

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

void get_addr_info6(const char* addr, const char* port, struct addrinfo** res){
  assert(res);
  int status;
  struct addrinfo hints;

  memset(&hints,0,sizeof(hints));

  hints.ai_family=AF_UNSPEC;
  hints.ai_socktype=SOCK_STREAM;
  // hints.ai_flags |= AI_NUMERICHOST; // fait que localhost ne marche plus

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

int do_socket6(struct addrinfo* res) {
    int sockfd;
    int yes = 1;
    //create the socket
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

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

CMD handle_server_response(int sock, char buffer[]){
  assert(buffer);
  char buffer2[BUFFER_SIZE];
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
  else if(buffer[0]=='/'){
    get_next_arg(buffer, buffer2);
    if(strcmp(buffer2,"/ftreq")==0) return FTREQ;
    if(strcmp(buffer2, "/ftreqP")==0) return FTREQP_C;
    if(strcmp(buffer2, "/username")==0) return USERNAME;
    if(strcmp(buffer2, "/newprompt")==0) return NEWPROMPT;
    sprintf(buffer,"ERROR client received unrecognized command %s from server", buffer2);
    error(buffer);
  }
  else{
    printf(" %s", buffer);
    return NONE;
  }
}

int prompt_user_for_file_transfer(char buffer[], int sock){
  char file_name[BUFFER_SIZE];
  memset(file_name, 0, BUFFER_SIZE);
  char user_name[BUFFER_SIZE];
  memset(user_name, 0, BUFFER_SIZE);

  get_next_arg(buffer, user_name);
  get_next_arg(buffer, file_name);

  while(1){
    printf("> %s wants to send file %s to you. Do you accept ? (y/n)\n", user_name, file_name );
    readline(0,buffer,BUFFER_SIZE);
    if(strncmp(buffer,"y",1)==0){
      memset(buffer, 0, BUFFER_SIZE);
      sprintf(buffer, "/ftreqP %s", user_name);
      writeline(sock,"","", buffer, BUFFER_SIZE);
      return 1;
    }
    if(strncmp(buffer,"n",1)==0){
      memset(buffer, 0, BUFFER_SIZE);
      sprintf(buffer, "/ftreqN %s", user_name);
      writeline(sock,"","", buffer, BUFFER_SIZE);
      return 0;
    }
  }
}

int set_up_peer_2_peer_file_transfer(){
  printf("Suck my weewee\n");
}

int main(int argc,char** argv) {

    if (argc != 3)
    {
        printf("usage: RE216_CLIENT hostname port\n");
        return 1;
    }
    struct addrinfo* res;
    int sock;
    struct sockaddr_in6 * c_addr;
    struct in6_addr serveraddr;
    int addrlen= sizeof(c_addr);

    //get address info from the server
    get_addr_info6(argv[1], argv[2], &res);
    //get the socket
    sock = do_socket6(res);
    //connect to remote socket
    switch(res->ai_addr->sa_family){
      case AF_INET:
        printf("> protocole used : IPv4\n");
        break;
      case AF_INET6:
        printf("> protocole used : IPv6\n");
        break;
      default:
      printf("> protocole used : Unknown\n");
        break;
      }
    do_connect(sock, res->ai_addr, res->ai_addrlen);


    char buffer[BUFFER_SIZE];
    char buffer2[BUFFER_SIZE];
    char nick[BUFFER_SIZE];
    strcpy(nick, "Guest");
    int cont=1;
    CMD cmd;
    struct pollfd fds[2];

    fds[0].fd = 0;
    fds[0].events = POLLIN;

    fds[1].fd = sock;
    fds[1].events = POLLIN;

    do{
      assert(fds);

      if(poll(fds, 2, -1)==-1) error("ERROR polling");

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
          case RECONNECT: //try to reconnect to server
            close(sock);
            sock = do_socket();
            do_connect(sock, res->ai_addr, res->ai_addrlen);
            break;

          case CLOSE:
            freeaddrinfo(res); //no longer needed
            close(sock);
            cont=0;
            break;

          case FTREQ: //ask user if he wants to accept file connection
            if(1==prompt_user_for_file_transfer(buffer, sock)) set_up_peer_2_peer_file_transfer();
            break;

          case FTREQP_C:
            printf("setting up connection to client\n");

          case USERNAME:
            get_next_arg(buffer, nick); //update nick
            printf("%s", buffer);
            break;

          case NEWPROMPT:
            break;

          case NONE:
            break;

          default:
            error("Unrecognized client-side command");
        }
        if(cmd == NEWPROMPT) printf(BOLDGREEN "<%s> " RESET, nick);
        else printf(BOLDGREEN "\n<%s> " RESET, nick);
        fflush(stdout); //to make sure above printf is displayed
      }

    }while(cont);
    printf("Stopping client\n");
    return 0;
}
