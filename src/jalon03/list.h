#ifndef _LIST_H_
#define _LIST_H_

struct list;
//struct client; Enfait c'est pire de mettre add_existing_client_to_list dans list.c parceque ça fait ajouter struct client ici

//give the fd of the client with name name
int get_fd_client_by_name(struct list *clients, char buffer[]);
//initialises a client structure and adds it to the linked list passed as argument
int add_client_to_list(struct list ** clients, int fd, char ip[], int port);

// add an existing client to a linked list clients (mainly usefull for a linked list of struct group)
//int add_existing_client_to_list(struct list ** clients, struct client * client);

//fill fd[] with all the fd of all the clients in the list in argument
int get_fd_client(struct list *clients, int fd[]);
//return the number of client in the linked list in argument
int nb_client_in_list(struct list *clients);

//sets name of corresponding client, returns -1 if error, 0 if success, 1 if name already taken
int set_nick(struct list * clients, int fd, char nick[]);

//formatted properly the buffer (mainly to remove trailing returnline)
void format_nick(char buffer[]);

//returns nickname in buffer, returns -1 if error, 0 if false, 1 if true
int has_nick(struct list * clients, char buffer[], int fd);

//removes corresponding client from linked list
int remove_client(struct list ** clients, int fd);

//print list of client nicknames (for testing)
int print_list(struct list * clients);

int get_online_users(struct list * clients, char buffer[]);

//returns 1 if client exists and 0 if not
int exists(struct list * clients, char nick[]);

//returns info on a users
int get_info(struct list * clients, char buffer[], char nick[]);

#endif
