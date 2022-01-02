#ifndef NVS_UTIL_H
#define NVS_UTIL_H
#include "nvs_flash.h"
#include "nvs.h"
#include "aes_util.h"
#include "authorization.h"

// enum user_type {
//     admin = 0,
//     owner = 1,
//     tenant = 2,
//     periodic_user = 3,
//     one_time_user = 4
// };

// typedef struct {
//     char user_id[10]; //FIXME change size (maybe 128)
//     enum user_type user;
//     uint8_t master_key[KEY_SIZE_BYTES];
// } authorization;




esp_err_t init_nvs();
esp_err_t open_nvs(const char* namespace, nvs_handle_t* my_handle);


esp_err_t get_authorization(const char* user_id, authorization* auth);
esp_err_t set_authorization(authorization* auth);

#endif // AES_UTIL_H