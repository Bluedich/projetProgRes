#include "common.h"
#include "file_trans.h"
// #include <pthread.h>
#include <netdb.h>

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
  hints.ai_socktype=0;
  // hints.ai_socktype=SOCK_STREAM;
  // hints.ai_flags |= AI_NUMERICHOST; // fait que localhost ne marche plus

  status = getaddrinfo(addr,port,&hints,res);
  if(status!=0){
    printf("getaddrinfo: returns %d aka %s\n", status, gai_strerror(status)); //fonction qui renvoie un message en rapport avec l'erreur détectée
    exit(1);
  }
}

void getCurrentTime(char current_time[]){
  time_t rawtime;
  time(&rawtime);
  strftime(current_time, 512, "%T", localtime(&rawtime));
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
}

void handle_client_message(int sock, char * buffer, int * cont){
  assert(buffer);
  char temp[BUFFER_SIZE];
  memset(temp,0,BUFFER_SIZE);
  sprintf(temp,"%s",buffer);
  char temp2[BUFFER_SIZE];
  memset(temp2,0,BUFFER_SIZE);
  sprintf(temp2,"%s",buffer);
  char file_name[BUFFER_SIZE];
  memset(file_name,0,BUFFER_SIZE);

  if( strncmp(buffer,"/quit",5)==0 && strlen(buffer)==6){
    *cont=0; //stop client
  }
  if( strncmp(buffer,"/send",5)==0){
    get_next_arg(temp2, file_name);
    memset(file_name,0,BUFFER_SIZE);// normalement inutile
    get_next_arg(temp2, file_name);
    memset(file_name,0,BUFFER_SIZE); // normalement inutile
    get_next_arg(temp2, file_name);
    sprintf(buffer,"%s %d",temp,size_of_file(file_name));
  }
  writeline(sock,"","", buffer, BUFFER_SIZE);
}

CMD handle_server_response(int sock, char buffer[]){
  assert(buffer);
  char buffer2[BUFFER_SIZE];
  char current_time[512];
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
    if(strcmp(buffer2, "/username")==0) return USERNAME;
    if(strcmp(buffer2, "/newprompt")==0) return NEWPROMPT;
    if(strcmp(buffer2, "/info_conn")==0) return INFO_CONN;
    sprintf(buffer,"ERROR client received unrecognized command %s from server", buffer2);
    error(buffer);
  }
  else{
    getCurrentTime(current_time);
    printf("\n[%s] %s", current_time, buffer);
    return NONE;
  }
}

int prompt_user_for_file_transfer(char user_name[], char file_name[],char f_size[], int sock){
  char buffer[BUFFER_SIZE];
  int file_size = atoi(f_size);
  while(1){
    printf("> %s wants to send file %s (%d bits) to you. Do you accept ? (y/n)\n", user_name, file_name, file_size);
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

int connect_to_peer_2_peer(int sock, char nick[], char buffer[]){
  printf("Preparing to send file\n");

  char user_name[BUFFER_SIZE];
  memset(user_name,0,BUFFER_SIZE);
  char ipAddr[INET_ADDRSTRLEN];
  memset(ipAddr,0,INET_ADDRSTRLEN);
  char portNum[BUFFER_SIZE];
  memset(portNum,0,BUFFER_SIZE);
  char file_name[BUFFER_SIZE];
  memset(file_name,0,BUFFER_SIZE);
  int f_size;

  int err;
  //receive connection information
  get_next_arg(buffer, user_name);
  get_next_arg(buffer, portNum);
  get_next_arg(buffer, file_name);
  get_next_arg(buffer, ipAddr);
  f_size = size_of_file(file_name);
  if (f_size==0){
    printf("File '%s' is an empty file, transfer denied",file_name);
    return -1;
  }
  //get address info
  struct addrinfo* res;
  get_addr_info6(ipAddr, portNum, &res);
  //get the socket
  int c_sock;
  c_sock = do_socket6(res);
  //connect to remote socket
  do_connect(c_sock, res->ai_addr, res->ai_addrlen);
  readline(c_sock, buffer, BUFFER_SIZE);
  printf("%s to %s\n Starting to send '%s'\n", buffer, nick,file_name);
  err = send_file(file_name, c_sock);
  if (err == -1){
    printf("File '%s' is an empty file, transfer denied",file_name);
    return -1;
  }
  printf("File '%s' successfully sent.",file_name);
  return 0;
}

int set_up_peer_2_peer_file_transfer(int sock, char nick[], char user_name[], char file_name[],char f_size[]){
  printf("Preparing to receive file\n");
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);
  // faudrait + ou moins passer en IPv6 ici
  struct sockaddr_in s_addr;
  int s_sock = do_socket();
  int file_size = atoi(f_size);
  struct sockaddr * c_addr = (struct sockaddr *) malloc(sizeof(struct sockaddr));
  int c_addrlen = (int) sizeof(*c_addr);
  int c_sock;
  int err;
  memset(&s_addr, 0, sizeof(s_addr));
  init_serv_addr(0, &s_addr);
  do_bind(s_sock, &s_addr);
  int s_addr_len = sizeof(s_addr);
  getsockname(s_sock, (struct sockaddr*) &s_addr, (socklen_t *) &s_addr_len);
  int port_num = (int) ntohs(s_addr.sin_port);
  listen(s_sock, -1);

  //send connection info to other client
  // int file_size = size_of_file(file_name);
  sprintf(buffer, "/conn_info %s %d %s\n", user_name, port_num, file_name); fflush(stdout);
  writeline(sock,"","",buffer, BUFFER_SIZE);

  //accept connection from client
  c_sock = do_accept(s_sock, c_addr, &c_addrlen);
  writeline(c_sock,nick,"peer2peer","Connected",9);
  printf("file of %d bits ready to be received\n",file_size);
  if (file_size == 0){
    printf("File '%s' is an empty file, transfer canceled",file_name);
    return -1;
  }
  err = rcv_file(file_name, c_sock, file_size);
  if (err == -2){
    printf("You have canceled the transfer");
    return -2;
  }
  // printf("File %s succesfully receive and store in your inbox\n",file_name); // fait dans file_trans.c
  return 0;
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
        printf("> Protocol used : IPv4\n");
        break;
      case AF_INET6:
        printf("> Protocol used : IPv6\n");
        break;
      default:
      printf("> Protocol used : Unknown\n");
        break;
      }
    do_connect(sock, res->ai_addr, res->ai_addrlen);


    char buffer[BUFFER_SIZE];
    char buffer2[BUFFER_SIZE];
    char nick[BUFFER_SIZE];
    char file_name[BUFFER_SIZE];
    memset(file_name, 0, BUFFER_SIZE);
    char file_size[BUFFER_SIZE];
    memset(file_size, 0, BUFFER_SIZE);
    char user_name[BUFFER_SIZE];
    memset(user_name, 0, BUFFER_SIZE);
    strcpy(nick, "Guest");
    int cont=1;
    int pid=0;
    CMD cmd;
    struct pollfd fds[2];
    char current_time[512];

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
            sock = do_socket6(res);
            do_connect(sock, res->ai_addr, res->ai_addrlen);
            break;

          case CLOSE:
            freeaddrinfo(res); //no longer needed
            close(sock);
            cont=0;
            break;

          case FTREQ: //ask user if he wants to accept file connection
            // pid = fork();
            // if (pid ==0){
              get_next_arg(buffer, user_name);
              get_next_arg(buffer, file_name);
              get_next_arg(buffer, file_size);
              if(1==prompt_user_for_file_transfer(user_name, file_name,file_size, sock)) set_up_peer_2_peer_file_transfer(sock, nick, user_name, file_name,file_size);
              break;
            // }
            // break;

          case INFO_CONN:
            connect_to_peer_2_peer(sock, nick, buffer);
            break;

          case USERNAME:
            get_next_arg(buffer, nick); //update nick
            getCurrentTime(current_time);
            printf("[%s] %s", current_time, buffer);
            break;

          case NEWPROMPT:
            break;

          case NONE:
            break;

          default:
            error("Unrecognized client-side command");
        }

        getCurrentTime(current_time);
        if(cmd == NEWPROMPT) printf("[%s] "BOLDGREEN"<%s> " RESET, current_time, nick);
        else printf("\n[%s] " BOLDGREEN "<%s> " RESET, current_time, nick);
        fflush(stdout); //to make sure above printf is displayed
      }

    }while(cont);
    printf("Stopping client\n");
    return 0;
}
