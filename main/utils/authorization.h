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
    char phone_id[16];
    enum userType user_type;
    uint8_t master_key[KEY_SIZE_BYTES];
    bool weekdays[7];
    int valid_from_ts;
    int valid_until_ts;
    int one_day_ts;
} authorization;


void print_authorization(authorization* auth);

int check_authorization_code(char* phone_id, char* auth_code_base64, uint8_t* seed);
enum userType get_user_type(char* phone_id);

#endif // AUTHORIZATION_H