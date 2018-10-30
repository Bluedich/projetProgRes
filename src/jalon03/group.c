#include "common.h"
#include <time.h>
#include "list.h"

/////////////////////////////////
/// Tout ça ne devrait pas être la mais dans list, mais je sais pas faire,
////////////////////
struct client{  // anomali   normalement je devrais pas en avoir besoin moi je penses, mais selon mon compilateur si
  char nickname[MAX_NICK_SIZE];
  char ip_addr[17];
  int port_nb;
  int hasNick;
  int fd;
  char con_date[512];
};

struct list{  // anomali, normalement je devrais pas en avoir besoin moi je penses, mais selon mon compilateur si
  struct client * client;
  struct list * next;
};


//////////
//Tout ce qui est plus haut ne devrait pas être là mais dans list.c
//////

// add an existing client to a linked list clients (mainly usefull for a linked list of struct group)
int add_existing_client_to_list(struct list ** clients, struct client * client){  // doit être mis dans list.c
  assert(clients);
  struct list * new_start = (struct list *) malloc(sizeof(struct list));
  assert(client);
  assert(new_start);

  new_start->client = client;
  new_start->next = *clients;
  *clients=new_start;
}

/*
struct client_fd{
  int fd;
  struct client_fd * next;
};
*/
// je pensais que j'avais trouver un truck pas trop mal, mais non sans linked lsit chainée générique faut tout réécrire
struct group{
  //struct client_fd * clients;
  struct list * clients;
  char name[512];
};

struct listg{
  struct group * group;
  struct listg * next;
};

int get_group_by_name(struct listg ** groups, struct group ** group, char name[]){
  struct listg * current = *groups;
  if(current == NULL)
    return -1;  // there is no this group

  while (current != NULL){
    printf("%s\n",current->group->name);
      if(strcmp(current->group->name, name)==0){
      *group = current->group;
      return 0;
    }
    //sinon on parcour la liste
    current = current->next;
  }
  return -1; // there is no this group
}

int group_exist(struct listg ** groups, char name[]){ // jepense que cette fonction pourrai juste être interne
  struct listg * current = *groups;                                               // group with name name exist ?
  if(current == NULL)
    return -1;  // there is no this group

  while (current != NULL){
    printf("%s\n",current->group->name);
      if(strcmp(current->group->name, name)==0){
      return 0;
    }
    //sinon on parcour la liste
    current = current->next;
  }
  return -1; // there is no this group
}

int create_group(struct listg ** groups, char name[]){
  assert(groups);
  struct listg * new_start = (struct listg *) malloc(sizeof(struct listg));
  struct group * group = (struct group *) malloc(sizeof(struct group));
  assert(group);
  assert(new_start);
  memset(group, 0, sizeof(struct group));


  if(group_exist(groups, name)==0){ // if the name exist, do not create the group
    printf("ERROR trying create the group %s : there already an existing group with this name\n",name);
    return 1;
  }
  strcpy(group->name,name);

  new_start->group = group;
  new_start->next = *groups;
  *groups=new_start;
  return 0;
}


int add_client_in_group(struct list ** clients, struct listg ** groups, int c_sock, char name[]){
  struct listg * before = *groups;
  struct client * client = (struct client *) malloc(sizeof(struct client));
  get_client_by_fd( *clients, &client, c_sock);
  //printf("Le fd devrait etre %d\n", c_sock);  // comment c'est deux truck peuvent être différent, c'est chaint aprceque du coup c'est sale, j'ai besoin de plein d'argument pourri dans cette fonction
  //printf("Le fd est %d\n", client->fd);       // comment c'est deux truck peuvent être différent

  if(before == NULL){
    printf("ERROR trying join the group : there no existing group\nYou can create one with /group <groupname> \n");
    return -1;
  }
  if( strcmp(before->group->name,name)==0){ // c'est le groupe dans lequel on veut ajouter le client
    if (get_client_by_fd((before->group->clients), NULL, c_sock)==0){ // test si le client est déjà dans la liste
      return 1; // il est déjà dans la liste
    }
    add_existing_client_to_list(&(before->group->clients), client);
    return 0;
  }

  struct listg * current = before->next;
  if(current == NULL)
  printf("ERROR trying BBjoin the group %s : no group with this tag\n",name);
    return -1; // there is no this group

  while (current->next != NULL){
    if(strcmp(current->group->name,name)==0){ // c'est le groupe dans lequel on veut ajouter le client
      if (get_client_by_fd((before->group->clients), NULL, c_sock)==0){
        return 1; // il est déjà dans le groupe
      }
        add_existing_client_to_list(&(current->group->clients), client);
      return 0;
    }
    //sinon on parcour la liste
    current = current->next;
    before = before->next;
  }
  if(strcmp(current->group->name,name)==0){ // c'est le groupe dans lequel on veut ajouter le client
    if (get_client_by_fd((before->group->clients), NULL, c_sock)==0){
      return 1; // il est déjà dans le groupe
    }
    add_existing_client_to_list(&(current->group->clients), client);
    return 0;
  }
  printf("ERROR trying AAjoin the group %s : no group with this tag\n",name);
  return -1; // le groupe existe pas

}


int client_is_in_group(struct listg ** groups, int c_sock, char name[]){ // test pour savoir si le client est dans la liste
  struct listg * before = *groups;

  if(before == NULL){
    printf("ERROR trying join the group : there no existing group\nYou can create one with /group <groupname> \n");
    return -1;  // the group does not exist
  }
  if( strcmp(before->group->name,name)==0){ // c'est le groupe dans lequel on veut savoir si le client y est
    if (get_client_by_fd((before->group->clients), NULL, c_sock)==0){
      return 1; // il est bien dans la liste
    }
    return 0;   // il y est pas
  }

  struct listg * current = before->next;
  if(current == NULL)
  printf("ERROR trying BBjoin the group %s : no group with this tag\n",name);
    return -1; // the group does not exist

  while (current->next != NULL){
    if(strcmp(current->group->name,name)==0){ // c'est le groupe dans lequel on veut ajouter le client
      if (get_client_by_fd((before->group->clients), NULL, c_sock)==0){
        return 1; // il est déjà dans la liste
      }
      return 0; // il y est pas
    }
    //sinon on parcour la liste
    current = current->next;
    before = before->next;
  }
  if(strcmp(current->group->name,name)==0){ // c'est le groupe dans lequel on veut ajouter le client
    if (get_client_by_fd((before->group->clients), NULL, c_sock)==0){
      return 1; // il est déjà dans la liste
    }
    return 0; // il y est pas
  }
  printf("ERROR trying AAjoin the group %s : no group with this tag\n",name);
  return -1; // the group does not exist

}


int remove_client_in_group(struct listg ** groups, int fd, char name[]){
  struct listg * before = *groups;
  if(before == NULL){
    printf("ERROR trying leave the group : there no existing group\nYou can create one with /group <groupname> \n");
    return -1;  // the group does not exist
  }
  if( strcmp(before->group->name,name)){
    remove_client(&(before->group->clients), fd);
    if (before->group->clients == NULL){
      return 10;                // code de retour signifiant qu'il faut supprimer le group
    }
    return 0;   // the client leave the group
  }

  struct listg * current = before->next;
  if(current == NULL)
  printf("ERROR trying leave the group %s : no group with this tag\n",name);
    return -1;    // the group does not exist

  while (current->next != NULL){
    if(strcmp(current->group->name,name)){ // c'est lui qu'on supprime
      remove_client(&(current->group->clients), fd);
      if (current->group->clients == NULL){
        return 10;                // code de retour signifiant qu'il faut supprimer le group
      }
      return 0;       // the client leave the group (peut être faudrait il plutot retrouné le code de retour de remove_client pour avoir plus de valeur de retour j'y est pas réfléchi, en vrai je crois pas qu'il y est besoin)
    }
    //sinon on parcour la liste
    current = current->next;
    before = before->next;
  }
  if(strcmp(current->group->name,name)){ // c'est lui qu'on supprime
    remove_client(&(current->group->clients), fd);
    if (current->group->clients == NULL){
      return 10;                // code de retour signifiant qu'il faut supprimer le group
    }
    return 0;    // the client leave the group
  }
  printf("ERROR trying leave the group %s : no group with this tag\n",name);
  return -1; // the group does not exist

}



int remove_group(struct listg ** groups, char name[]){
  struct listg * before = *groups;
  if(before == NULL){
    printf("ERROR trying to remove : there no existing group\n");
    return -1;  // the group does not exist
  }
  if( strcmp(before->group->name,name)){
    *groups = before->next;
    return 0;   // group has to be deleted
  }

  struct listg * current = before->next;
  if(current == NULL)
    return -1;  // the group does not exist

  while (current->next != NULL){
    if(strcmp(current->group->name,name)){ // c'est lui qu'on supprime
      before->next = current->next;
      return 0; // group has to be deleted
    }
    //sinon on parcour la liste
    current = current->next;
    before = before->next;
  }
  if(strcmp(current->group->name,name)){ // c'est lui qu'on supprime
    before->next = NULL;
    return 0; // group has to be deleted
  }
  printf("ERROR trying to remove the group '%s': this group no exist\n",name); // peut être qu'il est tout aussi pertinent d'afficher ça dans server.c
  return -1;  // the group does not exist
}





/*
int main(int argc, char ** agrv){ //for testing purposes
  struct listg * groups = (struct listg *) malloc(sizeof(struct listg));
  struct group * group = (struct group *) malloc(sizeof(struct group));
  groups->group=group;

  create_group(&groups, "CACA");
  create_group(&groups, "PIPI");
  create_group(&groups, "CACA");




}
*/
