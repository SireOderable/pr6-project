#include "ui.h"


/**
 * Print data_published_table and data_received_table of cl
 * 
 * @param cl : client
 * */
void showTables(client *cl)
{

    printf("\033c");

    printf("+------------------------------------------------------------+\n");
    printf("|                       Data Published                       |\n");
    printf("+------------------------------------------------------------+\n");
    printf("|  id  / expiration_date / refresh_date /  secret  /  data   |\n");
    printf("+------------------------------------------------------------+\n");

    for (data_published *d = cl->data_published_table; d != NULL; d = d->next)
        printf("  %lu  /  %ld  /  %ld  /  %ld  / %s \n", d->id, d->expiration_date, d->refresh_date, d->secret, d->data);

    printf("\n\n");

    printf("+------------------------------------------------------------+\n");
    printf("|                       Data Received                        |\n");
    printf("+------------------------------------------------------------+\n");
    printf("|        id        /    expiration_date     /      data      |\n");
    printf("+------------------------------------------------------------+\n");
    printf("\n");

    for (data_received *d = cl->data_received_table; d != NULL; d = d->next)
        printf("  %lu  /  %ld  /  %s  \n", d->id, d->expiration_date, d->data);
        
    printf("\n\n");
}



/**
 * Print the different commands to control the client
 * */
void showCommands()
{
    printf("Publish a new message : \"!publish new_message\"\n");
    printf("Modify an existing message : \"!modify id new_message\"\n");
    printf("Delete a published message : \"!delete id\"\n\n");
    printf("Shut down the client : \"!quit\"\n\n");
}