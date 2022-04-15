#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <esp_log.h>
#include "nonce.h"
#include "esp_system.h"


nonce_t* head = NULL;

//static const char TAG[] = "NONCE";
//static const int ONE_SECOND_IN_MILLIS = 1000;

void addNonceSorted(long nonce, int expires) {
    nonce_t* newNonce;

    newNonce = malloc(sizeof(nonce_t));
    newNonce->nonce = nonce;
    newNonce->expires = expires;

    if (head == NULL) {
        head = newNonce;
        head->next = NULL;
        return;
    }

    nonce_t* temp = head;

    while(temp->expires <= expires) {
        if(temp->next != NULL) {
            temp = temp->next;
        } else {
            newNonce->next = NULL;
            temp->next = newNonce;
            return;
        }
    }

    newNonce->next = temp->next;
    temp->next = newNonce;
}

bool checkNonce(long nonce) {
    int now_ts = getNowTimestamp();

    if (head == NULL) {
        return true;
    }

    nonce_t* temp = head;

    bool valid = temp->nonce != nonce;
    while (temp->next != NULL) {
        ESP_LOGE("NONCE", "%ld", temp->nonce);
        if (temp->nonce == nonce) {
            valid = false;
        }

        nonce_t* to_delete = NULL;
        if (temp->expires <= now_ts) {
            to_delete = head;
            head = temp->next;

        } else {
            if (!valid) break;
        }

        temp = temp->next;
        if (to_delete != NULL) free(to_delete);
    }

    return valid;
}

char* addTimestampsAndNonceToMsg(char* msg) {
    char timestamp_str_1[21];
    char timestamp_str_2[21];
    char nonce_str[21];

    int now_ts = getNowTimestamp();

    sprintf(timestamp_str_1, "%d", now_ts - 30);
    sprintf(timestamp_str_2, "%d", now_ts + 30);
    sprintf(nonce_str, "%u", esp_random());

    char* result = (char*) malloc((strlen(msg) + strlen(timestamp_str_1) + strlen(timestamp_str_2) + strlen(nonce_str)) * sizeof(char));

    sprintf(result, "%s %s %s %s", msg, timestamp_str_1, timestamp_str_2, nonce_str);

    return result;
}