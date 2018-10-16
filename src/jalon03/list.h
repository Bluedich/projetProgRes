#ifndef _LIST_H_
#define _LIST_H_

struct list;

//initialises a client structure and adds it to the linked list passed as argument
int add_client_to_list(struct list ** clients, int fd);

//sets name of corresponding client, returns -1 if error, 0 if success, 1 if name already taken
int set_nick(struct list * clients, int fd, char * nick);

//returns nickname in buffer, returns -1 if error, 0 if false, 1 if true
int has_nick(struct list * clients, char * buffer, int fd);

//removes corresponding client from linked list
int remove_client(struct list ** clients, int fd);

//print list of client nicknames (for testing and for use in who command)
void print_list(struct list * clients);

#endif
