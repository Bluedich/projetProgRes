#include "common.h"
#include "file_trans.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



int size_of_file(char *path){
  FILE * fichier;
  fichier = fopen(path,"a");
  long size;
  if(fichier){
    size=ftell (fichier);
    fclose (fichier);
    return size;
  }
  return -1;
}

int send_file( char *path, int fd_rcv_file){
  FILE * fichier;
  fichier = fopen(path,"r+");
  int read = 1;
  int i = 0;
  int to_send;
  int sent;
  char * temp = malloc(sizeof(char)*BUFFER_SIZE);
  memset(temp, 0, BUFFER_SIZE);
  while(read!=0){ // tant que y'a des truck dans le fichier
    read = 0;
    while(read<BUFFER_SIZE && i<1000){ // tant que l'on a pas lu BUFFER_SIZE octet
        read = read+fread(temp,sizeof(char),BUFFER_SIZE,fichier);
        if (read==-1) error("ERROR reading file (fread)");
        i++;
    }
    i=0;
    to_send = read;
    while(to_send>0 && i<1000){ //try maximum of 1000 times tant qu'on a pas tout envyer
      sent=write(fd_rcv_file, temp, to_send);
      if (sent==-1) error("ERROR writing line");
      to_send-=sent;
      i++;
    }
    i=0;
    sprintf(temp,"");
  }
  fclose(fichier);
}

int accept_file(char *path){
  FILE * fichier;
  char * local_path = malloc(sizeof(char)*BUFFER_SIZE);
  memset(local_path, 0, BUFFER_SIZE);
  sprintf(local_path,"../../inbox/%s",path);
  fichier = fopen(local_path,"r");
  if (fichier){
    printf("You already have a file with this name, do you want to overwrite it ?\n"); // c'est pas ici qui faudra le mettre mais c'est l'idée
    return -1;
  }
  fclose(fichier);
  fichier = fopen(local_path,"w");
  fclose(fichier);
  return 0;
}

int fill_file( char * buffer, char *path, int size){

  FILE * fichier;
  fichier = fopen(path,"a");  // normalement le test si le fichier existe déjà ou pas à déja été fait
  int nb_write = 0;
  while(nb_write<sizeof(buffer)){  // normalement on passe la dedans qu'une fois
      nb_write = nb_write+ fwrite(buffer,sizeof(char),size,fichier);
  }
  fclose(fichier);
}

int rcv_file(char * path, int fd_rcv_file, int size){
  int doo = accept_file(path);
  if (doo == -1){
    return 0;
  }
  char * buffer = malloc(sizeof(char)*BUFFER_SIZE);
  memset(buffer, 0, BUFFER_SIZE);
  char * local_path = malloc(sizeof(char)*BUFFER_SIZE);
  memset(local_path, 0, BUFFER_SIZE);
  sprintf(local_path,"../../inbox/%s",path);
  int read;
  int nb_read =0;
  fd_rcv_file = open(path,O_RDWR);
  while (nb_read<size){
  // read = 0;
    memset(buffer, 0, BUFFER_SIZE);
    read = readline(fd_rcv_file, buffer, BUFFER_SIZE);
    fill_file(buffer,local_path,read);
    nb_read=nb_read+read;
}

}

/*
int main(int argc, char * argv[]){
  send_file("toto.txt",0);
  printf("Fichier de %d bits\n",size_of_file("toto.txt"));
  char * buffer = malloc(sizeof(char)*BUFFER_SIZE);
  memset(buffer, 0, BUFFER_SIZE);
  sprintf(buffer,"toto.txt");
  rcv_file(buffer, 1,size_of_file("toto.txt"));
}
*/
