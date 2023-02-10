struct response_rpc {
    string value1<256>;
    int value2;
    float value3;
    int error_code;
};

program KEYS {
    version KEYSVERS {
        int INIT() = 1;
        int SET_VALUE(int key, string value1<256>, int value2, float value3) = 2;
        struct response_rpc GET_VALUE(int key) = 3;
        int MODIFY_VALUE(int key, string value1<256>, int value2, float value3) = 4;
        int DELETE_KEY(int key) = 5;
        int EXIST(int key) = 6;
        int NUM_ITEMS() = 7;
    } = 1;
} = 99;
