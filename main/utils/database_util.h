#ifndef DATABASE_UTIL_H
#define DATABASE_UTIL_H
#include "authorization.h"

typedef enum {
    UNKNOWN_STATUS = -1,
    NOT_REGISTERED = 0,
    REGISTERED = 1,
    COMPLETE = 2,
} lock_registration_status_t;

char* create_invite(int expiration, enum userType user_type, int valid_from, int valid_until, char* weekdays_str, int one_day);
esp_err_t get_authorization_db(char* user_id, authorization* auth);
void register_lock(char* certificate);

lock_registration_status_t get_registration_status();
#endif // DATABASE_UTIL_H