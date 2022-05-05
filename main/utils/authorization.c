#include <stdio.h>
#include <string.h>
#include "authorization.h"
#include "esp_log.h"
#include "base64_util.h"
#include "nvs_util.h"
#include "hmac_util.h"
#include "database_util.h"

static const char *TAG = "Authorization_Util";

void print_authorization(authorization* auth) {
    size_t base64_size;
    char* master_key_base64 = base64_encode(auth->master_key, sizeof(uint8_t) * KEY_SIZE_BYTES, &base64_size);

    ESP_LOGI("Authorization", "User with ID %s with type %d has the following master key: %s", auth->username, auth->user_type, master_key_base64);
}


int check_authorization_code(char* username, char* auth_code_base64, uint8_t* seed) {

    
    authorization* auth = malloc(sizeof(authorization));

    esp_err_t err = get_authorization(username, auth);

    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Get authorization from database.");
        err = get_authorization_db(username, auth);

        print_authorization(auth);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Could not get authorization");
            free(auth);
            return 0;
        }
    }
   
    uint8_t* hmac =  (uint8_t*) malloc(sizeof(uint8_t) * 32);

    err = gen_hmac_32(auth->master_key, seed, 10, hmac);

    if (err == ESP_OK) {
        size_t base64_size;
        char* hmac_base64 = base64_encode(hmac, 32, &base64_size);
//        hmac_base64[base64_size/sizeof(uint8_t) - 1] = '\0'; // Remove '\n' in the end of string

        int is_valid = strcmp(hmac_base64, auth_code_base64) == 0 ? 1 : 0;

        free(auth);
        free(hmac);
        free(hmac_base64);

        return is_valid;

    } else {
        ESP_LOGE("Authorization", "Error while calculating HMAC. Error: %s", esp_err_to_name(err));
        
        free(auth);
        free(hmac);
        return 0;
    }
}

enum userType get_user_type(char* username) {
    authorization* auth = malloc(sizeof(authorization));

    esp_err_t err = get_authorization(username, auth);

    if (err != ESP_OK) {
        free(auth);
        return INVALID_USER;
    } else {
        return auth->user_type;
    }
}


