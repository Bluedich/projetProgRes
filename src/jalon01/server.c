#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>

#include <errno.h>

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int do_socket(){
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if(fd == -1)
    error("Error : can't create socket");
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
    error("Error : can't bind");
}

int do_accept(int sock, struct sockaddr * c_addr, int * c_addrlen){
  int c_sock = accept(sock, c_addr, c_addrlen);
  if(c_sock == -1)
    error("Error : can't accept");
}

void do_read(int sock, char * buffer){
  assert(buffer);
  if(read(sock, buffer, 280) == -1)
    error("Error reading from client");
  printf(buffer);
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
        fprintf(stderr, "usage: RE216_SERVER port\n");
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
    fprintf(stdout,"En attente de connection : \n");

    //buffer init
    char * in_buf = (char *) malloc(6*sizeof(char));
    char * buffer = (char *) malloc(280*sizeof(char));
    struct sockaddr * c_addr;
    int c_addrlen;
    int c_sock;

    for (;;)
    {
      //accept connection from client
      c_addrlen = sizeof(*c_addr);
      c_sock = do_accept(sock, c_addr, &c_addrlen);
      fprintf(stdout,"> Connextion accepté \n");

      while(1){
        fprintf(stdout,"> En attente d'un message \n ");
        //read what the client has to say
        bzero(buffer,256);
        int n = read( c_sock,buffer,255 );
        if (n < 0) {
          perror("ERROR reading from socket");
          exit(1);
        }
        fprintf(stdout," > Message reçu : %s",buffer);

        // we write back to the client
        n = write(c_sock,buffer,255);
        if (n < 0) {
          perror("ERROR writing to socket");
          exit(1);
        }

        //disconect client if "/quit"
        if(strncmp(buffer,"/quit",5)==0){
          fprintf(stdout,"> Client out\n");
          break;
        }
      }

    //clean up client socket
      free(buffer);
      close(c_sock);
      break;        // only one client quit server if client quit
   }

    //clean up server socket
    free(in_buf);
    close(sock);
    return 0;
}
