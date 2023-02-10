#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <dirent.h>
#include <mqueue.h>
#include <fcntl.h>    /* For O_* constants */
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h>    /* Definition of AT_* constants */
#include <unistd.h>
#include <linux/limits.h>
#include <pthread.h>
#include <stdbool.h> //For true and false bools

#define NUM_MSG 10 // Number of messages in the message queue. * threads too
#define MAX_MSG_SIZE 1024

mqd_t server_q; // queue descriptor
int err;
int num_msgs = 0; // number of msgs in the mqueue and/or threads (will be the same)

pthread_mutex_t mx_dir; // Mutex to lock the directory
pthread_mutex_t mx_mq;  // Mutex for the message queue

int message_not_copied = true;
pthread_cond_t mq_full;  // Conditional variable to control if the msg queue is full
pthread_cond_t cond_msg; // Conditional variable to control if the msg has been copied

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

int returnFields(char *msg, int *key, char *value1, int *value2, float *value3)
{
    char *field;
    const char s[2] = "-";

    // get key
    if ((field = strtok(msg, s)) == NULL)
    {
        perror("[ERROR-returnFields]: error when parsing key");
        return -1;
    }
    *key = atoi(field);
    // get value1
    if ((field = strtok(NULL, s)) == NULL)
    {
        perror("[ERROR-returnFields]: error when parsing value1");
        return -1;
    }
    strcpy(value1, field);
    // get value2
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
    char dataToAppend[MAX_MSG_SIZE];
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
    // printf("\nEntrando a get_value() en server.c...\n");
    // printf("Parametros:\nkey = %d\nvalue1=%s\nvalue2=%d\nvalue3=%f\n", key, value1, *value2, *value3);

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

    if (returnFields(buffer, &key, value1, value2, value3) < 0)
    {
        perror("[ERROR-get_value]: error while returning the fields");
        return (-1);
    }

    // printf("\nSaliendo de get_value() en server.c...\n");
    // printf("Parametros:\nkey = %d\nvalue1=%s\nvalue2=%d\nvalue3=%f\n", key, value1, *value2, *value3);

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
Checks if the msg with ID key exists in de DB
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
void deal_request(void *request)
{
    struct request msg; /* Local message */
    mqd_t client_q;     /* Client message queue */
    response res;       /* Response of the operation */
    int result = 0;     /* Result of the operation*/

    /* Fields of the messages that will be inside the requests */
    char value1[256];
    int value2 = 0;
    float value3 = 0;

    // Thread copies the message to a local messsage
    pthread_mutex_lock(&mx_mq);
    msg = (*(struct request *)request);

    // You can wake up the server
    message_not_copied = false;
    pthread_cond_signal(&cond_msg);
    // Unlock the msg queue
    pthread_mutex_unlock(&mx_mq);

    // lock the thread to process the request (we block operations on the directory, i.e, our database)
    pthread_mutex_lock(&mx_dir);

    /* Deals with the request depending on the operation code and prepare the response*/
    switch (msg.opcode)
    {
    case '1':
        result = init();
        res.res = result;
        break;
    case '2':
        result = set_value(msg.key, msg.v1, msg.v2, msg.v3);
        res.res = result;
        printf("code: %d\n", res.res);
        break;
    case '3':
        result = get_value(msg.key, value1, &value2, &value3);
        strcpy(res.v1, value1);
        res.v2 = value2;
        res.v3 = value3;
        res.res = result;
        break;
    case '4':
        result = modify_value(msg.key, msg.v1, msg.v2, msg.v3);
        res.res = result;
        break;
    case '5':
        result = delete_key(msg.key);
        res.res = result;
        break;
    case '6':
        result = exist(msg.key);
        res.res = result;
        break;
    case '7':
        result = num_items();
        res.res = result;
        break;
    }
    pthread_mutex_unlock(&mx_dir);

    /* We send back the result to the client, for
    which we send the result to her queue */
    client_q = mq_open(msg.q_name, O_WRONLY);
    if (client_q == -1)
    {
        perror("Client's queue could not be opened");
        // PROBAR SI ESTO HAY QUE QUITARLO O NO (LO DEL SERVER)
        mq_close(server_q);
        mq_unlink("/server_q.txt");
    }
    else
    {
        if (mq_send(client_q, (const char *)&result, sizeof(int), 0) < 0)
        {
            perror("mq_send");
            // PROBAR SI ESTO HAY QUE QUITARLO O NO (LO DEL SERVER)
            mq_close(server_q);
            mq_unlink("/server_q.txt");
            mq_close(client_q);
        }
    }
    num_msgs--;
    pthread_exit(0);
}

int main(int argc, char *argv[])
{

    struct request req;
    struct mq_attr attr;          // Mqueue attributes
    pthread_attr_t t_attr;        // Threads' attribute
    pthread_t thid /*[NUM_MSG]*/; // Creation of threads
    attr.mq_maxmsg = NUM_MSG;
    attr.mq_msgsize = sizeof(struct request);

    // Initialization of mutex and conditional variables
    pthread_mutex_init(&mx_dir, NULL);
    pthread_mutex_init(&mx_mq, NULL);
    pthread_attr_init(&t_attr);
    // Set the threads as independent
    pthread_attr_setdetachstate(&t_attr, PTHREAD_CREATE_DETACHED);

    // Open the queue or create it
    if ((server_q = mq_open("/server_q.txt", O_RDONLY | O_CREAT, 0777, &attr)) < 0)
    {
        perror("[ERROR-MQ]: Error in mq_open");
        return (-1);
    }

    /* Main loop of the server for request management */
    while (1)
    {
        if (mq_receive(server_q, (char *)&req, sizeof(req), 0) < 0)
        {
            perror("[ERROR-MQ]: Error in mq_send");
            mq_close(server_q);
            return -1;
        }
        if (num_msgs < NUM_MSG)
        {
            if (pthread_create(&thid, &t_attr, (void *)deal_request, (void *)&req) == 0)
            {
                num_msgs++; // increment the number of threads created to stop receiving new requests if the limit was reached
                // Wait till the thread copies the message
                pthread_mutex_lock(&mx_mq);
                while (message_not_copied)
                {
                    pthread_cond_wait(&cond_msg, &mx_mq);
                }
                message_not_copied = true;
                pthread_mutex_unlock(&mx_mq);
            }
        }
        else
        {
            fprintf(stderr, "[ERROR-main]: cannot create a new thread: limit of concurrent threads reached");
        }
    }

    // Closing the queue. PODRIA NO HACER FALTA. ver luego

    return 0;
}
