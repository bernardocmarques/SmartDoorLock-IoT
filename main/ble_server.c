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
#include "utils/utils.h"
#include "base64.h"
//
#define TXD_PIN (CONFIG_EXAMPLE_UART_TXD)
#define RXD_PIN (CONFIG_EXAMPLE_UART_RXD)

#define ECHO_UART_PORT_NUM      (CONFIG_EXAMPLE_UART_PORT_NUM)
#define ECHO_UART_BAUD_RATE     (CONFIG_EXAMPLE_UART_BAUD_RATE)
#define ECHO_TASK_STACK_SIZE    4096

const char TAG_BLE[] = "BLE_SERVER";

#define BUF_SIZE (1024)

char* ble_user = "BLE_USER";



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
    ESP_LOGI("Data Sent: ", "Wrote %d bytes -> %s", txBytes, data);

    return txBytes;
}

void disconnect() {
    set_user_state(ble_user, DISCONNECTED);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    sendData("AT+DISC\r\n");
    vTaskDelay(100 / portTICK_PERIOD_MS);
}


static void echo_task(void *arg) {
    // Configure a temporary buffer for the incoming data
    char* data = (char *) malloc(BUF_SIZE);
    char* response;

    set_user_state(ble_user, CONNECTING);


    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, (BUF_SIZE - 1), 500 / portTICK_RATE_MS);
        // Write data back to the UART
        if (len) {
            data[len] = '\0';
            if (strncmp("AT+", (char *) data, 3) == 0 || strncmp("+", (char *) data, 1) == 0) {
                continue;
            }
            ESP_LOGI(TAG_BLE, "Received %d bytes: %s", len, data);

//            user_state_t state = get_user_state(addr_str);
//            esp_aes_context aes;
//            aes = get_user_AES_ctx(addr_str);
//            if (retrieve_session_credentials((char *) data, "teste")) {
//                ESP_LOGI(TAG_BLE, "Got key!");
//                sendData("ok");
////                disconnect();
//                continue;
//            }
//            disconnect();

            user_state_t state = get_user_state(ble_user);
            esp_aes_context aes;
            aes = get_user_AES_ctx(ble_user);

            if (state == CONNECTING) {
                if (retrieve_session_credentials(data, ble_user)) {

                    uint8_t* auth_seed = generate_random_seed(ble_user);

                    size_t base64_size;
                    char* seed_base64 = base64_encode(auth_seed, sizeof(uint8_t) * 10, &base64_size);

                    response = malloc((sizeof(char) * 5) + base64_size);

                    ESP_LOGI(TAG_BLE, "RAC %s", seed_base64); //FIXME remove

                    sprintf(response, "RAC %s", seed_base64);

//                    free(auth_seed);
//                    free(seed_base64);


                    aes = get_user_AES_ctx(ble_user);
                } else {
                    ESP_LOGE(TAG_BLE, "Disconnected by server! (Error getting session key)");
                    disconnect();
                    break;
                }
            } else if (state >= CONNECTED) { // Connected or Authorized
                aes = get_user_AES_ctx(ble_user);
                char* cmd = decrypt_base64_AES(aes, data);
                ESP_LOGI(TAG_BLE, "After Dec: %s", cmd);
                response = checkCommand(cmd, ble_user, 0); // FIXME remove t1 after
            } else {
                ESP_LOGE(TAG_BLE, "Disconnected by server! (Not CONNECTED)");
                disconnect();
                break;
            }


            char* response_enc = encrypt_str_AES(aes, response);

            sendData(response_enc);
//            len = (int) strlen(response_enc);
//
//
//            int to_write = len;
//            while (to_write > 0) {
//                int written = send(sock, response_enc + (len - to_write), to_write, 0);
//                if (written < 0) {
//                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
//                }
//                to_write -= written;
//            }

//            if (strcmp(response, NAK_MESSAGE) == 0) {
//                disconnect();
//                break;
//            }


//            sprintf(response, "DATA-> %s", data);


        }
    }
}




void ble_main(void) {
    init();
    xTaskCreate(echo_task, "uart_echo_task", ECHO_TASK_STACK_SIZE, (void*)NULL, 10, NULL);
//    xTaskCreate(echo_task, "uart_echo_task", ECHO_TASK_STACK_SIZE, (void*)NULL, 10, &bleTask);
}
