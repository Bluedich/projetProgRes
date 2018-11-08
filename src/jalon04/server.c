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

#include "common.h"
#include "list.h"
#include "group.h"

int do_socket(){
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if(fd == -1)
    error("ERROR creating socket");
  int yes = 1;
  // set socket option, to prevent "already in use" issue when rebooting the server right on
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
      error("ERROR setting socket options");
  return fd;
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
  //separate(buffer); // pas super beau, mais c'est pour enlever le "[<username>] : " avant les messages
  //separate(buffer); // et le problème que l'on a c'est que l'on peut aussi avoir des "[<groupname>] [<username>] : "
 // autant pour moi, ce dernier cas n'arrive enfait pas, car seul le client recevra ce type de message normalement
  if((s_read == 0) || (strncmp(buffer,"/quit",5)==0 && strlen(buffer)==6)) //s_read = 0 probably means client has closed socket
    return QUIT;

  if((strncmp(buffer,"/quit",5)==0 && strlen(buffer)>6))
    return LEAVE;  // leave a group

  if(strncmp(buffer,"/create ",8)==0 && strlen(buffer)>9)
    return CREATE;  // create a group

  if(strncmp(buffer,"/join ",6)==0 && strlen(buffer)>7)
    return JOIN;  // join a group

  if(strncmp(buffer,"/msg ",5)==0 && strlen(buffer)>6)
    return MSGW;  // whisper to a user

  if(strncmp(buffer,"/msgall ",8)==0 && strlen(buffer)>8)
    return MSGALL;  // send the message in broadcast

  if(strncmp(buffer,"/nick ",6)==0 && strlen(buffer)>7)
    return NICK;  //set a nickname

  if(strncmp(buffer,"/who",4)==0 && strlen(buffer)==5)
    return WHO;  // get the infomation of all the user connect to the server

  if(strncmp(buffer,"/whois ",7)==0 && strlen(buffer)>8)
    return WHOIS;

  return MSG;
}

int command(char * buffer, S_CMD cmd, struct list ** clients, struct pollfd * fd, struct listg ** groups){
    char nick[BUFFER_SIZE];
    memset(nick, 0, BUFFER_SIZE);
    char client_group[BUFFER_SIZE];
    memset(client_group, 0, BUFFER_SIZE);
    char nickw[BUFFER_SIZE];
    memset(nickw, 0, BUFFER_SIZE);
    char msg[BUFFER_SIZE];
    memset(nickw, 0, BUFFER_SIZE);
    int c_sock = fd->fd;// sock client
    int i;
    int res;
    int w_sock;         // stock the socket of the client you whisper to
    int nb_client;      // number of client conected to the server
    int sock_tab[20];   //array wich will contain all the sock of all the clients in a group
    int hasNick = has_nick(*clients, nick, c_sock);
    int hasGroup = has_group(*clients,client_group,c_sock);
    format_nick(nick);
    if(!hasNick && cmd!=NICK && cmd!=QUIT){ //Not allowed to talk until a nickname is chosen
      writeline(c_sock,"Server","", "Please use /nick <your_pseudo> to login.", BUFFER_SIZE);
      return 0;
    }

    switch(cmd){
      case MSG :  // on parle dans le groupe actif
        if (strlen(client_group)==0){
          writeline(c_sock,"Server","", "You are not in a channel. Please use /join <channel name> to join a channel", BUFFER_SIZE);
        }
        else if(pop_of_group(groups, client_group)==1){
          writeline(c_sock,"Server","", "No point in writing in the channel, no one else is here...", BUFFER_SIZE);
        }
        else {
          write_in_group(groups, client_group, nick, c_sock, buffer);
          writeline(c_sock,"","","/newprompt", BUFFER_SIZE); //to force a new <name> prompt in client
        }
        break;

      case QUIT :
        if(hasGroup){
          sprintf(buffer,"/quit %s\n",client_group);
          command(buffer, LEAVE, clients,  fd, groups);
      }
        remove_client(clients, c_sock);
        close(c_sock);
        fd->fd = -1; //flag structure as unused
        printf("> [%s] disconnected\n", nick);
        break;

      case LEAVE :
        format_nick(buffer);  // pour utiliser separate
        separate(buffer);     // enlève la commande

        if (hasGroup==0){
          printf("Client '%s' tried to leave the channel '%s' but isn't in any channel.\n",nick,buffer);
          sprintf(msg,"You are not in any channel.\n");
          writeline(c_sock,"Server","", msg, BUFFER_SIZE);
          break;
        }
        res = remove_client_in_group(groups,c_sock,buffer);
        if (res==-1){
          printf("> Client '%s' try to leave the channel '%s' but this channel does not exist.\n",nick,buffer);
          sprintf(msg,"The group does not exist '%s', you can't leave it.",buffer);
          writeline(c_sock,"Server","", msg, BUFFER_SIZE);
          break;
        }
        if (res==-2){
          printf("> Client '%s' tried to leave the channel '%s' but he is not in this channel.\n",nick,buffer);
          sprintf(msg,"You are not in the channel '%s', you can't leave it.",buffer);
          writeline(c_sock,"Server","", msg, BUFFER_SIZE);
          break;
        }
        if (res==0){
          change_group(*clients,"",c_sock);
          printf("> Client '%s' has left the channel '%s'.\n",nick,buffer);
          sprintf(msg,"/username %s "BOLDBLACK"<Server>"RESET" You have quit the channel '%s'.", nick, buffer);
          writeline(c_sock,"","", msg, BUFFER_SIZE);
          break;
        }
        if (res==10){
          remove_group(groups,buffer);
          printf("> Client '%s' has left the channel '%s' and Channel '%s' destroyed.\n",nick,buffer, buffer);
          change_group(*clients,"",c_sock);
          sprintf(msg,"/username %s "BOLDBLACK"<Server>"RESET" You have quit the channel '%s' and the channel is destroyed.", nick, buffer);
          writeline(c_sock,"","", msg, BUFFER_SIZE);
          break;
        }


      case CREATE :
        format_nick(buffer);  // pour utiliser separate
        separate(buffer);     // enlève la commande du buffer

        if (create_group(groups, buffer)==0){
          printf("> Channel %s created.\n", buffer);
          sprintf(msg,"You have created the channel '%s'.",buffer);
          writeline(c_sock,"Server","", msg, BUFFER_SIZE);
        }
        else {
          printf("> Channel %s already exist.\n", buffer);
          sprintf(msg,"Channel %s already exist.", buffer);
          writeline(c_sock,"Server","", msg, BUFFER_SIZE);
        }
        break;

      case JOIN :
        separate(buffer);     // removes command from buffer

        if( hasGroup!=0 ){
          sprintf(msg,"You're already in the channel '%s'. Please leave it before trying join the channel '%s'.",client_group,buffer);
          writeline(c_sock,"Server","", msg, BUFFER_SIZE);
          break;
        }
        res = add_client_in_group( groups, c_sock, buffer);

        if (res == -2){;
          sprintf(msg,"Channel '%s' is full.", buffer);
          writeline(c_sock,"Server","", msg, BUFFER_SIZE);
        }
        else if (res == -1){;
          sprintf(msg,"Channel '%s' doesn't exist.", buffer);
          writeline(c_sock,"Server","", msg, BUFFER_SIZE);
        }
        else if (res == -3){
          change_group(*clients,buffer,c_sock);
          sprintf(msg," You are already in the channel '%s'.",buffer);
          writeline(c_sock,"Server","", msg, BUFFER_SIZE);
        }
        else {
          change_group(*clients,buffer,c_sock);
          sprintf(msg,"/username %s><%s "BOLDBLACK"<Server>"RESET" You have joined the channel '%s'.",nick, buffer, buffer);
          writeline(c_sock,"","", msg, BUFFER_SIZE);
        }
        break;

      case MSGW :
        separate(buffer);     // pour enlever la commande
        get_arg_in_command( buffer, nickw);  // pour obtenir le nom (pourrait plus ou moins être utilise pour obtenir la commande)
        w_sock = get_fd_client_by_name(*clients, nickw ); // new function in list.c
        if (w_sock == -1){
          writeline(c_sock,"Server","", "This user does not exist, use /who to see all the user connected.", BUFFER_SIZE);
          break;
        }
        if (w_sock== c_sock){
          writeline(c_sock,"Server","", "You can't whisper to yourself.", BUFFER_SIZE);
          break;
        }
        separate(buffer);
        sprintf(msg, "%s\n", buffer);
        writeline(w_sock,"whisper", nick, buffer, BUFFER_SIZE);
        writeline(c_sock,"","","/newprompt", BUFFER_SIZE); //to force a new <name> prompt in client
        break;

      case MSGALL :
        //format_nick(buffer);
        separate(buffer);     // pour enlever le "/msgall "
        nb_client=nb_client_in_list(*clients);
        memset(sock_tab,0,20);
        res = get_fd_client(*clients,sock_tab); // rempli un tableau d'entier avec tous les fd des clients
        if (res == 1){// ça n'arrivera plus ou moins jamais
          writeline(c_sock,"Server","", "You are the unique user of the Server", BUFFER_SIZE);
          break;
        }
        for (res=0;res<nb_client;res++){// utilisation de res pour faire le clochard et pas definir un i, c'est un peu con en vrai
          if (sock_tab[res]!=c_sock){// ne l'affiche pas à l'expéditeur
          sprintf(msg,"%s",buffer);
            writeline(sock_tab[res], "broadcast", nick, buffer, BUFFER_SIZE); // peut être il faudra rajouter un argument s_sock à wirteline pour savoir qui parle, c'est pas frocément le server mtn
          }
        }
        writeline(c_sock,"","","/newprompt", BUFFER_SIZE); //to force a new <name> prompt in client
        break;

      case NICK :
          separate(buffer);
          res = set_nick(*clients, c_sock, buffer);
          if(res==1){
            printf("> Client can't change nick : nick %s already taken.\n", buffer);
            writeline(c_sock,"Server","", "Nickname is already taken. Please chose an other one.", BUFFER_SIZE);
          }
          else if(res==0){
            printf("> [%s] Nickname set.\n", buffer);
            memset(buffer, 0, BUFFER_SIZE);
            has_nick(*clients, nick, c_sock);
            if(hasGroup){
              sprintf(buffer, "/username %s><%s "BOLDBLACK"<Server>"RESET" Nickname set. You can now use the server !", nick, client_group);
            }
            else{
              sprintf(buffer, "/username %s "BOLDBLACK"<Server>"RESET" Nickname set. You can now use the server !", nick);
            }
            writeline(c_sock,"","",buffer, BUFFER_SIZE);
          }
          else if(res==2){
            printf("> [%s] Tried to use nickname beginning with 'Guest'", buffer);
            writeline(c_sock,"Server","", "Can't use nickname beginning with 'Guest'", BUFFER_SIZE);
          }
          else if(res==3){
            printf("> [%s] Tried to use nickname containing ' '", buffer);
            writeline(c_sock,"Server","", "Can't use nickname containing ' '", BUFFER_SIZE);
          }
        break;

      case WHO :
        printf("> [%s] Requested to see online users", nick);
        get_online_users(*clients, buffer);
        writeline(c_sock,"Server","", buffer, BUFFER_SIZE);
        break;

      case WHOIS :
        //check if user exists
        format_nick(buffer);// je sais pas si c'est nécessaire
        separate(buffer);
        printf("> [%s] \n", nick);
        if(exists(*clients, buffer)){
          printf("Requested info on %s.\n", buffer);
          get_info(*clients, buffer, buffer);
          writeline(c_sock,"Server","", buffer, BUFFER_SIZE);
        }
        else{
          printf("Requested info on non-existent user %s.\n", buffer);
          writeline(c_sock,"Server","", "This user doesn't exist.", BUFFER_SIZE);
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
          printf("> Closing server");
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
          printf("> Connection accepted ");
          writeline(c_sock,"Server","", "Welcome to the server. Please use /nick <your_pseudo> to login", BUFFER_SIZE);  // welcome message for the client
        }

        else{
          c_addrlen = sizeof(*c_addr);
          c_sock = do_accept(sock, c_addr, &c_addrlen);
          fds[get_available_fd_index(fds)].fd = c_sock;
          writeline(c_sock,"Server","", "Server cannot accept incoming connections anymore. Please try again later.", BUFFER_SIZE);
          printf("> Connection to a client denied, too many clients already connected");
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
