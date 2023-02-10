#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>    /* For O_* constants */
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h>    /* Definition of AT_* constants */
#include <pthread.h>
#include <dirent.h>
#include <linux/limits.h>
#include <string.h>
#include <strings.h>

#define MAX_THREADS 10

#define MAX_LINE 1024

char PORT[5];
int err;

// SOCKET VARIABLES
socklen_t ssize;
int sock_d; /* Server-side socket for data */
int sock_c; /* TCP socket for connections establishment */

// THREADING VARIABLES
int num_th = 0;                                 // number of threads
pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER; // Mutex to protect critical section
int num_conns = 0;                              /* Number of current connections*/

/*************************************************/
/* CODE IN CHARGE OF MANAGING THE DATA STRUCTURE */
/*************************************************/
/* User */
// username|status|IP|Port|Pending messages (to receive)|Identifier of the last message received
/* Message */
// 0|Content of the message <max 255 chars>|usernameSender

/*************************/
/*** AUXILIARY METHODS ***/
/*************************/

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
Concatenates the string that will be stored into a user file
*/
int toUsrStr(char buffer[], char *username, char *status, char *IP, char *port, char *pendingMsgs, char *lastMsgRcvd)
{
    if (sprintf(buffer, "%s|%s|%s|%s|%s|%s", username, status, IP, port, pendingMsgs, lastMsgRcvd) < 0)
    {
        perror("Error in toUsrStr");
        return -1;
    }
    return 0;
}
/*
Concatenates the string that will be stored into a msg file
*/
int toMsgStr(char buffer[], int msgId, char *msg, char *sender)
{
    if (sprintf(buffer, "%d|%s|%s", msgId, msg, sender) < 0)
    {
        perror("Error in toMsgStr");
        return -1;
    }
    return 0;
}
/*
returns the user fields individually from the buffer
*/
int return_user_fields(char *buffer, char *username, char *status, char *ip, char *port, char *pending, char *last_rcvd)
{
    char *field;
    const char s[2] = "|";
    // get username
    if ((field = strtok(buffer, s)) == NULL)
    {
        perror("Error return_user_fields fetching username");
        return -1;
    }
    // sprintf(username, "%s", field);
    //  get status
    if ((field = strtok(NULL, s)) == NULL)
    {
        perror("Error return_user_fields fetching status");
        return -1;
    }
    sprintf(status, "%s", field);
    // get IP
    if ((field = strtok(NULL, s)) == NULL)
    {
        perror("Error return_user_fields fetching IP");
        return -1;
    }
    sprintf(ip, "%s", field);
    // get port
    if ((field = strtok(NULL, s)) == NULL)
    {
        perror("Error return_user_fields fetching port");
        return -1;
    }
    sprintf(port, "%s", field);
    // get pending messages
    if ((field = strtok(NULL, s)) == NULL)
    {
        perror("Error return_user_fields fetching pendign msgs");
        return -1;
    }
    sprintf(pending, "%s", field);
    // get last message received
    if ((field = strtok(NULL, s)) == NULL)
    {
        perror("Error return_user_fields fetching last msg received");
        return -1;
    }
    sprintf(last_rcvd, "%s", field);

    return 0;
}
/*
returns the msg fields individually from the buffer
*/
int return_msg_fields(char *buffer, int *msgID, char *msg, char *sender)
{
    char *field;
    const char s[2] = "|";
    // get msg ID
    if ((field = strtok(buffer, s)) == NULL)
    {
        perror("Error return_msg_fields");
        return -1;
    }
    //*msgID = atoi(buffer);
    // get msg contents
    if ((field = strtok(NULL, s)) == NULL)
    {
        perror("Error return_msg_fields");
        return -1;
    }
    sprintf(msg, "%s", field);
    // get sender
    if ((field = strtok(NULL, s)) == NULL)
    {
        perror("Error return_msg_fields");
        return -1;
    }
    sprintf(sender, "%s", field);

    return 0;
}

/*****************************************************/
/*** SERVER-SIDE METHODS TO MANAGE DATA STRUCTURES ***/
/*****************************************************/

/*
Initializes the database (directories) that will store the different messages and users.
For that it deletes the folders with everything and creates them again.
*/
int init()
{
    /* Removes the directories ./messages and ./users */
    if ((remove_directory("./messages") == -1))
    {
        perror("[ERROR-init]: there was an error when removing the directory");
        return -1;
    }
    if ((remove_directory("./users") == -1))
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
    if (stat("./users", &st) == -1)
    {
        mkdir("./users", 0777);
    }

    return 0;
}

/* Registers the user into the DB by creating its entry
    Return codes:
        0 -- OK
        1 -- User already registered
        2 -- Any other error
*/
int register_user(char username[])
{
    FILE *fPtr;
    char filePath[PATH_MAX];

    /* Parameters for the file */
    //@arg username
    char status[] = "Off";
    // Use the character "?" as a placeholder when there is no data in that field
    char IP[] = "?";
    char Port[] = "?";
    char pending[] = "?";
    char lastRcvd[] = "?";

    sprintf(filePath, "./users/%s", username);

    fPtr = fopen(filePath, "r");
    if (fPtr != NULL)
    {
        // username already registered in DB
        fprintf(stderr, "REGISTER %s FAIL\n", username);
        fclose(fPtr);
        return 1;
    }

    fPtr = fopen(filePath, "w+");
    if (fPtr == NULL)
    {
        fprintf(stderr, "Error while creating the user file");
        fclose(fPtr);
        return 2;
    }

    char buffer[500];
    toUsrStr(buffer, username, status, IP, Port, pending, lastRcvd);

    if (fputs(buffer, fPtr) == EOF)
    {
        perror("Error while writing contents to file");
        fclose(fPtr);
        return 2;
    }

    fclose(fPtr);
    fprintf(stderr, "REGISTER %s OK\n", username);
    return 0;
}

/* Unregisters the user from the DB by removing its file
    Return codes:
        0 -- OK
        1 -- User does not exist
        2 -- Any other error
*/
int unregister_user(char username[])
{
    char filePath[PATH_MAX];

    sprintf(filePath, "./users/%s", username);

    if (remove(filePath) < 0)
    {
        fprintf(stderr, "UNREGISTER %s FAIL\n", username);
        return 1;
    }
    fprintf(stderr, "UNREGISTER %s OK\n", username);
    return 0;
}

/* Connects a user to the service to receive messages
    Return codes
        0 -- OK
        1 -- User does not exist
        2 -- User is already connected
        3 -- Any other error
*/
int connect_user(char username[], char ip_user[], char port[])
{
    char buffer[512];
    char filePath[PATH_MAX];
    FILE *fPtr;

    char conn_ip[16];
    char conn_port[6];
    char status[4];
    char pending_msgs[5000];
    char last_msg_rcvd[2];

    // Arrange the file path to access the file that is storing the message
    sprintf(filePath, "./users/%s", username);

    fPtr = fopen(filePath, "r");
    if (fPtr == NULL)
    {
        fprintf(stderr, "[connect_user error]: Unable to open the file '%s'.\n", filePath);
        fprintf(stderr, "CONNECT %s FAIL\n", username);
        fclose(fPtr);
        return (1);
    }
    if (fgets(buffer, 512, fPtr) == NULL)
    {
        perror("[connect_user error]: there was an error reading the user from the file");
        fprintf(stderr, "CONNECT %s FAIL\n", username);
        return (1);
    }

    return_user_fields(buffer, username, status, conn_ip, conn_port, pending_msgs, last_msg_rcvd);
    // First, we check if the status is already on
    if (strcmp(status, "On") == 0)
    {
        fprintf(stderr, "CONNECT %s FAIL\n", username);
        return 2;
    }
    fclose(fPtr);

    // Modification of the values in the user file

    // Modify status before concatenating values
    sprintf(status, "On");
    char buffer_new[512];
    toUsrStr(buffer_new, username, status, ip_user, port, pending_msgs, last_msg_rcvd);
    // remove file to later re-insert it
    if (remove(filePath) < 0)
    {
        fprintf(stderr, "[connect_user error]: error modifying the user file\n");
        fprintf(stderr, "CONNECT %s FAIL\n", username);
        return 3;
    }
    // insert the file modified
    fPtr = fopen(filePath, "w+");
    if (fPtr == NULL)
    {
        fprintf(stderr, "[connect_user error]: error modifying the user file\n");
        fprintf(stderr, "CONNECT %s FAIL\n", username);
        fclose(fPtr);
        return 3;
    }

    if (fputs(buffer_new, fPtr) == EOF)
    {
        fprintf(stderr, "[connect_user error]: error modifying the user file\n");
        fprintf(stderr, "CONNECT %s FAIL\n", username);
        fclose(fPtr);
        return 3;
    }
    fclose(fPtr);

    fprintf(stderr, "CONNECT %s OK\n", username);
    return 0;
}

/* Disconnects the user from the system, not allowing her to continue receiving messages
    Return codes
        0 -- OK
        1 -- User does not exist
        2 -- User is already disconnected
        3 -- Any other error
*/

int disconnect_user(char username[], char ip_user[], char port[])
{
    // strings to compare the values of the file
    char status[4];
    char comp_ip[16];
    char comp_port[6];
    char pending_msgs[5000];
    char last_msg_rcvd[2];

    FILE *fPtr;
    char buffer[512];
    char filePath[PATH_MAX];

    // Arrange the file path to access the file that is storing the message
    sprintf(filePath, "./users/%s", username);

    fPtr = fopen(filePath, "r");
    if (fPtr == NULL)
    {
        fprintf(stderr, "[disconnect_user error]: Unable to open the file '%s'.\n", filePath);
        fprintf(stderr, "DISCONNECT %s FAIL\n", username);
        fclose(fPtr);
        return (1);
    }
    if (fgets(buffer, 512, fPtr) == NULL)
    {
        perror("[disconnect_user error]: there was an error reading the user from the file");
        fprintf(stderr, "DISCONNECT %s FAIL\n", username);
        return (1);
    }

    return_user_fields(buffer, username, status, comp_ip, comp_port, pending_msgs, last_msg_rcvd);
    fclose(fPtr);
    // First, we check if the status is already off
    if (strcmp(status, "Off") == 0)
    {
        fprintf(stderr, "DISCONNECT %s FAIL\n", username);
        return 2;
    }
    else
    {
        if (strcmp(comp_ip, ip_user) != 0)
        {
            fprintf(stderr, "DISCONNECT %s FAIL\n", username);
            return 3;
        }

        // Modification of the values in the user file
        // Modify status, ip and port before concatenating values
        sprintf(status, "Off");
        sprintf(comp_ip, "?");
        sprintf(comp_port, "?");

        char buffer_new[512];
        toUsrStr(buffer_new, username, status, comp_ip, comp_port, pending_msgs, last_msg_rcvd);
        // remove file to later re-insert it
        if (remove(filePath) < 0)
        {
            fprintf(stderr, "[disconnect_user error]: error modifying the user file\n");
            fprintf(stderr, "DISCONNECT %s FAIL\n", username);
            return 3;
        }
        // insert the file modified
        fPtr = fopen(filePath, "w+");
        if (fPtr == NULL)
        {
            fprintf(stderr, "[disconnect_user error]: error modifying the user file\n");
            fprintf(stderr, "DISCONNECT %s FAIL\n", username);
            fclose(fPtr);
            return 3;
        }

        if (fputs(buffer_new, fPtr) == EOF)
        {
            fprintf(stderr, "[disconnect_user error]: error modifying the user file\n");
            fprintf(stderr, "DISCONNECT %s FAIL\n", username);
            fclose(fPtr);
            return 3;
        }

        fclose(fPtr);
        fprintf(stderr, "DISCONNECT %s OK\n", username);
    }
    return 0;
}

/*  Sends a message from a user to another
    Return codes:
        0 -- OK
        1 -- user doesn't exist
        2 -- any other error
*/
int send_u2u(char sender[], char recipient[], char msg[])
{

    return 0;
}

/**************************************/
/*** METHODS TO DEAL WITH A REQUEST ***/
/**************************************/
ssize_t readLine(int fd, void *buffer, size_t n)
{
    ssize_t numRead; /* num of bytes fetched by last read() */
    size_t totRead;  /* total bytes read so far */
    char *buf;
    char ch;

    if (n <= 0 || buffer == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    buf = buffer;
    totRead = 0;

    for (;;)
    {
        numRead = read(fd, &ch, 1); /* read a byte */

        if (numRead == -1)
        {
            if (errno == EINTR) /* interrupted -> restart read() */
                continue;
            else
                return -1; /* some other error */
        }
        else if (numRead == 0)
        {                     /* EOF */
            if (totRead == 0) /* no bytes read; return 0 */
                return 0;
            else
                break;
        }
        else
        { /* numRead must be 1 if we get here*/
            if (ch == '\n')
                break;
            if (ch == '\0')
                break;
            if (totRead < n - 1)
            { /* discard > (n-1) bytes */
                totRead++;
                *buf++ = ch;
            }
        }
    }

    *buf = '\0';
    return totRead;
}

int deal_request(void *socket)
{
    char operation[15];
    char userSender[20];
    char userReceiver[20];
    char clientListeningPort[6];
    char msg[256];
    int retCode = 2; // Error por defecto.
    char buffer[MAX_LINE];
    int *socketPtr = (int *)socket;

    // Get client's IP for the method
    struct sockaddr *client_address;
    socklen_t len = sizeof(client_address);
    int ipsocket = getsockname(*socketPtr, (struct sockaddr *)&client_address, &len);
    if (ipsocket < 0)
    {
        perror("There was an error getting the client IP address");
    }
    struct sockaddr_in *pV4Addr = (struct sockaddr_in *)&client_address;
    struct in_addr ipAddr = pV4Addr->sin_addr;

    char ip_client[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ipAddr, ip_client, INET_ADDRSTRLEN);

    // Parse the operation code. 10 B is the max
    int read = readLine(*socketPtr, buffer, MAX_LINE);
    if (read == -1)
    {
        perror("There was an error reading the socket line");
        return -1;
    }
    else
    {
        // Copy operation code to work with
        strcpy(operation, buffer);
    }

    // check which operation we are doing
    if (strcmp(operation, "REGISTER") == 0)
    {
        int read_user = readLine(*socketPtr, buffer, MAX_LINE);
        if (read_user == -1)
        {
            perror("There was an error reading the socket");
            return -1;
        }
        else
        {
            strcpy(userSender, buffer);
        }
        pthread_mutex_lock(&mx);
        retCode = register_user(userSender);
        pthread_mutex_unlock(&mx);
    }
    else if (strcmp(operation, "UNREGISTER") == 0)
    {
        int read_user = readLine(*socketPtr, buffer, MAX_LINE);
        if (read_user == -1)
        {
            perror("There was an error reading the socket");
            return -1;
        }
        else
        {
            strcpy(userSender, buffer);
        }
        pthread_mutex_lock(&mx);
        retCode = unregister_user(userSender);
        pthread_mutex_unlock(&mx);
    }
    else if (strcmp(operation, "CONNECT") == 0)
    {
        int read_user = readLine(*socketPtr, buffer, MAX_LINE);
        if (read_user == -1)
        {
            perror("There was an error reading the socket");
            return -1;
        }
        else
        {
            strcpy(userSender, buffer);
        }

        int read_port = readLine(*socketPtr, buffer, MAX_LINE);
        if (read_port == -1)
        {
            perror("There was an error reading the socket");
            return -1;
        }
        else
        {
            strcpy(clientListeningPort, buffer);
        }
        pthread_mutex_lock(&mx);
        retCode = connect_user(userSender, ip_client, clientListeningPort);
        pthread_mutex_unlock(&mx);
    }
    else if (strcmp(operation, "DISCONNECT") == 0)
    {
        int read_user = readLine(*socketPtr, buffer, MAX_LINE);
        if (read_user == -1)
        {
            perror("There was an error reading the socket");
            return -1;
        }
        else
        {
            strcpy(userSender, buffer);
        }

        int read_port = readLine(*socketPtr, buffer, MAX_LINE);
        if (read_port == -1)
        {
            perror("There was an error reading the socket");
            return -1;
        }
        else
        {
            strcpy(clientListeningPort, buffer);
        }
        pthread_mutex_lock(&mx);
        retCode = disconnect_user(userSender, ip_client, clientListeningPort);
        pthread_mutex_unlock(&mx);
    }
    else if (strcmp(operation, "SEND") == 0)
    {
    }

    char buff[2];
    sprintf(buff, "%d", retCode);

    int result = write(*socketPtr, (char *)&buff, sizeof(char) * 2);
    if (result < 0)
    {
        perror("Failure in writing");
    }

    return retCode;
    // UNFINISHED
}

int main(int argc, char *argv[])
{
    // Check that there are no errors when calling the server
    if (argc != 3 || (strcmp(argv[1], "-p") != 0) || (atoi(argv[2]) < 0) || (atoi(argv[2]) > 65535))
    {
        fprintf(stderr, "[SERVER ERROR]: Usage: ./server -p <PORT NUMBER>\n");
        return -1;
    }
    else
    {
        // Assignation of the port given as input to our local variable
        sprintf(PORT, "%s", argv[2]);
    }

    if (init() < 0)
        perror("There was an error initializing the database");

    struct sockaddr_in server_addr, client_addr; // Structs holding the client and server address information

    pthread_attr_t t_attr;       // Threads' attributes
    pthread_t thid[MAX_THREADS]; // Creation of threads
    char sock_content[512];      // Buffer to send & receive content through socket
    int val;
    char shell_prompt[] = "s> ";

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

    // Initialization of pthread attributes
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
    // Closing the connection and destroying mutexes
    // pthread_mutex_destroy(&mx_msg);

    if (close(sock_d < 0))
    {
        perror("Connection closing failed");
    }

    return 0;
}