#ifndef HMAC_UTIL_H
#define HMAC_UTIL_H
#include "aes/esp_aes.h"
#include "esp_err.h" 

typedef struct {
    size_t  size;
    uint8_t* p_data;
} uint8_array_t;


esp_err_t gen_hmac_32(uint8_t* key, uint8_t* message, size_t message_size, uint8_t* hmac);

#endif // HMAC_UTIL_H