#include <string.h>
#include <stdlib.h>
#include <esp_spiffs.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_sleep.h"

#include "tcp_server.c"
#include "ble_server.c"
#if CONFIG_IDF_TARGET_ESP32S3
#include "ble_s3_server.c"
#endif

#include "esp_touch_util.h"
#include "wifi_connect_util.h"

#include "utils/database_util.h"
#include "rsa_util.h"
#include "power_util.h"

#define IDF_TARGET      (CONFIG_IDF_TARGET)

#define ESP32_S3_TARGET "esp32s3"
#define ESP32_S2_TARGET "esp32s2"

const static char* TAG_MAIN = "MAIN";

void init_rsa_key() {
    print_rsa_key();

    ESP_LOGI(TAG_MAIN, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
            .base_path = "/spiffs",
            .partition_label = NULL,
            .max_files = 5,
            .format_if_mount_failed = false
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG_MAIN, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG_MAIN, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG_MAIN, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }


    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_MAIN, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG_MAIN, "Partition size: total: %d, used: %d", (int)total, (int)used);
    }


    ESP_LOGI(TAG_MAIN, "Reading private.key");

    // Open for reading private.key
    FILE* f_key = fopen("/spiffs/private.key", "r");
    if (f_key == NULL) {
        ESP_LOGE(TAG_MAIN, "Failed to open private.key");
        return;
    }
    size_t buf_size = 2048;
    char* buf = malloc(buf_size);
    memset(buf, 0, buf_size);
    fread(buf, 1, buf_size, f_key);
    fclose(f_key);

    set_rsa_private_key(buf);

    if (get_registration_status() == NOT_REGISTERED) {
        // Open for reading certificate.crt
        FILE* f_cert = fopen("/spiffs/certificate.crt", "r");
        if (f_cert == NULL) {
            ESP_LOGE(TAG_MAIN, "Failed to open certificate.crt");
            return;
        }

        char ble_mac[18];


        if (strcmp(IDF_TARGET, ESP32_S2_TARGET) == 0) {
            // todo get ble addrs if esp32-s2
        } else {
            uint8_t ble_mac_array[6] = {0};

            esp_err_t err = esp_read_mac(ble_mac_array, ESP_MAC_BT);
            if (err != ESP_OK) {
                ESP_LOGE(TAG_MAIN, "Error: Could not get BLE Address");
            }
            sprintf(ble_mac, "%02x:%02x:%02x:%02x:%02x:%02x", ble_mac_array[0], ble_mac_array[1], ble_mac_array[2], ble_mac_array[3], ble_mac_array[4], ble_mac_array[5]);
        }


        memset(buf, 0, buf_size);
        fread(buf, 1, buf_size, f_cert);
        fclose(f_cert);

        register_lock(buf, ble_mac);
        force_get_registration_status();
    }

    // All done, unmount partition and disable SPIFFS
    esp_vfs_spiffs_unregister(NULL);
    ESP_LOGI(TAG_MAIN, "SPIFFS unmounted");
    print_rsa_key();

}

void init_led_lock_state() {
    lock_state_t lock_state = get_lock_state();

    if (lock_state == locked) {
        lock_lock();
    } else if (lock_state == unlocked) {
        unlock_lock();
    }
}


void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());

    bool wakeup_from_deep_sleep = (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER);

    ESP_LOGE(TAG_MAIN, "%s", wakeup_from_deep_sleep ? "Wake up from deep sleep" : "First Boot");

    if (!wakeup_from_deep_sleep) {
//        if (gpio_get_level(14) != 0) { // if LOW normal behavior, if HIGH reset memory
//        delete_saved_wifi(); // fixme remove
//        delete_authorization("T8hA27vnH0wdRn3"); // fixme remove
//        ESP_LOGE(TAG_MAIN, "reset"); // fixme remove
//        delete_authorization("AA4PFbrPYOpq7fe"); // fixme remove
//    restart_esp(3); // fixme remove
//        }
    }

    if (!wakeup_from_deep_sleep) {
        if (strcmp(IDF_TARGET, ESP32_S2_TARGET) == 0) {
            // pass
        } else {
            #if CONFIG_IDF_TARGET_ESP32S3
            ble_s3_main();
            #endif
        }
    }

    wifi_config_t wifiConfig;
    if (get_saved_wifi(&wifiConfig) == ESP_OK) {
        if(!connect_to_wifi(wifiConfig)) {
            try_to_connect_to_wifi_esp_touch();
        }
    } else {
        try_to_connect_to_wifi_esp_touch();
    }

    set_led_status(led_idle);

    obtain_time();
    if (!wakeup_from_deep_sleep) init_rsa_key();

    tcp_main();

    setWifiConnected(true);

    if (strcmp(IDF_TARGET, ESP32_S2_TARGET) == 0) {
        ble_main();
    } else {
        #if CONFIG_IDF_TARGET_ESP32S3
        server_online = true;
        if (!wakeup_from_deep_sleep) {
            send_data_ble_s3("LOK");
        } else {
            ble_s3_main();
        }
        #endif
    }




//    heap_caps_check_integrity_all(1); // fixme remove

//    restart_esp(3); // fixme remove

//    create_invite(1, admin, -1, -1, NULL, -1);

    init_led_lock_state();

    if (get_registration_status() == COMPLETE) start_deep_sleep_timer(DEFAULT_SLEEP_DELAY, DEFAULT_SLEEP_TIME);
    ESP_LOGI(TAG_MAIN, "All components initialized, system working!");


}