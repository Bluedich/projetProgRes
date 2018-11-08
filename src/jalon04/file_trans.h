#ifndef _FILE_H_
#define _FILE_H_


int size_of_file(char *path);

int send_file( char *path, int fd_rcv_file);


int fill_file(char * buffer, char *path, int size);


int rcv_file(char *path, int fd_rcv_file,int size);

#endif
