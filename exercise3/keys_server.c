/*
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */

#include "keys.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>	  /* For O_* constants */
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h>	  /* Definition of AT_* constants */
#include <unistd.h>
#include <linux/limits.h>
#include <pthread.h>
#include <stdbool.h> //For true and false bools

// Macro to initialize the mutex
pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;

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

/*** SERVER-SIDE METHODS TO MANAGE DATA STRUCTURES ***/

/*
Initializes the database ("messages" directory) that will store the different messages.
For that it deletes the folder with everything and creates it again.
*/

bool_t
init_1_svc(int *result, struct svc_req *rqstp)
{
	bool_t retval = TRUE;
	pthread_mutex_lock(&mx);
	/* Removes the current directory with all the messages stored in it (database) */
	if ((remove_directory("./messages") == -1))
	{
		perror("[ERROR-init]: there was an error when removing the directory");
		*result = -1;
		pthread_mutex_unlock(&mx); // Unlocks the mutex in case that it fails so it doesn't stay open
		return retval;
	}
	/* We create it again, basically we are reseting the directory */
	struct stat st = {0};
	if (stat("./messages", &st) == -1)
	{
		mkdir("./messages", 0777);
	}
	pthread_mutex_unlock(&mx);
	*result = 0; /* Default value for result is 1 but the exercise asks us that when the method is correct it must return a 0. So it doesn't fail for our tests */

	return retval;
}

/*
With this function, we will append to a new created file the message of the queue. There will be a file per message.
(our Database System)
*/

bool_t
set_value_1_svc(int key, char *value1, int value2, float value3, int *result, struct svc_req *rqstp)
{
	bool_t retval = TRUE;

	/* File pointer to hold reference of input file */
	FILE *fPtr;
	char filePath[PATH_MAX];

	/* To be safe, since message is 256 characters max, adding possible float value, we suppose the max number of characters is 512*/
	char dataToAppend[512];

	if ((toString(dataToAppend, key, value1, value2, value3)) < 0)
	{
		perror("[ERROR set_value]: there was an error in toString()");
		*result = -1;
		pthread_mutex_unlock(&mx); // Unlocks the mutex in case that it fails so it doesn't stay open
		return retval;
	}

	/* We get the first element of the message (the key) with strtok.*/
	char token[20];

	/* get the first token */
	sprintf(token, "%d", key);

	/* Declaration of the file name. There will be a file for each message of the queue and
	its name will be that of its key (input by client)*/
	sprintf(filePath, "./messages/%s.txt", token);
	
	pthread_mutex_lock(&mx);

	if ((fPtr = fopen(filePath, "r")) != NULL)
	{
		fprintf(stderr, "[ERROR-set_value]: the file '%s' already exists", filePath);
		fclose(fPtr);
		*result = -1;
		pthread_mutex_unlock(&mx); // Unlocks the mutex in case that it fails so it doesn't stay open
		return retval;
	}

	/*  Creates an empty file for reading and writing */
	fPtr = fopen(filePath, "w+");

	/* fopen() return NULL if unable to open file in given mode. */
	if (fPtr == NULL)
	{
		/* Unable to open file hence exit */
		fprintf(stderr, "[ERROR-set_value]: Unable to open the file '%s'.\n", filePath);
		*result = -1;
		pthread_mutex_unlock(&mx); // Unlocks the mutex in case that it fails so it doesn't stay open
		return retval;
	}
	/* Append data to file */
	if (fputs(dataToAppend, fPtr) == EOF)
	{
		perror("[ERROR-set_value]: error when writing content to file");
		*result = -1;
		pthread_mutex_unlock(&mx); // Unlocks the mutex in case that it fails so it doesn't stay open
		return retval;
	}

	fclose(fPtr); // close the file stream

	pthread_mutex_unlock(&mx);

	*result = 0; /* Default value for result is 1 but the exercise asks us that when the method is correct it must return a 0. So it doesn't fail for our tests */
	return retval;
}

/*
This method will search in the database a message with the ID key and return the different fields
*/

bool_t
get_value_1_svc(int key, struct response_rpc *result, struct svc_req *rqstp)
{
	result->value1 = malloc(256);
	bool_t retval = TRUE;
	
	char buffer[512];
	char filePath[PATH_MAX];
	FILE *fPtr;

	// ALLOCATE MEMORY FOR THE VALUE1 AND STRUCT. (RESULT IS AUTOMATICALLY MEMORY ASSIGNED)
	// ADD 2 MALLOCS: ONE IN THE SERVER SIDE IN GET_VALUE. ADD ANOTHER MALLOC IN KEYS.C
	// ya teniamos el de keys.c pero no habiamos asignado en server.c


	// Arrange the file path to access the file that is storing the message
	sprintf(filePath, "./messages/%d.txt", key);

	pthread_mutex_lock(&mx);
	
	fPtr = fopen(filePath, "r");
	if (fPtr == NULL)
	{
		fprintf(stderr, "[ERROR-get_value]: Unable to open the file '%s'.\n", filePath);
		result->error_code = -1;
		pthread_mutex_unlock(&mx); // Unlocks the mutex in case that it fails so it doesn't stay open
		//free(result->value1);
		return retval;
	}

	if (fgets(buffer, 512, fPtr) == NULL)
	{
		perror("[ERROR-get_value]: there was an error reading the message from the file");
		result->error_code = -1;
		pthread_mutex_unlock(&mx); // Unlocks the mutex in case that it fails so it doesn't stay open
		//free(result->value1);
		return retval;
	}
	// VER AQUI &
	if (returnFields(buffer, key, result->value1, &result->value2, &result->value3) < 0)
	{
		perror("[ERROR-get_value]: error while returning the fields");
		result->error_code = -1;
		pthread_mutex_unlock(&mx); // Unlocks the mutex in case that it fails so it doesn't stay open
		//free(result->value1);
		return retval;
	}

	// fprintf(stderr, "\nSaliendo de get_value() en server.c...\n");
	// fprintf(stderr, "Parametros:\nkey= %d\nvalue1= %s\nvalue2= %d\nvalue3= %f\n", key, value1, *value2, *value3);

	fclose(fPtr); // close the file stream
	pthread_mutex_unlock(&mx);
	//free(result->value1);
	
	return retval;
}

/*
Modifies the message of a file with the ID key
*/
bool_t
modify_value_1_svc(int key, char *value1, int value2, float value3, int *result, struct svc_req *rqstp)
{
	bool_t retval = TRUE;

	// THE FOLLOWING CODE WAS OUR PREVIOUS IMPLEMENTATION BUT THE FILE WAS CORRUPTED AND SHOWED WEIRD CHARACTERS.
	/* char filePath[PATH_MAX];
	FILE *fPtr;

	sprintf(filePath, "./messages/%d.txt", key);

	pthread_mutex_lock(&mx);

	// Open the file in read-only mode to figure out if it exists
	fPtr = fopen(filePath, "r");
	if (fPtr == NULL)
	{
		fprintf(stderr, "[ERROR-modify_value]: Unable to open the file '%s'.\n", filePath);
		*result = -1;
		pthread_mutex_unlock(&mx); // Unlocks the mutex in case that it fails so it doesn't stay open
		return retval;
	}

	// Reopen it in write-only mode to write the new contents

	char outputBuff[512];
	if (toString(outputBuff, key, value1, value2, value3) < 0)
	{
		perror("[ERROR-modify_value]: error when modifying the message");
		*result = -1;
		pthread_mutex_unlock(&mx); // Unlocks the mutex in case that it fails so it doesn't stay open
		return retval;
	}

	// We reopen, replacing the current stream with the output buffer previously gathered.
	fprintf(stderr, "[ERROR-modify_value]: error when opening file '%s' in write mode.\n", outputBuff);
	fPtr = freopen(NULL, "w", fPtr);
	if (fPtr == NULL)
	{
		fprintf(stderr, "[ERROR-modify_value]: error when opening file '%s' in write mode.\n", filePath);
		*result = -1;
		pthread_mutex_unlock(&mx); // Unlocks the mutex in case that it fails so it doesn't stay open
		return retval;
	}
	// Now we write into the file the buffer
	fwrite(outputBuff,1, 512, fPtr);

	fclose(fPtr); // close the file stream
	pthread_mutex_unlock(&mx);
	*/

	delete_key_1_svc(key, result, rqstp);
	set_value_1_svc(key, value1,  value2,  value3, result, rqstp);

	*result = 0; /* Default value for result is 1 but the exercise asks us that when the method is correct it must return a 0. So it doesn't fail for our tests */
	return retval;
}

/*
Deletes a file msg with the ID key
*/

bool_t
delete_key_1_svc(int key, int *result, struct svc_req *rqstp)
{
	bool_t retval = TRUE;

	char filePath[PATH_MAX];

	sprintf(filePath, "./messages/%d.txt", key);

	pthread_mutex_lock(&mx);

	if (remove(filePath) < 0)
	{
		perror("[ERROR-delete_key]: there was an error when deleting the file");
		*result = -1;
		pthread_mutex_unlock(&mx); // Unlocks the mutex in case that it fails so it doesn't stay open
		return retval;
	}

	pthread_mutex_unlock(&mx);

	*result = 0; /* Default value for result is 1 but the exercise asks us that when the method is correct it must return a 0. So it doesn't fail for our tests */
	return retval;
}

/*
Checks if the msg with ID key exists in the DB
*/
bool_t
exist_1_svc(int key, int *result, struct svc_req *rqstp)
{
	bool_t retval = TRUE;

	char filePath[PATH_MAX];
	FILE *fPtr;

	sprintf(filePath, "./messages/%d.txt", key);

	pthread_mutex_lock(&mx);

	// Open the file in read-only mode to figure out if it exists
	fPtr = fopen(filePath, "r");
	if (fPtr == NULL)
	{
		fclose(fPtr);
		*result = 0; // msg does not exists
		pthread_mutex_unlock(&mx); // Unlocks the mutex in case that it fails so it doesn't stay open
		return retval;
	}
	fclose(fPtr);

	pthread_mutex_unlock(&mx);

	*result = 1; // msg exists
	return retval;
}

/*
Checks the number of messages in the database
*/
bool_t
num_items_1_svc(int *result, struct svc_req *rqstp)
{
	bool_t retval = TRUE;

	char filePath[PATH_MAX] = "./messages";
	DIR *dirp;
	struct dirent *dp;
	int items = 0;

	pthread_mutex_lock(&mx);
	// Open the directory
	dirp = opendir(filePath);
	if (NULL == dirp)
	{
		perror("[ERROR-num_items]: there was an error when opening the directory");
		*result = -1;
		pthread_mutex_unlock(&mx); // Unlocks the mutex in case that it fails so it doesn't stay open
		return retval;
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

	pthread_mutex_unlock(&mx);

	*result = items;
	return retval;
}

int keys_1_freeresult(SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result)
{
	xdr_free(xdr_result, result);

	/*
	 * Insert additional freeing code here, if needed
	 */
	pthread_mutex_destroy(&mx);

	return 1;
}
