#include <stdint.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "rsa_util.h"
#include "base64.h"
#include "user_info.h"

static const char *TAG = "Utils";





uint8_t* get_random_array(int len) {
    size_t size = sizeof(uint8_t) * len;
    uint8_t* buf = malloc(size);

    esp_fill_random(buf, size);

    return buf;
}


int retrieve_session_credentials(char* cred_enc, char* user_addr) {
    uint8_t* key_256;
    ESP_LOGI(TAG, "Cred Enc %s", cred_enc);

    char* cred = decrypt_base64_RSA(cred_enc);
    ESP_LOGI(TAG, "cred %s", cred);

    char* sep = " ";

    char *pt;
    size_t size;
    int base64_size;

    pt = strtok(cred, sep);

    if (strcmp(pt, "SSC") != 0) return 0;

    pt = strtok (NULL, sep);
    base64_size = strlen(pt);
    key_256 = base64_decode(pt, base64_size, &size);

    esp_aes_context aes = init_AES(key_256);
    set_AES_ctx(user_addr, aes);
    set_user_state(user_addr, CONNECTED);

    return 1;
}