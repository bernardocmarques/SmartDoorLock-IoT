#ifndef NVS_UTIL_H
#define NVS_UTIL_H

#include <esp_wifi_types.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "aes_util.h"
#include "authorization.h"
#include "lock_status.h"


esp_err_t init_nvs();
esp_err_t open_nvs(const char* namespace, nvs_handle_t* my_handle);


esp_err_t get_authorization(const char* username, authorization* auth);
esp_err_t set_authorization(authorization* auth);
esp_err_t delete_authorization(char* username);

esp_err_t get_saved_wifi(wifi_config_t* wifi_config);
esp_err_t set_saved_wifi(wifi_config_t* wifi_config);
esp_err_t delete_saved_wifi();

lock_state_t get_lock_state();
esp_err_t set_lock_state(lock_state_t lock_state);

#endif // AES_UTIL_H