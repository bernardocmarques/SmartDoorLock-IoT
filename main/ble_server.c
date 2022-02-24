#include <sys/queue.h>
#include <sys/cdefs.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
//
#define TXD_PIN (CONFIG_EXAMPLE_UART_TXD)
#define RXD_PIN (CONFIG_EXAMPLE_UART_RXD)

#define ECHO_UART_PORT_NUM      (CONFIG_EXAMPLE_UART_PORT_NUM)
#define ECHO_UART_BAUD_RATE     (CONFIG_EXAMPLE_UART_BAUD_RATE)
#define ECHO_TASK_STACK_SIZE    (CONFIG_EXAMPLE_TASK_STACK_SIZE)

static const char *TAG_BLE = "BLE_SERVER";

#define BUF_SIZE (1024)


void init(void) {
    const uart_config_t uart_config = {
            .baud_rate = ECHO_UART_BAUD_RATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_APB,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(ECHO_UART_PORT_NUM, &uart_config);
    uart_set_pin(ECHO_UART_PORT_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

int sendData(const char* data) {
    const size_t len = strlen(data);
    const int txBytes = uart_write_bytes(ECHO_UART_PORT_NUM, data, len);
    ESP_LOGI("Data Sent: ", "Wrote %d bytes", txBytes);
    return txBytes;
}

void disconnect() {
    vTaskDelay(100 / portTICK_PERIOD_MS);
    sendData("AT+DISC\r\n");
    vTaskDelay(100 / portTICK_PERIOD_MS);
}


_Noreturn static void echo_task(void *arg) {
    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    char* response = (char *) malloc(BUF_SIZE);

    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, (BUF_SIZE - 1), 500 / portTICK_RATE_MS);
        // Write data back to the UART
        if (len) {
            data[len] = '\0';
//            if (strncmp("AT+", (char *) data, 3) == 0 || strncmp("+", (char *) data, 1) == 0) {
//                continue;
//            }
            ESP_LOGI(TAG_BLE, "Received %d bytes: %s", len, (char *) data);

//            user_state_t state = get_user_state(addr_str);
//            esp_aes_context aes;
//            aes = get_user_AES_ctx(addr_str);


            sprintf(response, "DATA-> %s", (char *) data);

            sendData((char *) data);
//            disconnect();
        }
    }
}



void app_main_ble(void) {
    init();
    xTaskCreate(echo_task, "uart_echo_task", ECHO_TASK_STACK_SIZE, (void*)NULL, 10, NULL);
}
