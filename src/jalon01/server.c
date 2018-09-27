#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>

#include <errno.h>
#define BUFFER_SIZE 280

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

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

void do_read(int sock, char * buffer){
  assert(buffer);
  bzero(buffer,BUFFER_SIZE);
  if(read(sock, buffer, BUFFER_SIZE) == -1)
    error("Error reading from client");
  printf("> [Client] : %s",buffer);
}

void do_write(int sock, char * buffer){
  assert(buffer);
  int to_send = strlen(buffer);
  int sent;
  while(to_send>0){
    sent=write(sock, buffer, to_send);
    if (sent==-1)
      error("Error writing to client");
    to_send-=sent;
  }
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
    listen(sock, 20);
    printf("> Waiting for connection : \n");

    //buffer init
    char * in_buf = (char *) malloc(6*sizeof(char));
    char buffer[BUFFER_SIZE];
    struct sockaddr * c_addr = (struct sockaddr *) malloc(sizeof(struct sockaddr));
    int c_addrlen;
    int c_sock;

    for (;;)
    {
      //accept connection from client
      c_addrlen = sizeof(*c_addr);
      c_sock = do_accept(sock, c_addr, &c_addrlen);
      printf("> Connection accepted \n");

      while(1){
        printf("> Waiting for message\n");
        //read what the client has to say
        do_read(c_sock, buffer);
        do_write(c_sock, buffer);

        //disconect client if "/quit"
        if(strncmp(buffer,"/quit",5)==0){
          printf("> Client disconnected\n");
          break;
        }
      }

    //clean up client socket
    //free(buffer);
    close(c_sock);
   }

    //clean up server socket
    free(in_buf);
    close(sock);
    return 0;
}
