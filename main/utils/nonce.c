#include <stdlib.h>
#include <stdbool.h>
#include <esp_log.h>
#include "nonce.h"


nonce_t* head = NULL;

static const char TAG[] = "NONCE";

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
    long now_ts = (long) getNowTimestamp();

    if (head == NULL) {
        return true;
    }

    nonce_t* temp = head;

    bool valid = temp->nonce != nonce;
    while (temp->next != NULL) {

        if (temp->nonce == nonce) {
            valid = false;
        }

        if (temp->expires <= now_ts) {
            nonce_t* to_delete = head;
            head = temp->next;
            temp = temp->next;
            free(to_delete);
        } else {
            if (!valid) break;
        }
    }

    return valid;
}