#include "common.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>

#define MAX_CL 20

struct client{
  char nickname[BUFFER_SIZE];
  int c_fd;
};

char * strlncpy(char *dest, const char *src, size_t n, int size) {
  size_t i;
  for (i = n; i < size && src[i] != '\0'; i++)
    dest[i-n] = src[i];
  for ( ; i < size; i++)
    dest[i-n] = '\0';
  return dest;
}

int my_strlen(char s[]) {
  int i;
  i=0;
  while(s[i]!='\0')
    ++i;
  return i;
}

void do_nick(char *buffer, struct client *client, int c_sock){
  assert(client[c_sock].nickname);
  //bzero(client[c_sock].nickname,BUFFER_SIZE);
  //int m = my_strlen(client[c_sock].nickname); // pas besoinde ça
  int k = my_strlen(buffer)-1;
  memset(client[c_sock].nickname, 0, BUFFER_SIZE);
  strlncpy(client[c_sock].nickname, buffer, 6, k);  //5 est une constante correspong a /nick
  //memset(buffer, 0, BUFFER_SIZE);

}

/*
void do_whois(char *buffer, struct client *client, int c_sock){   // buffer= nicknae's users
  assert(client[c_sock].nickname);
  //bzero(client[c_sock].nickname,BUFFER_SIZE);
  //int m = my_strlen(client[c_sock].nickname); // pas besoinde ça
  int k = my_strlen(buffer)-1;
  memset(client[c_sock].nickname, 0, BUFFER_SIZE);
  strlncpy(client[c_sock].nickname, buffer, 6, k);  //6 est une constante correspong a /nick
  //memset(buffer, 0, BUFFER_SIZE);

}
*/

int do_socket(){
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if(fd == -1)
    error("ERROR creating socket");
  return fd;
}

void init_serv_addr(int port, struct sockaddr_in * s_addr){
  (*s_addr).sin_family = AF_INET;
  (*s_addr).sin_port = htons(port);
  (*s_addr).sin_addr.s_addr = INADDR_ANY;
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

int get_available_fd_index(struct pollfd * fds){
  int i;
  for(i=2; i<MAX_CL+2; i++){
    if(fds[i].fd==-1)
      return i;
  }
  return -1;
  error("ERROR too many connected clients");
}


int command(char * buffer, struct client * client, int c_sock, struct pollfd * fds, int i,int s_read){

    if( (strncmp(buffer,"/quit",5)==0 && strlen(buffer)==6) || s_read==0){ //s_read = 0 probably mean client has closed socket
      printf("> Client disconnected\n");
      fds[i].fd=-1; //we want to ignore this fd in the future
      close(c_sock);
      return -1;
    }

    if( (strncmp(buffer,"/nick",5)!=0) && (strncmp(client[c_sock-4].nickname,"USER",4)==0) ){   // pas propre cette deuxieme condition je crois
    writeline(c_sock, "Please use /nick <your_pseudo> to logon\n",BUFFER_SIZE);
    printf("> [%s%d] try to comunicate but is not identified\n", client[c_sock-4].nickname,c_sock);
    return -1;
    }

  if(strncmp(buffer,"/nick",5)==0 ){
    do_nick(buffer, client, c_sock-4);                       // il faut isolé le nickname et remplir la structure client
       // en gros ça ressemble à ça
    writeline(c_sock, "Pseudo change, you can communicate\n",BUFFER_SIZE);
   }


  if(strncmp(buffer,"/who",4)==0 && strlen(buffer)==5){
    writeline(c_sock,"> Client connected\n",BUFFER_SIZE);
    for (i=0;i<2;i++){
      writeline(c_sock,client[i].nickname,BUFFER_SIZE);
      writeline(c_sock,"\n",BUFFER_SIZE);
    }
    return -1;
  }

  if(strncmp(buffer,"/whois",6)==0){

  }

  else{
    printf("> [%s] : %s", client[c_sock-4].nickname, buffer);
    //server response
    writeline(c_sock, buffer, BUFFER_SIZE);
  }
  return 0;
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("usage: RE216_SERVER port\n");
        return 1;
    }
    //create the socket, check for validity!
    int sock = do_socket(sock);

    //init the serv_add structure
    struct  sockaddr_in * s_addr;
    init_serv_addr(atoi(argv[1]), s_addr);

    //perform the binding
    //we bind on the tcp port specified
    do_bind(sock, s_addr);

    //specify the socket to be a server socket and listen for at most 20 concurrent client (but only one use Jalon01)
    listen(sock, MAX_CL);
    printf("> Waiting for connection : \n");

    //buffer init
    char * in_buf = (char *) malloc(6*sizeof(char));
    char buffer[BUFFER_SIZE];
    struct sockaddr * c_addr = (struct sockaddr *) malloc(sizeof(struct sockaddr));
    int c_addrlen;
    int c_sock;
    int s_read;

    struct pollfd fds[MAX_CL+2]; // = (struct pollfd *) malloc((MAX_CL+2)*sizeof(struct pollfd));
    //fds = memset(fds, 0, (MAX_CL+2)*sizeof(struct pollfd));

    fds[0].fd = 0;
    fds[0].events = POLLIN;

    fds[1].fd = sock;
    fds[1].events = POLLIN;

    int i;
    for(i=2;i<MAX_CL+2;i++){
      fds[i].fd = -1;
      fds[i].events = POLLIN;
    }

    struct client client[20];

    // pour une bocule whos
    int m;
    int n_client=2;
    // c'est pas beau

    for (;;)
    {

      assert(fds);
      if(poll(fds, MAX_CL+2, -1)==-1)
        error("ERROR polling");

      if(fds[0].revents & POLLIN){
        memset(buffer, 0, BUFFER_SIZE);
        readline(0, buffer, BUFFER_SIZE);
        if(strncmp(buffer,"/quit",5)==0){
          printf("> Closing server\n");
          break;
        }
      }

      if(fds[1].revents & POLLIN){
        //accept connection from client
        if(get_available_fd_index(fds) != -1){
          c_addrlen = sizeof(*c_addr);
          c_sock = do_accept(sock, c_addr, &c_addrlen);
          fds[get_available_fd_index(fds)].fd = c_sock;
          printf("> Connection accepted \n");
          writeline(c_sock, "Welcome to the server.\nPlease use /nick <your_psuedo> to logon\n", BUFFER_SIZE);    // welcome message for the client
          do_nick("/nick USERXX", client, c_sock-4);
        }

        else{
          c_addrlen = sizeof(*c_addr);
          c_sock = do_accept(sock, c_addr, &c_addrlen);
          fds[get_available_fd_index(fds)].fd = c_sock;
          writeline(c_sock, "Server cannot accept incoming connections anymore. Please try again later.\n", BUFFER_SIZE);
          printf("> Connection to a client denied, too many clients already connected\n");
          fds[i].fd=-1;            //we want to ignore this fd next loops
          close(c_sock);
        }
      }

      for(i=2;i<MAX_CL+2; i++){
        if(fds[i].revents & POLLIN){
          c_sock = fds[i].fd;
          //read what the client has to say
          s_read = readline(c_sock, buffer, BUFFER_SIZE);

          // Plutôt que des conditions comme ça faudrait peut être mieux faire des appels à des fonctions qui font quelque chose ou pas
          // en foction de buffer par exemple
          // A MODIFIER
          command(buffer,client,c_sock,fds,i,s_read);
        }
      }

    //clean up client socket
   }

    //clean up server socket
    free(c_addr);
    close(sock);
    return 0;

}
