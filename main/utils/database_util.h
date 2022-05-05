#ifndef DATABASE_UTIL_H
#define DATABASE_UTIL_H
#include "authorization.h"

char* create_invite(int expiration, enum userType user_type, int valid_from, int valid_until, char* weekdays_str, int one_day);
esp_err_t get_authorization_db(char* user_id, authorization* auth);

#endif // DATABASE_UTIL_H