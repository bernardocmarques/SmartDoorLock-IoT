#ifndef AUTHORIZATION_H
#define AUTHORIZATION_H

#include "aes_util.h"
#include <limits.h>


enum userType {  // order of permissions. 0 is higher than 1, admin is higher than owner
    admin = 0,
    owner = 1,
    tenant = 2,
    periodic_user = 3,
    one_time_user = 4
};

static const enum userType INVALID_USER = INT_MAX;

typedef struct {
    char username[16];
    enum userType user_type;
    uint8_t master_key[KEY_SIZE_BYTES];
} authorization;


void print_authorization(authorization* auth);

int check_authorization_code(char* username, char* auth_code_base64, uint8_t* seed);
enum userType get_user_type(char* username);

#endif // AUTHORIZATION_H