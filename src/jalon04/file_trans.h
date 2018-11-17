#ifndef _FILE_H_
#define _FILE_H_

// On a besoin car scv_file à besoin de connaitre la taille du fichier
int size_of_file(char *path);

//sends file through specified socket
int send_file( char *path, int fd_rcv_file);

//receive file from specified socket
int rcv_file(char *path, int fd_rcv_file,int size);

#endif
