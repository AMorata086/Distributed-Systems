int init();

int set_value(int key, char *value1, int value2, float value3);

int get_value(int key, char *value1, int *value2, float *value3);

int modify_value(int key, char *value1, int value2, float value3);

int delete_key(int key);

int exist(int key);

int num_items();