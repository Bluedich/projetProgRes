#ifndef _LIST_H_
#define _LIST_H_

struct listg;
struct list;

//initialises a client structure and adds it to the linked list passed as argument
int add_list_client_to_group(struct listg ** group, int fd, char ip[], int port);

//create a group with the name name
int create_group(struct listg ** groups, char name);

//add client to the group with the name name
int add_client_in_group(struct listg * groups, struct client * client, char name[]);

//removes corresponding group from linked list of group
int remove_group(struct listg ** groups, char name);

//print list of client nicknames (for testing)
int print_list(struct list * clients);

//remove client with fd fd, in the group with name name
int remove_client_in_group(struct listg * groups, int fd, char name[]);

//returns 1 if client exists and 0 if not
int exists(struct list * clients, char nick[]);

//returns info on a users
int get_info(struct list * clients, char buffer[], char nick[]);

#endif
