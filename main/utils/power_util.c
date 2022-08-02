#include <esp_sleep.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <sys/unistd.h>
#include "power_util.h"

static const char *TAG = "POWER_UTIL";

esp_timer_handle_t deep_sleep_timer;

static void deep_sleep_timer_callback(void* arg) {
    int seconds = (int) (size_t) arg;
    deep_sleep_for_n_seconds(seconds);
}

void deep_sleep_for_n_seconds(int seconds) {
    ESP_LOGI(TAG, "Enabling timer wakeup, %ds\n", seconds);
    esp_sleep_enable_timer_wakeup(seconds * 1000000);

    esp_deep_sleep_start();
}

void start_deep_sleep_timer(int delay_seconds, int sleep_time_seconds) {
    ESP_LOGI(TAG, "Going to deep sleep in %d seconds\n", delay_seconds);

    esp_timer_create_args_t deep_sleep_timer_args = {
            .callback = &deep_sleep_timer_callback,
            /* argument specified here will be passed to timer callback function */
            .arg =  (void *) (size_t) sleep_time_seconds,
            .name = "deep-sleep-timer"
    };

    ESP_ERROR_CHECK(esp_timer_create(&deep_sleep_timer_args, &deep_sleep_timer));
    ESP_ERROR_CHECK(esp_timer_start_once(deep_sleep_timer, delay_seconds * 1000000));
}