#ifndef PEER_H_   
#define PEER_H_

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <sys/stat.h>
#include <time.h>


struct register_info {

    char data[1024];
    char filename[1024];
    int chiuso;
    struct register_info *next;
};

struct peer_info {
    int port;
    struct register_info *register_list;
    char dataRemota[1024];

    int left_peer;
    int right_peer;
};

struct socket_info {
    int socket;
    int peer_left;
};

struct map {
    int peer;
    char **dates_sent;
    char **dates_received;

    int sent;
    int received;
};




#endif