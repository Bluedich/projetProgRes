#include "common.h"

struct client{
  char nickname[BUFFER_SIZE];
  int hasNick;
  int fd;
};

struct list{
  struct client * client;
  struct list * next;
};

int add_client_to_list(struct list ** clients, int fd){
  assert(clients);
  struct list * new_start = (struct list *) malloc(sizeof(struct list));
  struct client * client = (struct client *) malloc(sizeof(struct client));
  assert(client);
  assert(new_start);
  client->fd = fd;
  client->hasNick = 0;
  strcpy(client->nickname, "Guest");
  new_start->client = client;
  new_start->next = *clients;
  *clients=new_start;
}

int set_nick(struct list * clients, int fd, char * nick){
  struct client * client = (struct client *) malloc(sizeof(struct client));
  memset(client, 0, sizeof(struct client));
  if(get_client_by_nick(clients, &client, nick)==0){
    return 1;
  }
  if(get_client_by_fd(clients, &client, fd) == -1){
    printf("ERROR getting client fd\n");
    return -1;
  }
  client->hasNick = 1;
  strcpy(client->nickname, nick);
  //free(client);
  return 0;
}

int has_nick(struct list * clients, char * buffer, int fd){ //returns nick in "buffer", returns -1 if error, 0 if false, 1 if true
  struct client * client = (struct client *) malloc(sizeof(struct client));
  memset(client, 0, sizeof(struct client));
  if(get_client_by_fd(clients, &client, fd) == -1){
    printf("ERROR getting client fd\n");
    return -1;
  }
  strcpy(buffer, client->nickname);
  return client->hasNick;
  //free(client);
}

int remove_client(struct list ** clients, int fd){
  struct list * before = *clients;
  if(before == NULL)
    return -1;

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

int get_client_by_nick(struct list * clients, struct client ** client, char * nick){ //internal function
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

int get_client_by_fd(struct list * clients, struct client ** client, int fd){ //internal function
  struct list * current = clients;
  if(current == NULL)
    return -1;

  while (current != NULL){
    if(current->client->fd==fd){
      *client = current->client;
      return 0;
    }
    //sinon on parcour la liste
    current = current->next;
  }

  return -1; // il n'existe pas
}

/*
int main(int argc, char ** agrv){
  struct list * clients = NULL;
  char buffer[BUFFER_SIZE];
  add_client_to_list(&clients, -123543);
  add_client_to_list(&clients, -1243);
  add_client_to_list(&clients, -543);
  add_client_to_list(&clients, 235);
  struct client * client = (struct client *) malloc(sizeof(struct client));
  memset(client, 0, sizeof(struct client));

  change_nick(clients, 235, "Ta mère");
  change_nick(clients, -543, "Ta mère");
}*/
