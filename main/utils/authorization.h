#ifndef AUTHORIZATION_H
#define AUTHORIZATION_H

#include "aes_util.h"


enum user_type {
    admin = 0,
    owner = 1,
    tenant = 2,
    periodic_user = 3,
    one_time_user = 4
};

typedef struct {
    char user_id[10]; //FIXME change size (maybe 128)
    enum user_type user;
    uint8_t master_key[KEY_SIZE_BYTES];
} authorization;


void print_authorization(authorization* auth);

int check_authorization_code(char* user_id, char* auth_code_base64, uint8_t* seed);


#endif // AUTHORIZATION_H