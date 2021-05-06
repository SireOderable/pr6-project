#define _GNU_SOURCE

#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <netinet/in.h>
#include <time.h>
#include <netdb.h>
#include <unistd.h>

#include "client.h"
#include "packet.h"
#include "ui.h"

extern time_t timer;

int main()
{
    srand(time(NULL));
    timer = 0;

    /** initialization of the socket **/

    int err;

    int params[3];
    char *address = (char *)malloc(128);

    getParams(params, address);

    //printf("address : %s\n", address);

    int sock = socket(params[0], SOCK_DGRAM, 0);

    if (params[0] == PF_INET6)
    {
        struct sockaddr_in6 sin6;
        memset(&sin6, 0, sizeof(sin6));

        sin6.sin6_family = AF_INET6;
        err = inet_pton(AF_INET6, address, &sin6.sin6_addr);
        if (err < 0)
            abort();
        sin6.sin6_port = htons(params[1]);
        bind(sock, &sin6, sizeof(sin6));

        err = connect(sock, &sin6, sizeof(sin6));
        if (err >= 0)
            printf("Connected to %s on port %d\n\n\n", address, params[1]);
        else
        {
            printf("Error : connection failed\n\n\n");
            abort();
        }
    }

    else
    {
        struct sockaddr_in sin;
        memset(&sin, 0, sizeof(sin));

        sin.sin_family = AF_INET;
        err = inet_pton(AF_INET, address, &sin.sin_addr);
        if (err < 0)
            abort();
        sin.sin_port = htons(params[1]);
        bind(sock, &sin, sizeof(sin));

        err = connect(sock, &sin, sizeof(sin));
        if (err >= 0)
            printf("Connected to %s on port %d\n\n\n", address, params[1]);
        else
        {
            printf("Error : connection failed\n\n\n");
            abort();
        }
    }

    free(address);

    err = fcntl(sock, F_GETFL);
    if (err < 0)
        perror("fcntl");

    err = fcntl(sock, F_SETFL, err | O_NONBLOCK);
    if (err < 0)
        perror("fcntl2");

    err = fcntl(STDIN_FILENO, F_GETFL);
    if (err < 0)
        perror("fcntl stdin");

    err = fcntl(STDIN_FILENO, F_SETFL, err | O_NONBLOCK);
    if (err < 0)
        perror("fcntl stdin 2");

    /**********************************/

    /** Initialization of the client **/

    client *cl = initClient(params[2]);

    /**********************************/

    /*************** DUMP *************/

    printf("Synchronizing with the server...\n");
    while (!cl->synchronized)
    {
        if (timer >= 30)
        {
            perror("Non synchronized with server, abort\n");
            free(cl);
            return 0;
        }

        packet *pack = initPacket();
        tlv *dump = initTlv(DUMP);
        Dump(dump, randId());
        err = addTlv(pack, dump, 0);

        unsigned char *msg = packetToChar(pack);

        send(sock, msg, pack->body_length + 4, 0);

        free(msg);
        free(pack);
        free(dump);

        fd_set readfs;
        struct timeval timeout = {3, 0};

        FD_ZERO(&readfs);
        FD_SET(sock, &readfs);

        err = select(sock + 1, &readfs, NULL, NULL, &timeout);

        if (err < 0)
            perror("select dump");

        else if (FD_ISSET(sock, &readfs))
        {
            unsigned char *reply = calloc(1024, 1);

            err = recvfrom(sock, reply, 1024, 0, NULL, NULL);
            if (err < 0)
            {
                perror("recvfrom");
            }

            packetReceived(reply, cl, sock);
            free(reply);
        }

        else if (err == 0)
            timer += 3;

        else
        {
        }
    }
    timer = 0;

    /**********************************/

    /** Execution of the protocol **/

    fd_set readfs;

    struct timeval timeout = {8, 0};

    while (1)
    {

        if (timer >= 30)
        {
            //printf("30s\n");
            packet *p = initPacket();
            unsigned char *msg = packetToChar(p);
            send(sock, msg, p->body_length + 4, 0);

            timer = 0;
            free(p);
            free(msg);
        }

        FD_ZERO(&readfs);
        FD_SET(sock, &readfs);
        FD_SET(STDIN_FILENO, &readfs);

        err = select(sock + 1, &readfs, NULL, NULL, &timeout);

        if (err < 0)
            perror("select()");

        else if (FD_ISSET(sock, &readfs))
        {
            //printf("Packet received :\n");
            unsigned char *reply = calloc(1024, 1);

            err = recvfrom(sock, reply, 1024, 0, NULL, NULL);
            if (err < 0)
            {
                perror("recvfrom");
            }

            /*for (int i = 0; i < err; i++)
                printf(" |%x| ", reply[i]);
            printf("\n");*/

            packetReceived(reply, cl, sock);
            free(reply);
        }

        else if (FD_ISSET(STDIN_FILENO, &readfs)) // User has typed something
        {
            unsigned char *command = (unsigned char *)calloc(286, 1); // UINT64_T_MAX = 18446744073709551615 -> 20 digits max
            int length = read(STDIN_FILENO, command, 286) - 1;        // -1 because of \n
            if (length > 0)
            {

                if (memcmp(command, "!quit", 5) == 0)
                {
                    printf("Shutting down client...\n");
                    free(cl);
                    close(sock);
                    return 0;
                }

                else if (memcmp(command, "!publish", 8) == 0) // !publish new_message
                {
                    unsigned char *msg = (unsigned char *)calloc(length - 9, 1); // 8 + 1 (space)
                    memcpy(msg, command + 9, length - 9);

                    /** Create and add a new data_published **/

                    data_published *new = initDataPublished(randId(), randId(), cl->timeout, msg, length);
                    addDataPublished(new, cl);

                    /*****************************************/

                    /** Create and send the packet **/

                    packet *p = initPacket();
                    tlv *t = initTlv(PUBLISH);
                    Publish(t, cl->timeout, new->id, new->secret, new->data, length - 9);
                    addTlv(p, t, 0);

                    unsigned char *pack = packetToChar(p);
                    send(sock, pack, p->body_length + 4, 0);
                    timer = 0;

                    /*********************************/

                    showTables(cl);
                    showCommands();
                    printf("New message published : %s\n", msg);

                    free(msg);
                    free(p);
                    free(t);
                    free(pack);
                }

                else if (memcmp(command, "!modify", 7) == 0) // !modify id new_msg
                {

                    /** Get inputs of the user **/
                    char *id_str = (char *)calloc(length - 8, 1); // 7 + 1 (space)
                    char *next = memccpy(id_str, command + 8, ' ', length - 8);

                    if (next == NULL)
                    {
                        printf("Error : your command is malformed.\n");
                        break;
                    }

                    uint64_t id = strtoull(id_str, NULL, 10);

                    unsigned char *data = (unsigned char *)calloc(length - 9 - strlen(id_str), 1); // 7 + 2 spaces
                    memcpy(data, command + 8 + strlen(id_str), length - 8 - strlen(id_str));

                    /*****************************/

                    /** Modify data in the table of client **/

                    data_published *d = findIdDataPublished(id, cl);
                    if (d == NULL)
                    {
                        printf("Error : ID not found.\n");
                    }

                    /****************************************/

                    else
                    {
                        d->data = data;

                        /** Create and send the packet **/

                        packet *p = initPacket();
                        tlv *t = initTlv(PUBLISH);
                        Publish(t, cl->timeout, d->id, d->secret, d->data, length - 8 - strlen(id_str));
                        addTlv(p, t, 0);

                        unsigned char *pack = packetToChar(p);
                        send(sock, pack, p->body_length + 4, 0);
                        timer = 0;

                        /*********************************/

                        showTables(cl);
                        showCommands();
                        printf("Message Modified : %s\n", data);

                        free(p);
                        free(t);
                        free(pack);
                    }
                    free(id_str);
                }

                else if (memcmp(command, "!delete", 7) == 0) // !delete id
                {
                    char *msg = (char *)calloc(length - 8, 1); // 7 + 1 (space)
                    memcpy(msg, command + 8, length - 8);

                    uint64_t id = strtoull(msg, NULL, 10);

                    /** Delete data from table (if it exists) **/

                    data_published *del = findIdDataPublished(id, cl);
                    if (del == NULL)
                    {
                        printf("Error : ID not found.\n");
                    }

                    /*****************************************/

                    else
                    {

                        /** Create and send the packet **/

                        packet *p = initPacket();
                        tlv *t = initTlv(PUBLISH);
                        Publish(t, 0, del->id, del->secret, (unsigned char *)"", 0);
                        addTlv(p, t, 0);

                        unsigned char *pack = packetToChar(p);
                        send(sock, pack, p->body_length + 4, 0);
                        timer = 0;

                        /*********************************/

                        deleteDataPublished(del, cl);

                        showTables(cl);
                        showCommands();
                        printf("Message deleted !\n");

                        free(p);
                        free(t);
                        free(pack);
                    }
                    free(msg);
                }

                else
                {
                    printf("Error : Command not found !\n");
                }
            }
            free(command);
        }

        else if (err == 0)
        {
            timer += 8;
            checkTables(cl, sock);
            checkDataPublished(cl, sock);
            checkDataReceived(cl);
            //printf("8s\n");

            timeout.tv_sec = 8;
            timeout.tv_usec = 0;
        }

        else
        {
        }
    }

    /*******************************/
}