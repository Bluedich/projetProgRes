#include "common.h"

struct client{
  char nickname[MAX_NICK_SIZE];
  char group[MAX_NICK_SIZE];
  char ip_addr[BUFFER_SIZE];
  int port_nb;
  int hasNick;
  int fd;
  char con_date[512];
};

struct list{
  struct client * client;
  struct list * next;
};

int print_list(struct list * clients){
  struct list * current = clients;
  while(current!=NULL){
    printf("-nick:%s fd:%d\n", current->client->nickname, current->client->fd);
    current = current->next;
  }
  printf("End of list\n");
  return 0;
}

int get_client_by_nick(struct list * clients, struct client ** client, char nick[]){ //internal function
  struct list * current = clients;
  if(current == NULL)
    return -1;

  while (current != NULL){
    if(current->client->hasNick && strcmp(current->client->nickname, nick)==0){
      *client = current->client;
      return 0;
    }
    //sinon on parcour la liste
    current = current->next;
  }
  return -1; // il existe pas
}

void format_nick(char buffer[]){ //(internal) function to make sure nickname is formatted properly (mainly to remove trailing returnline)
  int i=0;
  while(i<BUFFER_SIZE-1 && buffer[i]!='\n'){
    i++;
  }
  buffer[i]='\0';
}
/*
int add_existing_client_to_list(struct list ** clients, struct client * client){  // doit être mis dans list.c
  assert(clients);
  struct list * new_start = (struct list *) malloc(sizeof(struct list));
  assert(client);
  assert(new_start);

  new_start->client = client;
  new_start->next = *clients;
  *clients=new_start;
}
*/

int get_fd_client_by_name(struct list * clients, char name[]){
  struct client * client = (struct client *) malloc(sizeof(struct client));
  int res = get_client_by_nick(clients, &client, name);
  if (res==-1) {
    printf("ERROR : Tried to get fd of non-existing client.\n");
    return -1;
  }
  return client->fd;
}

int get_client_by_fd(struct list * clients, struct client ** client, int fd){ //internal function
  struct list * current = clients;
  if(current == NULL) return -1;
  while (current != NULL){
    if(current->client->fd==fd){
      if (client==NULL){
        return 0;
      }
      *client = current->client;
      return 0;
    }
    //sinon on parcour la liste
    current = current->next;
  }
  return -1; // il n'existe pas
}

int get_fd_client(struct list *clients, int fd[]){  // return the fd of all the client in the linked list
  struct list * current = clients;
  int i =0;
  if(current == NULL)
    return i; //renvoi le nombre de client

  while (current != NULL){
    fd[i] = current->client->fd;
    i++;
    // on parcour la liste
    current = current->next;
  }

  return i; // renvoie le nombre de client
}

int nb_client_in_list(struct list *clients){  // return the number of client in the linked list
  struct list * current = clients;
  int i =0;
  if(current == NULL)
    return i; // il n'y pas pas de client

  while (current != NULL){
    i++;
    // on parcour la liste
    current = current->next;
  }
  return i; // il existe pas
}



int exists(struct list * clients, char nick[]){ // return if the client is on the linked list or not
  struct list * current = clients;
  if(current == NULL)
    return 0;

  while (current != NULL){
    format_nick(nick);
    if(current->client->hasNick && strcmp(current->client->nickname, nick)==0){
      return 1;
    }
    current = current->next;
  }
  return 0; // il existe pas
}

int add_client_to_list(struct list ** clients, int fd, char ip[], int port){
  assert(clients);
  struct list * new_start = (struct list *) malloc(sizeof(struct list));
  struct client * client = (struct client *) malloc(sizeof(struct client));
  assert(client);
  assert(new_start);
  client->fd = fd;
  client->hasNick = 0;
  strcpy(client->ip_addr, ip);
  client->port_nb = port;
  strcpy(client->nickname, "Guest");
  //get time info
  time_t rawtime;
  time(&rawtime);
  strcpy(client->con_date, asctime(localtime(&rawtime)));
  memset(client->group, 0, MAX_NICK_SIZE);

  new_start->client = client;
  new_start->next = *clients;
  *clients=new_start;
}

int get_online_users(struct list * clients, char buffer[]){
  struct list * current = clients;
  int nb_guest=0;
  memset(buffer, 0, BUFFER_SIZE);
  char temp[BUFFER_SIZE];
  strcat(buffer, "Online users :\n");
  while(current!=NULL){
    memset(temp, 0, BUFFER_SIZE);
    if ( current->client->hasNick){
        sprintf(temp, "               - %s\n", current->client->nickname);
        strcat(buffer, temp);
    }
    else{
      nb_guest++;
    }
    current = current->next;
  }
  if (nb_guest>1){
      sprintf(temp, "               ... and %d guests\n", nb_guest);
      strcat(buffer, temp);
  }
  else if (nb_guest==1){
    sprintf(temp, "                 ... and 1 guest)\n");
    strcat(buffer, temp);
  }
  strcat(buffer, " Use /whois <nickname> to get more info on a user\n");
  return 0;
}

int get_ip_addr(struct list * clients, char nick[], char buffer[]){
  struct client * client;
  char nickname[BUFFER_SIZE];
  strcpy(nickname, nick);
  format_nick(nickname);
  get_client_by_nick(clients, &client, nickname);
  memset(buffer, 0, BUFFER_SIZE);
  strcpy(buffer, client->ip_addr);
  return 0;
}

int get_info(struct list * clients, char buffer[], char nick[]){
  struct client * client;
  char nickname[BUFFER_SIZE];
  strcpy(nickname, nick);
  format_nick(nickname);
  get_client_by_nick(clients, &client, nickname);
  memset(buffer, 0, BUFFER_SIZE);
  if (strlen(client->group)==0){
      sprintf(buffer, "%s connected on %s with IP address %s and port number %d.", client->nickname, client->con_date, client->ip_addr, client->port_nb);
  }
  else {
    sprintf(buffer, "%s is in the channel %s. He has connected on %s with IP address %s and port number %d.", client->nickname, client->group, client->con_date, client->ip_addr, client->port_nb);
  }
  return 0;
}

int set_nick(struct list * clients, int fd, char nick[]){
  struct client * client = (struct client *) malloc(sizeof(struct client));
  memset(client, 0, sizeof(struct client));
  int i;
  format_nick(nick);
  if (strlen(nick)>MAX_NICK_SIZE){
    printf("ERROR unvalid nick name (too long)");
    return 4;
  }
  if (strncmp( nick, "Guest", 5) == 0){
    printf("ERROR unvalid nick name 'Guest'");
    return 2;
    }
  if(get_client_by_nick(clients, &client, nick)==0){
    printf("ERROR unvalid nick name (already taken)");
    return 1;
  }
  if(get_client_by_fd(clients, &client, fd) == -1){
    printf("ERROR getting client by fd in set_nick\n");
    return -1;
  }
  client->hasNick = 1;
  for (i=0;i<strlen(nick);i++){
    if (strncmp(nick+i," ",1)==0){
      printf("ERROR unvalid nick name");
      return 3;
    }
  }
  strcpy(client->nickname, nick);
  return 0;
}

int get_nick(struct list * clients, int fd, char nick[]){
  struct client * client = (struct client *) malloc(sizeof(struct client));
  memset(client, 0, sizeof(struct client));
  int i;
  format_nick(nick);
  if(get_client_by_fd(clients, &client, fd) == -1){
    printf("ERROR getting client by fd in get_nick\n");
    return -1;
  }
  for (i=0;i<strlen(nick);i++){ // inutile
    if (strncmp(nick+i," ",1)==0){
      printf("ERROR unvalid nick name");
      return 3;
    }
  }
  strcpy(nick, client->nickname);
  return 0;
}

int has_nick(struct list * clients, char buffer[], int fd){ //returns nick in "buffer", returns -1 if error, 0 if false, 1 if true
  struct client * client = (struct client *) malloc(sizeof(struct client));
  memset(client, 0, sizeof(struct client));
  if(get_client_by_fd(clients, &client, fd) == -1){
    printf("ERROR getting client by fd in has_nick\n");
    return -1;
  }
  strcpy(buffer, client->nickname);
  return client->hasNick;
}

int has_group(struct list * clients, char buffer[], int fd){ //returns nick in "buffer", returns -1 if error, 0 if false > 0 if true
  struct client * client = (struct client *) malloc(sizeof(struct client));
  memset(client, 0, sizeof(struct client));
  if(get_client_by_fd(clients, &client, fd) == -1){
    printf("ERROR getting client by fd in has_group\n");
    return -1;
  }
  strcpy(buffer, client->group);
  int len = strlen(client->group);
  return len;
}

int change_group(struct list * clients, char buffer[], int fd){ //returns nick in "buffer", returns -1 if error, 0 if it worked
  struct client * client = (struct client *) malloc(sizeof(struct client));
  memset(client, 0, sizeof(struct client));
  if(get_client_by_fd(clients, &client, fd) == -1){
    printf("ERROR getting client by fd in change_group\n");
    return -1;
  }
  strcpy(client->group, buffer);
  return 0;
}

int remove_client(struct list ** clients, int fd){
  struct list * before = *clients;
  if(before == NULL){
    printf("ERROR trying to remove : list empty\n");
    return -1;
  }
  if(before->client->fd == fd){
    *clients = before->next;
    return 0;
  }

  struct list * current = before->next;
  if(current == NULL)
    return -1;

  while (current->next != NULL){
    if(current->client->fd==fd){ // c'est lui qu'on supprime
      before->next = current->next;
      return 0;
    }
    //sinon on parcour la liste
    current = current->next;
    before = before->next;
  }
  if(current->client->fd==fd){ // c'est lui qu'on supprime
    before->next = NULL;
    return 0;
  }
  return -1; // il existe pas
}

/*int main(int argc, char ** agrv){ //for testing purposes
  struct list * clients = NULL;
  char buffer[BUFFER_SIZE];
  struct client * client = (struct client *) malloc(sizeof(struct client));
  memset(client, 0, sizeof(struct client));
  //add_client_to_list(&clients, -1243);
  //print_list(clients);
  add_client_to_list(&clients, -123543);
  print_list(clients);
  set_nick(clients, -123543, "Ta mère");
  print_list(clients);
  remove_client(&clients, -123543);
  print_list(clients);
  add_client_to_list(&clients, -543);
  print_list(clients);
  /*add_client_to_list(&clients, 235);
  int res;
  res = get_client_by_nick(clients, &client, "Ta mère");
  printf("%d\n", res);
  print_list(clients);
  res = get_client_by_nick(clients, &client, "Ta mère");
  print_list(clients);
  printf("%d\n", res);
  //add_client_to_list(&clients, 4);
  //set_nick(clients, 4, "Ta mère");
}*/
