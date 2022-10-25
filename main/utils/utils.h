#ifndef UTILS_H
#define UTILS_H
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <freertos/event_groups.h>
#include "esp_system.h"


#define NAK_MESSAGE "NAK"
#define ACK_MESSAGE "ACK"

static char* ble_user = "BLE_USER";

typedef enum {
    UNKNOWN_STATUS = -1,
    NOT_REGISTERED = 0,
    REGISTERED = 1,
    COMPLETE = 2,
} lock_registration_status_t;

void restart_esp(int delay_seconds);

uint8_t* get_random_array(int len);
int retrieve_session_credentials(char* cred_enc, char* user_addr);
lock_registration_status_t get_registration_status();
lock_registration_status_t force_get_registration_status();

int getDefaultExpirationTimestamp();
#endif // UTILS_H