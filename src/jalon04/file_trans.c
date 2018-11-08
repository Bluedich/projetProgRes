#include "common.h"



int send_file( char *path, int fd_rcv_file){
  FILE * fichier;
  fichier = fopen(path,"r+");
  int nb_read = 1;
  int read = 1;
  int i = 0;
  int to_send;
  int sent;
  char * temp = malloc(sizeof(char)*BUFFER_SIZE);
  memset(temp, 0, BUFFER_SIZE);
  while(nb_read!=0){ // tant que y'a des truck dans le fichier
    nb_read = 0;
    while(read>0 && i<1000){ // tant que l'on a pas lu BUFFER_SIZE octet
        read = fread(temp,sizeof(char),BUFFER_SIZE,fichier);
        if (read==-1) error("ERROR reading file (fread)");
        nb_read+=read;
        i++;
    }
    i=0;
    to_send = nb_read;
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

int fill_file(char * buffer, char *path, int size){
  FILE * fichier;
  fichier = fopen(path,"a+");  // normalement le test si le fichier existe déjà ou pas à déja été fait
  int nb_write = 0;
  while(nb_write<sizeof(buffer)){  // normalement on passe la dedans qu'une fois
      nb_write = nb_write+ fwrite(buffer,sizeof(char),size,fichier);

  }
  fclose(fichier);
}

int rcv_file(char *path, int fd_rcv_file){
  char * buffer = malloc(sizeof(char)*BUFFER_SIZE);
  memset(buffer, 0, BUFFER_SIZE);
  int read=readline(fd_rcv_file, buffer, BUFFER_SIZE);
  fill_file(buffer,path,read);
}



/*
int main(int argc, char * argv[]){
  send_file("toto.txt",0);
  rcv_file("tata.txt", 0);

}
*/
