#include "common.h"
#include "list.h"
#include "group.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <arpa/inet.h>

#define MAX_CL 20

struct listg{
  struct group * group;
  struct listg * next;
};

int do_socket(){
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if(fd == -1)
    error("ERROR creating socket");
  return fd;
  int yes = 1;
  // set socket option, to prevent "already in use" issue when rebooting the server right on
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
      error("ERROR setting socket options");
}

void init_serv_addr(int port, struct sockaddr_in * s_addr){
  s_addr->sin_family = AF_INET;
  s_addr->sin_port = htons(port);
  s_addr->sin_addr.s_addr = INADDR_ANY;
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

S_CMD get_command(char * buffer, int s_read){

  if((s_read == 0) || (strncmp(buffer,"/quit",5)==0 && strlen(buffer)==6)) //s_read = 0 probably means client has closed socket
    return QUIT;

  if((strncmp(buffer,"/quit",5)==0 && strlen(buffer)>6))
    return LEAVE;  // leave a group

  if(strncmp(buffer,"/create ",8)==0 && strlen(buffer)>9)
    return CREATE;  // create a group

  if(strncmp(buffer,"/msg ",5)==0 && strlen(buffer)>6)
    return MSGW;  // whisper a user

  if(strncmp(buffer,"/msgall ",8)==0 && strlen(buffer)>8)
    return MSGALL;  // envoi un message en broadcast

  if(strncmp(buffer,"/join ",6)==0 && strlen(buffer)>7)
    return JOIN;  // envoi un message en broadcast

  if(strncmp(buffer,"/nick ",6)==0 && strlen(buffer)>7)
    return NICK;

  if(strncmp(buffer,"/who",4)==0 && strlen(buffer)==5)
    return WHO;

  if(strncmp(buffer,"/whois ",7)==0 && strlen(buffer)>8)
    return WHOIS;

  return MSG;
}

int command(char * buffer, S_CMD cmd, struct list ** clients, struct pollfd * fd, struct listg ** groups){
    char nick[BUFFER_SIZE];
    memset(nick, 0, BUFFER_SIZE);
    struct client * client=NULL;
    int c_sock = fd->fd;
    int res;
    int w_sock; // sock du client à qui on veut whisper
    int hasNick = has_nick(*clients, nick, c_sock);
    if(!hasNick && cmd!=NICK && cmd!=QUIT){ //Not allowed to talk until a nickname is chosen
      writeline(c_sock, "Please use /nick <your_pseudo> to login.\n", BUFFER_SIZE);
      return 0;
    }

    switch(cmd){
      case MSG :  // command plus ou moins obsolète
        printf("> [%s] %s", nick, buffer);
        writeline(c_sock, buffer, BUFFER_SIZE);
        break;

      case QUIT :
        remove_client(clients, c_sock);
        close(c_sock);
        fd->fd = -1; //to let the program know to ignore this structure in the future
        printf("> [%s] disconnected\n", nick);
        break;

      case LEAVE :
        res = remove_client_in_group(groups,c_sock,buffer+6); // hummm ça marche pas... Il me dit que le channel exist pas
        if (res==0){
          writeline(c_sock, "you have quit", BUFFER_SIZE);// oui c'est immonde
          writeline(c_sock, buffer+6, BUFFER_SIZE);// oui c'est immonde
          break;
        }
        if (res==10){
          writeline(c_sock, "you have quit", BUFFER_SIZE);// oui c'est immonde
          writeline(c_sock, buffer+6, BUFFER_SIZE);// oui c'est immonde
          remove_group(groups,buffer+6);
          writeline(c_sock, "And", BUFFER_SIZE);// oui c'est immonde
          writeline(c_sock, buffer+6, BUFFER_SIZE);// oui c'est immonde
          writeline(c_sock, "is destroyed", BUFFER_SIZE);// oui c'est immonde
          printf("> Channel %s destroyed\n", buffer+6);
          break;
        }
        writeline(c_sock, "You are not in channel", BUFFER_SIZE);  // oui c'est immonde
        writeline(c_sock, buffer+6, BUFFER_SIZE);// oui c'est immonde
        break;

      case CREATE :
        if (create_group(groups, buffer+8)==0){// y'a un problème d'argument pour le buffer, y'a un \n qui fait chier
          writeline(c_sock, "You have create channel", BUFFER_SIZE);  // oui c'est immonde mais c'est pas problématique alors calm down
          writeline(c_sock, buffer+8, BUFFER_SIZE);
          writeline(c_sock, "\n", BUFFER_SIZE);
          printf("> Channel %s create\n", buffer+8);
          break;
        }
        writeline(c_sock, "channel ", BUFFER_SIZE);  // oui c'est immonde
        writeline(c_sock, buffer+8, BUFFER_SIZE);
        writeline(c_sock, "Already exist", BUFFER_SIZE);
        printf("> Channel %s already exist\n", buffer+8);
        break;

      case MSGW :
        w_sock = get_fd_client_by_name(*clients,buffer +5); // new function in list.c
        if (w_sock == -1){
          writeline(c_sock, "This user does not exist, use /who to see all the user connected", BUFFER_SIZE);
          break;
        }
        writeline(w_sock, buffer, BUFFER_SIZE); // peut être il faudra rajouter un argument s_sock à wirteline pour savoir qui parle, c'est pas frocément le server mtn
        // quelque probleme encore
        break;

      case MSGALL :
        w_sock = get_fd_client_by_name(*clients,buffer +5); // la moi je crérai une fonction qui rempli un tableau d'entier avec tous les fd des clients
        if (w_sock == -1){
          writeline(c_sock, "This user does not exist, use /who to see all the user connected\n", BUFFER_SIZE);
          break;
        }
        writeline(w_sock, buffer, BUFFER_SIZE); // peut être il faudra rajouter un argument s_sock à wirteline pour savoir qui parle, c'est pas frocément le server mtn
        // quelque probleme encore
        break;

      case JOIN :
        get_client_by_fd( *clients, &client, c_sock);
        res = add_client_in_group( groups,  client, buffer +6);

        if (res == -1){
          writeline(c_sock, "The channel", BUFFER_SIZE);
          writeline(c_sock, buffer+6, BUFFER_SIZE);
          writeline(c_sock, " does not exsit\n", BUFFER_SIZE);
          break;
        }
        writeline(c_sock, "You have joined the channel", BUFFER_SIZE); // peut être il faudra rajouter un argument s_sock à wirteline pour savoir qui parle, c'est pas frocément le server mtn
        writeline(c_sock, buffer+6, BUFFER_SIZE);
        // quelque probleme encore
        break;

      case NICK :
          res = set_nick(*clients, c_sock, buffer+6);  //the first 6 bytes are taken by the command (c'est pas plutot 'ignore' que taken)
          if(res==1){
            printf("> Client can't change nick : nick %s already taken.\n", buffer+6);
            writeline(c_sock, "Nickname is already taken. Please chose an other one.\n", BUFFER_SIZE);
          }
          else if(res==0){
            printf("> [%s] Nickname set\n", buffer+6);
            writeline(c_sock, "Nickname set. You can now use the server !\n", BUFFER_SIZE);
          }
          else if(res==2){
            printf("> [%s] Tried to use nickname beginning with 'Guest'\n", buffer+6);
            writeline(c_sock, "Can't use nickname beginning with 'Guest'\n", BUFFER_SIZE);
          }
        break;

      case WHO :
        printf("> [%s] Requested to see online users\n", nick);
        get_online_users(*clients, buffer);
        writeline(c_sock, buffer, BUFFER_SIZE);
        break;

      case WHOIS :
        //check if user exists
        printf("> [%s] ", nick);
        if(exists(*clients, buffer+7)){
          printf("Requested info on %s.\n", buffer+7);
          get_info(*clients, buffer, buffer+7);
          writeline(c_sock, buffer, BUFFER_SIZE);
        }
        else{
          printf("Requested info on non-existent user %s.\n", buffer+7);
          writeline(c_sock, "This user doesn't exist.\n", BUFFER_SIZE);
        }
    }

  /*if(strncmp(buffer,"/who",4)==0 && strlen(buffer)==5){
    writeline(c_sock,"> Client connected\n",BUFFER_SIZE);
    int i;
    for (i=0;i<2;i++){
      writeline(c_sock,clients[i].nickname,BUFFER_SIZE);
      writeline(c_sock,"\n",BUFFER_SIZE);
    }
    return -1;
  }

  if(strncmp(buffer,"/whois",6)==0){

  }

  else{
    printf("> [%s] : %s", clients[c_sock-4].nickname, buffer);
    //server response
    writeline(c_sock, buffer, BUFFER_SIZE);
  }
  return 0;*/
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

    //init the serv_addr structure
    struct sockaddr_in s_addr;
    memset(&s_addr, 0, sizeof(s_addr));
    init_serv_addr(atoi(argv[1]), &s_addr);

    //perform the binding
    //we bind on the tcp port specified

    do_bind(sock, &s_addr);

    //specify the socket to be a server socket
    listen(sock, MAX_CL);
    printf("> Waiting for connection : \n");

    //buffer init
    char * in_buf = (char *) malloc(6*sizeof(char));
    char buffer[BUFFER_SIZE];
    struct sockaddr * c_addr = (struct sockaddr *) malloc(sizeof(struct sockaddr));
    int c_addrlen;
    int c_sock;
    int s_read;
    S_CMD s_cmd;
    struct pollfd fds[MAX_CL+2]; // = (struct pollfd *) malloc((MAX_CL+2)*sizeof(struct pollfd));
    //fds = memset(fds, 0, (MAX_CL+2)*sizeof(struct pollfd));
    struct list * clients = NULL;
    struct listg * groups = NULL;

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
          memset(c_addr, 0, sizeof(*c_addr));
          c_sock = do_accept(sock, c_addr, &c_addrlen);
          fds[get_available_fd_index(fds)].fd = c_sock;
          add_client_to_list(&clients, c_sock, inet_ntoa( ((struct sockaddr_in * ) c_addr)->sin_addr)/*ip address*/, (int) ntohs( ((struct sockaddr_in * ) c_addr)->sin_port)/*port nb*/);
          printf("> Connection accepted \n");
          writeline(c_sock, "Welcome to the server. Please use /nick <your_pseudo> to login\n", BUFFER_SIZE);  // welcome message for the client
        }

        else{
          c_addrlen = sizeof(*c_addr);
          c_sock = do_accept(sock, c_addr, &c_addrlen);
          fds[get_available_fd_index(fds)].fd = c_sock;
          writeline(c_sock, "Server cannot accept incoming connections anymore. Please try again later.\n", BUFFER_SIZE);
          printf("> Connection to a client denied, too many clients already connected\n");
          fds[i].fd=-1; //we want to ignore this fd next loops
          //no point in adding client to client list
          close(c_sock);
        }
      }

      for(i=2;i<MAX_CL+2; i++){
        if(fds[i].revents & POLLIN){
          c_sock = fds[i].fd;
          //read what the client has to say
          s_read = readline(c_sock, buffer, BUFFER_SIZE);
          s_cmd = get_command(buffer, s_read);
          command(buffer, s_cmd, &clients, fds+i,&groups);
        }
      }

    //clean up client socket
  }

    //clean up server socket
    free(c_addr);
    close(sock);
    return 0;

}
