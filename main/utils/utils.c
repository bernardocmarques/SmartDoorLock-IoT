#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <freertos/event_groups.h>
#include <sys/unistd.h>
#include "esp_system.h"
#include "esp_log.h"
#include "rsa_util.h"
#include "base64.h"
#include "user_info.h"

static const char *TAG = "Utils";


void restart_esp(int delay_seconds) {
    if (delay_seconds < 0) delay_seconds = 0;

    if (delay_seconds == 0) {
        ESP_LOGI(TAG, "Restarting now!");
    } else if (delay_seconds == 1) {
        ESP_LOGI(TAG, "Restarting in 1 second...");
    } else {
        ESP_LOGI(TAG, "Restarting in %d seconds...", delay_seconds);
    }
    sleep(delay_seconds);
    esp_restart(); // fixme remove
}


uint8_t* get_random_array(int len) {
    size_t size = sizeof(uint8_t) * len;
    uint8_t* buf = malloc(size);

    esp_fill_random(buf, size);

    return buf;
}


/********************* Retrieve credentials ***********************/

//#define SESSIONS_CREDENTIALS_RETRIEVED_BIT BIT0
//#define SESSIONS_CREDENTIALS_FAIL_BIT      BIT1
//static EventGroupHandle_t s_session_credentials_event_group;
//
//
//
//char* credEnc;
//char* userAddr;
//
//
//void retrieve_session_credentials_task() {
//    uint8_t* key_256;
//    ESP_LOGI(TAG, "Cred Enc %s", credEnc);
//
//    char* cred = decrypt_base64_RSA(credEnc);
//    ESP_LOGI(TAG, "cred %s", cred);
//
//    char* sep = " ";
//
//    char *pt;
//    size_t size;
//    int base64_size;
//
//    pt = strtok(cred, sep);
//
//    if (strcmp(pt, "SSC") != 0) {
//        xEventGroupSetBits(s_session_credentials_event_group, SESSIONS_CREDENTIALS_FAIL_BIT);
//        vTaskDelete(NULL);
//        return;
//    }
//
//    pt = strtok (NULL, sep);
//    base64_size = strlen(pt);
//    key_256 = base64_decode(pt, base64_size, &size);
//
//    esp_aes_context aes = init_AES(key_256);
//    set_AES_ctx(userAddr, aes);
//    set_user_state(userAddr, CONNECTED);
//
//    xEventGroupSetBits(s_session_credentials_event_group, SESSIONS_CREDENTIALS_RETRIEVED_BIT);
//    vTaskDelete(NULL);
//}
//
//int retrieve_session_credentials(char* cred_enc, char* user_addr) {
//    credEnc = cred_enc;
//    userAddr = user_addr;
//
//    xTaskCreate(retrieve_session_credentials_task, "retrieve_session_credentials_task", 4096, NULL, 5, NULL);
//
//    EventBits_t bits = xEventGroupWaitBits(s_session_credentials_event_group,
//                                           SESSIONS_CREDENTIALS_RETRIEVED_BIT | SESSIONS_CREDENTIALS_FAIL_BIT,
//                                           pdFALSE,
//                                           pdFALSE,
//                                           portMAX_DELAY);
//
//
//    if (bits & SESSIONS_CREDENTIALS_RETRIEVED_BIT) {
//        return 1;
//    } else if (bits & SESSIONS_CREDENTIALS_FAIL_BIT) {
//        return 0;
//
//    } else {
//        return 0;
//    }
//}


int retrieve_session_credentials(char* cred_enc, char* user_addr) {
    uint8_t* key_256;

    char* cred = decrypt_base64_RSA(cred_enc);

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
