#include <string.h>
#include <stdlib.h>
//#include <esp_spiffs.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "tcp_server.c"
#include "ble_server.c"
#include "ble_s3_server.c"
//#include "ble_s3_server(old).c" //fixme remove

#include "esp_touch_util.h"
#include "wifi_connect_util.h"

#include "utils/database_util.h"

#define IDF_TARGET      (CONFIG_IDF_TARGET)

#define ESP32_S3_TARGET "esp32s3"
#define ESP32_S2_TARGET "esp32s2"

const static char* TAG_MAIN = "MAIN";

//void init_rsa_key() {
//    ESP_LOGI(TAG_MAIN, "Initializing SPIFFS");
//
//    esp_vfs_spiffs_conf_t conf = {
//            .base_path = "/spiffs",
//            .partition_label = NULL,
//            .max_files = 5,
//            .format_if_mount_failed = false
//    };
//
//    // Use settings defined above to initialize and mount SPIFFS filesystem.
//    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
//    esp_err_t ret = esp_vfs_spiffs_register(&conf);
//
//    if (ret != ESP_OK) {
//        if (ret == ESP_FAIL) {
//            ESP_LOGE(TAG_MAIN, "Failed to mount or format filesystem");
//        } else if (ret == ESP_ERR_NOT_FOUND) {
//            ESP_LOGE(TAG_MAIN, "Failed to find SPIFFS partition");
//        } else {
//            ESP_LOGE(TAG_MAIN, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
//        }
//        return;
//    }
//
//
//    size_t total = 0, used = 0;
//    ret = esp_spiffs_info(NULL, &total, &used);
//    if (ret != ESP_OK) {
//        ESP_LOGE(TAG_MAIN, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
//    } else {
//        ESP_LOGI(TAG_MAIN, "Partition size: total: %d, used: %d", (int)total, (int)used);
//    }
//
//
//    ESP_LOGI(TAG_MAIN, "Reading private.key");
//
//    // Open for reading private.key
//    FILE* f_key = fopen("/spiffs/private.key", "r");
//    if (f_key == NULL) {
//        ESP_LOGE(TAG_MAIN, "Failed to open private.key");
//        return;
//    }
//    size_t buf_size = 2048;
//    char* buf = malloc(buf_size);
//    memset(buf, 0, buf_size);
//    fread(buf, 1, buf_size, f_key);
//    fclose(f_key);
//
//    // Display the read contents from the file
//    ESP_LOGI(TAG_MAIN, "Read from private.key:\n%s", buf);
//
//    set_rsa_private_key(buf);
//
//    // Open for reading certificate.crt
//    FILE* f_cert = fopen("/spiffs/certificate.crt", "r");
//    if (f_cert == NULL) {
//        ESP_LOGE(TAG_MAIN, "Failed to open certificate.crt");
//        return;
//    }
//
//    memset(buf, 0, buf_size);
//    fread(buf, 1, buf_size, f_cert);
//    fclose(f_cert);
//
//    ESP_LOGI(TAG_MAIN, "Read from cert:\n%s", buf);
//
////    register_lock(buf);
//
//    // All done, unmount partition and disable SPIFFS
//    esp_vfs_spiffs_unregister(NULL);
//    ESP_LOGI(TAG_MAIN, "SPIFFS unmounted");
//}

void app_main(void) {

    ESP_ERROR_CHECK(nvs_flash_init());

    wifi_config_t wifiConfig;
    if (get_saved_wifi(&wifiConfig) == ESP_OK) {
        if(!connect_to_wifi(wifiConfig)) try_to_connect_to_wifi_esp_touch();
    } else {
        try_to_connect_to_wifi_esp_touch();
    }

    obtain_time();

//    init_rsa_key();

    tcp_main();
//    restart_esp(3); // fixme remove

//    create_invite(1, admin, -1, -1, NULL, -1);
//    delete_authorization("I9CUJwR1u2XK0fJ");
    if (strcmp(IDF_TARGET, ESP32_S2_TARGET) == 0) {
        ble_main();
    } else {
        ble_s3_main();
    }

    ESP_LOGI(TAG_MAIN, "Chega aqui!");

    set_lock_status(idle);
}