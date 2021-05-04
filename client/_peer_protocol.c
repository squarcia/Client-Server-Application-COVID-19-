#include "_peer_protocol.h"


int started = 0;
int periodoAnalisi = 0;
int numRegister = 0;
int canCalculate = 0;
int numPeers = 1;

const char *types[] = {"TAMPONE", "NUOVO_CASO"};

const char* valid_cmds[] = {"start", "add", "get", "stop"};

cmd_executor executors[] = {
    *start_executor,
    *add_executor,
    *get_executor,
    *stop_executor
};


const char* help_msg =
    "\n\n   ****************************************** PEER ******************************************\n\n"
    "\t!start   <DS_addr> <DS_port>         --> effettua la connessione al network\n"
    "\t!add     <type> <quantity>           --> aggiunge una tupla al register corrente\n"
    "\t!get     <aggr> <type> <period>      --> effettua una richiesta di elaborazione\n"
    "\t!stop                                --> disconnette il peer dal network\n\n\n";



void getTime(char *buffer) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(buffer, "%02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void getData(char *buffer, int giorno_dopo) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(buffer, "%d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday + giorno_dopo);
}

void getHour(char *buffer) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(buffer, "%02d\n", tm.tm_hour);
}

void printList(struct peer_info *peer) {

    struct register_info *current_node = peer->register_list;

   	while (current_node != NULL) {
        printf("DATA: %s\nFILENAME: %s\nCHIUSO: %d\n", current_node->data, current_node->filename, current_node->chiuso);
        current_node = current_node->next;
    }
}

void inserisciRegistro(struct register_info *item, struct peer_info *peer) {

    struct register_info *current_node = peer->register_list;

    if (current_node == NULL) {

        peer->register_list = item;  
    }else {   
            
        while(current_node->next != NULL) {
            current_node = current_node->next;
        }
        current_node->next = item;
    }
        
    numRegister++;
}

int numTamponiByString(char *arg, char**type) {

    char *token;

    token=strtok(arg, DELIM_LINE);
    token = strtok(NULL, DELIM_LINE);

    if (strcmp(*type, token) != 0) {
        return 0;
    }else {
        token=strtok(NULL, DELIM_LINE);
        return atoi(token);
    }
}

int numNuoviCasiByString(char *arg, char**type) {

    char *token;

    token=strtok(arg, DELIM_LINE);
    token = strtok(NULL, DELIM_LINE);

    if (strcmp(*type, token) != 0) {
        return 0;
    }else {
        token=strtok(NULL, DELIM_LINE);
        return atoi(token);
    }
}

void splitString(char *arg, char **aggr, char **type, char **data_inizio, char** data_fine) {

    char *token;
    int i = 0;

    token=strtok(arg, DELIM);
        while(token)
        {    
            switch (i)
            {
                case 0:
                    *aggr = token;
                    break;
                
                case 1:
                    *type = token;
                    break;

                case 2:
                    *data_inizio = token;
                    break;

                case 3:
                    *data_fine = token;
                    break;
                
                default:
                    break;
            }
            token=strtok(NULL, DELIM); 
            i++;      
        }
}

void stopEngine() {
    
    for (int i=0; i < MAXLINE; i++) {

        for (int j=0; j < MAXLINE; j++) {
            free(entry_engine[i]->dates_sent[j]);
            free(entry_engine[i]->dates_received[j]);
        }

        free(entry_engine[i]->dates_sent); 
        free(entry_engine[i]->dates_received); 

        free(entry_engine[i]);
    }
}

void splitDate(char **date, int *day, int *month, int *year) {

    char *token;
    int i = 0;

    token=strtok(*date, DELIM_DATE);
        while(token)
        {    
            switch (i)
            {
                case 0:
                    *year = atoi(token);
                    break;
                
                case 1:
                    *month = atoi(token);
                    break;

                case 2:
                    *day = atoi(token);
                    break;
                
                default:
                    break;
            }

            token=strtok(NULL, DELIM_DATE); 
            i++;      
        }
}


void convertDate(char **data_inizio, char **data_fine, int *dl, int *ml, int *yl, int *du, int *mu, int *yu) {

    splitDate(data_fine, du, mu, yu);
    splitDate(data_inizio, dl, ml, yl);
}

int calcolaGiorni(int dayL, int monthL, int yearL, int dayU, int monthU, int yearU) {

    int day_diff;

    if(dayU < dayL)
    {      
        // borrow days from february
        if (monthU == 3)
        {
            //  check whether year is a leap year
            if ((yearU % 4 == 0 && yearU % 100 != 0) || (yearU % 400 == 0)) 
            {
                dayU += 29;
            }

            else
            {
                dayU += 28;
            }                        
        }

        // borrow days from April or June or September or November
        else if (monthU == 5 || monthU == 7 || monthU == 10 || monthU == 12) 
        {
           dayU += 30; 
        }

        // borrow days from Jan or Mar or May or July or Aug or Oct or Dec
        else
        {
           dayU += 31;
        }

        monthU = monthU - 1;
    }

    if (monthU < monthL) {
        monthU += 12;
        yearU -= 1;
    }       

    day_diff = dayU - dayL;

    if (day_diff == 1)
        day_diff = -1;

    return day_diff;
}


int calcolaPeriodo(char**data_inizio, char** data_fine) {

    int dayL = 0;
    int monthL = 0;
    int yearL = 0;

    int dayU = 0;
    int monthU = 0;
    int yearU = 0;

    convertDate(data_inizio, data_fine, &dayL, &monthL, &yearL, &dayU, &monthU, &yearU);

    return calcolaGiorni(dayL, monthL, yearL, dayU, monthU, yearU);
}


int dividiStringa(char *arg, char **aggr, char **type, char **data_inizio, char **data_fine, struct peer_info *peer) {

    struct register_info *current_node = peer->register_list;

    splitString(arg, aggr, type, data_inizio, data_fine);

    if (!(*aggr && *aggr[0])) {
        printf("NESSUNA AGGREGAZIONE: ERRORE.\n\n");
        return -1;
    }

    if (!(*type && *type[0])) {
        printf("NESSUNA TIPO: ERRORE.\n\n");
        return -1;
    }

    if (strcmp(*data_fine, "*") == 0 && strcmp(*data_inizio, "*") == 0) {
        printf("\t\t     [  AGGREGAZIONE NON POSSIBILE: SOLO UN CAMPO PUO' VALERE (*)  ]\n\n");
        return -1;
    }

    if (!(*data_fine && *data_fine[0]) || strcmp(*data_fine, "*") == 0) {

        while (current_node->next != NULL) {
            current_node = current_node->next;
        }
      
        strcpy(*data_fine, current_node->data);

        printf("\t\t NESSUNA DATA FINALE:   CALCOLO FINO ALLA DATA PIU RECENTE  ----> %s\n\n", *data_fine);
    }

    if (!(*data_inizio && *data_inizio[0]) || strcmp(*data_inizio, "*") == 0) {
        strcpy(*data_inizio, peer->dataRemota);
        printf("\t\t NESSUNA DATA INIZIALE: USO DATA INIZIALE ASSOLUTA          ----> %s\n\n", *data_inizio);
    }

    return 0;
}

int start_executor(char* arg, struct peer_info *peer) {

   
    char *token, *addr, *port;
    char *bufferInvio;
    struct sockaddr_in srv_addr; 
    int sd, ret;

    printf("\n\n\n\t\t\t**********   START EXECUTOR RUNNING   **********\n\n");

    /* Mi ricavo il l'indirizzo IP e il numero di porta inserito da tastiera */
    token=strtok(arg, DELIM);
    addr = token;
    port = strtok(NULL, DELIM);


    /* Se l'indirizzo del server è diverso da questo termino il processo */
    if (strcmp(addr, "127.0.0.1") != 0) {
        printf("\t\t\t\t     INDIRIZZO IP NON VALIDO. \n\n\n");
        return -1;
    }

    /* Se la porta di ascolto del server è diversa da questa termino il processo */
    if (strcmp(port, "4242") != 0) {
        printf("\t\t\t\t       PORTA NON VALIDA. \n\n\n");
        return -1;
    }
        
    printf("\t\t\t\tINDIRIZZO IP :\t\t %s\n\t\t\t\tPORTA DI ASCOLTO :\t %s\n", addr, port);
    printf("\n\t\t\t************************************************\n\n");

    /* *** Effettuo l'invio dei dati tramite un socket UDP, serializzando le informazioni in formato Text Protocol *** */

    /* Creazione socket */
    sd = socket(AF_INET, SOCK_DGRAM, 0);

    /* Creazione indirizzo del server */
	memset(&srv_addr, 0, sizeof(srv_addr)); 
	srv_addr.sin_family = AF_INET; 
	srv_addr.sin_port = htons(PORT); 
	inet_pton(AF_INET, "127.0.0.1", &srv_addr.sin_addr);
	

    /* Allocazione del buffer */
    bufferInvio = (char* )malloc(sizeof(struct peer_info));

    /* Serializzazione */
    sprintf(bufferInvio, "%d %s %s %s %d", peer->port, peer->dataRemota, peer->register_list->filename, peer->register_list->data, peer->register_list->chiuso);

    printf("\n\n\n\t\t\t\t INFORMAZIONI INVIATE AL DS SERVER\n");

    ret = sendto(sd, bufferInvio, sizeof(bufferInvio), 0, (struct sockaddr *)&srv_addr, sizeof(srv_addr));

    if (ret < 0) {
        perror("Errore in fase di invio: \n");
        exit(-1);
    }

    free(bufferInvio);

    started = 1;

    return 1;
}


void verifyTime(struct peer_info *peer, int chiusura_forzata) {

    char *hour, *data, *today, *filename;
    int hours = 0;

    struct register_info *register_item;
    struct register_info *current_node = peer->register_list;

    /* Allocazione */
    hour = (char *) malloc(MAXLINE);
    data = (char *) malloc(MAXLINE);
    filename = (char *) malloc(MAXLINE);
    today = (char *) malloc(MAXLINE);

    register_item = (struct register_info *) malloc(sizeof(struct register_info));

    /* Pulizia */
    memset(hour, 0, MAXLINE);
    memset(data, 0, MAXLINE);
    memset(filename, 0, MAXLINE);
    memset(register_item, 0, sizeof(struct register_info));

    /* Ricavo ora corrente */
    getHour(hour);
    hours = atoi(hour);

    getData(today, 0);

    while (current_node->next != NULL) {
        current_node = current_node->next;
    }

    /* Il register corrente viene chiuso nei seguenti casi: 
        
        1)      Sono passate le 18:00 del giorno corrente
        2)      Il DS ha richiesto la chiusura immediata
        
    */

   if ((strcmp(current_node->data, today) == 0 && hours >= 00) || chiusura_forzata) {
       
        printf("\t\t\t\t      [  REGISTER CHIUSO  ]\n\n");

        /* Mi ricavo la data del prossimo register */
        getData(data, numRegister);

        /* Serializzo */
        sprintf(filename, "register/%s_%d.txt", data, peer->port);

        strcpy(register_item->filename, filename);
        strcpy(register_item->data, data);

        fopen(filename, "a+");

        register_item->chiuso = 0;  
        register_item->next = NULL;

        /* Chiudo register corrente */
        current_node->chiuso = 1;

        inserisciRegistro(register_item, peer);

        printf("\t\t\t\t   [  NUOVO REGISTER APERTO  ]\n\n\n\n");
   }
}


int add_executor(char* arg, struct peer_info *peer) {

    char *token, *quantity, *type;
    char *entry;
    char *date;
    FILE *fp;

    struct register_info *temp = peer->register_list;

    printf("\n\n\n\t\t\t  **********   ADD EXECUTOR RUNNING   **********\n\n");
    
    /* Verifico se posso inserire la nuova entry nel register corrente */
    verifyTime(peer, 0);

    /* Scorro fino all'ultimo elemento della lista */
    while (temp->next!= NULL) 
        temp = temp->next;

    /* Ricavo il tipo */
    token=strtok(arg, DELIM);
    type = token;

    /* Ricavo la quantità */
    token=strtok(NULL, DELIM);
    quantity = token;

    /* Confronto il tipo inserito con le due costanti TAMPONE e NUOVO_CASO */
    if (strcmp(type, types[0]) != 0 && strcmp(type, types[1]) != 0) {
        printf("Errore, tipo non valido!");
        return -1;
    }
    
    printf("  TYPE :        [  %s  ]\n\n", type);
    printf("  QUANTITY :    [  %s  ]\n\n", quantity);

    /* Allocazione memoria */
    entry = (char *) malloc(MAXLINE);
    date = (char *) malloc(MAXLINE);
    
    /* Pulizia */
    memset(entry, 0, MAXLINE);
    memset(date, 0, MAXLINE);

    /* Mi ricavo la data in base al numero di register già inseriti */
    getData(date, numRegister - 1);

    printf("  DATA :        [  %s  ]\n\n", date);
    printf("  FILE :        [  %s  ]\n\n", temp->filename);

    /* Serializzazione */
    sprintf(entry, "%s,%s,%s.\n",date, type, quantity);

    /* Gestione del file */
    fp = fopen(temp->filename, "a+");
    fprintf(fp, "%s", entry);
    fclose(fp);

    printf("\n\n\t\t\t       [  NUOVO ENTRY INSERITA CON SUCCESSO!  ]\n\n");

    /* Deallocazione */
    free(entry);
    free(date);

    return 0;
}

void calcolaVariazione(int *period, char **data_inizio, char **data_fine, struct peer_info *peer, char **tipologia){

    /* Puntatore all'elemento iniziale del periodo di riferimento */
    struct register_info *start_date = peer->register_list;

    /* Puntatore all'elemento finale del periodo di riferimento */
    struct register_info *final_date = peer->register_list;

    int totaleTamponiOggi = 0;
    int totaleTamponiDomani = 0;
    int numTamponi = 0;

    int totaleCasiOggi = 0;
    int totaleCasiDomani = 0;
    int numCasi = 0;

    int diffCasi = 0;
    int diffTamponi = 0;

    char* line = NULL;
    char dataTargetInizio[MAXLINE];
    char dataTargetFine[MAXLINE];

    /* Variabili per la gestione dei file */
    size_t len = 0;
    ssize_t read;
    FILE *fp;

    /* Copia delle date in variabili di appoggio */
    strcpy(dataTargetFine, *data_fine);
    strcpy(dataTargetInizio, *data_inizio);

    /* Cerco l'elemento inziale del periodo di riferimento */
    while (strcmp(start_date->data, dataTargetInizio) != 0 && start_date != NULL) {
            start_date = start_date->next;
    }

    /* Cerco l'elemento finale del periodo di riferimento */
    while (strcmp(final_date->data, dataTargetFine) != 0 && final_date != NULL) {
        final_date = final_date->next;
    }

    /* Vado avanti fin quando non arrivo alla data finale */

    while (strcmp(start_date->data, final_date->data) != 0) {

        if (start_date->chiuso) {
        
            fp = fopen(start_date->filename, "r");

            while ((read = getline(&line, &len, fp)) != -1) {

                if (strcmp(*tipologia, "TAMPONE") == 0) {

                    numTamponi = numTamponiByString(line, tipologia);
                    //printf("Numero tamponi: %d\n\n", numTamponi);
                    //printf("%s\n", line);
                    totaleTamponiOggi += numTamponi;
                } else {
                    
                    numCasi = numNuoviCasiByString(line, tipologia);
                    //printf("Numero casi: %d\n\n", numCasi);
                    //printf("%s\n", line);
                    totaleCasiOggi += numCasi;
                }
            }
        }

        fclose(fp);
      
        /* Puntatore al register di domani (all'interno del periodo) */
        start_date = start_date->next;  

        if (start_date->chiuso) {

            fp = fopen(start_date->filename, "r");
            
            while ((read = getline(&line, &len, fp)) != -1) {

                if (strcmp(*tipologia, "TAMPONE") == 0) {

                    numTamponi = numTamponiByString(line, tipologia);
                    //printf("Numero tamponi: %d\n\n", numTamponi);
                    //printf("%s\n", line);
                    totaleTamponiDomani += numTamponi;
                } else {
                    
                    numCasi = numNuoviCasiByString(line, tipologia);
                    //printf("Numero casi: %d\n\n", numCasi);
                    //printf("%s\n", line);
                    totaleCasiDomani += numCasi;
                }
            }
        }

        fclose(fp);

        printf("TOTALE TAMPONI: %d\n\n", totaleTamponiDomani);


        if (strcmp(*tipologia, "TAMPONE") == 0) {

            diffTamponi = totaleTamponiDomani - totaleTamponiOggi;

            if (diffTamponi < 0) {

                printf("\tIl numero di casi positivi ai tamponi sono in diminuzione: %d\n\n", diffTamponi);
            } else{

                printf("\tIl numero di casi positivi ai tamponi sono in aumento: %d\n\n", diffTamponi);
            } 
        }else {

            if (diffCasi < 0) {

                printf("\tIl numero di nuovi casi sono in diminuzione: %d\n\n", diffCasi);
            } else {

                printf("\tIl numero di nuovi casi  sono in aumento: %d\n\n", diffCasi);
            }
        }

    /* Fine While Loop */
    }

    /* Alla prossima richiesta di aggregazione è necessario richiedere le entry mancanti nell'anello */
    canCalculate = 0;
}

void calcolaRisultato(int *period, char **data_inizio, char **data_fine, struct peer_info *peer, char **tipologia) {

    /* Puntatore all'elemento iniziale del periodo di riferimento */
    struct register_info *start_date = peer->register_list;

     /* Puntatore all'elemento finale del periodo di riferimento */
    struct register_info *final_date = peer->register_list;

    int totaleTamponi = 0;
    int numTamponi = 0;

    int totaleCasi = 0;
    int numCasi = 0;

    char* line = NULL;
    char dataTargetInizio[MAXLINE];
    char dataTargetFine[MAXLINE];
    
    /* Variabili per la gestione dei file */
    size_t len = 0;
    ssize_t read;
    FILE *fp;

    /* Copia delle date in variabili di appoggio */
    strcpy(dataTargetFine, *data_fine);
    strcpy(dataTargetInizio, *data_inizio);
    
    if (*period == -1) {

    /* Se il periodo di riferimento comprende un giorno, forniamo i tamponi/casi di oggi */
        /* Apertura file */
        fp = fopen(start_date->filename, "r");
        
        while ((read = getline(&line, &len, fp)) != -1) {

            if (strcmp(*tipologia, "TAMPONE") == 0) {
                printf("Line: %s\n", line);
                numTamponi = numTamponiByString(line, tipologia);
                totaleTamponi += numTamponi;
            } else {
                
                numCasi = numNuoviCasiByString(line, tipologia);
                totaleCasi += numCasi;
            }
         } 

        fclose(fp);

    } else {
        /* Periodo di riferimento più lungo di un giorno */

        /* Cerco l'elemento inziale del periodo di riferimento */
        while (strcmp(start_date->data, dataTargetInizio) != 0 && start_date != NULL) {
                start_date = start_date->next;
        }

        /* Cerco l'elemento finale del periodo di riferimento */
        while (strcmp(final_date->data, dataTargetFine) != 0 && final_date != NULL) {
            final_date = final_date->next;
        }

        /* Vado avanti fin quando non arrivo alla data finale */

        while (start_date != final_date) {

            if (start_date->chiuso) {
               // printf("Entro: %s %d", start_date->data, start_date->chiuso);
                fp = fopen(start_date->filename, "r");

                while ((read = getline(&line, &len, fp)) != -1) {

                    if (strcmp(*tipologia, "TAMPONE") == 0) {

                        numTamponi = numTamponiByString(line, tipologia);
                        totaleTamponi += numTamponi;

                    } else {
                        
                        numCasi = numNuoviCasiByString(line, tipologia);
                        totaleCasi += numCasi;
                    }
                }

                start_date = start_date->next;
                
                fclose(fp);
            }
        }
    }


    if (strcmp(*tipologia, "TAMPONE") == 0) {

        printf("\n\t\t\t\t    [  TOTALE TAMPONI :\t%d  ]\n\n\n", totaleTamponi);
    }else {

        printf("\n\t\t\t\t    [  TOTALE CASI :\t%d  ]\n\n\n", totaleCasi);
    }

    /* Alla prossima richiesta di aggregazione è necessario richiedere le entry mancanti nell'anello */
    canCalculate = 0;
}

void richiediEntries(int *period, char **data_inizio, char **data_fine, char** type, struct peer_info *peer) {

    char now[MAXLINE];
    char bufferInvio[MAXLINE];
   
    int ret = 0;

    struct sockaddr_in peer_addr;

    /* Ricavo l'ora corrente */
    getTime(now);

    if (peer->left_peer == 0) {
        printf("\n\t\t\t  [  NESSUN NEIGHBOR A CUI RICHIEDERE ENTRIES  ]\n\n\n");
        canCalculate = 1;
        return;
    }

    printf("\n\t\t\t[  RICHIEDO TUTTE LE ENTRY MANCANTI. ATTENDO...  ]\n\n\n");
    sprintf(bufferInvio, "%s-%d:", *type, peer->port);

    /* Creazione socket */
    newfd = socket(AF_INET,SOCK_STREAM,0);

    /* Creazione indirizzo del server */
    memset(&peer_addr, 0, sizeof(peer_addr)); 
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(peer->left_peer);
    inet_pton(AF_INET, "127.0.0.1", &peer_addr.sin_addr);

    ret = connect(newfd, (struct sockaddr*)&peer_addr, sizeof(peer_addr));

    if (ret < 0){
        perror("Errore in fase di comunicazione con il client: \n");
        return;
    }

    /* CASO 2 PEER: viene riutilizzata la connessione TCP */
    if (peer->left_peer == peer->right_peer) {

        FD_SET(newfd, &master);
        fdmax = (newfd > fdmax) ? newfd : fdmax;
    }

    /* Invio */
    send(newfd, bufferInvio, sizeof(bufferInvio), 0);

    /* CASO N PEER: viene chiusa la connessione TCP */
    if (peer->left_peer != peer->right_peer) {
        close(newfd);
    }
}

int get_executor(char* arg, struct peer_info *peer) {

    printf("\n\n\n\t\t\t  **********   GET EXECUTOR RUNNING   **********\n\n");

    char *aggr, *type, *data_inizio, *data_fine;
    char *temp_fine, *temp_inizio;
    int period = 0;
    int ret = 0;

    if (!peer->register_list->chiuso) {
        printf("\t\t     [  AGGREGAZIONE NON POSSIBILE. NESSUN REGISTER CHIUSO  ]\n\n");
        return -1;
    }

    /* Allocazione delle variabili */
    temp_inizio = (char*) malloc(MAXLINE);
    memset(temp_inizio, 0, MAXLINE);

    temp_fine = (char*) malloc(MAXLINE);
    memset(temp_fine, 0, MAXLINE);

    data_inizio = (char*) malloc(MAXLINE);
    memset(data_inizio, 0, MAXLINE);

    data_fine = (char*) malloc(MAXLINE);
    memset(data_fine, 0, MAXLINE);

    aggr = (char*) malloc(MAXLINE);
    memset(aggr, 0, MAXLINE);

    type = (char*) malloc(MAXLINE);
    memset(type, 0, MAXLINE);

    /* Ricavo i parametri passati tramite input */
    ret = dividiStringa(arg, &aggr, &type, &data_inizio, &data_fine, peer);

    /* Errore */
    if (ret < 0)
        return -1;

    /* Copia in variabili di appoggio */
    strcpy(temp_fine, data_fine);
    strcpy(temp_inizio, data_inizio);

    /* Ricavo i giorni */
    period = calcolaPeriodo(&temp_inizio, &temp_fine);

    /* Aggiustamento parametri */
    if (strcmp(type, "tampone") == 0 || strcmp(type, "tamponi") == 0)
        strcpy(type, "TAMPONE");

    /* Aggiustamento parametri */
    if (strcmp(type, "nuovo_caso") == 0 || strcmp(type, "nuovi_casi") == 0)
        strcpy(type, "NUOVO_CASO");

    periodoAnalisi = period;

    if (!canCalculate)
        richiediEntries(&period, &data_inizio, &data_fine, &type, peer);

    if (strcmp(aggr, "totale") == 0 && canCalculate) 
        calcolaRisultato(&period, &data_inizio, &data_fine, peer, &type);

    if (strcmp(aggr, "variazione") == 0 && canCalculate) 
        calcolaVariazione(&period, &data_inizio, &data_fine, peer, &type);

    return 0;
}

int stop_executor(char* arg, struct peer_info *peer) {

    int sd, ret;
    char *bufferInvio;
    struct sockaddr_in srv_addr;

    printf("\n\n\n\t\t\t **********   STOP EXECUTOR RUNNING   **********\n\n");

    /* Creazione socket */ 
    sd = socket(AF_INET, SOCK_DGRAM, 0);

    /* Inizializzazione struttura server */
	memset(&srv_addr, 0, sizeof(srv_addr)); 
	srv_addr.sin_family = AF_INET; 
	srv_addr.sin_port = htons(PORT); 
	inet_pton(AF_INET, "127.0.0.1", &srv_addr.sin_addr);
	
    /* Invio il codice -1 al DS Server */
    bufferInvio = (char* )malloc(10);
    sprintf(bufferInvio, "-1,%d.", peer->port);

    printf("\n\n\n\t\t\t\t INFORMAZIONI INVIATE AL DS SERVER");
    printf("\n\n\n\n\t\t\t\t            ARRIVEDERCI!\n\n\n\n");

    /* Invio messaggio tramite UDP */
    ret = sendto(sd, bufferInvio, sizeof(bufferInvio), 0, (struct sockaddr *)&srv_addr, sizeof(srv_addr));

    if (ret < 0) {
        perror("Errore in fase di invio: \n");
        exit(-1);
    }

    exit(1);
    return 0;
}



void print_help(){
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

int process_command(const char* cmd, char* arg, struct peer_info *peer) {

    int i, ris;

    for (i = 0; i < COMMANDS; ++i){
        if (strcmp(cmd, valid_cmds[i]) == 0){
            ris = executors[i](arg, peer);
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

int _handle_cmd(struct peer_info *peer) {

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

    if (strlen(arg) == 0 && strcmp(cmd, "stop") != 0) {
        printf("\n\n\n\t\t***** COMANDO NON VALIDO: INSERIMENTO PARAMETRI NECESSARIO. *****\n\n\n");
        return -1;
    }

    if (!started && strcmp(cmd, "start") != 0) {
        printf("\n\n\n\t\t***** COMANDO NON VALIDO: CONNESSIONE AL SERVER RICHIESTA. *****\n\n\n");
        return -1;
    }

    ris = process_command(cmd, arg, peer);
    free(buf);
    return ris;
}

void inserisciEntrate(struct register_info *temp, int index, char (*buffer)[], int i) {

    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while (temp != NULL) { 

        fp = fopen(temp->filename, "r");

        if((read = getline(&line, &len, fp)) == -1) {
            //printf("FILE VUOTO.\n\n");
            sprintf(*buffer + strlen(*buffer),"*.");
        }

        /* Serializzo tutte le righe del file e le inserisco nel buffer */
        while ((read = getline(&line, &len, fp)) != -1) {
           
            line[strcspn(line, "\n")] = 0;
            sprintf(*buffer + strlen(*buffer),"%s", line);
        }

        fclose(fp); 

        strcpy(entry_engine[i]->dates_sent[index], temp->data);   

        index++;
        temp = temp->next;
    }
}

void caricaBuffer(char (*buffer)[], struct peer_info *peer, int *port) {

    char today[MAXLINE];
    char data[MAXLINE];

    int index = 0;

    struct register_info *temp = peer->register_list;

    /* Verifico se fra tutti i peer a cui ho inviato le entry compare il requester */
    for (int i=0; i < numPeers; i++) {

        /* Se è presente il peer requester */
        if (entry_engine[i]->peer == *port) {
            printf("\n\t\t\t\t[  TROVATO PEER ALL'INTERNO DEL REGISTRO.  ] \n\n\n");

            /* Scorro la lista dei register inviati fino a quando non trovo "0 "*/
            while (strcmp(entry_engine[i]->dates_sent[index], "0") != 0)
                index++;

            /* Se mi fermo subito significa che non ho mai inviato register a quel peer */
            if (!index) {
                
                /* Riempio buffer */
                inserisciEntrate(temp, index, buffer, i);
                break;
            }else {
                printf("\n\t\t\t[  DATA ULTIMO REGISTER INVIATO : %s  ]\n\n\n", entry_engine[i]->dates_sent[index - 1]);

                /* Copio l'ultima data del register inviato (sarà quello più recente) */
                strcpy(data, entry_engine[i]->dates_sent[index - 1]);

                /* Ricavo la data di oggi */
                getData(today, 0);

                /* Se la data di oggi è maggiore devo inviare tutte le entry di tutti i register nel periodo: 

                    [ data, today)
                    
                */
                if (strcmp(today, data) > 0) {
                    
                    while (temp != NULL && strcmp(temp->data, data) != 0) {
                        temp = temp->next;
                        inserisciEntrate(temp, index, buffer, i);
                    }
                }
                break;
            }
            
        } else {

            /* Se non trovo il peer, significa che non ho mai inviato register a quel peer */
            printf("\t\t\t   [  PEER NON PRESENTE. INVIO REGISTERS...  ]\n\n");     
            inserisciEntrate(temp, index, buffer, i);
            entry_engine[i]->peer = *port;
            numPeers++;  
            break; 
        } 
    }
}

void inviaDentroAnello(char (*buffer)[], struct peer_info *peer, int *port) {

    int ret, left_sd;
    struct sockaddr_in peer_addr;
    char bufferInvio[SUPERMAXLINE];

    strcpy(bufferInvio, *buffer);
    printf("BUFFER IN INVIA ANELLO: %s\n\n", bufferInvio);

    /* Creazione socket */
    left_sd = socket(AF_INET,SOCK_STREAM,0);
  
    // Creazione indirizzo del server 
    memset(&peer_addr, 0, sizeof(peer_addr)); // Pulizia 
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(peer->left_peer);
    inet_pton(AF_INET, "127.0.0.1", &peer_addr.sin_addr);

    ret = connect(left_sd, (struct sockaddr*)&peer_addr, sizeof(peer_addr));

    if (ret < 0){
        perror("Errore in fase di comunicazione con il client: \n");
        exit(-1);
    }

    /* Aggiungo il socket "left_sd" ai socket monitorati */
    FD_SET(left_sd, &master);
    fdmax = (left_sd > fdmax) ? left_sd : fdmax;

    /* Invio */
    send(left_sd, bufferInvio, sizeof(bufferInvio), 0);

    /* Chiusura e rimozione socket "left_sd" */
    close(left_sd);
    FD_CLR(left_sd, &master);
}

int verificaDestinatario(int *port, char (*arg)[], struct peer_info *peer) {

    char *token, type[MAXLINE];
    int port_buffer = 0;

    /* Ricavo il tipo di dato richiesto */
    token=strtok(*arg, DELIM_DATE);
    strcpy(type, token);

    /* Ricavo il numero di porta del mittente */
    token=strtok(NULL, DELIM_BUFFER);
    port_buffer = atoi(token);
    
    if (*port == port_buffer) {
        printf("\t\t\t\t[  DATI RICEVUTI CORRETTAMENTE  ]\n\n");
        return -1;
    } else {
        printf("\t\t    [  SONO UN PEER DELL'ANELLO. INVIO BUFFER IN CORSO...  ]\n\n");
        return port_buffer;
    }
}

void memorizzaEntrate(char (*buffer)[], struct peer_info *peer) {


    /*
        Si è usata la funzione strtok_r(...) per avanzare nel buffer    
        ricevuto, mentre si è usata la funzione strtok(...) per elaborare
        la singola linea letta
    
    */

    char *token;
    char *data, filename[SUPERMAXLINE];
    FILE *fp;
    char *pun = *buffer;
    char entry[1024];
    char entryReady[1024];
    
    //printf("BUFFER RICEVUTO: %s\n", *buffer);

    token=strtok_r(*buffer, ":", &pun);

    /* DEBUGGING */
    //  printf("\n\n\nToken TAMPONE: %s\n\n", token);
    //  printf("\n\n\nToken 2 TAMPONE: %s\n\n", pun);

    token=strtok_r(NULL, ".", &pun);
    /* DEBUGGING */
    //  printf("\n\n\nToken TAMPONE: %s\n\n", token);
    //  printf("\n\n\nToken 2 TAMPONE: %s\n\n", pun);

    while(token) {

        if (strcmp(token, "*") != 0) {

            /* Ricavo la entry */
            strcpy(entry, token);

            /* Ricavo la data */
            data = strtok(token, ",");

            /* DEBUGGING */
            //  printf("Token DATA: %s\n\n", data);
            //  printf("Inserisco entry %s nel file %s_4000.txt\n\n", entry, data);
            
            /* Creo nome file */
            sprintf(filename, "register/%s_%d.txt", data, peer->port);
            //  printf("Token FILENAME: %s\n\n", filename);

            /* Creo la entry */
            sprintf(entryReady, "%s.", entry);
            //  printf("Entry Ready : %s\n\n", entryReady);

            /* Gestione file */
            fp = fopen(filename, "a+");
            fprintf(fp, "%s\n", entryReady);
            fclose(fp);

            token = strtok_r(NULL, ".", &pun);
            //  printf("Token .: %s\n\n", token);
        }else {
            token = strtok_r(NULL, ".", &pun);
            //  printf("Token ENTRY: %s\n\n", token);
            //  printf("\n\n\nToken 2 TAMPONE: %s\n\n", pun);
        }
    }

    /* Abilito la possibilità di effettuare l'aggregazione */
    canCalculate = 1;
}

void creaRegistro(struct peer_info *peer, struct register_info *register_item) {

    char *filename, *data;

    data =  (char*) malloc(MAXLINE);
    filename = (char*) malloc(MAXLINE);

    /* Ricavo la data di oggi */
    getData(data, 0);

    /* Mi salvo la data più remota */
    strcpy(peer->dataRemota, data);

    /* Creazione primo register */
    sprintf(filename, "register/%s_%d.txt", data, peer->port);

    strcpy(register_item->filename, filename);
    strcpy(register_item->data, data);

    fopen(filename, "w+");
    register_item->chiuso = 0;  // true --> test
    register_item->next = NULL;
    
    /* Inserisco il register nella coda */
    inserisciRegistro(register_item, peer);

    free(filename);
    free(data);
}

void azzeraEngine() {

    for (int j=0; j < MAXLINE; j++) { 
        strcpy(entry_engine[0]->dates_sent[j], "0");
        strcpy(entry_engine[0]->dates_received[j], "0");
    }
}

void startEngine() {
    /*
        Un peer avrà solamente due neighbor, per cui si suppone che:

            1)  entry_engine[0] sia il neighbor-left
            2)  entry_engine[1] sia il neighbor-right
    */

    entry_engine = malloc(MAXLINE * sizeof(struct map));  

    for (int i=0; i < MAXLINE; i++) {

        entry_engine[i] = (struct map *) malloc(sizeof(struct map));

        entry_engine[i]->peer = 0;
        entry_engine[i]->peer = 0;

        entry_engine[i]->dates_sent = malloc(MAXLINE * sizeof(char*));
        entry_engine[i]->dates_received = malloc(MAXLINE * sizeof(char*));


            for (int j=0; j < MAXLINE; j++) {
                entry_engine[i]->dates_sent[j] = malloc(MAXLINE+1);
                entry_engine[i]->dates_received[j] = malloc(MAXLINE+1);
                
                strcpy(entry_engine[0]->dates_sent[j], "0");
                strcpy(entry_engine[0]->dates_received[j], "0");
            }
    }
}


