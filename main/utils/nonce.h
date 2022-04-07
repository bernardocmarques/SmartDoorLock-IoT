#include <stdio.h>
#include <stdlib.h>
#include "time_util.h"

typedef struct Nonce {
    long nonce;
    long expires;
    struct Nonce* next;
} nonce_t;


void addNonceSorted(long nonce, int expires);
bool checkNonce(long nonce);

char* addTimestampsAndNonceToMsg(char* msg);