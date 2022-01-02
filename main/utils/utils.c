#include <stdint.h>
#include "esp_system.h"
#include "esp_log.h"

uint8_t* get_random_array(int len) {
    size_t size = sizeof(uint8_t) * len;
    uint8_t* buf = malloc(size);

    esp_fill_random(buf, size);

    return buf;
}