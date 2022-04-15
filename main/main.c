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

const static char* TAG = "MAIN";

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

    char* invite_id = create_invite(1649787416, admin, 1649787416, 1649787416);

    if (invite_id != NULL) ESP_LOGE(TAG, "%s", invite_id);
    ble_main();
    set_lock_status(idle);
    
}