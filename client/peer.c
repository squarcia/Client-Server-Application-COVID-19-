// Client side implementation of UDP client-server model 
#include "_peer_protocol.h"

int main(int argc, char *argv[]) { 

    int ret, i, addrlen, listener, left, right; 
    char *token, *signal;

    struct peer_info *peer;
    struct register_info *register_item;

    /* Buffer di ricezione/appoggio/appoggio */
    char bufferRicezione[MAXLINE], 
         bufferCopy[MAXLINE],   
         bufferNeighbor[MAXLINE];

    /* Struttura indirizzo server/client */
    struct sockaddr_in my_addr, cl_addr;

    /* Controllo input */
    if ((atoi(argv[1]) > 65536) || (atoi(argv[1]) < 1024)) {
        printf("NUMERO DI PORTA NON VALIDO. (VALORI CONSENTITI:\t 1024 - 65535)\n\n");
        exit(-1);
    }

    /* Allocazione memoria */
    peer = (struct peer_info *) malloc(sizeof(struct peer_info));
    register_item = (struct register_info *) malloc(sizeof(struct register_info));
    peer->register_list = (struct register_info *) malloc(sizeof(struct register_info));

    /* Inizializzazione */
    peer->register_list = NULL;
    peer->port = atoi(argv[1]);

    /* Inizializzo struttura dati */
    startEngine();

    /* Creo primo registro */
    creaRegistro(peer, register_item);

    /* Reset dei descrittori */
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    /* Aggiungo il File Descriptor dello STD-INPUT ai socket monitorati */
    FD_SET(STDIN_FILENO, &master);

    /* Creazione indirizzio server */
    listener = socket(AF_INET, SOCK_STREAM, 0);
    memset(&my_addr, 0, sizeof(my_addr)); 
    my_addr.sin_family = AF_INET; 
    my_addr.sin_port = htons(peer->port); 
    my_addr.sin_addr.s_addr = INADDR_ANY;      

    /* Aggancio */
    ret = bind(listener, (struct sockaddr*)&my_addr, sizeof(my_addr));

    if (ret < 0) {     
        perror("Bind non riuscita\n");
        exit(-1); 
    }

    /* Apro la coda */
    listen(listener, 10);

    printf("\n\n\n\n\t\t\t\t PEER IN ASCOLTO SULLA PORTA :\t%d\n\n", peer->port);

    /* Aggiungo il socket "listener" ai socket monitorati */
    FD_SET(listener, &master);
    fdmax = (listener > fdmax) ? listener : fdmax;

    for (;;) {

        print_help();

        read_fds = master;
        
        select(fdmax + 1, &read_fds, NULL, NULL, NULL);

        for (i=0; i<=fdmax; i++) {

            if(FD_ISSET(i, &read_fds)) {

                if (i == STDIN_FILENO) {
                    _handle_cmd(peer);

                } else if (i != listener) {

                    printf("\n\n\n\n\t\t\t\t   [  RICEZIONE IN CORSO...  ]\n\n");
                    recv(i, bufferRicezione, MAXLINE, 0);

                    /* Verifico che sia un segnale da parte del server DS */
                    strcpy(bufferNeighbor, bufferRicezione);
                    token = strtok(bufferNeighbor, DELIM_NEIGHBOR);
                    signal = token;

                    /* SIGNAL 22: Il server comunica che questo è il primo peer del network */
                    if (atoi(bufferNeighbor) == 22) {

                        printf("\n\t\t    SEI IL PRIMO PEER DEL NETWORK. PER ORA NON HAI NEIGHBORS.\n\n");
                        break;
                    } 
                    
                    /* SIGNAL 10: Il server comunica che questo messsaggio contiene i nuovi vicini del peer */
                    if (atoi(signal) == 10) {

                        printf("\t\t\t      [  RICEZIONE NEIGHBORS IN CORSO...  ]\n\n");

                        /* Mi ricavo il left neighbor */
                        token = strtok(NULL, DELIM_LINE);
                        left = atoi(token);

                        /* Mi ricavo il right neighbor */
                        token = strtok(NULL, DELIM_LINE);
                        right = atoi(token);

                        /* Assegnamento */
                        peer->left_peer = left;
                        peer->right_peer = right;

                        printf("\t\t\tLEFT: [  %d  ]\t\t RIGHT: [  %d  ]\n\n", peer->left_peer, peer->right_peer);
                        break;
                    }
                    
                    /* SIGNAL 18: Il server comunica la chiusura del register corrente */
                    if(atoi(signal) == 18) {

                        printf("\t\t\t\t [  CHIUDO REGISTER CORRENTE  ]\n\n");

                        /* Il secondo parametro delle verifyTime forza la chiusura (vedi funzione) */
                        verifyTime(peer, 1);
                        break;
                    }

                    /* SIGNAL 11: è terminato il DS Server, termino il peer */
                    if (atoi(signal) == 11) {
                        stop_executor(token, peer);
                        break;
                    }

                    /* Se non è un segnale allora è stat effettuata una richiesta per le entry del giorno corrente */
                    strcpy(bufferCopy, bufferRicezione);

                    /* Verifico se sono il mittente del messaggio o un peer dell'anello */
                    ret = verificaDestinatario(&peer->port, &bufferCopy, peer);

                    if (ret > 0) {
                        /* Sono un peer dell'anello */

                        caricaBuffer(&bufferRicezione, peer, &ret);

                        if (peer->left_peer == ret && peer->right_peer == ret) {  
                            /* CASO SPECIALE: se ci sono due peer nel network ho già una connessione aperta con il mittente */
                            send(i, bufferRicezione, sizeof(bufferRicezione), 0);    
                        }else{
                            /* Ci sono più di due peer, necessito di una nuova connessione TCP */
                            inviaDentroAnello(&bufferRicezione, peer, &ret); 
                        }

                    } else {

                        /* Sono il mittente, ricevo il buffer */
                        memorizzaEntrate(&bufferRicezione, peer);
                    }

                    /* Chiudo il socket che ho servito */
                    close(i);
                    printf("\t\t\t\t       [  CHIUSURA SOCKET  ]\n\n");

                    /* Rimuovo il socket 'i' dal set dei socket monitorati */
                    FD_CLR(i, &master);
                    
                }else {
                    /* È arrivata una nuova richiesta di connessione */

                    /* Calcolo la lunghezza dell'indirizzo del client */
                    addrlen = sizeof(cl_addr);
                
                    /* Accetto la connessione e creo il socket connesso ('newfd') */
                    newfd = accept(listener, (struct sockaddr *)&cl_addr, (socklen_t*)&addrlen);
                    
                    /* Aggiungo il socket connesso al set dei descrittori monitorati */
                    FD_SET(newfd, &master); 
                    
                    /* Aggiorno l'ID del massimo descrittore */
                    if (newfd > fdmax)
                        fdmax = newfd;  

                    printf("\n\n\n\n\t\t\t\t   [  CONNECTION ESTABLISHED!  ]\n\n\n");
                }        
            }
        }
       
    }

    return 0;
} 





