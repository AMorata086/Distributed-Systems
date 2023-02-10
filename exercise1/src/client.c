#include <mqueue.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "keys.h"
#include <stdlib.h>
#include <pthread.h>

void sendData() /* Sends a set of values corresponding to one file in our DB*/
{
    int key = 1;
    char *value1 = "Lorem ipsum dolor sit amet, consectetur adipiscing elit.";
    int ivalue2 = 1;
    float fvalue3 = 0.5;
    set_value(key, value1, ivalue2, fvalue3);
}

/* Sends different sets of values corresponding to different files in our DB */
void sendMultipleData()
{
    /* First set of values */
    int key = 1;
    char *value1 = "Hello world! First message";
    int number = 2;
    float fnumber = 0.6;
    set_value(key, value1, number, fnumber);

    /* Second set of values */
    key = 2;
    value1 = "This is the second message";
    number = 3;
    fnumber = 1.7;
    set_value(key, value1, number, fnumber);

    /* Third set of values */
    key = 3;
    value1 = "And here goes the third one!";
    number = 4;
    fnumber = 2.8;
    set_value(key, value1, number, fnumber);

    /* Fourth set of values */
    key = 4;
    value1 = "Fourth";
    number = 5;
    fnumber = 3.9;
    set_value(key, value1, number, fnumber);

    /* Fifth set of values */
    key = 5;
    value1 = "High five!";
    number = 6;
    fnumber = 4.1;
    set_value(key, value1, number, fnumber);
}

/* Resets our DB (deletes the files stored in ./messages) and sends the first tuple */
void reset()
{
    init();
    sendData();
}

/* Does the same as reset, but inserts 5 files into the DB */
void resetMultiple()
{
    init();
    sendMultipleData();
}

void init_Test()
{
    printf("[TEST]: init()\n");
    int result = init();

    if (result == 0)
    {
        printf("[TEST]: init() -> CORRECT\n");
    }
    else
    {
        printf("[TEST]: init() -> FAILED\n");
    }
}

void set_valueTest()
{
    printf("[TEST]: set_value()\n");
    // Data to send to the test function
    int key = 1;
    char *value1 = "TEST SET VALUE";
    int int_number = 2;
    float float_number = 3.5;

    int result = set_value(key, value1, int_number, float_number);

    if (result == 0)
    {
        printf("[TEST]: set_value() -> CORRECT\n");
    }
    else
    {
        printf("[TEST]: set_value() -> FAILED\n");
    }
}

void get_valueTest()
{
    printf("[TEST]: get_value()\n");

    // Data to receive for the test function
    int key = 1;
    char value1[256] = "";
    int int_number = 0;
    float float_number = 0;

    int result = get_value(key, value1, &int_number, &float_number);
    // Check that the function is executed without error
    if (result == 0)
    {
        printf("[TEST]: (1) get_value() -> CORRECT\n");

        // Check that the function retrieves the correct values from the database
        char *comp_svalue1 = "Lorem ipsum dolor sit amet, consectetur adipiscing elit.";
        int comp_ivalue2 = 1;
        float comp_fvalue3 = 0.5;

        int comparison = strcmp(value1, comp_svalue1);
        // printf("\n%s - %s\n", &value1, comp_svalue1);

        // printf("%d %d %d %f %f \n", comparison, comp_ivalue2, int_number, comp_fvalue3, float_number);
        if (comparison == 0 && comp_ivalue2 == int_number && comp_fvalue3 == float_number)
        {
            printf("[TEST]: (2) get_value() -> CORRECT\n");
        }
        else
        {
            printf("[TEST]: (2) get_value() -> FAILED\n");
        }
    }
    else
    {
        printf("[TEST]: (1) get_value() -> FAILED\n");
    }
}

void modify_valueTest()
{
    printf("[TEST]: modify_value()");

    int key = 2;
    char *value1 = "This is the second message";
    int number = 3;
    float fnumber = 1.7;

    int result = modify_value(key, value1, number, fnumber);
    if (result == 0)
    {
        printf("[TEST]: (1) get_value() -> CORRECT\n");
    }
    else
    {
        printf("[TEST]: (1) get_value() -> FAILED\n");
    }
}

void delete_keyTest()
{
    printf("[TEST]: delete_value()\n");

    int key = 1;
    int result = delete_key(key);

    if (result == 0)
    {
        printf("[TEST]: (1) delete_key() -> CORRECT\n");
    }
    else
    {
        printf("[TEST]: (1) delete_key() -> FAILED\n");
    }
}

void existTest()
{
    printf("[TEST]: exist()\n");

    int key = 1;

    int result = exist(key);

    if (result == 1)
    {
        printf("[TEST]: (1) exist() -> CORRECT\n");
    }
    else
    {
        printf("[TEST]: (1) exist() -> FAILED\n");
    }

    // Deletion of the key
    printf("cpt 1\n");
    int del_result = delete_key(key);
    printf("cpt 2\n");
    // After deleting the key, we run the exist function again
    result = exist(key);
    printf("cpt 3\n");

    // Check that the key was deleted and that the result
    // was not 1 (it would mean the key existed after deletion)
    if (del_result != 0 || result != 0)
    {
        printf("[TEST]: (2) exist() -> FAILED\n");
    }
    else
    {
        printf("[TEST]: (2) exist() -> CORRECT\n");
    }
}

void num_itemsTest()
{
    printf("[TEST]: num_items()\n");

    // We store in the variable how many items there are
    int result = num_items();
    // We will assert this after inserting just one element
    if (result == 1)
    {
        printf("[TEST]: (1) num_items() -> CORRECT\n");
    }
    else
    {
        printf("[TEST]: (1) num_items() -> FAILED\n");
    }
}

void *test_function()
{
    // reset data = reset (init + insertdata) rompe la DB y mete 1 tupla
    // clean data = resetmultiple (in) rompe la DB y mete 5 tuplas
    reset();
    init_Test();

    set_valueTest();

    reset();
    num_itemsTest();

    // reset();
    // get_valueTest();

    // reset();
    // existTest();
}

int main()
{
    pthread_t thread1, thread2;
    int fthread1, fthread2;
    init();

    fthread1 = pthread_create(&thread1, NULL, test_function, NULL);
    if (fthread1 != 0)
    {
        perror("fthread1 unsuccessfully created");
    }
    // fthread2 = pthread_create(&thread1, NULL, test_function, NULL);
    // if (fthread2 != 0)
    // {
    //     perror("fthread2 unsuccessfully created");
    // }

    // pthread_join(thread2, NULL);
    pthread_join(thread1, NULL);

    return 0;
}
