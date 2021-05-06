#ifndef PACKET_H
#define PACKET_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "client.h"
#include "ui.h"


#define PACKET_SIZE 1024


#define PAD1 0
#define PADN 1
#define NOTIFY 2
#define NOTIFY_ACK 3
#define PUBLISH 4
#define DUMP 5
#define WARNING 6
#define DUMP_ACK 7

/** Structures **/

struct TLV{

    uint8_t type;
    uint8_t length;
    void * tlv_body;
    struct TLV * next;

}typedef tlv;


struct Packet{

    uint8_t magic; // 95
    uint8_t version;  // 2
    uint16_t body_length;
    tlv * packet_body;
    tlv * last;

}typedef packet;

/****************/


/** Creations of packets **/

packet * initPacket();
tlv * initTlv(int type);

int PadN(tlv * t, uint8_t length);
int NotifyAck(tlv * t, uint32_t tag, uint64_t id);
int Publish(tlv * t, uint16_t timeout, uint64_t id, uint64_t secret, unsigned char * data, int length);
int Dump(tlv * t, uint32_t tag);

int addTlv(packet * p, tlv * t, int addLength);

/**************************/


/** Packet emission **/

unsigned char * packetToChar(packet * p);

/*********************/


/** Packets reception **/

packet *charToPacket(unsigned char *msg);
void packetReceived(unsigned char *msg, client * cl, int socket);

/**********************/

void checkTables(client *cl, int sock);
void checkDataPublished(client * cl, int sock);

#endif