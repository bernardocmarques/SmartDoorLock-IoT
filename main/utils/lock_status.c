#include <esp_log.h>
#include "lock_status.h"
#include "led_strip.h"


lock_status_t status = idle;


static led_strip_t *pStrip_a;
int led_configured = 0;


const int red[3] = {25,0,0};
const int green[3] = {0,25,0};
const int blue[3] = {0,0,25};
const int yellow[3] = {25,25,0};
int color_from_status[3][3]  = {{25,25,0}, {0,0,25}, {0,25,0}};


static void configure_led() {
    char* TAG = "LEE Configuration";
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    pStrip_a = led_strip_init(1, 18, 1);
    /* Set all LED off to clear all pixels */
    pStrip_a->clear(pStrip_a, 50);
    ESP_LOGI(TAG, "LED config done");
}

static void change_led_color(int rgb[3]) {
    pStrip_a->clear(pStrip_a, 50);
    pStrip_a->set_pixel(pStrip_a, 0, rgb[0], rgb[1], rgb[3]);
    pStrip_a->refresh(pStrip_a, 100);
}

void update_led_color() {
    if (led_configured == 0) {
        configure_led();
    }

    change_led_color(color_from_status[status]);
}

void set_lock_status(lock_status_t s) {
    status = s;
    update_led_color();
}

void lock_lock() {
    set_lock_status(locked);
}

void unlock_lock() {
    set_lock_status(unlocked);
}

void disconnect_lock() {
    set_lock_status(unlocked);
}