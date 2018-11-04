#ifndef _GROUP_H_
#define _GROUP_H_

struct listg;
struct group;

//initialises a client structure and adds it to the linked list passed as argument
int add_list_client_to_group(struct listg ** group, int fd, char ip[], int port);

//create a group with the name name
int create_group(struct listg ** groups, char name[]);

//return the group with the name in argument
int get_group_by_name(struct listg ** groups, struct group ** group, char name[]);

//resturn if the group with name exist
int group_exist(struct listg ** groups, char name[]);

//return if the group is empty or not 0 if it is not empty, -1 if it is empty
int group_is_empty(int fds[]);

//add client to the group with the name name
int add_client_in_group(struct listg ** groups, int c_sock, char name[]);

//return if the client is in the group or not
int client_is_in_group(struct listg ** groups, int c_sock, char name[]);

//removes corresponding group from linked list of group
int remove_group(struct listg ** groups, char name[]);

//writes buffer to all members of group except user with fd c_sock
int write_in_group(struct group * group, char nick[], int c_sock, char buffer[]);

//print the list of the channel in the server with the number of client in which server
int print_group(struct listg * groups, char group_list[]);

//remove client with fd c_sock, in the group with name name
int remove_client_in_group(struct listg ** groups, int c_sock, char name[]);

//returns 1 if client exists and 0 if not
int exists(struct list * clients, char nick[]);

//returns info on a users
int get_info(struct list * clients, char buffer[], char nick[]);

#endif
