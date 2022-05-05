#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "tcp_server.c"
#include "ble_server.c"
#include "esp_touch_util.h"
#include "wifi_connect_util.h"

#include "utils/database_util.h"

#define IDF_TARGET      (CONFIG_IDF_TARGET)

#define ESP32_S3_TARGET "esp32s3"
#define ESP32_S2_TARGET "esp32s2"

//const static char* TAG = "MAIN";

void app_main(void) {

    ESP_ERROR_CHECK(nvs_flash_init());

    wifi_config_t wifiConfig;
    if (get_saved_wifi(&wifiConfig) == ESP_OK) {
        if(!connect_to_wifi(wifiConfig)) try_to_connect_to_wifi_esp_touch();
    } else {
        try_to_connect_to_wifi_esp_touch();
    }

    obtain_time();

    tcp_main();
//    restart_esp(3); // fixme remove

    create_invite(1, admin, -1, -1, NULL, -1);

    if (strcmp(IDF_TARGET, ESP32_S2_TARGET) == 0) {
        ble_main();
    } else {
        ESP_LOGI("main", "Esp3");
    }



    set_lock_status(idle);
}