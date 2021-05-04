int main(int argc, char *argv[]) { 

	int sd, ret; 
    int n, len; 
	char buffer[MAXLINE]; 
    char *client_port;
	struct sockaddr_in srv_addr; 

    int listener;
    struct sockaddr_in my_addr, cl_addr;

    fd_set read_fds;

    fd_set master;

    int fdmax;

	sd = socket(AF_INET, SOCK_DGRAM, 0);

	memset(&srv_addr, 0, sizeof(srv_addr)); 
	srv_addr.sin_family = AF_INET; 
	srv_addr.sin_port = htons(PORT); 
	srv_addr.sin_addr.s_addr = INADDR_ANY; 
	
    ret = connect(sd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
    if (ret < 0) { 
		perror("Errore in fase di connessione: \n"); 
		exit(-1); 
	} 

    ret = send(sd, (const char *)client_port, sizeof(client_port), 0);
    if (ret < 0) {
        perror("Errore in fase di invio porta: \n");
        exit(-1);
    }

	printf("Port message sent.\n"); 
    
   // ret = recv(sd, (void*)buffer, MAXLINE, 0);
    
    if(ret < 0) {
        perror("Errore in fase di ricezione: \n");
        exit(-1);
    }

    close(sd); 

    listener = socket(AF_INET, SOCK_STREAM, 0);
    memset(&my_addr, 0, sizeof(my_addr)); 
	my_addr.sin_family = AF_INET; 
	my_addr.sin_port = htons(4242); 
	my_addr.sin_addr.s_addr = INADDR_ANY;      

    ret = bind(listener, (struct sockaddr*)&my_addr, sizeof(my_addr));

    if (ret < 0) {     
        perror("Bind non riuscita\n");
        exit(-1); 
    }

    listen(listener, 10);

    FD_ZERO(&master);
    FD_ZERO(&read_fds);


    FD_SET(listener, &master);
    FD_SET(STDIN_FILENO, &master);
    fdmax = (listener > STDIN_FILENO) ? listener : STDIN_FILENO;

    for (;;) {

        print_help();

        read_fds = master;
        
        select(fdmax + 1, &read_fds, NULL, NULL, NULL);

        if (FD_ISSET(STDIN_FILENO, &read_fds))
           _handle_cmd();
        
      //  if (FD_ISSET(listener, &read_fds))
            // funzione per nuova connessione
    }

    return 0;
} 


