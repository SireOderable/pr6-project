CC=gcc

CFLAGS=-Wall -g -std=c11

LDLIBS= -lm

ALL = packet.o client.o protocol.o ui.o

all : cleanall $(ALL)
	$(CC) *.o -o client

packet.o : packet.c
	$(CC) -c $(CFLAGS) packet.c 

client.o : client.c
	$(CC) -c $(CFLAGS) client.c

protocol.o : protocol.c
	$(CC) -c $(CFLAGS) protocol.c

ui.o : ui.c
	$(CC) -c $(CFLAGS) ui.c

cleanall:

	rm -rf *~ $(ALL)
