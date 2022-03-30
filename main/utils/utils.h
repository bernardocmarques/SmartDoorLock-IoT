#ifndef UTILS_H
#define UTILS_H
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <freertos/event_groups.h>
#include "esp_system.h"


#define NAK_MESSAGE "NAK"
#define ACK_MESSAGE "ACK"

static TaskHandle_t tcpTask;
static TaskHandle_t bleTask;


void restart_esp(int delay_seconds);

uint8_t* get_random_array(int len);
int retrieve_session_credentials(char* cred_enc, char* user_addr);


#endif // UTILS_H