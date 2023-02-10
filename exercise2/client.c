#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "keys.h"
#include <stdlib.h>
#include <pthread.h>

/* Command to set the environment variables */
// export IP_TUPLES=127.0.0.1 && export PORT_TUPLES=50069

void sendData() /* Sends a set of values corresponding to one file in our DB*/
{
    int key = 1;
    char value1[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit.";
    int ivalue2 = 1;
    float fvalue3 = 0.5f;
    set_value(key, value1, ivalue2, fvalue3);
}

void sendData2() /* Sends a set of values corresponding to one file in our DB*/
{
    int key = 6;
    char value1[] = "Test sendData2!";
    int ivalue2 = 7;
    float fvalue3 = 8.5f;
    set_value(key, value1, ivalue2, fvalue3);
}

/* Sends different sets of values corresponding to different files in our DB */
void sendMultipleData()
{
    /* First set of values */
    int key = 1;
    char value1[] = "Hello world! First message";
    int number = 2;
    float fnumber = 0.6f;
    set_value(key, value1, number, fnumber);

    /* Second set of values */
    key = 2;
    char value12[] = "This is the second message";
    number = 3;
    fnumber = 1.7f;
    set_value(key, value12, number, fnumber);

    /* Third set of values */
    key = 3;
    char value13[] = "And here goes the third one!";
    number = 4;
    fnumber = 2.8f;
    set_value(key, value13, number, fnumber);

    /* Fourth set of values */
    key = 4;
    char value14[] = "Fourth";
    number = 5;
    fnumber = 3.9f;
    set_value(key, value14, number, fnumber);

    /* Fifth set of values */
    key = 5;
    char value15[] = "High five!";
    number = 6;
    fnumber = 4.1f;
    set_value(key, value15, number, fnumber);
}

void sendMultipleData2()
{
    /* First set of values */
    int key = 7;
    char value1[] = "Hello world! First message 7";
    int number = 2;
    float fnumber = 0.6f;
    set_value(key, value1, number, fnumber);

    /* Second set of values */
    key = 8;
    char value12[] = "This is the second message 8";
    number = 3;
    fnumber = 1.7f;
    set_value(key, value12, number, fnumber);

    /* Third set of values */
    key = 9;
    char value13[] = "And here goes the third one! 9";
    number = 4;
    fnumber = 2.8f;
    set_value(key, value13, number, fnumber);

    /* Fourth set of values */
    key = 10;
    char value14[] = "Fourth 10";
    number = 5;
    fnumber = 3.9f;
    set_value(key, value14, number, fnumber);

    /* Fifth set of values */
    key = 11;
    char value15[] = "High five! 11";
    number = 6;
    fnumber = 4.1f;
    set_value(key, value15, number, fnumber);
}

/* Resets our DB (deletes the files stored in ./messages) and sends the first tuple */
void reset()
{
    init();
    sendData();
}

void reset2()
{
    init();
    sendData2();
}
/* Does the same as reset, but inserts 5 files into the DB */
void resetMultiple()
{
    init();
    sendMultipleData();
}

void resetMultiple2()
{
    init();
    sendMultipleData2();
}

void init_Test()
{
    printf("--- [TEST]: init() ---\n");
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
    printf("--- [TEST]: set_value() ---\n");
    // Data to send to the test function
    int key = 1;
    char value1[] = "TEST SET VALUE";
    int int_number = 2;
    float float_number = 3.5f;

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

void set_valueTest2()
{
    printf("--- [TEST]: set_value2() ---\n");
    // Data to send to the test function
    int key = 7;
    char value1[] = "TEST SET VALUE 2";
    int int_number = 3;
    float float_number = 4.5f;

    int result = set_value(key, value1, int_number, float_number);

    if (result == 0)
    {
        printf("[TEST]: set_value2() -> CORRECT\n");
    }
    else
    {
        printf("[TEST]: set_value2() -> FAILED\n");
    }
}

void get_valueTest()
{
    printf("[TEST]: get_value()\n");

    // Data to receive for the test function
    int key = 1;
    char value1[256];
    int int_number = 0;
    float float_number = 0.0f;

    int result = get_value(key, value1, &int_number, &float_number);
    // Check that the function is executed without error
    // fprintf(stderr, "%d %s %d %f \n", key, value1, int_number, float_number);
    if (result == 0)
    {
        printf("--- [TEST]: (1) get_value() -> CORRECT ---\n");

        // Check that the function retrieves the correct values from the database
        char comp_svalue1[256] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit.";
        int comp_ivalue2 = 1;
        float comp_fvalue3 = 0.5f;

        int comparison = strcmp(value1, comp_svalue1);
        // fprintf(stderr, "\nEl valor de comparison es: %d\n", comparison);
        // fprintf(stderr, "\n%s - %s\n", value1, comp_svalue1);

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

void get_valueTest2()
{
    printf("--- [TEST]: get_value2() ---\n");

    // Data to receive for the test function
    int key = 6;
    char value1[256];
    int int_number = 0;
    float float_number = 0.0f;

    int result = get_value(key, value1, &int_number, &float_number);
    // Check that the function is executed without error
    // fprintf(stderr, "%d %s %d %f \n", key, value1, int_number, float_number);
    if (result == 0)
    {
        printf("[TEST]: (1) get_value2() -> CORRECT\n");

        // Check that the function retrieves the correct values from the database
        char comp_svalue1[256] = "Test sendData2!";
        int comp_ivalue2 = 7;
        float comp_fvalue3 = 8.5f;

        int comparison = strcmp(value1, comp_svalue1);
        // fprintf(stderr, "\nEl valor de comparison es: %d\n", comparison);
        // fprintf(stderr, "\n%s - %s\n", value1, comp_svalue1);

        // printf("%d %d %d %f %f \n", comparison, comp_ivalue2, int_number, comp_fvalue3, float_number);
        if (comparison == 0 && comp_ivalue2 == int_number && comp_fvalue3 == float_number)
        {
            printf("[TEST]: (2) get_value2() -> CORRECT\n");
        }
        else
        {
            printf("[TEST]: (2) get_value2() -> FAILED\n");
        }
    }
    else
    {
        printf("[TEST]: (1) get_value2() -> FAILED\n");
    }
}

void modify_valueTest()
{
    printf("--- [TEST]: modify_value() ---\n");

    int key = 2;
    char value1[] = "This is the second message - modified";
    int number = 3;
    float fnumber = 2.9f;

    int result = modify_value(key, value1, number, fnumber);
    if (result == 0)
    {
        printf("[TEST]: (1) modify_value() -> CORRECT\n");
    }
    else
    {
        printf("[TEST]: (1) modify_value() -> FAILED\n");
    }
}

void modify_valueTest2()
{
    printf("--- [TEST]: modify_value2() ---\n");

    int key = 8;
    char value1[] = "This is the second message 8 - modified";
    int number = 6;
    float fnumber = 2.7f;

    int result = modify_value(key, value1, number, fnumber);
    if (result == 0)
    {
        printf("[TEST]: (1) modify_value2() -> CORRECT\n");
    }
    else
    {
        printf("[TEST]: (1) modify_value2() -> FAILED\n");
    }
}

void delete_keyTest()
{
    printf("--- [TEST]: delete_key() ---\n");

    int key = 3;
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

void delete_keyTest2()
{
    printf("--- [TEST]: delete_key2() ---\n");

    int key = 9;
    int result = delete_key(key);

    if (result == 0)
    {
        printf("[TEST]: (1) delete_key2() -> CORRECT\n");
    }
    else
    {
        printf("[TEST]: (1) delete_key2() -> FAILED\n");
    }
}

void existTest()
{
    printf("--- [TEST]: exist() --- \n");

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
}

void existTest2()
{
    printf("--- [TEST]: exist2() ---\n");

    int key = 7;

    int result = exist(key);

    if (result == 1)
    {
        printf("[TEST]: (1) exist2() -> CORRECT\n");
    }
    else
    {
        printf("[TEST]: (1) exist2() -> FAILED\n");
    }
}

void num_itemsTest()
{
    printf("--- [TEST]: num_items() ---\n");

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
    reset();
    init_Test();

    set_valueTest();

    reset();
    num_itemsTest();

    reset();
    get_valueTest();

    resetMultiple();

    existTest();
    modify_valueTest();
    delete_keyTest();
}

// THERE'S A NEED OF A SECOND TEST FUNCTION THAT CALLS OTHER TEST FUNCTIONS WITH DIFFERENT VALUES SO THERE ARE NO REPEATED METHODS
void *test_function2()
{
    reset2();
    init_Test(); // The same one works

    set_valueTest2(); // ok

    reset2();
    printf("NUM_ITEMS2");
    num_itemsTest(); // The same one works

    reset2();
    get_valueTest2(); // ok

    resetMultiple2();

    existTest2();
    modify_valueTest2();
    delete_keyTest2();
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

    fthread2 = pthread_create(&thread2, NULL, test_function2, NULL);
    if (fthread2 != 0)
    {
        perror("fthread2 unsuccessfully created");
    }

    pthread_join(thread2, NULL);
    pthread_join(thread1, NULL);

    return 0;
}
