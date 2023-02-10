#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>    /* For O_* constants */
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h>    /* Definition of AT_* constants */
#include <unistd.h>
#include <linux/limits.h>
#include <pthread.h>
#include <stdbool.h> //For true and false bools

#define MAX_THREADS 10

char PORT[5];

int num_conns = 0; /* Number of current connections*/

socklen_t ssize;
int sock_d; /* Server-side socket for data */
int sock_c; /* TCP socket for connections establishment */
int err;

int num_th = 0; // number of threads

pthread_mutex_t mx_dir; // Mutex to lock the directory

int message_not_copied = true;

// SIGNAL VARIABLES:

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

/*************************************************/
/* CODE IN CHARGE OF MANAGING THE DATA STRUCTURE */
/*************************************************/
// KEY FORMAT  <key-val1-val2-val3>

/*** AUXILIARY METHODS ***/
/*
Removes a directory and its contents
*/
int remove_directory(const char *path)
{
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int r = -1;

    if (d)
    {
        struct dirent *p;

        r = 0;
        while (!r && (p = readdir(d)))
        {
            int r2 = -1;
            char *buf;
            size_t len;

            /* Skip the names "." and ".." as we don't want to recurse on them. */
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
                continue;

            len = path_len + strlen(p->d_name) + 2;
            buf = malloc(len);

            if (buf)
            {
                struct stat statbuf;

                snprintf(buf, len, "%s/%s", path, p->d_name);
                if (!stat(buf, &statbuf))
                {
                    if (S_ISDIR(statbuf.st_mode))
                        r2 = remove_directory(buf);
                    else
                        r2 = unlink(buf);
                }
                free(buf);
            }
            r = r2;
        }
        closedir(d);
    }

    if (!r)
        r = rmdir(path);

    return r;
}

/*
Returns the concatenation of the different elements of the message into a single string (result)
*/
int toString(char result[], int key, char *value1, int value2, float value3)
{
    if (sprintf(result, "%d-%s-%d-%f", key, value1, value2, value3) < 0)
    {
        perror("[ERROR-toString]: Error while concatenating the message fields");
        return -1;
    }
    return 0;
}

/*
Returns the fields of a message separately
*/
int returnFields(char *msg, int key, char *value1, int *value2, float *value3)
{
    char *field;
    const char s[2] = "-";

    // get key
    if ((field = strtok(msg, s)) == NULL)
    {
        perror("[ERROR-returnFields]: error when parsing key");
        return -1;
    }
    if (atoi(field) != key)
    {
        perror("[ERROR-returnFields]: key value is not equal to the one contained in the string");
        return -1;
    }
    // key = atoi(field);
    // get value1
    if ((field = strtok(NULL, s)) == NULL)
    {
        perror("[ERROR-returnFields]: error when parsing value1");
        return -1;
    }
    sprintf(value1, "%s", field);
    // strcpy(value1, field);
    //  get value2
    if ((field = strtok(NULL, s)) == NULL)
    {
        perror("[ERROR-returnFields]: error when parsing value2");
        return -1;
    }
    *value2 = atoi(field);
    // get value3
    if ((field = strtok(NULL, s)) == NULL)
    {
        perror("[ERROR-returnFields]: error when parsing value3");
        return -1;
    }
    *value3 = atof(field);

    return 0;
}

/*** SERVER-SIDE METHODS TO MANAGE DATA STRUCTURES (keys.h methods) ***/

/*
Initializes the database ("messages" directory) that will store the different messages.
For that it deletes the folder with everything and creates it again.
*/
int init()
{
    /* Removes the current directory with all the messages stored in it (database) */
    if ((remove_directory("./messages") == -1))
    {
        perror("[ERROR-init]: there was an error when removing the directory");
        return -1;
    }
    /* We create it again, basically we are reseting the directory */
    struct stat st = {0};
    if (stat("./messages", &st) == -1)
    {
        mkdir("./messages", 0777);
    }

    return 0;
}

/*
With this function, we will append to a new created file the message of the queue. There will be a file per message.
(our Database System)
*/
int set_value(int key, char *value1, int value2, float value3)
{
    /* File pointer to hold reference of input file */
    FILE *fPtr;
    char filePath[PATH_MAX];

    /* To be safe, since message is 256 characters max, adding possible float value, we suppose the max number of characters is 512*/
    char dataToAppend[512];

    if ((toString(dataToAppend, key, value1, value2, value3)) < 0)
    {
        perror("[ERROR set_value]: there was an error in toString()");
        return -1;
    }

    /* We get the first element of the message (the key) with strtok.*/
    char token[20];

    /* get the first token */
    sprintf(token, "%d", key);

    /* Declaration of the file name. There will be a file for each message of the queue and
    its name will be that of its key (input by client)*/
    sprintf(filePath, "./messages/%s.txt", token);

    if ((fPtr = fopen(filePath, "r")) != NULL)
    {
        fprintf(stderr, "[ERROR-set_value]: the file '%s' already exists", filePath);
        return -1;
        fclose(fPtr);
    }

    /*  Creates an empty file for reading and writing */
    fPtr = fopen(filePath, "w+");

    /* fopen() return NULL if unable to open file in given mode. */
    if (fPtr == NULL)
    {
        /* Unable to open file hence exit */
        fprintf(stderr, "[ERROR-set_value]: Unable to open the file '%s'.\n", filePath);
        return (-1);
    }
    /* Append data to file */
    if (fputs(dataToAppend, fPtr) == EOF)
    {
        perror("[ERROR-set_value]: error when writing content to file");
    }

    fclose(fPtr); // close the file stream

    return 0;
}

/*
This method will search in the database a message with the ID key and return the different fields
*/
int get_value(int key, char *value1, int *value2, float *value3)
{
    // fprintf(stderr, "\nEntrando a get_value() en server.c...\n");
    // fprintf(stderr, "Parametros:\nkey= %d\nvalue1= %s\nvalue2= %d\nvalue3= %f\n", key, value1, *value2, *value3);

    char buffer[512];
    char filePath[PATH_MAX];
    FILE *fPtr;

    // Arrange the file path to access the file that is storing the message
    sprintf(filePath, "./messages/%d.txt", key);

    fPtr = fopen(filePath, "r");
    if (fPtr == NULL)
    {
        fprintf(stderr, "[ERROR-get_value]: Unable to open the file '%s'.\n", filePath);
        return (-1);
    }

    if (fgets(buffer, 512, fPtr) == NULL)
    {
        perror("[ERROR-get_value]: there was an error reading the message from the file");
    }

    if (returnFields(buffer, key, value1, value2, value3) < 0)
    {
        perror("[ERROR-get_value]: error while returning the fields");
        return (-1);
    }

    // fprintf(stderr, "\nSaliendo de get_value() en server.c...\n");
    // fprintf(stderr, "Parametros:\nkey= %d\nvalue1= %s\nvalue2= %d\nvalue3= %f\n", key, value1, *value2, *value3);

    fclose(fPtr); // close the file stream
    return 0;
}

/*
Modifies the message of a file with the ID key
*/
int modify_value(int key, char *value1, int value2, float value3)
{
    char filePath[PATH_MAX];
    FILE *fPtr;

    sprintf(filePath, "./messages/%d.txt", key);

    // Open the file in read-only mode to figure out if it exists
    fPtr = fopen(filePath, "r");
    if (fPtr == NULL)
    {
        fprintf(stderr, "[ERROR-modify_value]: Unable to open the file '%s'.\n", filePath);
        return (-1);
    }
    // Reopen it in write-only mode to write the new contents
    fPtr = freopen(NULL, "w", fPtr);
    if (fPtr == NULL)
    {
        fprintf(stderr, "[ERROR-modify_value]: error when opening file '%s' in write mode.\n", filePath);
        return (-1);
    }
    char outputBuff[512];
    if (toString(outputBuff, key, value1, value2, value3) < 0)
    {
        perror("[ERROR-modify_value]: error when modifying the message");
        return -1;
    }

    fclose(fPtr); // close the file stream
    return 0;
}

/*
Deletes a file msg with the ID key
*/
int delete_key(int key)
{
    char filePath[PATH_MAX];

    sprintf(filePath, "./messages/%d.txt", key);

    if (remove(filePath) < 0)
    {
        perror("[ERROR-delete_key]: there was an error when deleting the file");
        return -1;
    }

    return 0;
}

/*
Checks if the msg with ID key exists in the DB
*/
int exist(int key)
{
    char filePath[PATH_MAX];
    FILE *fPtr;

    sprintf(filePath, "./messages/%d.txt", key);

    // Open the file in read-only mode to figure out if it exists
    fPtr = fopen(filePath, "r");
    if (fPtr == NULL)
    {
        fclose(fPtr);
        return 0; // msg does not exists
    }
    fclose(fPtr);
    return 1; // msg exists
}

/*
Checks the number of messages in the database
*/
int num_items()
{
    char filePath[PATH_MAX] = "./messages";
    DIR *dirp;
    struct dirent *dp;
    int items = 0;

    // Open the directory
    dirp = opendir(filePath);
    if (NULL == dirp)
    {
        perror("[ERROR-num_items]: there was an error when opening the directory");
        return -1;
    }
    // Traverse the directory in a loop till we reach the value NULL
    while (NULL != (dp = readdir(dirp)))
    {
        // Check if it is a regular file
        if (dp->d_type == 8) // For some reason, DT_REG constant of dirent.h does not work, so we use its assigned value instead
        {
            items += 1;
        }
    }
    // Close the directory stream
    closedir(dirp);
    return items;
}

/*
This method will process a request received by by the message queue
*/
void deal_request(void *socket)
{
    int sock_c = *((int *)socket); // casting?
    response res;                  /* Response of the operation */
    int result = 0;                /* Result of the operation*/
    struct request req;            /* Local request */

    /* Fields of the messages that will be inside the requests */
    char value1[256];
    int value2 = 0;
    float value3 = 0;

    // Read request from client
    result = read(sock_c, &req, sizeof(req));

    // lock the thread to process the request (we block operations on the directory, i.e, our database)
    pthread_mutex_lock(&mx_dir);

    /* Deals with the request depending on the operation code and prepare the response*/
    switch (req.opcode)
    {
    case '1':
        result = init();
        res.res = result;
        break;
    case '2':
        result = set_value(req.key, req.v1, req.v2, req.v3);
        res.res = result;
        // printf("code: %d\n", res.res);
        break;
    case '3':
        // fprintf(stderr, "ENTRANDO EN DEAL REQUEST PARA GET_VALUE");
        result = get_value(req.key, value1, &value2, &value3);
        strcpy(res.v1, value1);
        res.v2 = value2;
        res.v3 = value3;
        res.res = result;
        // fprintf(stderr, "\nres.v1=%s ;; value1=%s\n ;; res.v2=%d\n ;; value2=%d\n", res.v1, value1, res.v2, value2);
        break;
    case '4':
        result = modify_value(req.key, req.v1, req.v2, req.v3);
        res.res = result;
        break;
    case '5':
        result = delete_key(req.key);
        res.res = result;
        break;
    case '6':
        result = exist(req.key);
        res.res = result;
        break;
    case '7':
        result = num_items();
        res.res = result;
        break;
    }
    // Unlocking of the mutex since we already performed the operation in the database (directory)
    pthread_mutex_unlock(&mx_dir);

    /* We send back the result to the client, for
    which we write the result to his socket's file descriptor */

    result = write(sock_c, (char *)&res, sizeof(res));

    if (result < 0)
    {
        perror("Failure in writing");
    }

    result = close(sock_c); // rename of the varible
    if (result < 0)
    {
        perror("Failure in socket closing");
    }

    num_conns--; /* Once the request has been dealt with, there's a decrement on the number of current connections */
}

/***********************************/
/************ MAIN LOOP ************/
/***********************************/
int main(int argc, char *argv[])
{
    // Check if the program was called correctly
    if (argc != 2)
    {
        perror("[SERVER ERROR]: Invalid number of arguments");
        fprintf(stderr, "[SERVER]: call procedure: ./server <PORT NUMBER>\n");
        return -1;
    }
    else
    {
        // Assignation of the port given as input to our local variable
        sprintf(PORT, "%s", argv[1]);
    }

    struct sockaddr_in server_addr, client_addr; // Structs holding the client and server address information

    pthread_attr_t t_attr;       // Threads' attributes
    pthread_t thid[MAX_THREADS]; // Creation of threads
    char sock_content[512];      // Buffer to send & receive content through socket
    int val;

    sock_d = socket(PF_INET, SOCK_STREAM, 0);
    if (sock_d < 0)
    {
        perror("[SERVER ERROR]: Error in socket creation");
        return -1;
    }
    val = 1;
    /* SOCKET OPTIONS */
    if (setsockopt(sock_d, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof(int)) < 0)
    {
        perror("[SERVER ERROR]: error while setting the socket options");
        return -1;
    }

    /* Assignation of IP and PORT*/
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(PORT));

    /* Connecting server socket to the IP */
    if (bind(sock_d, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("[SERVER ERROR]: error while binding the socket");
        return -1;
    }

    /* Listening for requests */
    if (listen(sock_d, SOMAXCONN) < 0)
    {
        perror("[SERVER ERROR]: error while listening to the port");
        return -1;
    }

    ssize = sizeof(client_addr);

    // Initialization of mutex and conditional variables
    pthread_mutex_init(&mx_dir, NULL);
    pthread_attr_init(&t_attr);
    // Set the threads as independent
    pthread_attr_setdetachstate(&t_attr, PTHREAD_CREATE_DETACHED);

    /* Main loop of the server for request management */
    int i = 0;
    while (1)
    {
        fprintf(stdout, "Waiting for connection...\n");
        sock_c = accept(sock_d, (struct sockaddr *)&client_addr, (socklen_t *)&ssize);

        if (sock_c < 0)
        {
            perror("[SERVER ERROR]: Error while accepting the connection");
            return -1;
        }

        printf("Accepted connection - %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        if (num_conns < SOMAXCONN)
        {
            if (pthread_create(&thid[i++], &t_attr, (void *)deal_request, (void *)&sock_c) < 0)
            {
                perror("[SERVER ERROR]: Error while creating a new thread");
                return -1;
            }
            num_conns++; // Increment the number of concurrent connections (threads) to stop receiving new requests if limit was reached

            if (i >= MAX_THREADS)
            {
                i = 0;
                while (i < MAX_THREADS)
                {
                    pthread_join(thid[i++], NULL);
                }
                i = 0;
            }
        }
    }
    // Closing the connection

    pthread_mutex_destroy(&mx_dir);

    if (close(sock_d < 0))
    {
        perror("Connection closing failed");
    }

    return 0;
}
