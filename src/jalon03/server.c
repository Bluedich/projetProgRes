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
struct group{
  struct list * clients;
  char name[512];
};
struct client{
  char nickname[MAX_NICK_SIZE];
  char activegroup[MAX_NICK_SIZE];
  char ip_addr[17];
  int port_nb;
  int hasNick;
  int fd;
  char con_date[512];
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
    return JOIN;  // envoi un message en broadcast

  if(strncmp(buffer,"/msg ",5)==0 && strlen(buffer)>6)
    return MSGW;  // whisper to a user

  if(strncmp(buffer,"/msgall ",8)==0 && strlen(buffer)>8)
    return MSGALL;  // send the message in broadcast
    // On pourrait considérer le broadcast également comme un groupe, par défault actif, avec 3 moyen différent de parler en broadcast /msgall juste normalement

 // Il faudrait aussi une fonction pour parler dans un groupe même si il est pas actif /msggroup <groupname> <msg>
 // Il faut alors bien gérer le fait si l'on est bien dans le groupe, et puis envoyer le message dans le format [<groupname>] [<username>] : <msg>
 // Tout ça se fait enfait relativement bien, mais proprement je sais pas trop

  if(strncmp(buffer,"/nick ",6)==0 && strlen(buffer)>7)
    return NICK;  //set a nickname

  if(strncmp(buffer,"/who",4)==0 && strlen(buffer)==5)
    return WHO;  // get the infomation of all the user connect to the server
// On pourrait rajouter une commande du style /whoareingroup <channel> (qui renvoie toues les user d'un groupes) et /group (qui renvoi tous les groups existant)
  if(strncmp(buffer,"/whois ",7)==0 && strlen(buffer)>8)
    return WHOIS;
// On pourrait rajouter l'information des groupes auxquels appartient le joueur
  return MSG;
}

int command(char * buffer, S_CMD cmd, struct list ** clients, struct pollfd * fd, struct listg ** groups){
    char nick[BUFFER_SIZE];
    memset(nick, 0, BUFFER_SIZE);
    char nickw[BUFFER_SIZE];
    memset(nickw, 0, BUFFER_SIZE);
    struct client * client=NULL;
    struct group * group=NULL;
    int c_sock = fd->fd;// sock client
    int res;
    int w_sock;         // stock the socket of the client you whisper to
    int nb_client;      // number of client conected to the server
    int sock_tab[20];   //array wich will contain all the sock of all the clients in a group
    int hasNick = has_nick(*clients, nick, c_sock);
    format_nick(nick);
    if(!hasNick && cmd!=NICK && cmd!=QUIT){ //Not allowed to talk until a nickname is chosen
      writeline(c_sock,"Server","", "Please use /nick <your_pseudo> to login.\n", BUFFER_SIZE);
      return 0;
    }

    switch(cmd){
      case MSG :  // on parle dans le groupe actif
        //format_nick(buffer);
        get_client_by_fd( *clients, &client, c_sock);
        res = get_group_by_name(groups, &group, client->activegroup);
        if (res == -1){
          writeline(c_sock,"Server","", "You are not active in an existing group\n Use /active to actived a group\n", BUFFER_SIZE);
          break;
        }
        // la ça me fait penser que je penses que l'on a le problème que lorsqu'un a client quitte un groupe, cela ne change pas son activegroup, pas dur à changer
        // il faut regarder d'abord si le client est dans le groupe aussi
        nb_client=nb_client_in_list(group->clients);
        memset(sock_tab,0,20);
        res = get_fd_client(group->clients,sock_tab); // rempli un tableau d'entier avec tous les fd des clients
        if (res == -1){               // ça n'arrivera plus ou moins jamais
          writeline(c_sock,"Server",client->activegroup, "You are the unique user of the group\n", BUFFER_SIZE);
          break;
        }
        for (res=0;res<nb_client;res++){    // utilisation de res pour faire le clochard et pas definir un i, c'est un peu con en vrai
          if (sock_tab[res]!=c_sock){       // ne l'affiche pas à l'expéditeur
            writeline(sock_tab[res], nick,client->activegroup, buffer, BUFFER_SIZE);
          }
        }
        break;
        printf("> [%s] %s", nick, buffer);
        writeline(c_sock,nick,"", buffer, BUFFER_SIZE);
        break;

      case QUIT :
        remove_client(clients, c_sock);
        close(c_sock);
        fd->fd = -1; //to let the program know to ignore this structure in the future
        printf("> [%s] disconnected\n", nick);
        break;

      case LEAVE :
        format_nick(buffer);  // pour utiliser separate
        separate(buffer);     // enlève la commande
        get_client_by_fd( *clients, &client, c_sock);     // get the struct client of the client
        res = remove_client_in_group(groups,c_sock,buffer);
        if (res==0){
          buffer = strcat(buffer," is not one of your channel anymore\n");
          // on pourrait remettre l'active groupe par défault par exemple si c'était le même channel que active groupe que l'on quitte
          memset(client->activegroup,0,BUFFER_SIZE);
          writeline(c_sock,"Server","", buffer, BUFFER_SIZE);
          break;
        }
        if (res==10){
          printf("> Channel %s destroyed\n", buffer);
          // on pourrait remettre l'active groupe par défault par exemple si c'était le même channel que active groupe que l'on quitte
          buffer = strcat(buffer," is not one of your channel anymore, and the channel is destroyed\n");
          memset(client->activegroup,0,BUFFER_SIZE);
          writeline(c_sock,"Server","", buffer, BUFFER_SIZE);
          remove_group(groups,buffer);
          break;
        }
        buffer = strcat(buffer," is not one of your channel\nUse /whois <your_pseudo> to see in wich channel you are\n");
        writeline(c_sock,"Server","", buffer, BUFFER_SIZE);
        break;

      case CREATE :
        format_nick(buffer);  // pour utiliser separate
        separate(buffer);     // enlève la commande de buffer

        if (create_group(groups, buffer)==0){
          printf("> Channel %s created\n", buffer);
          buffer = strcat(buffer," is now new channel\n");
          writeline(c_sock,"Server","", buffer, BUFFER_SIZE);

          break;
        }
        printf("> Channel %s already exist\n", buffer);
        buffer = strcat(buffer," is already an existing channel\n");
        writeline(c_sock,"Server","", buffer, BUFFER_SIZE);
        break;

      case JOIN :
        format_nick(buffer);  // pour utiliser separate (rajoute \0)pas sure que ce soit utile en réalité, en tout cas on peut suremnt mieux faire
        separate(buffer);     // enlève la commande de buffer

        get_client_by_fd( *clients, &client, c_sock);     // get the struct client of the client
        printf(">  (%s) \n", client->nickname);
        res = add_client_in_group( clients, groups, c_sock, buffer);
        if (res == -1){
          buffer = strcat(buffer," is not an existing channel\n");
          writeline(c_sock,"Server","", buffer, BUFFER_SIZE);
          break;
        }
        if (res == 1){
          strcpy(client->activegroup,buffer);
          buffer = strcat(buffer," is your channel\n");
          writeline(c_sock,"Server","", buffer, BUFFER_SIZE);
          break;
        }
        strcpy(client->activegroup,buffer);
        buffer = strcat(buffer," is now your channel\n");
        writeline(c_sock,"Server","", buffer, BUFFER_SIZE);
        break;

      case MSGW :
        format_nick(buffer);  // pour utiliser separate
        separate(buffer);     // pour enlever la commande
        get_name_in_command( nickw, buffer);  // pour obtenir le nom (pourrait plus ou moin être utilise pour obtenir la commande)
        w_sock = get_fd_client_by_name(*clients,nickw ); // new function in list.c
        if (w_sock == -1){
          writeline(c_sock,"Server","", "This user does not exist, use /who to see all the user connected\n", BUFFER_SIZE);
          break;
        }
        separate(buffer);
        writeline(w_sock,nick,"", buffer, BUFFER_SIZE);
        break;

      case MSGALL :
        format_nick(buffer);
        separate(buffer);     // pour enlever le "/msgall "
        nb_client=nb_client_in_list(*clients);
        memset(sock_tab,0,20);
        res = get_fd_client(*clients,sock_tab); // rempli un tableau d'entier avec tous les fd des clients
        if (res == 1){// ça n'arrivera plus ou moins jamais
          writeline(c_sock,"Server","", "You are the unique user of the Server\n", BUFFER_SIZE);
          break;
        }
        for (res=0;res<nb_client;res++){// utilisation de res pour faire le clochard et pas definir un i, c'est un peu con en vrai
          if (sock_tab[res]!=c_sock){// ne l'affiche pas à l'expéditeur
            writeline(sock_tab[res], nick,"", buffer, BUFFER_SIZE); // peut être il faudra rajouter un argument s_sock à wirteline pour savoir qui parle, c'est pas frocément le server mtn
          }
        }
        break;

      case NICK :
          separate(buffer);
          res = set_nick(*clients, c_sock, buffer);  //the first 6 bytes are taken by the command (c'est pas plutot 'ignore' que taken)
          if(res==1){
            printf("> Client can't change nick : nick %s already taken.\n", buffer);
            writeline(c_sock,"Server","", "Nickname is already taken. Please chose an other one.\n", BUFFER_SIZE);
          }
          else if(res==0){
            printf("> [%s] Nickname set\n", buffer);
            writeline(c_sock,"Server","", "Nickname set. You can now use the server !\n", BUFFER_SIZE);
          }
          else if(res==2){
            printf("> [%s] Tried to use nickname beginning with 'Guest'\n", buffer);
            writeline(c_sock,"Server","", "Can't use nickname beginning with 'Guest'\n", BUFFER_SIZE);
          }
          else if(res==3){
            printf("> [%s] Tried to use nickname containing ' '\n", buffer);
            writeline(c_sock,"Server","", "Can't use nickname containing ' '\n", BUFFER_SIZE);
          }
        break;

      case WHO :
        printf("> [%s] Requested to see online users\n", nick);
        get_online_users(*clients, buffer);
        writeline(c_sock,"Server","", buffer, BUFFER_SIZE);
        break;

      case WHOIS :
        //check if user exists
        format_nick(buffer);// je sais pas si c'est nécessaire
        separate(buffer);
        printf("> [%s] ", nick);
        if(exists(*clients, buffer)){
          printf("Requested info on %s.\n", buffer);
          get_info(*clients, buffer, buffer);
          writeline(c_sock,"Server","", buffer, BUFFER_SIZE);
        }
        else{
          printf("Requested info on non-existent user %s.\n", buffer);
          writeline(c_sock,"Server","", "This user doesn't exist.\n", BUFFER_SIZE);
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
          writeline(c_sock,"Server","", "Welcome to the server. Please use /nick <your_pseudo> to login\n", BUFFER_SIZE);  // welcome message for the client
        }

        else{
          c_addrlen = sizeof(*c_addr);
          c_sock = do_accept(sock, c_addr, &c_addrlen);
          fds[get_available_fd_index(fds)].fd = c_sock;
          writeline(c_sock,"[Server] :","", "Server cannot accept incoming connections anymore. Please try again later.\n", BUFFER_SIZE);
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
