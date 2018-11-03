#include "common.h"
#include <time.h>
#include "list.h"

#define MAX_CL 20

struct group{
  //struct client_fd * clients;
  int fd[MAX_CL];
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
  int i;

  if(group_exist(groups, name)==0){ // if the name exist, do not create the group
    printf("ERROR trying create the group %s : there already an existing group with this name\n",name);
    return 1;
  }
  for (i=0;i<MAX_CL;i++){
    group->fd[i]=-1;
  }
  strcpy(group->name,name);

  new_start->group = group;
  new_start->next = *groups;
  *groups=new_start;
  return 0;
}


int get_available_fd_index2(int fds[]){
  int i;
  for(i=0; i<MAX_CL; i++){
    if(fds[i]==-1)
      return i;
  }
  return -1;
  error("ERROR too many connected clients (in the group)");
}

int fd_in_tab(int fds[], int c_sock){
  int i;
  for(i=0; i<MAX_CL; i++){
    if(fds[i]==c_sock)
      return i;
  }
  return -1;
  error("Pas dans le groupe");
}

int group_is_empty(int fds[]){
  int i;
  for(i=0; i<MAX_CL; i++){
    //printf("Fds[i] =%d\n",fds[i]);
    if(fds[i]!=-1){
      //printf("Fds[i] =%d\n",fds[i]);
      return 0;
    }
  }
  return -1;
  error("Pas dans le groupe");
}


int add_client_in_group(struct listg ** groups, int c_sock, char name[]){
  struct listg * before = *groups;
  int fd_index;

  if(before == NULL){
    printf("ERROR trying join the group : there no existing group\nYou can create one with /group <groupname> \n");
    return -1;
  }
  if( strcmp(before->group->name,name)==0){ // c'est le groupe dans lequel on veut ajouter le client
    if ( fd_in_tab(before->group->fd,c_sock) !=-1 ){
      return -3; // Le client est déja dans le groupe
    }
    fd_index=get_available_fd_index2(before->group->fd);
    if ( fd_index ==-1 ){
      return -2; // Y'a plus de place dans le groupe
    }
    before->group->fd[fd_index]=c_sock;
    return 0;
  }

  struct listg * current = before->next;
  if(current == NULL){
    printf("ERROR trying BBjoin the group %s : no group with this tag\n",name);
    return -1; // there is no this group
  }

  while (current->next != NULL){
    if(strcmp(current->group->name,name)==0){ // c'est le groupe dans lequel on veut ajouter le client
    if ( fd_in_tab(current->group->fd,c_sock) !=-1 ){
      return -3; // Le client est déja dans le groupe
    }
    fd_index=get_available_fd_index2(current->group->fd);
    if ( fd_index ==-1 ){
      return -2; // Y'a plus de place dans le groupe
    }
    current->group->fd[fd_index]=c_sock;
    return 0;
  }
    //sinon on parcour la liste
    current = current->next;
    before = before->next;
  }
  if(strcmp(current->group->name,name)==0){ // c'est le groupe dans lequel on veut ajouter le client
    fd_index=get_available_fd_index2(current->group->fd);
    if ( fd_in_tab(current->group->fd,c_sock) !=-1 ){
      return -3; // Le client est déja dans le groupe
    }
    fd_index=get_available_fd_index2(current->group->fd);
    if ( fd_index ==-1 ){
      return -2; // Y'a plus de place dans le groupe
    }
    current->group->fd[fd_index]=c_sock;
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
    return fd_in_tab(before->group->fd,c_sock); // -1 : Le client est pas dans la liste, sinon il y est
}

  struct listg * current = before->next;
  if(current == NULL)
  printf("ERROR trying BBjoin the group %s : no group with this tag\n",name);
    return -1; // the group does not exist

  while (current->next != NULL){
    if(strcmp(current->group->name,name)==0){ // c'est le groupe dans lequel on veut ajouter le client
    return fd_in_tab(current->group->fd,c_sock);
    }
    //sinon on parcour la liste
    current = current->next;
    before = before->next;
  }
  if(strcmp(current->group->name,name)==0){ // c'est le groupe dans lequel on veut ajouter le client
  return fd_in_tab(current->group->fd,c_sock);
  }
  printf("ERROR trying AAjoin the group %s : no group with this tag\n",name);
  return -1; // the group does not exist

}


int remove_client_in_group(struct listg ** groups, int c_sock, char name[]){
  struct listg * before = *groups;
  int fd_index;
  if(before == NULL){
    printf("ERROR trying leave the group : there no existing group\nYou can create one with /group <groupname> \n");
    return -1;  // the group does not exist
  }
  if( strcmp(before->group->name,name)==0){
    fd_index=fd_in_tab(before->group->fd,c_sock);
    if (fd_index == -1){
      return -2; //The client isn't in the group
    }

    before->group->fd[fd_index]=-1;
    if ( (group_is_empty(before->group->fd)==-1) ){
      return 10;                // code de retour signifiant qu'il faut supprimer le group
    }
    return 0;   // the client leave the group
  }

  struct listg * current = before->next;
  if(current == NULL){
    printf("ERROR trying leave the group %s : no group with this tag\n",name);
    return -1;    // the group does not exist
}
  while (current->next != NULL){
    if(strcmp(current->group->name,name)==0){ // c'est lui qu'on supprime
    fd_index=fd_in_tab(current->group->fd,c_sock);
    if (fd_index == -1){
      return -2; //The client isn't in the group
    }

    current->group->fd[fd_index]=-1;
    if ( group_is_empty(current->group->fd)==-1 ){
      return 10;                // code de retour signifiant qu'il faut supprimer le group
    }
    return 0;   // the client leave the group

  }
    //sinon on parcour la liste
    current = current->next;
    before = before->next;
  }
  if(strcmp(current->group->name,name)==0){ // c'est lui qu'on supprime
  fd_index=fd_in_tab(current->group->fd,c_sock);
  if (fd_index == -1){
    return -2; //The client isn't in the group
  }

  current->group->fd[fd_index]=-1;
  if ( group_is_empty(current->group->fd)==-1 ){
    return 10;                // code de retour signifiant qu'il faut supprimer le group
  }
  return 0;   // the client leave the group
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
  if( (strcmp(before->group->name,name)==0) ){
    *groups = before->next;
    return 0;   // group deleted
  }

  struct listg * current = before->next;
  if(current == NULL)
    return -1;  // the group does not exist

  while (current->next != NULL){
    if( (strcmp(current->group->name,name)==0) ){ // c'est lui qu'on supprime
      before->next = current->next;
      return 0; // group deleted
    }
    //sinon on parcour la liste
    current = current->next;
    before = before->next;
  }
  if( (strcmp(current->group->name,name)==0) ){ // c'est lui qu'on supprime
    before->next = NULL;
    return 0; // group deleted
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
