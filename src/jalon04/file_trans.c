#include "common.h"
#include "file_trans.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


/*
// Pour le test, on l'enlève pas encore svp
int separate(char buffer[]){
  assert(buffer);
  int i=0;
  while(buffer[i]!=' '){ //go to first blank space
      if(buffer[i]=='\n' && buffer[i]=='\0'){
        memset(buffer, '\0', BUFFER_SIZE);
        return 0;
      }
      i++;
  }
  i++; //to skip blank space
  int j=i;
  while(buffer[j]!='\n' && buffer[j]!='\0'){
    buffer[j-i]=buffer[j];
    j++;
  }
  buffer[j-i]='\0';
  return 0;
}

void format_nick(char buffer[]){ //(internal) function to make sure nickname is formatted properly (mainly to remove trailing returnline)
  int i=0;
  while(i<BUFFER_SIZE-1 && buffer[i]!='\n'){
    i++;
  }
  buffer[i]='\0';
}

int readline(int fd, char * buffer, int maxlen){
  assert(buffer);
  memset(buffer, 0, maxlen);
  int size_read = read(fd, buffer, maxlen);
  if(size_read==-1){
    error("ERROR reading line");
  }
  return size_read;
}

*/
int format_name_file( char *name){
  int i;
  for ( i = 0; i < strlen(name); i++) {
    if ( strncmp(name+i,"/",1)==0 ){
      return -1;
    }
    if (strncmp(name+i," ",1)==0){
      strncpy(name+i,"_",1);
    }
  }
  strncpy(name+i,"\0",1);
  return 0;
}


int path_to_name(char * path, char *name){
  int i;
  int j=-1;
  while( i<strlen(path) ){
    if (strncmp(path+i,"/",1)==0) {
      j=i;
    }
    i++;
  }

  strcpy(name,path+j+1);
  return 0;
}


int size_of_file(char *path){
  FILE * fichier;
  fichier = fopen(path,"a");
  long size;
  if(fichier){
    size=ftell (fichier);
    fclose (fichier);
    return size;
  }
  fclose (fichier);
  return -1;
}

int send_file( char *path, int fd_rcv_file){
  FILE * fichier;
  fichier = fopen(path,"r+");
  int read = 0;
  int i = 0;
  int to_send;
  int total_to_send=size_of_file(path); // comme cela ou en argument
  int total_sent=0;
  int sent;
  char * temp = malloc(sizeof(char)*BUFFER_SIZE);
  memset(temp, 0, BUFFER_SIZE);
  while(total_sent<total_to_send){ // tant que y'a des truck dans le fichier
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
    // printf("%d bits sent over %d\n",total_sent,total_to_send);
    if (to_send !=0) error("ERROR writing line, everyhthing is not write");
    total_sent = total_sent + read;
    i=0;
    read = 0;
    // sprintf(temp,""); les deux lignes font la même choses
    memset(temp, 0, BUFFER_SIZE);
    // printf("%d bits sent over %d\n",total_sent,total_to_send);
  }
  fclose(fichier);
}

int accept_file(char *namefile){    // test if the file can be store
  FILE * fichier;
  char * buffer = malloc(sizeof(char)*BUFFER_SIZE);
  memset(buffer, 0, BUFFER_SIZE);
  char * local_path = malloc(sizeof(char)*BUFFER_SIZE);
  memset(local_path, 0, BUFFER_SIZE);
  // sprintf(local_path,"../../inbox/%s",namefile);
  sprintf(local_path,"../inbox/%s",namefile);
  fichier = fopen(local_path,"r");
  int m=-1;
  if (fichier){
    fclose(fichier);
    while(1){
      printf("> You already have a file with this name, do you want to overwrite it ? (y/n)\n");
      memset(buffer, 0, BUFFER_SIZE);  équivalent avec ce qui est au dessus
      readline(0,buffer,BUFFER_SIZE);
      if(strncmp(buffer,"y",1)==0){
        break;
      }
      if(strncmp(buffer,"n",1)==0){
        while(1){
          printf("> Do you want to save the file with an other name (if you respond n, you dined the transfer) ? (y/n)\n");
          memset(buffer, 0, BUFFER_SIZE);  équivalent avec ce qui est au dessus
          readline(0,buffer,BUFFER_SIZE);
          if(strncmp(buffer,"y",1)==0){
            printf("> Please enter the name of the file\n");
            while(m!=0){
              memset(buffer, 0, BUFFER_SIZE);  équivalent avec ce qui est au dessus
              readline(0,buffer,BUFFER_SIZE);
              if (format_name_file(buffer)==-1){
                printf("> Do not enter a path, your file will be downloaded in your inbox directory.\n");
              }
              else {
                sprintf(local_path,"../../inbox/%s",buffer);
                fichier = fopen(local_path,"r");
                if (fichier){
                  fclose(fichier);
                  printf("> Please use an other name, this one is already taken by an other file.\n");
                }
                else {
                  m=0;
                }
              }
            }
            memset(namefile, 0, BUFFER_SIZE);  équivalent avec ce qui est au dessus
            sprintf(namefile,"%s",buffer);
            return 0;
          }
          if(strncmp(buffer,"n",1)==0){
            return -1;
          }
        }
      }
    }
  }
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
  // fd_rcv_file = open(path,O_RDWR); // juste pour le test en vrai y'a une petit interrogation de si ça marche ou pas du coup
  char * name = malloc(sizeof(char)*BUFFER_SIZE);
  memset(name, 0, BUFFER_SIZE);
  path_to_name(path,name);
  int doo = accept_file(name);
  if (doo == -1){
    printf("You have dined the transfer\n");
    return 0;
  }
  char * buffer = malloc(sizeof(char)*BUFFER_SIZE);
  memset(buffer, 0, BUFFER_SIZE);
  char * local_path = malloc(sizeof(char)*BUFFER_SIZE);
  memset(local_path, 0, BUFFER_SIZE);
  sprintf(local_path,"../inbox/%s",name);
  // sprintf(local_path,"../../inbox/%s",name);

  int read;
  int nb_read =0;
  while (nb_read<size){
  // read = 0;
    memset(buffer, 0, BUFFER_SIZE);
    read = readline(fd_rcv_file, buffer, BUFFER_SIZE);
    fill_file(buffer,local_path,read);      // read is the number of bits read from the socket to write in the file in each passage on the loop
    nb_read=nb_read+read;
  }
}


/*int main(int argc, char * argv[]){
  int f_size = size_of_file("~/helloworld.txt");
  printf("%d\n", f_size);
  int pipefd[2];
  int caca = pipe(pipefd);

  char * name = malloc(sizeof(char)*BUFFER_SIZE);
  memset(name, 0, BUFFER_SIZE);
  sprintf(name,"./toto.txt");
  printf("Fichier de %d bits\n",size_of_file("toto.txt"));
  send_file(name,pipefd[1]);
  printf("Fichier de %d bits sent !\n",size_of_file("toto.txt"));

  rcv_file(name, pipefd[0],size_of_file("toto.txt"));
}*/
