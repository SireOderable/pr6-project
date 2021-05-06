#include "packet.h"


time_t timer = 0;

/**
 * Initialize a packet with the number of tlv given by nbtlv
 * 
 * @return a new packet with nbtlv tlv
 * */
packet *initPacket()
{
    packet *p = (packet *)malloc(sizeof(packet));
    p->magic = (uint8_t)95;
    p->version = (uint8_t)2;
    p->body_length = 0;
    p->packet_body = NULL;
    

    return p;
}

/**
 * Initialize a TLV with the type given
 * */
tlv *initTlv(int type)
{
    tlv *t = (tlv *)malloc(sizeof(tlv));
    t->next = NULL;

    switch (type)
    {
    case PAD1:
        t->type = 0;

        break;

    case PADN:
        t->type = 1;

        break;

    case NOTIFY_ACK:
        t->type = 3;

        break;

    case PUBLISH:
        t->type = 4;

        break;

    case DUMP:
        t->type = 5;
        t->length = 0;

        break;

    default:
        perror("wrong type of tlv given");
    }

    return t;
}

/**
 * Complete the tlv of type PadN
 * This tlv contains a block of @length zeros
 * 
 * @param t : tlv to complete
 * @param length : length of the zeros block
 * 
 * @return size of the completed tlv or -1 if an error occured
 * */
int PadN(tlv *t, uint8_t length)
{
    if (t->type != PADN)
        return -1;

    t->length = length;

    char *body = (char *)calloc(length, 1);
    t->tlv_body = body;

    return length + 2;
}

/**
 * Complete the tlv of type NotifyAck.
 * 
 * @param t : tlv to complete
 * @param tag : used to paired notify tlv and notify_ack tlv
 * @param id : id of the data given in the notify tlv
 * 
 * @return size of the completed tlv or -1 if an error occured
 * */
int NotifyAck(tlv *t, uint32_t tag, uint64_t id)
{

    if (t->type != NOTIFY_ACK)
        return -1;

    uint16_t mbz = 0;

    tag = htonl(tag);
    id = convert(id);

    void *body = malloc(14); // sizeof(mbz) + sizeof(tag) + sizeof(id) = 14
    memcpy(body, &mbz, 2);
    memcpy(body + 2, &tag, 4);
    memcpy(body + 6, &id, 8);

    t->tlv_body = body;
    t->length = 14;

    return 16; // 14 + 2
}

/**
 * Complete the tlv of type Publish
 * 
 * @param t : tlv to complete
 * @param timeout : expiration date of the data in seconds
 * @param id : id of client
 * @param secret : secret shared between server and client
 * @param data : data to publish
 * 
 * @return size of the completed tlv or -1 if an error occured
 * */
int Publish(tlv *t, uint16_t timeout, uint64_t id, uint64_t secret, unsigned char *data, int length)
{

    if (t->type != PUBLISH)
        return -1;

    if(timeout != 0)
        timeout = htons(timeout);
    id = convert(id);
    secret = convert(secret);

    void *body = malloc(18 + length); // sizeof(timeout) + sizeof(id) + sizeof(secret) = 18
    memcpy(body, &timeout, 2);
    memcpy(body + 2, &id, 8);
    memcpy(body + 10, &secret, 8);
    memcpy(body + 18, data, length);

    t->tlv_body = body;
    t->length = length + 18;

    return length + 20; // length + 18 + 2
}

/** Complete the tlv of type Dump (long)
 * 
 * @param t : tlv to complete
 * @param tag : tag of client
 * 
 * @return size of the completed tlv or -1 if an error occured
 * */
int Dump(tlv *t, uint32_t tag)
{
    if (t->type != DUMP)
        return -1;

    void *body = malloc(6); // sizeof(tag) + sizeof(MBZ) = 6

    uint16_t mbz = 0;
    tag = htonl(tag);

    memcpy(body, &mbz, 2);
    memcpy(body + 2, &tag, 4);

    t->tlv_body = body;
    t->length = 6;

    return 6;
}

/**
 * Add the tlv to the packet if it s possible
 * 
 * @param p : packet to complete
 * @param t : tlv to add
 * @param addLength : tell to the function if the size of the packet must be updated or not
 * 
 * @return 0 if the packet is added or -1 if there is not enough place in the packet
 * */
int addTlv(packet *p, tlv *t, int addLength)
{
    if (p->body_length + t->length + 2 > PACKET_SIZE)
        return -1;

    if(p->packet_body == NULL)
    {
        p->packet_body = t;
        p->last = t;
    }

    else
    {
        p->last->next = t;
        p->last = t;
    }

    if (addLength != -1)
        p->body_length += t->length + 2;

    return 0;
}

/**
 * Convert packet to a char * for sending it to the server.
 * 
 * @param p : packet to convert
 * 
 * @return a char * representing p
 * */
unsigned char *packetToChar(packet *p)
{
    unsigned char *msg = (unsigned char *)malloc(p->body_length + 4);
    memcpy(msg, p, 2);
    uint16_t l = htons(p->body_length);
    memcpy(msg + 2, &l, 2);

    int count = 4;
    int temp;

    for (tlv *t = p->packet_body; t != NULL || count < p->body_length; t = t->next)
    {
        temp = t->type;
        t->type = htons(t->type) >> 8;

        memcpy(msg + count, &t->type, 1);

        count++;

        if (temp != PAD1)
        {
            temp = t->length;
            t->length = htons(t->length) >> 8;
            memcpy(msg + count, &t->length, 1);
            memcpy(msg + count + 1, t->tlv_body, temp);

            count += temp + 1;
        }
    }

    return msg;
}

/**
 * convert msg to a packet.
 * 
 * @param msg : message
 * 
 * @return the packet represented by msg
 * */
packet *charToPacket(unsigned char *msg)
{
    packet *p = initPacket();

    p->magic = msg[0];
    p->version = msg[1];
    memcpy(&(p->body_length), msg + 2, 2);
    //printf("length_body : %d\n", p->body_length);

    p->body_length = ntohs(p->body_length);

    //printf("length_body BIG ENDIAN : %d\n", p->body_length);

    int count = 4;

    //p->packet_body = t;

    while (count < p->body_length + 4)
    {
        tlv * t = (tlv *) malloc(sizeof(tlv));
        t->next = NULL;

        ////printf("type :   %d       %d\n", msg[count], t->type);
        t->type = ntohs(msg[count]) >> 8;
        count++;

        if ((t->type != PAD1) && (count < p->body_length + 4))
        {
            t->length = ntohs(msg[count]) >> 8;
            count++;

            ////printf("length : %d      %d\n", msg[count], t->length);

            void *m = malloc(t->length);
            memcpy(m, msg + count, t->length);

            t->tlv_body = m;
            count += t->length;

        }

        addTlv(p, t, -1);
    }

    return p;
}

/**
 * Analyze msg and answer to it if an answer is required.
 * 
 * @param msg : message to analyze.
 * @param client
 * @param socket
 * @param addr : informations concerning the server
 * */
void packetReceived(unsigned char *msg, client *cl, int socket)
{

    packet *p = charToPacket(msg);

    //printf("type : %d\n", p->packet_body->type);

    packet *answer = initPacket();

    for (tlv *t = p->packet_body; t != NULL; t = t->next)
    {

        if (t->type == NOTIFY)
        {

            /** Extract informations of the packet **/

            uint16_t timeout;
            memcpy(&timeout, t->tlv_body, 2);
            timeout = ntohs(timeout);

            uint32_t tag;
            memcpy(&tag, t->tlv_body + 2, 4);
            tag = ntohl(tag);

            uint64_t id;
            memcpy(&id, t->tlv_body + 6, 8);
            id = convert(id);

            unsigned char *data = (unsigned char *)malloc(t->length - 14);
            memcpy(data, t->tlv_body + 14, t->length - 14);

            /****************************************/

            /** Treatment of the packet **/

            data_received *d_r = findIdDataReceived(id, cl);

            if (timeout == 0)
            {
                if (d_r != NULL)
                {
                    deleteDataReceived(d_r, cl);
                    showTables(cl);
                    showCommands();
                }
            }

            else
            {
                if (d_r != NULL)
                {
                    d_r->id = id;
                    d_r->expiration_date = time(NULL) + (time_t)timeout;
                    free(d_r->data);
                    d_r->data = data;
                    showTables(cl);
                    showCommands();
                }

                else
                {
                    data_received *new_data_received = initDataReceived(id, timeout, data, t->length - 14);

                    addDataReceived(new_data_received, cl);
                    showTables(cl);
                    showCommands();
                }
            }

            /*****************************/

            /** Creation of TLV for answer **/

            tlv *notify_ack = initTlv(NOTIFY_ACK);
            NotifyAck(notify_ack, tag, id);
            addTlv(answer, notify_ack, 0);

            /********************************/
        }

        else if (t->type == WARNING)
        {
            char *warning = (char *)malloc(t->length);
            memcpy(warning, t->tlv_body, t->length);

            printf("Warning : %s\n", warning);

            free(warning);
        }

        else if(t->type == DUMP_ACK)
        {
            cl->synchronized = 1;
            showTables(cl);
            showCommands();
            printf("Synchronized with the server.\n");
        }

        else
        {
        }
    }

    /** Answer of the client **/

    if (answer->packet_body != NULL)
    {
        unsigned char *msg = packetToChar(answer);
        send(socket, msg, answer->body_length + 4, 0);
        timer = 0;
        free(msg);
    }
    free(p);
    free(answer);

    /**************************/
}



/**
 * Check informations between data_received_table and data_published_table of client. If informations are not the same,
 * p will be completed to send to the server
 * 
 * @param cl : client
 * @param p : packet to complete
 * */
void checkTables(client *cl, int sock)
{
    data_received *temp;

    for (data_published *data = cl->data_published_table; data != NULL; data = data->next)
    {
        temp = findIdDataReceived(data->id, cl);

        if (temp == NULL || memcmp(data->data, temp->data, data->data_length) != 0 || difftime(data->expiration_date, temp->expiration_date) > 1)
        {
            packet * p = initPacket();

            tlv *t = initTlv(PUBLISH);
            Publish(t, difftime(data->expiration_date, time(NULL)), data->id, data->secret, data->data, data->data_length);
            addTlv(p, t, 0);

            unsigned char * msg = packetToChar(p);

            send(sock, msg, p->body_length + 4, 0);
            timer = 0;
        }
    }
}



/**
 * Check if published data need to be refresh, if its the case, send packets to refresh data
 * 
 * @param d_p : published data
 * @param sock : socket 
 * */
void checkDataPublished(client * cl, int sock)
{
    for(data_published * data = cl->data_published_table; data != NULL; data = data->next)
    {
        if(difftime(data->refresh_date, time(NULL)) < 0)
        {
            data->expiration_date = time(NULL) + cl->timeout;
            data->refresh_date = time(NULL) + ((3 * cl->timeout) / 4);

            packet * p = initPacket();

            tlv * t = initTlv(PUBLISH);
            Publish(t, cl->timeout, data->id, data->secret, data->data, data->data_length);
            addTlv(p, t, 0);
            unsigned char * msg = packetToChar(p);

            send(sock, msg, p->body_length + 4, 0);
            timer = 0;
        }
    }
}



