#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "nvs_util.h"
#include <string.h>
#include <esp_wifi_types.h>
#include "esp_log.h"

static const char TAG[] = "NVS_UTIL";


esp_err_t init_nvs() {
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    return err;
}

esp_err_t open_nvs(const char* namespace, nvs_handle_t* my_handle) {

    // Open
    esp_err_t err = nvs_open(namespace, NVS_READWRITE, my_handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS!", esp_err_to_name(err));
        return err;
    }

    return err;
}


esp_err_t get_authorization(const char* username, authorization* auth) {
    esp_err_t err = init_nvs();

    if (err != ESP_OK) return err;

    nvs_handle_t my_handle;
    err = open_nvs("authorizations", &my_handle);

    if (err != ESP_OK) return err;


    size_t required_size = 0;  // value will default to 0, if not set yet in NVS    
    err = nvs_get_blob(my_handle, username, NULL, &required_size);
    
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;


    if (required_size == 0) {
        ESP_LOGE(TAG, "Authorization for user %s does not exist!", username);
        return err;
    } else {

        err = nvs_get_blob(my_handle, username, auth, &required_size);

        if (err != ESP_OK) {
            free(auth);
            return err;
        }
        
        return err;
    }
}


esp_err_t set_authorization(authorization* auth) {
    esp_err_t err = init_nvs();
    nvs_handle_t my_handle;

    if (err != ESP_OK) return err;

    err = open_nvs("authorizations", &my_handle);

    if (err != ESP_OK) return err;

    size_t required_size = sizeof(authorization);

    err = nvs_set_blob(my_handle, auth->username, auth, required_size);
    
    if (err != ESP_OK) return err;

    // Commit
    err = nvs_commit(my_handle);
    if (err != ESP_OK) return err;

    // Close
    nvs_close(my_handle);
    return err;
}

esp_err_t delete_authorization(char* username) {
    esp_err_t err = init_nvs();
    nvs_handle_t my_handle;

    if (err != ESP_OK) return err;

    err = open_nvs("authorizations", &my_handle);

    if (err != ESP_OK) return err;

    err = nvs_erase_key(my_handle, username);
    if (err != ESP_OK) return err;

    // Commit
    err = nvs_commit(my_handle);
    if (err != ESP_OK) return err;

    // Close
    nvs_close(my_handle);
    return err;
}

esp_err_t get_saved_wifi(wifi_config_t* wifi_config) {
    esp_err_t err = init_nvs();

    if (err != ESP_OK) return err;

    nvs_handle_t my_handle;
    err = open_nvs("saved_params", &my_handle);

    if (err != ESP_OK) return err;


    size_t required_size = 0;  // value will default to 0, if not set yet in NVS
    err = nvs_get_blob(my_handle, "saved_wifi", NULL, &required_size);

    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;


    if (required_size == 0) {
        ESP_LOGE(TAG, "There is no saved wifi!");
        return err;
    } else {

        err = nvs_get_blob(my_handle, "saved_wifi", wifi_config, &required_size);

        if (err != ESP_OK) {
            return err;
        }

        return err;
    }
}

esp_err_t set_saved_wifi(wifi_config_t* wifi_config) {
    esp_err_t err = init_nvs();
    nvs_handle_t my_handle;

    if (err != ESP_OK) return err;

    err = open_nvs("saved_params", &my_handle);

    if (err != ESP_OK) return err;

    size_t required_size = sizeof(wifi_config_t);

    err = nvs_set_blob(my_handle, "saved_wifi", wifi_config, required_size);

    if (err != ESP_OK) return err;

    // Commit
    err = nvs_commit(my_handle);
    if (err != ESP_OK) return err;

    // Close
    nvs_close(my_handle);
    return err;
}

esp_err_t delete_saved_wifi() {
    esp_err_t err = init_nvs();
    nvs_handle_t my_handle;

    if (err != ESP_OK) return err;

    err = open_nvs("saved_params", &my_handle);

    if (err != ESP_OK) return err;

    err = nvs_erase_key(my_handle, "saved_wifi");
    if (err != ESP_OK) return err;

    // Commit
    err = nvs_commit(my_handle);
    if (err != ESP_OK) return err;

    // Close
    nvs_close(my_handle);
    return err;
}


lock_state_t get_lock_state() {
    esp_err_t err = init_nvs();

    if (err != ESP_OK) return err;

    nvs_handle_t my_handle;
    err = open_nvs("saved_params", &my_handle);

    if (err != ESP_OK) return err;
    lock_state_t lock_state;

    err = nvs_get_i32(my_handle, "lock_state", &lock_state);

    ESP_LOGE(TAG, "Get lock state %d", lock_state);


    if (err == ESP_ERR_NVS_NOT_FOUND) {
        lock_lock();
        lock_state = locked;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error getting lock state: %x", err);
    }

    return lock_state;
}

esp_err_t set_lock_state(lock_state_t lock_state) {
    esp_err_t err = init_nvs();
    nvs_handle_t my_handle;

    ESP_LOGE(TAG, "Save lock state %d", lock_state);

    if (err != ESP_OK) return err;

    err = open_nvs("saved_params", &my_handle);

    if (err != ESP_OK) return err;

    err = nvs_set_i32(my_handle, "lock_state", lock_state);

    if (err != ESP_OK) return err;

    // Commit
    err = nvs_commit(my_handle);
    if (err != ESP_OK) return err;

    // Close
    nvs_close(my_handle);
    return err;
}

// void test() {
//     esp_err_t err = init_nvs();

//     if (err != ESP_OK) return;

//     nvs_handle_t my_handle = open_nvs();

//     // Read run time blob
//     size_t required_size = 0;  // value will default to 0, if not set yet in NVS
//     // obtain required memory space to store blob being read from NVS

//     err = nvs_get_blob(my_handle, "test", NULL, &required_size);

//     if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

//     ESP_LOGI(TAG, "Info:");
//     if (required_size == 0) {
//         ESP_LOGI(TAG, "Nothing saved yet!");
//     } else {
//         authorization* auth = malloc(required_size);


//         err = nvs_get_blob(my_handle, "test", auth, &required_size);
//         if (err != ESP_OK) {
//             free(auth);
//             return err;
//         }
        
//         ESP_LOGI(TAG, "username: %s", auth->username);

//         free(auth);
//     }

    
//     authorization* auth = malloc(sizeof(authorization));

//     // Write value including previously saved blob if available
//     required_size = sizeof(autorization);

//     strcpy(auth->username, "0vn3kfl3n");

//     err = nvs_set_blob(my_handle, "test", auth, required_size);

//     free(auth);

//     if (err != ESP_OK) return err;

//     // Commit
//     err = nvs_commit(my_handle);
//     if (err != ESP_OK) return err;

//     // Close
//     nvs_close(my_handle);
// }