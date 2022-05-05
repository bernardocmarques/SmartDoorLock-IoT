#ifndef NVS_UTIL_H
#define NVS_UTIL_H

#include <esp_wifi_types.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "aes_util.h"
#include "authorization.h"

// enum userType {
//     admin = 0,
//     owner = 1,
//     tenant = 2,
//     periodic_user = 3,
//     one_time_user = 4
// };

// typedef struct {
//     char username[128];
//     enum userType user;
//     uint8_t master_key[KEY_SIZE_BYTES];
// } authorization;



esp_err_t init_nvs();
esp_err_t open_nvs(const char* namespace, nvs_handle_t* my_handle);


esp_err_t get_authorization(const char* username, authorization* auth);
esp_err_t set_authorization(authorization* auth);
esp_err_t delete_authorization(char* username);

esp_err_t get_saved_wifi(wifi_config_t* wifi_config);
esp_err_t set_saved_wifi(wifi_config_t* wifi_config);
esp_err_t delete_saved_wifi();

#endif // AES_UTIL_H