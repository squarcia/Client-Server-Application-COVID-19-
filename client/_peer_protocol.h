#include "peer.h"

#ifndef _PEER_PROTOCOL_H_   
#define _PEER_PROTOCOL_H_

#define MAXLINE 1024 
#define PORT	 4242 
#define COMMANDS 6

#define SUPERMAXLINE 86400
#define COMMAND_PREFIX '!'
#define DELIM " "
#define DELIM_DATE "-"
#define DELIM_LINE ","
#define DELIM_BUFFER "::"
#define DELIM_STRING "."
#define DELIM_NEIGHBOR ":"
#define DELIM_FILE "_"

typedef int (*cmd_executor)(char* arg, struct peer_info *peer);

int start_executor(char* arg, struct peer_info *peer);
int add_executor(char* arg, struct peer_info *peer);
int get_executor(char* arg, struct peer_info *peer);
int stop_executor(char* arg, struct peer_info *peer);


/* Verifica di connessione al server */
int started;

/* File Descriptor */
fd_set master;
fd_set read_fds;
int fdmax;

/* Socket */
int listener;
int newfd;

int periodoAnalisi;

/* Conta il numero di register creati */
int numRegister;

/* Verifica se è stata inviata o meno la richiesta di FLOODING */
int canCalculate;

/* Variabile per gestione entry*/
struct map **entry_engine;

/* Variabile che mi traccia il numero di peers aggiunti  */
int numPeers;    


void getTime(char *buffer);
void getData(char *buffer, int giorno_dopo);
void getHour(char *buffer);
void printList(struct peer_info *peer);
void inserisciRegistro(struct register_info *item, struct peer_info *peer);
int numTamponiByString(char *arg, char**type);
int numNuoviCasiByString(char *arg, char**type);
void splitString(char *arg, char **aggr, char **type, char **data_inizio, char** data_fine);
void stopEngine();
void splitDate(char **date, int *day, int *month, int *year);
void convertDate(char **data_inizio, char**data_fine, int *dl, int *ml, int *yl, int *du, int *mu, int *yu);
int calcolaGiorni(int dayL, int monthL, int yearL, int dayU, int monthU, int yearU);
int calcolaPeriodo(char**data_inizio, char** data_fine);
int dividiStringa(char *arg, char **aggr, char **type, char **data_inizio, char** data_fine, struct peer_info *peer);
void verifyTime(struct peer_info *peer, int chiusura_forzata);
void calcolaVariazione(int *period, char **data_inizio, char **data_fine, struct peer_info *peer, char **tipologia);
void calcolaRisultato(int *period, char **data_inizio, char **data_fine, struct peer_info *peer, char **tipologia);
void richiediEntries(int *period, char **data_inizio, char **data_fine, char** type, struct peer_info *peer);
void print_help();
int _parse_command(char* line, size_t line_len, char** cmd, char** arg);
int process_command(const char* cmd, char* arg, struct peer_info *peer);
int _handle_cmd(struct peer_info *peer);
void inserisciEntrate(struct register_info *temp, int index, char (*buffer)[], int i);
void caricaBuffer(char (*buffer)[], struct peer_info *peer, int *port);
void inviaDentroAnello(char (*buffer)[], struct peer_info *peer, int *port);
int verificaDestinatario(int *port, char (*arg)[], struct peer_info *peer);
int prelevaData(char (*buffer)[], char (*data)[]);
void memorizzaEntrate(char (*buffer)[], struct peer_info *peer);
void creaRegistro(struct peer_info *peer, struct register_info *register_item);
void azzeraEngine();
void startEngine();




#endif