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

char *host;
// CHECK LATER THE RANGE VALUES

int checkenv()
{
    host = getenv("IP_TUPLES");

    if (host == NULL)
    {
        perror("\nEnvironment variables not defined\n");
        return -1;
    }
    return 0;
}

int init()
{
    /* Handle the retrieving of the environment variables before creating any client*/
    int environment = checkenv();
    if (environment < 0)
    {
        return -1; 
    }

    CLIENT *clnt;
    enum clnt_stat retval;
    int result;

    clnt = clnt_create(host, KEYS, KEYSVERS, "tcp");
    if (clnt == NULL)
    {
        clnt_pcreateerror(host);
        exit(1);
    }

    retval = init_1(&result, clnt);
    if (retval != RPC_SUCCESS)
    {
        clnt_pcreateerror(host);
    }

    clnt_destroy(clnt);
    return result;
}

int set_value(int key, char *value1, int value2, float value3)
{
    CLIENT *clnt;
    enum clnt_stat retval;
    int result;

    clnt = clnt_create(host, KEYS, KEYSVERS, "tcp");
    if (clnt == NULL)
    {
        clnt_pcreateerror(host);
        exit(1);
    }

    retval = set_value_1(key, value1, value2, value3, &result, clnt);
    if (retval != RPC_SUCCESS)
    {
        clnt_pcreateerror(host);
    }

    clnt_destroy(clnt);
    return result;
}

int get_value(int key, char *value1, int *value2, float *value3)
{
    CLIENT *clnt;
    enum clnt_stat retval;
    response_rpc result;
    int resultcode = 0; // success by default
    result.value1 = (char *)malloc(256);

    clnt = clnt_create(host, KEYS, KEYSVERS, "tcp");
    if (clnt == NULL)
    {
        clnt_pcreateerror(host);
        resultcode = -1;
        exit(1);
    }

    retval = get_value_1(key, &result, clnt);
    if (retval != RPC_SUCCESS)
    {
        clnt_pcreateerror(host);
        resultcode = -1; // Error code
    }

    clnt_destroy(clnt);

    sprintf(value1, "%s", result.value1);
    //strcpy(value1, result.value1);
    *value2 = result.value2;
    *value3 = result.value3;
    free(result.value1);

    return resultcode; // VER QUE DEVOLVER
}

int modify_value(int key, char *value1, int value2, float value3)
{
    CLIENT *clnt;
    enum clnt_stat retval;
    int result;

    clnt = clnt_create(host, KEYS, KEYSVERS, "tcp");
    if (clnt == NULL)
    {
        clnt_pcreateerror(host);
        exit(1);
    }

    retval = modify_value_1(key, value1, value2, value3, &result, clnt);
    if (retval != RPC_SUCCESS)
    {
        clnt_pcreateerror(host);
    }

    clnt_destroy(clnt);
    return result;
}

int delete_key(int key)
{
    CLIENT *clnt;
    enum clnt_stat retval;
    int result;

    clnt = clnt_create(host, KEYS, KEYSVERS, "tcp");
    if (clnt == NULL)
    {
        clnt_pcreateerror(host);
        exit(1);
    }

    retval = delete_key_1(key, &result, clnt);
    if (retval != RPC_SUCCESS)
    {
        clnt_pcreateerror(host);
    }

    clnt_destroy(clnt);
    return result;
}

int exist(int key)
{
    CLIENT *clnt;
    enum clnt_stat retval;
    int result;

    clnt = clnt_create(host, KEYS, KEYSVERS, "tcp");
    if (clnt == NULL)
    {
        clnt_pcreateerror(host);
        exit(1);
    }

    retval = exist_1(key, &result, clnt);
    if (retval != RPC_SUCCESS)
    {
        clnt_pcreateerror(host);
    }

    clnt_destroy(clnt);
    return result;
}

int num_items()
{
    CLIENT *clnt;
    enum clnt_stat retval;
    int result;

    clnt = clnt_create(host, KEYS, KEYSVERS, "tcp");
    if (clnt == NULL)
    {
        clnt_pcreateerror(host);
        exit(1);
    }

    retval = num_items_1(&result, clnt);
    if (retval != RPC_SUCCESS)
    {
        clnt_pcreateerror(host);
    }

    clnt_destroy(clnt);
    return result;
}
