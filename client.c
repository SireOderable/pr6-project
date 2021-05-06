#include "client.h"

/**
 * Initialize a client
 * 
 * @param timeout : expiration date of a packet
 * 
 * @return new client
 * */
client *initClient(int timeout)
{
    client *cl = (client *)malloc(sizeof(client));

    cl->data_received_table = NULL;
    cl->data_published_table = NULL;
    cl->timeout = timeout;
    cl->synchronized = 0;

    return cl;
}



/**
 * Initialize a new data_published
 * 
 * @param id
 * @param secret
 * @param timeout
 * @param data
 * @param length : length of data
 * 
 * @return a new data_published initialized and completed
 * */
data_published * initDataPublished(uint64_t id, uint64_t secret, time_t timeout, unsigned char * d, int length)
{
    data_published * new = (data_published *) malloc(sizeof(data_published));

    new->id = id;
    new->secret = secret;
    new->expiration_date = time(NULL) + timeout;
    new->refresh_date = time(NULL) + ((3 * timeout) / 4);
    new->data_length = length;

    unsigned char * data = (unsigned char *) calloc(length, 1);
    memcpy(data, d, length);
    new->data = data;

    new->next = NULL;
    new->previous = NULL;

    return new;
}



/**
 * Initialize a new data_received
 * 
 * @param id
 * @param secret
 * @param timeout
 * @param data
 * @param length : length of data
 * 
 * @return a new data_published initialized and completed
 * */
data_received * initDataReceived(uint64_t id, time_t timeout, unsigned char * d, int length)
{
    data_received * new = (data_received *) malloc(sizeof(data_received));

    new->id = id;
    new->expiration_date = time(NULL) + timeout;
    new->data_length = length;

    unsigned char * data = (unsigned char *) calloc(length, 1);
    memcpy(data, d, length);
    new->data = data;

    new->next = NULL;
    new->previous = NULL;

    return new;
}




/**
 * Initialize and return a socket by reading the configuration file params
 * */
void getParams(int * params, char * address)
{
    FILE * file = fopen("params.config", "r");

    char *line = NULL;
    size_t n = 0;

    char *port_c, *expiration_date_c;

    int size = getline(&line, &n, file);
    while (size != -1)
    {
        if (memcmp(line, "ipv4", (size_t) 4) == 0)
        {
            if ((line[7] - '0') == 1)
            {
                params[0] = PF_INET;
            }

            else
            {
                params[0] = PF_INET6;
            }

            //printf("ipv4 ? : %d\n", params[0]);
        }

        else if (memcmp(line, "server_address", (size_t) 14) == 0)
        {
            memcpy(address, line + 17, size - 18);

            //printf("adresse : %s, taille : %d\n", address, size - 18);
        }

        else if (memcmp(line, "port", (size_t) 4) == 0)
        {
            port_c = (char *)malloc(size - 7);
            memcpy(port_c, line + 7, size - 7);
            params[1] = atoi(port_c);

            //printf("port : %d\n", params[1]);
        }

        else if (memcmp(line, "data_expiration", (size_t) 15) == 0)
        {
            expiration_date_c = (char *)malloc(size - 18);
            memcpy(expiration_date_c, line + 18, size - 18);
            params[2] = atoi(expiration_date_c);

            //printf("timeout : %d\n", params[2]);
        }

        size = getline(&line, &n, file);
    }
}

/**
 * Try to find data with represented by id in data_received_table of client.
 * 
 * @param id
 * @param cl
 * 
 * @return data_received represented by id and NULL if no data_received exists with this id.
 * */
data_received *findIdDataReceived(uint64_t id, client *cl)
{
    for (data_received *d = cl->data_received_table; d != NULL; d = d->next)
    {
        if (d->id == id)
            return d;
    }

    return NULL;
}

/**
 * Add data to data_received_table of client
 * 
 * @param data : data to add
 * @param cl : client
 * */
void addDataReceived(data_received *data, client *cl)
{
    if(cl->data_received_table != NULL)
    {
        data->next = cl->data_received_table;
        cl->data_received_table->previous = data;
    }
    cl->data_received_table = data;
}

/**
 * Delete data from data_received_table of client
 * 
 * @param data : data to delete
 * @param cl : client
 * */
void deleteDataReceived(data_received *data, client *cl)
{
    if(data == cl->data_received_table)
    {
        cl->data_received_table = data->next;
        if(cl->data_received_table != NULL)
            cl->data_received_table->previous = NULL;
    }

    else if(data->next == NULL)
    {
        data->previous->next = NULL;
    }

    else
    {
        data->previous->next = data->next;
        data->next->previous = data->previous;    
    } 
    free(data);
}

/**
 * Try to find data with represented by id in data_received_table of client.
 * 
 * @param id
 * @param cl
 * 
 * @return data_received represented by id and NULL if no data_received exists with this id.
 * */
data_published *findIdDataPublished(uint64_t id, client *cl)
{
    
    for (data_published *d = cl->data_published_table; d != NULL; d = d->next)
    {
        if (d->id == id)
            return d;
    }

    return NULL;
}


/**
 * Check data received and delete those which are out of date
 * 
 * @param cl : client
 * */
void checkDataReceived(client * cl)
{
    for(data_received * data = cl->data_received_table; data != NULL; data = data->next)
    {
        if(difftime(data->expiration_date, time(NULL)) < 0)
            deleteDataReceived(data, cl);
    }
}


/**
 * Add data to data_published_table of client
 * 
 * @param data : data to add
 * @param cl : client
 * */
void addDataPublished(data_published *data, client *cl)
{
    if(cl->data_published_table != NULL)
    {
        data->next = cl->data_published_table;
        cl->data_published_table->previous = data;
    }
    cl->data_published_table = data;
}

/**
 * Delete data from data_published_table of client
 * 
 * @param data : data to delete
 * @param cl : client
 * */
void deleteDataPublished(data_published *data, client *cl)
{
    if(data == cl->data_published_table)
    {
        cl->data_published_table = data->next;
        if(cl->data_received_table != NULL)
            cl->data_received_table->previous = NULL;
    }

    else if(data->next == NULL)
    {
        data->previous->next = NULL;
    }

    else
    {
        data->previous->next = data->next;
        data->next->previous = data->previous;    
    } 
    free(data);
}


/**
 * Generate a random Id
 * */
uint64_t randId()
{
    return rand();
}



/**
 * Reverse the endianness of num (BE -> LE or LE -> BE)
 * 
 * @param num
 * 
 * @return num reversed
 * */
uint64_t convert (uint64_t num){

    uint64_t b0,b1,b2,b3,b4,b5,b6,b7;
    uint64_t res = 0;

    b0 = (num & 0x00000000000000ff) << 56;
    b1 = (num & 0x000000000000ff00) << 40;
    b2 = (num & 0x0000000000ff0000) << 24;
    b3 = (num & 0x00000000ff000000) << 8;

    b4 = (num & 0x000000ff00000000) >> 8;
    b5 = (num & 0x0000ff0000000000) >> 24;
    b6 = (num & 0x00ff000000000000) >> 40;
    b7 = (num & 0xff00000000000000) >> 56;

    res = b0 | b1 | b2 | b3 | b4 | b5 | b6 | b7;

    return res;
}