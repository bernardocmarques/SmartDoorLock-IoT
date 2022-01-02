#ifndef UTILS_H
#define UTILS_H
#include <stdint.h>
#include "esp_system.h"

#define NAK_MESSAGE "NAK"
#define ACK_MESSAGE "ACK"

uint8_t* get_random_array(int len);



#endif // UTILS_H