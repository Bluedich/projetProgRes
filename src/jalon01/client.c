#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <errno.h>



int do_socket(int domain, int type, int protocol) {
    int sockfd;
    int yes = 1;
//create the socket
    sockfd = socket(domain, type, protocol);
//check for socket validity

// set socket option, to prevent "already in use" issue when rebooting the server right on
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        error("ERROR setting socket options");
    return sockfd;
}


void init_client_addr(int port, struct sockaddr_in *client_addr) {
//clean the client_add structure
    memset(client_addr,'\0', sizeof(*client_addr) );  //
//cast the port from a string to an int
//internet family protocol
    client_addr->sin_family = AF_INET;
//we bind to the IP of the server (@ de loopback)
    inet_aton("127.0.0.1", &client_addr->sin_addr );
//    fprintf(stdout,"serv_addr ? :%d erreur %d\n",&client_addr->sin_addr, errno);
//we bind on the tcp port specified
    client_addr->sin_port = htons(port);

}


void do_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    int res = connect(sockfd, addr, addrlen);
    if (res != 0) {
      printf("Probleme de connextion au server\n");
    }

}

ssize_t do_read(int fd, void *buf, size_t count ){
  return read( fd, buf, count);
}

ssize_t do_write(int fd, void *buf, size_t count){
  return write(fd, buf, count);
}

int main(int argc,char** argv)
{

    if (argc != 3)
    {
        fprintf(stderr,"usage: RE216_CLIENT hostname port\n");
        return 1;
    }
    char buffer[256];
//get address info from the server
//get_addr_info();

//get the socket
int s = do_socket(AF_INET, SOCK_STREAM, 0);
fprintf(stdout,"socket crée \n");

struct sockaddr_in c_addr;
init_client_addr(atoi(argv[2]), &c_addr);
fprintf(stdout,"init_client_add \n");

//connect to remote socket
do_connect(s, (const struct sockaddr *)&c_addr, sizeof(c_addr));
fprintf(stdout,"connection réussi \n");


//get user input
//const void *buf = readline();         // uselss
while (1){
  printf("> Please enter the message: ");
   bzero(buffer,256);
   fgets(buffer,255,stdin);

//send message to the server

   int n = write(s, buffer, strlen(buffer));

   if (n < 0) {
     perror("ERROR writing to socket");
     exit(1);
  }
//char *txt = readline(" Quel est votre message ? :");  // uselss
//ssize_t reslt = send(s, buffer, sizeof(buffer), 0);   // useles

  // Now read server response
   bzero(buffer,256);
   n = read(s, buffer, 255);

   if (n < 0) {
      perror("ERROR reading from socket");
      exit(1);
   }

   if(strncmp(buffer,"/quit",5)==0){
     fprintf(stdout,"> Client down\n");
     break;
   }
   printf("> [SERVER] : %s",buffer);

}



//handle_client_message()


    return 0;


}
