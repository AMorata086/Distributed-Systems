#include <stdio.h>
#include <mqueue.h>
#include <stdlib.h>
#include <stddef.h>
#include "keys.h"
#include <string.h>
#include <pthread.h>

struct request
{
    /*
        Operation codes:

        '1' : init
        '2' : set_value
        '3' : get_value
        '4' : modify_value
        '5' : delete_key
        '6' : exist
        '7' : num_items
    */

    char opcode;
    int key;
    char v1[256];
    int v2;
    float v3;
    char q_name[256]; /* Name of the queue that the client will read from */
};

struct response
{
    int res; /*  */
    char v1[256];
    int v2;
    float v3;
};

/* */
typedef struct request request;
typedef struct response response;

/* Method to send a request and obtain a response, hence the response type */
response send_request(request req)
{

    response res;
    int error;

    mqd_t server_q; /* Server message queue */
    mqd_t client_q; /* Client message queue */

    char queuename[256];
    struct mq_attr attr;

    sprintf(queuename, "/Queue-%d", (int)pthread_self());

    strcpy(req.q_name, queuename);

    attr.mq_maxmsg = 1;
    attr.mq_msgsize = sizeof(response);

    /* Create and open the client queue with 0644 permissions */
    client_q = mq_open(queuename, O_CREAT | O_RDONLY, 0644, &attr);
    if (client_q < 0)
    {
        perror("mq_open on client for client\n");
        printf("%s\n", queuename);
    }

    /* Create and open the server queue */
    server_q = mq_open("/server_q.txt", O_WRONLY);
    if (server_q < 0)
    {
        perror("mq_open on client for server\n");
    }

    /* Error handling when sending a message */
    error = mq_send(server_q, (const char *)&req, sizeof(req), 0);
    if (error < 0)
    {
        perror("Error mq_send");
        exit(-1);
    }
    //printf("CONTROL POINT FOR EXIST 3\n");
    /* Error handling when receiving a message */
    error = mq_receive(client_q, (char *)&res, sizeof(res), 0);
    if (error < 0)
    {
        perror("Error mq_receive");
        exit(-1);
    }
    //printf("CONTROL POINT FOR EXIST 4\n");
    /* Close the queues */
    mq_close(server_q);

    mq_close(client_q);

    mq_unlink(queuename);

    /* Obtain response */
    return res;
}

int init()
{
    /* Initialization of instances of request and response for the structs declared before*/
    struct request req;
    struct response res;

    /* Fill request data */
    req.opcode = '1';

    /* Send the request and return the response */
    res = send_request(req);
    return res.res;
}

int set_value(int key, char *value1, int value2, float value3)
{
    /* Initialization of instances of request and response for the structs declared before*/
    struct request req;
    struct response res;

    /* Fill request data */
    req.opcode = '2';
    req.key = key;
    strcpy(req.v1, value1);
    req.v2 = value2;
    req.v3 = value3;

    /* Send the request and return the response */
    res = send_request(req);
    return res.res;
}

int get_value(int key, char *value1, int *value2, float *value3)
{
    // printf("\nEntrando a get_value() en keys.c...\n");
    // printf("Parametros:\nkey = %d\nvalue1=%s\nvalue2=%d\nvalue3=%f\n", key, value1, *value2, *value3);
    /* Initialization of instances of request and response for the structs declared before*/
    struct request req;
    struct response res;

    /* Fill request data */
    req.opcode = '3';
    req.key = key;

    res = send_request(req);
    sprintf(value1, "%s", res.v1);
    // strcpy(value1, res.v1);
    *value2 = res.v2;
    *value3 = res.v3;

    // printf("\nSaliendo de get_value() en keys.c...\n");
    // printf("Parametros:\nkey = %d\nvalue1=%s\nvalue2=%d\nvalue3=%f\n", key, value1, *value2, *value3);
    return res.res;
}

int modify_value(int key, char *value1, int value2, float value3)
{
    /* Initialization of instances of request and response for the structs declared before*/
    struct request req;
    struct response res;

    /* Fill request data */
    req.opcode = '4';
    req.key = key;
    strcpy(req.v1, value1);
    req.v2 = value2;
    req.v3 = value3;

    /* Send the request and return the response */
    res = send_request(req);
    return res.res;
}

int delete_key(int key)
{
    /* Initialization of instances of request and response for the structs declared before*/
    struct request req;
    struct response res;

    /* Fill request data */
    req.opcode = '5';
    req.key = key;

    /* Send the request and return the response */
    res = send_request(req);
    return res.res;
}

int exist(int key)
{
    /* Initialization of instances of request and response for the structs declared before*/
    struct request req;
    struct response res;

    /* Fill request data */
    req.opcode = '6';
    req.key = key;

    /* Send the request and return the response */
    res = send_request(req);
    return res.res;
}

int num_items()
{
    /* Initialization of instances of request and response for the structs declared before*/
    struct request req;
    struct response res;
    /* Fill request data */
    req.opcode = '7';

    /* Send the request and return the response */
    res = send_request(req);
    return res.res;
}