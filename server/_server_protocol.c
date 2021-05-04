#include "_server_protocol.h"

int global_peers_number = 0;
struct peer_list *peer_list = NULL;

const char* valid_cmds[] = {"help", "showpeers", "showneighbor", "close", "esc"};

const char* help_msg =
    "\n\n****************************************** DS COVID ******************************************\n\n"
    "               !help                  --> mostra il significato dei comandi e ciò che fanno\n"
    "               !showpeers             --> mostra l’elenco dei peer connessi alla rete\n"
    "               !showneighbor  <peer>  --> mostra i neighbor di un peer\n"
    "               !close <peer>          --> chiude il register di un peer\n"
    "               !esc                   --> termina il DS\n";


const char* help_verbose_msg =
    "\n\n\t=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=  HELP COMMAND SECTION =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n\n"
    "\tSHOWPEERS:\t\t Mostra l'elenco completo dei peers connessi alla rete in modalità verbose\n\n"
    "\tSHOWNEIGHBOR:\t\t Mostra i neighbor di un peer passato come parametro."
    "\n\t\t\t\t Se non viene passato nessun parametro vengono mostrati"
    "\n\t\t\t\t i neighbor di ogni peer. \n\n"
    "\tCLOSE:\t\t\t provoca la chiusura del register del peer specificato come parametro.\n\n"
    "\tESC:\t\t\t termina il DS. La terminazione del DS causa la terminazione"
    "\n\t\t\t\t di tutti i peer."
    "\n\t\t\t\t Opzionalmente, prima di chiudersi, i peer possono salvare"
    "\n\t\t\t\t le loro informazioni su un file,"
    "\n\t\t\t\t ricaricato nel momento in cui un peer torna a far parte del network.\n\n";



cmd_executor executors[] = {
    *help_executor,
    *showpeers_executor,
    *showneighbor_executor,
    *close_executor,
    *esc_executor
};

void printSocketList() {

    struct socket_info *current_node = socket_list;

    while (current_node != NULL) {
        printf("\n\n\nSOCKET [ %d ]:\tLEFT: [ %d ]\tRIGHT: [ %d ]\n\n", 
                                                                    current_node->socket, 
                                                                    (current_node->peer->left_peer == NULL) ? 0 : current_node->peer->left_peer->port, 
                                                                    (current_node->peer->right_peer == NULL) ? 0 : current_node->peer->right_peer->port);
        current_node = current_node->next;
    }

}

int help_executor(char* arg) {
    printf("%s", help_verbose_msg);
    return 0;
}

int showpeers_executor(char* arg) {

    struct peer_list *current_node = peer_list;

    printf("\n\n\n\t\t\t\tI PEER CONNESSI AL NETWORK SONO : \n\n");

    if (current_node == NULL) {
        printf("\t\t\t\t[  NESSUN PEER CONNESSO ALLA RETE  ]\n\n");
        return 0;
    }

   	while (current_node != NULL) {

        printf("\n\t\t\t\t\tPEER [  %d  ] \n", current_node->peer->port);
        current_node = current_node->next;
    }
    printf("\n\n\n\n\n\t\t\t\t    ( IN ORDINE DI ARRIVO ) \n\n\n\n\n");

    return 0;
}

int showneighbor_executor(char* arg) {

    int port = atoi(arg);
    int trovato = 0;

    struct peer_list *current_node = peer_list;

   	while (current_node != NULL) {
        if (current_node->peer->port == port) {
            trovato = 1;
            break;
        }
            
        current_node = current_node->next;
    }


    if (!trovato) {
        printf("\n\n\n\t\t\t\t[  PEER NON PRESENTE NEL NETWORK  ]\n\n");
        return -1;
    }

    if (global_peers_number == 1) {
        printf("\n\n\n\t\t\t     [  IL PEER NON HA ANCORA NEIGHBORS  ]\n\n");
        return -1;
    }

    printf("\n\n\n\t\t\t\tI NEIGHBOR DEL PEER SONO : \n\n");
    printf("\n\t\t\t\t    PEER [  %d  ] \n", current_node->peer->port);    
    printf("\n\n\n\t\tNEIGHBOR_LEFT [  %d  ]\t\tNEIGHBOR_RIGHT [  %d  ]  \n\n", current_node->peer->left_peer->port, current_node->peer->right_peer->port);

    return 0;
}

int esc_executor(char* arg) {

    printf("\n\n\n\t\t\t\t  [  DS SERVER IN CHIUSURA...  ]\n\n");

    struct socket_info *current_node;
    char buffer[MAXLINE];
    int n = 0;

    current_node = socket_list;

    while (current_node != NULL) {

        sprintf(buffer, "11:%d%s%d", current_node->peer->left_peer->port, ",", current_node->peer->right_peer->port);

        n = send(current_node->socket, buffer, sizeof(buffer), 0);
        current_node = current_node->next;
    }

    exit(1);
    return 0;
}

int close_executor(char* arg) {

    char buffer[MAXLINE];
    int n, trovato = 0;
    struct socket_info *current_node = socket_list;

    while (current_node != NULL) {

        if (current_node->peer->port == atoi(arg)) {

            trovato = 1;

            sprintf(buffer, "18");
            n = send(current_node->socket, buffer, sizeof(buffer), 0);

            if (n < 0) {
                perror("Errore in fase di invio: \n");
                break;
            }

            break;
        }
       
        current_node = current_node->next;
    }

    if (!trovato)
        printf("\n\n\n\t\t\t\t[ IL PEER NON È CONNESSO AL NETWORK  ]\n\n");
    else
        printf("\n\n\n\t\t\t\t   [  CHIUSURA REGISTER INVIATA  ]\n\n");
    
    return 0;
}

void print_help() {
    printf("%s", help_msg);
}

int _parse_command(char* line, size_t line_len, char** cmd, char** arg){
    if (line[line_len - 1] == '\n'){
        line[line_len - 1] = 0;
        --line_len;
    }

    if (line_len == 1)
        return -1;
    
    /* line + 1 excludes '!' */
    *cmd = strtok(line + 1, " ");
    *arg = (*cmd + strlen(*cmd) + 1);
    return 0;
}

int process_command(const char* cmd, char* arg) {

    int i, ris;

    for (i = 0; i < COMMANDS; ++i){
        if (strcmp(cmd, valid_cmds[i]) == 0){
            ris = executors[i](arg);
            if (ris == -2){
                perror("Errore di comunicazione con il server");
                return -1;
            }
            return ris;
        }
    }

    /* Invalid command */
    printf("Error: comando non trovato\n");
    return 1;
}

int _handle_cmd() {

    char* buf = NULL;
    size_t buf_len = 0;
    char* cmd = NULL;
    char* arg = NULL;
    int ris;

    buf_len = getline(&buf, &buf_len, stdin);

    if (buf_len > 0 && buf[0] != COMMAND_PREFIX) {
        printf("Errore: i comandi devono iniziare con '%c'\n", COMMAND_PREFIX);
        free(buf);
        return -1;
    }

    if (_parse_command(buf, buf_len, &cmd, &arg) == -1) {
         /* line contains only '!' */
        printf("Errore: comando non specificato\n");
        free(buf);
        return -1;
    }

    ris = process_command(cmd, arg);
    free(buf);
    return ris;
}

void swap(int* xp, int* yp) { 

    int temp = *xp; 
    *xp = *yp; 
    *yp = temp; 
} 

void sort(int arr[], int n) 
{ 
    int i, j, min_idx; 
  
    for (i = 0; i < n - 1; i++) { 
        min_idx = i; 

        for (j = i + 1; j < n; j++) 
            if (arr[j] < arr[min_idx]) 
                min_idx = j; 

        swap(&arr[min_idx], &arr[i]); 
    } 
} 

void printList() {

    struct peer_list *current_node = peer_list;

   	while (current_node != NULL) {
        printf("\n\n\t\t\t\tPEER [ %d ]:\tLEFT: [ %d ]\tRIGHT: [ %d ]\n\n", 
                                                                    current_node->peer->port, 
                                                                    (current_node->peer->left_peer == NULL) ? 0 : current_node->peer->left_peer->port, 
                                                                    (current_node->peer->right_peer == NULL) ? 0 : current_node->peer->right_peer->port);
        current_node = current_node->next;
    }
}

void print() {

    struct peer_list *current_node = peer_list;

   	while (current_node != NULL) {
        printf("--> PEER [ %d ] ", current_node->peer->port);
        current_node = current_node->next;
    }
}

void findPeer(int *port, int *left, int *right, struct peer_info *peer) {

    struct peer_list *current_node = peer_list;

    for (int i=0; i < global_peers_number; i++) {

        if (global_peers_array_port[i] < *port && (*port != network_info->min_peer->port)) {
            *left = global_peers_array_port[i];
        }

        if (*port == network_info->min_peer->port && (global_peers_array_port[i] == network_info->max_peer->port)) {
            *left = global_peers_array_port[i];
        }
        
        if (global_peers_array_port[i] > *port && (*port != network_info->max_peer->port)) {
            *right = global_peers_array_port[i];
        }

        if (*port == network_info->min_peer->port) {
            *right = global_peers_array_port[i];
        }
    }  

    if (*port == global_peers_array_port[0]) {

        printf("E' il minimo: %d\n", network_info->min_peer->port);
        *left = network_info->min_peer->port;
        *right = network_info->max_peer->port;
        printf("LEFT: %d\t RIGHT: %d\t GLOBAL: %d\n", *left, *right, global_peers_number);

    } 
        
    if (*port == global_peers_array_port[global_peers_number - 1]) {
        printf("E' il massimo: \n");
        *left = network_info->min_peer->port;
        *right = network_info->max_peer->port;
        printf("LEFT: %d\t RIGHT: %d\t GLOBAL: %d\n", *left, *right, global_peers_number);

    }   
    
    if (*left == 0 && *right == 0) {
        printf("Entro");
        *right = global_peers_array_port[global_peers_number];
        *left = global_peers_array_port[global_peers_number - 1];
    }

    while (current_node != NULL) {

        if (*left == current_node->peer->port) {
            current_node->peer->right_peer = peer;
            peer->left_peer = current_node->peer;
        } 

        if (*right == current_node->peer->port) {
            current_node->peer->left_peer = peer;
            peer->right_peer = current_node->peer;
        }

        current_node = current_node->next;
    }

    if (*port < network_info->min_peer->port)
        network_info->min_peer = peer;
    else 
        network_info->max_peer = peer;

    printList();
}

void addNeighbor(struct peer_info *peer) {

    int left = 0;
    int right = 0;

    switch (global_peers_number) {

        case 2:

            peer->right_peer = peer_list->next->peer;
            peer->left_peer = peer_list->next->peer;
            peer_list->next->peer->left_peer = peer;
            peer_list->next->peer->right_peer = peer;

            if (peer->port < peer_list->next->peer->port) {

                network_info->min_peer = peer;
                network_info->max_peer = peer_list->next->peer;
            }else {

                network_info->max_peer = peer;
                network_info->min_peer = peer_list->next->peer;
            }

            printList();
            break;
    
        default:
        
            findPeer(&peer->port, &left, &right, peer);
            break;
    }
}

int binarySearch(int *key) {

    int i = 0;

    for (; i < global_peers_number; i++) {

        if (global_peers_array_port[i] == *key)
            break;
    }

    return (i == 0) ? 0 : i - 1;
} 

void deleteElement(int *index) {

    for (int i=*index; i < global_peers_number; i++) {
        global_peers_array_port[i] = global_peers_array_port[i + 1];
    }

    sort(global_peers_array_port, global_peers_number);
}

void disconnettiPeer(int port) {

    struct peer_list *current_node = peer_list;
    struct peer_list *prev_node = NULL;

    struct socket_info *current_socket = socket_list;
    struct socket_info *current_node_socket;
    struct socket_info *prev_socket = NULL;
   
    int port_parameter = port;
    int index;
    int n;

    char buffer[MAXLINE];

    if (global_peers_number == 1) {

        peer_list = NULL;
    } else {

        while (current_node->next != NULL) {

            if (current_node->peer->port == port_parameter) {
                printf("\n\n\n\n\t\t\t\t   [  DISCONESSIONE IN CORSO...  ]\n\n");

                if (global_peers_number == 2) {

                    current_node->peer->left_peer->left_peer = NULL;
                    current_node->peer->left_peer->right_peer = NULL;
                } else {
                    current_node->peer->left_peer->right_peer = current_node->peer->right_peer;
                    current_node->peer->right_peer->left_peer = current_node->peer->left_peer;
                }
                
                if (prev_node == NULL) {
                    // head
                    peer_list = current_node->next;
                }else {

                    prev_node->next = current_node->next;
                }
                break;             
            }

            prev_node = current_node;
            current_node = current_node->next;
        }
    }

    /* Rimozione dall'array contenenti le porte */
    index = binarySearch(&port);
    deleteElement(&index);

    global_peers_number--;
    
    /* Rimozione del socket */
    while (current_socket != NULL) {
        if (current_socket->peer->port == port) {

             if (prev_socket == NULL) {
                // head
                socket_list = current_socket->next;
            } else {

            prev_socket->next = current_socket->next;
            }
            break;
        }
        prev_socket = current_socket;
        current_socket = current_socket->next;
    }

    current_node_socket = socket_list;

    while (current_node_socket != NULL) {
    
        sprintf(buffer, "10:%d%s%d", (current_node_socket->peer->left_peer == NULL) ? 0 : current_node_socket->peer->left_peer->port, ",", 
                                        (current_node_socket->peer->right_peer == NULL) ? 0 : current_node_socket->peer->right_peer->port);

        n = send(current_node_socket->socket, buffer, sizeof(buffer), 0);

        if (n < 0) {
                perror("Errore in fase di invio: \n");
                break;
        }
            
        current_node_socket = current_node_socket->next;
    }

    printf("\n\n\n\n\t\t\t\t    [  PEER %d DISCONESSO  ]\n\n", port);

}
