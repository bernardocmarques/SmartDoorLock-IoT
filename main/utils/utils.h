#ifndef UTILS_H
#define UTILS_H
#include <stdint.h>
#include "esp_system.h"


#define NAK_MESSAGE "NAK"
#define ACK_MESSAGE "ACK"


uint8_t* get_random_array(int len);
int retrieve_session_credentials(char* cred_enc, char* user_addr);


#endif // UTILS_H