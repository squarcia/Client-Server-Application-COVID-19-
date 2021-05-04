// Server side implementation of UDP client-server model 
#include "_server_protocol.h"

int main(int argc, char *argv[]) { 

	int sockfd, port; 
    int len, ret; 
    int port_disconnect;
    int socket_tcp;
    int n;

	char buffer[MAXLINE]; 
    char *token, bufferCopy[MAXLINE];
    
	struct sockaddr_in servaddr, cliaddr; 
    struct sockaddr_in first_peer;

    struct peer_info *peer;
    struct peer_list *peer_item;
    struct socket_info *socket_item;
    struct socket_info *current_node;

    /* Set di descrittori da monitorare */
    fd_set master;

    /* Set di descrittori pronti */
    fd_set read_fds;
    
    /* Descrittore massimo */
    int fdmax;

	/* Creazione socket */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
		perror("Errore in fase di creazione del socket: \n"); 
		exit(EXIT_FAILURE); 
	} 
    
    /* Pulizia */
	memset(&servaddr, 0, sizeof(servaddr)); 
	memset(&cliaddr, 0, sizeof(cliaddr)); 

    /* Ricavo porta */ 
    port = atoi(argv[1]);
	
	/* Creazione indirizzo di bind */
	servaddr.sin_family = AF_INET;  
	servaddr.sin_addr.s_addr = INADDR_ANY; 
	servaddr.sin_port = htons(port); 
	
	/* Aggancio */
	if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ) 
	{ 
		perror("Errore in fase di bind: \n"); 
		exit(EXIT_FAILURE); 
	} 

    /* Allocazione */
    network_info = (struct global_network_info*) malloc(sizeof(struct global_network_info*));
    network_info->min_peer = (struct peer_info*) malloc(sizeof(struct peer_info*));
    network_info->max_peer = (struct peer_info*) malloc(sizeof(struct peer_info*));
    
    /* Reset dei descrittori */
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    /* Aggiungo il socket "sockfd" ai socket monitorati */
    FD_SET(sockfd, &master);

    /* Aggiungo il socket "sockfd" ai socket monitorati */
    FD_SET(STDIN_FILENO, &master);


    fdmax = (sockfd > STDIN_FILENO) ? sockfd : STDIN_FILENO;

    printf("\n\n\n\n\t\t\t    SERVER IN ASCOLTO SULLA PORTA :\t%d\n\n", atoi(argv[1]));
	
    while (1) {

        print_help();

        read_fds = master;

        select(fdmax + 1, &read_fds, NULL, NULL, NULL);

        if (FD_ISSET(STDIN_FILENO, &read_fds))
            _handle_cmd();
        
        if (FD_ISSET(sockfd, &read_fds)) {
            /* Ho ricevuto un messaggio */
            printf("\n\n\n\n\t\t\t\t   [  RICEZIONE IN CORSO...  ]\n\n");
            len = sizeof(cliaddr); 

            /* Ricevo il messaggio e lo salvo in buffer */
	        n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *)&cliaddr,(socklen_t*)&len); 

            if (n < 0) {
                perror("Errore in fase di ricezione: \n");
                break;
            }

            /* Copia in variabile di appoggio */
            strcpy(bufferCopy, buffer);

            /* Esamino il contenuto del buffer */
            token = strtok(bufferCopy, DELIM_BUFFER);

            /* SIGNAL -1: il peer ha richiesto una disconnessione dal server */
            if (atoi(token) == -1) {

                printf("\n\n\n\n\t\t\t       [  RICHIESTA DISCONNESSIONE PEER ]\n\n");

                token = strtok(NULL, DELIM_BUFFER_FINISH);

                /* Ricavo il numero di porta dal messaggio */
                port_disconnect = atoi(token);

                /* Disconnessione del peer dal network */
                disconnettiPeer(port_disconnect);
                
            } else {
                
                /* Allocazione memoria */
                peer = (struct peer_info*) malloc(sizeof(struct peer_info));
                peer_item = (struct peer_list*) malloc(sizeof(struct peer_list));
                peer->register_list = (struct register_info *) malloc(sizeof(struct register_info));
                socket_item = (struct socket_info*) malloc(sizeof(struct socket_info));

                /* Inizializzazione */
                peer->left_peer = NULL;
                peer->right_peer = NULL;
                peer_item->next = NULL;

                /* Ricezione secondo messaggio contentente dati peer */
                sscanf(buffer, "%d %s %s %s %d", &peer->port, peer->dataRemota, peer->register_list->filename, peer->register_list->data, &peer->register_list->chiuso);

                /* Inserimento in testa nella lista peer_list del nuovo peer */
                peer_item->peer = peer;
                peer_item->next = peer_list;
                peer_list = peer_item;
           
                /* Inserimento del numero di porta all'interno dell'array */
                global_peers_array_port[global_peers_number] = peer->port;
                global_peers_number++;

                /* Ordino l'array per numeri di porta crescenti */
                sort(global_peers_array_port, global_peers_number);

                /* Creazione socket */
                socket_tcp = socket(AF_INET, SOCK_STREAM, 0);

                if (socket_tcp < 0) {
                    perror("Errore in fase di creazione del socket primo peer: \n");
                    return -1;
                }

                /* Inserimento dell'elemento all'interno della lista */
                socket_item->peer = peer;
                socket_item->socket = socket_tcp;

                socket_item->next = socket_list;
                socket_list = socket_item;

                /* Creazione indirizzo */
                memset(&first_peer, 0, sizeof(first_peer)); 
                first_peer.sin_addr.s_addr = INADDR_ANY; 
                first_peer.sin_family = AF_INET; 
                first_peer.sin_port = htons(peer->port); 

                ret = connect(socket_tcp, (struct sockaddr*)&first_peer, sizeof(first_peer));

                if (ret < 0) {
                    perror("Connect TCP failed: \n");
                    return -1;
                }

                if (global_peers_number == 1) {
                    printf("\t\t\t       [  PEER %d AGGIUNTO AL NETWORK ]\n\n", peer->port);
                    sprintf(buffer, "%d", 22);
                    n = send(socket_list->socket, buffer, sizeof(buffer), 0);
                }else {

                    /* Ricalcolo tutti i vicini di ogni peer */
                    printf("\t\t\t          [  AGGIUNGO PEER AL NETWORK ]\n\n");
                    addNeighbor(peer);

                    printf("\n\n\n\n\t\t         [  INVIO AGGIORNAMENTO NEIGHBORS AI PEER DEL NETWORK  ]\n\n\n");
                    //printSocketList();    // TEST

                    current_node = socket_list;

                    while (current_node != NULL) {

                        sprintf(buffer, "10:%d%s%d", current_node->peer->left_peer->port, ",", current_node->peer->right_peer->port);

                        n = send(current_node->socket, buffer, sizeof(buffer), 0);

                        current_node = current_node->next;
                    }
                }
            }
        }
    }

	return 0; 
} 



