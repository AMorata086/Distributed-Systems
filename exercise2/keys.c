#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stddef.h>
#include "keys.h"
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

char IP_TUPLES[15];  // An IP should have max 15 characters e.g: 255.255.255.255
char PORT_TUPLES[5]; // Port range goes up to 5 digits. (Max 65535)

// CHECK LATER THE RANGE VALUES

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
};
typedef struct request request;

struct response
{
    int res; /*  */
    char v1[256];
    int v2;
    float v3;
};
typedef struct response response;

int checkenv()
{
    sprintf(IP_TUPLES, "%s", getenv("IP_TUPLES"));
    sprintf(PORT_TUPLES, "%s", getenv("PORT_TUPLES"));

    // printf("IP_TUPLES=%s\n",IP_TUPLES);
    // printf("PORT_TUPLES=%s\n",PORT_TUPLES);

    if (IP_TUPLES == NULL || PORT_TUPLES == NULL)
    {
        perror("\nEnvironment variables not defined\n");
        return -1;
    }
    return 0;
}

/* Method to send a request and obtain a response, hence the response type */
response send_request(request req)
{
    checkenv();
    response res;

    int ret;
    int sd_server;
    struct sockaddr_in address;

    // Opening of server socket
    sd_server = socket(AF_INET, SOCK_STREAM, 0);
    if (sd_server <= 0)
    {
        perror("Socket error");
        exit(-1);
    }

    // Connection to the port
    address.sin_family = AF_INET;
    address.sin_port = htons(atoi(PORT_TUPLES));

    ret = inet_pton(AF_INET, IP_TUPLES, &address.sin_addr);
    if (ret <= 0)
    {
        perror("Invalid address");
        close(sd_server);
        exit(-1);
    }
    ret = connect(sd_server, (struct sockaddr *)&address, sizeof(address));
    if (ret < 0)
    {
        perror("Connection failed");
        close(sd_server);
        exit(-1);
    }

    // Sending a request
    ret = write(sd_server, (char *)&req, sizeof(request));
    if (ret < 0)
    {
        perror("Wrong syntaxis");
        close(sd_server);
        exit(-1);
    }

    // Receiving a response
    ret = read(sd_server, (char *)&res, sizeof(response));
    if (ret < 0)
    {
        perror("Response failed");
        close(sd_server);
        exit(-1);
    }

    // Socket closing
    ret = close(sd_server);
    if (ret < 0)
    {
        perror("Socket closing failed");
        exit(-1);
    }

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
    // fprintf(stderr, "\nEntrando a get_value() en keys.c...\n");
    // fprintf(stderr, "Parametros:\nkey = %d\nvalue1=%s\nvalue2=%d\nvalue3=%f\n", key, value1, *value2, *value3);
    /* Initialization of instances of request and response for the structs declared before*/
    struct request req;
    struct response res;

    /* Fill request data */
    req.opcode = '3';
    req.key = key;

    res = send_request(req);
    // fprintf(stderr, "\nvalor de res.v1: '%s'\n", res.v1);
    sprintf(value1, "%s", res.v1);
    // strcpy(value1, res.v1);
    *value2 = res.v2;
    *value3 = res.v3;

    // fprintf(stderr, "\nSaliendo de get_value() en keys.c...\n");
    // fprintf(stderr, "Parametros:\nkey = %d\nvalue1=%s\nvalue2=%d\nvalue3=%f\n", key, value1, *value2, *value3);
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
