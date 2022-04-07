#include <esp_log.h>
#include "lock_status.h"
#include "led_strip.h"

char* TAG_LED = "LED";

lock_status_t status = idle;


static led_strip_t *pStrip_a;
int led_configured = 0;


const int red[3] = {10,0,0};
const int green[3] = {0,10,0};
const int blue[3] = {0,0,10};
const int yellow[3] = {10,10,0};
int color_from_status[3][3]  = {{10,10,0}, {0,0,10}, {0,10,0}};


static void configure_led() {
    if (led_configured != 0) {
        return;
    }
    led_configured = 1;
    ESP_LOGI(TAG_LED, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    pStrip_a = led_strip_init(0, 18, 1);
    /* Set all LED off to clear all pixels */
    pStrip_a->clear(pStrip_a, 50);
    ESP_LOGI(TAG_LED, "LED config done");
}

static void change_led_color(int rgb[3]) {
    ESP_LOGI(TAG_LED, "Change color to (%d, %d, %d)", rgb[0], rgb[1], rgb[2]);
//    pStrip_a->clear(pStrip_a, 50);
    pStrip_a->set_pixel(pStrip_a, 0, rgb[0], rgb[1], rgb[2]);
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
    set_lock_status(idle);
}
