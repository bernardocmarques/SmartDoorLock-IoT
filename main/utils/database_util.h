#ifndef DATABASE_UTIL_H
#define DATABASE_UTIL_H
#include "authorization.h"

char* create_invite(int expiration, enum user_type userType, int validFrom, int validUntil);


#endif // DATABASE_UTIL_H