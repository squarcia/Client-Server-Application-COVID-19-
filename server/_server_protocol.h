#include "ds.h"

#ifndef _SERVER_PROTOCOL_H_   
#define _SERVER_PROTOCOL_H_

#define PORT	    8080 
#define MAXLINE     1024 
#define EMPTY_LIST  22
#define COMMANDS 5
#define COMMAND_PREFIX '!'
#define DELIM_BUFFER ","
#define DELIM_BUFFER_FINISH "."

typedef int (*cmd_executor)(char* arg);

int help_executor(char* arg);
int showpeers_executor(char* arg);
int showneighbor_executor(char* arg);
int close_executor(char* arg);
int esc_executor(char* arg);

struct socket_info *socket_list;

/* Lista di tutti i peers connessi alla rete*/
struct peer_list *peer_list;

/* Struttura che tiene traccia del minimo Peer e del massimo Peer*/
struct global_network_info *network_info;

/* Contatore del numero di peer connessi alla rete*/
int global_peers_number;

/* Array ordinato in ordine crescente per numero di porta */
int global_peers_array_port[1024];

void printSocketList();
void printList();
void print();
void findPeer(int *port, int *left, int *right, struct peer_info *peer);
void addNeighbor(struct peer_info *peer);
void disconnettiPeer(int port);

void print_help();

int _parse_command(char* line, size_t line_len, char** cmd, char** arg);
int process_command(const char* cmd, char* arg);
int _handle_cmd();

void swap(int* xp, int* yp);
void sort(int arr[], int n);
int binarySearch(int *key);
void deleteElement(int *index);



#endif