#include "common.h"
#include <time.h>
#include "list.h"

/////////////////////////////////
/// Tout ça ne devrait pas être la mais dans list, mais je sais pas faire,
////////////////////
struct client{  // anomali
  char nickname[MAX_NICK_SIZE];
  char ip_addr[17];
  int port_nb;
  int hasNick;
  int fd;
  char con_date[512];
};

struct list{  // anomali
  struct client * client;
  struct list * next;
};


int remove_client(struct list ** clients, int fd){  // existe dejà dans list.c
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

int add_existing_client_to_list(struct list ** clients, struct client * client){  // doit être mis dans list.c
  assert(clients);
  struct list * new_start = (struct list *) malloc(sizeof(struct list));
  assert(client);
  assert(new_start);

  new_start->client = client;
  new_start->next = *clients;
  *clients=new_start;
}
//////////
//Tout ce qui est plus haut ne devrait pas être là mais dans list.c
//////
struct group{
  struct list * clients;
  char name[512];
};

struct listg{
  struct group * group;
  struct listg * next;
};

int get_group_by_name(struct listg ** groups, /*struct group ** group,*/ char name[]){ //internal function
  struct listg * current = *groups;                                               // group with name name exist ?
  if(current == NULL)
    return -1;

  while (current != NULL){
      if(strcmp(current->group->name, name)==0){
      //*group = current->group;            /
      return 0;
    }
    //sinon on parcour la liste
    current = current->next;
  }
  return -1; // il existe pas
}

int get_client_fd_in_group(struct listg ** groups, char name, int *fd[]){ // le but est de renvoyé un tableau avec les fd de tous les clients dans le groupe
  assert(groups);
  struct group * group = (struct group *) malloc(sizeof(struct group));
   get_group_by_name(groups, /* group,*/  name[]);  // on obtient le bon groupe
   //une fonction dans client qui renvoi les fd de tous les clients(en fait, faut le gros de la fonction plutot dans list.c je pense);

  return 0;
}

int create_group(struct listg ** groups, char name[]){
  assert(groups);
  struct listg * new_start = (struct listg *) malloc(sizeof(struct listg));
  struct group * group = (struct group *) malloc(sizeof(struct group));
  assert(group);
  assert(new_start);

  memset(group, 0, sizeof(struct group));

  if(get_group_by_name(groups,/* &group,*/ name)==0){ // if the name exist, do not create the group
    printf("ERROR trying create the group %s : there already an existing group with this name\n",name);
    return 1;
  }

  strcpy(group->name, name);

  new_start->group = group;
  new_start->next = *groups;
  *groups=new_start;
  return 0;
}

int add_client_to_group(struct group * group, struct client * client){// inutile function
  assert(client);
  assert(group);
  add_existing_client_to_list(&(group->clients), client);
}

int add_client_in_group(struct listg ** groups, struct client * client, char name[]){
  struct listg * before = *groups;
  if(before == NULL){
    printf("ERROR trying join the group : there no existing group\nYou can create one with /group <groupname> \n");
    return -1;
  }
  if( strcmp(before->group->name,name)){
    add_existing_client_to_list(&(before->group->clients), client);
    return 0;
  }

  struct listg * current = before->next;
  if(current == NULL)
  printf("ERROR trying join the group %s : no group with this tag\n",name);
    return -1; // there is no this group

  while (current->next != NULL){
    if(strcmp(current->group->name,name)){ // c'est lui qu'on supprime
        add_existing_client_to_list(&(current->group->clients), client);
      return 0;
    }
    //sinon on parcour la liste
    current = current->next;
    before = before->next;
  }
  if(strcmp(current->group->name,name)){ // c'est lui qu'on supprime
    add_existing_client_to_list(&(current->group->clients), client);
    return 0;
  }
  printf("ERROR trying join the group %s : no group with this tag\n",name);
  return -1; // il existe pas

}

int remove_client_to_group(struct group * group, int fd){ // ne sert à rien
  assert(group);
  int err = remove_client(&(group->clients), fd);
  if (group->clients == NULL){
    return 10;                // code de retour signifiant qu'il faut supprimer le group
  }
  return err;                 // code de retour de remove_client
}

int remove_client_in_group(struct listg ** groups, int fd, char name[]){
  struct listg * before = *groups;
  if(before == NULL){
    printf("ERROR trying join the group : there no existing group\nYou can create one with /group <groupname> \n");
    return -1;
  }
  if( strcmp(before->group->name,name)){
    remove_client(&(before->group->clients), fd);
    return 0;
  }

  struct listg * current = before->next;
  if(current == NULL)
  printf("ERROR trying join the group %s : no group with this tag\n",name);
    return -1; // there is no this group

  while (current->next != NULL){
    if(strcmp(current->group->name,name)){ // c'est lui qu'on supprime
      remove_client(&(current->group->clients), fd);
      return 0;
    }
    //sinon on parcour la liste
    current = current->next;
    before = before->next;
  }
  if(strcmp(current->group->name,name)){ // c'est lui qu'on supprime
    remove_client(&(current->group->clients), fd);
    return 0;
  }
  printf("ERROR trying join the group %s : no group with this tag\n",name);
  return -1; // il existe pas

}



int remove_group(struct listg ** groups, char name[]){
  struct listg * before = *groups;
  if(before == NULL){
    printf("ERROR trying to remove : there no existing group\n"); // peut être qu'il est tout aussi pertinent d'afficher ça dans server.c
    return -1;
  }
  if( strcmp(before->group->name,name)){
    *groups = before->next;
    return 0;
  }

  struct listg * current = before->next;
  if(current == NULL)
    return -1;

  while (current->next != NULL){
    if(strcmp(current->group->name,name)){ // c'est lui qu'on supprime
      before->next = current->next;
      return 0;
    }
    //sinon on parcour la liste
    current = current->next;
    before = before->next;
  }
  if(strcmp(current->group->name,name)){ // c'est lui qu'on supprime
    before->next = NULL;
    return 0;
  }
  printf("ERROR trying to remove the group '%s': this group no exist\n",name); // peut être qu'il est tout aussi pertinent d'afficher ça dans server.c
  return -1; // il existe pas
}






int main(int argc, char ** agrv){ //for testing purposes
  struct listg * groups = (struct listg *) malloc(sizeof(struct listg));
  struct group * group = (struct group *) malloc(sizeof(struct group));
  groups->group=group;

  create_group(&groups, "CACA");
  create_group(&groups, "PIPI");
  create_group(&groups, "CACA");




}
