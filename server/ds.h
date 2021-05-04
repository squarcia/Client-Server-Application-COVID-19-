#ifndef DS_H_   
#define DS_H_

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

struct register_info {
    FILE *registro;
    char data[1024];
    char filename[1024];
    int chiuso;
    struct register_info *next;
};

struct peer_info {

    int port;
    char dataRemota[1024];

    struct register_info *register_list;

    struct peer_info *left_peer;
    struct peer_info *right_peer;
};

struct peer_list {

    struct peer_info *peer;
    struct peer_list *next;
};

struct global_network_info {

    struct peer_info *max_peer;
    struct peer_info *min_peer;
};

struct socket_info {

    struct peer_info *peer;
    int socket;
    struct socket_info *next;
};

#endif
