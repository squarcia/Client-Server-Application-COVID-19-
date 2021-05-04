HEADERS_SERVER = ./server/_server_protocol.h

OBJS_SERVER = ./server/ds.o ./server/_server_protocol.o

HEADERS_CLIENT = ./client/_peer_protocol.h

OBJS_CLIENT = ./client/peer.o ./client/_peer_protocol.o

OBJS = ./client/peer.o ./client/_peer_protocol.o ./server/ds.o ./server/_server_protocol.o

CFLAGS = -Wall

CC = gcc

all: ds peer

ds: $(OBJS_SERVER)
	$(CC) $(CFLAGS) $(OBJS_SERVER) -o ./server/ds

ds.o: ./server/ds.c $(HEADERS_SERVER)
	$(CC) $(CFLAGS) -c ./server/ds.c

_server_protocol.o: ./server/_server_protocol.c $(HEADERS_SERVER)
	$(CC) $(CFLAGS) -c ./server/_server_protocol.c

peer: $(OBJS_CLIENT)
	$(CC) $(CFLAGS) $(OBJS_CLIENT) -o ./client/peer
   
peer.o: ./client/peer.c $(HEADERS_CLIENT)
	$(CC) $(CFLAGS) -c ./client/peer.c

_peer_protocol.o: ./client/_peer_protocol.c $(HEADERS_CLIENT)
	$(CC) $(CFLAGS) -c ./client/_peer_protocol.c

clean:
	-rm -f $(OBJS)



