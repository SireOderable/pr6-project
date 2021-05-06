#ifndef CLIENT_H
#define CLIENT_H

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>



struct Data_Received{

    uint64_t id;
    time_t expiration_date;
    unsigned char * data;
    int data_length;
    struct Data_Received * next;
    struct Data_Received * previous;

}typedef data_received;


struct Data_Published{

    uint64_t id;
    time_t expiration_date;
    time_t refresh_date;
    uint64_t secret;
    unsigned char * data;
    int data_length;
    struct Data_Published * next;
    struct Data_Published * previous;

}typedef data_published;


struct Client{

    data_received * data_received_table;
    data_published * data_published_table;
    uint16_t timeout;
    int synchronized;

}typedef client;


client * initClient(int timeout);
data_published * initDataPublished(uint64_t id, uint64_t secret, time_t timeout, unsigned char * d, int length);
data_received * initDataReceived(uint64_t id, time_t timeout, unsigned char * d, int length);
void getParams(int * params, char * address);


data_received * findIdDataReceived(uint64_t id, client * cl);
void checkDataReceived(client * cl);
void addDataReceived(data_received * data, client * cl);
void deleteDataReceived(data_received * data, client * cl);


data_published * findIdDataPublished(uint64_t id, client * cl);
void addDataPublished(data_published * data, client * cl);
void deleteDataPublished(data_published * data, client * cl);


uint64_t randId();
uint64_t convert(uint64_t num);


#endif