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
          writeline(c_sock, "Welcome to the server.\n", BUFFER_SIZE);    // welcome message for the client
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

          if(strncmp(buffer,"/quit",5)==0 || s_read==0){ //s_read = 0 probably mean client has closed socket
            printf("> Client disconnected\n");
            fds[i].fd=-1; //we want to ignore this fd in the future
            close(c_sock);
            break;
          }
          else{
            printf("> [Client %d] : %s", c_sock, buffer);
            //server response
            writeline(c_sock, buffer, BUFFER_SIZE);
          }
        }
      }

    //clean up client socket
   }

    //clean up server socket
    free(c_addr);
    close(sock);
    return 0;

}
