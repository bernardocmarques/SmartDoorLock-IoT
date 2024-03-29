#include <sys/cdefs.h>
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
#include "base64_util.h"
#include "wifi_connect_util.h"
//
#define TXD_PIN (CONFIG_EXAMPLE_UART_TXD)
#define RXD_PIN (CONFIG_EXAMPLE_UART_RXD)

#define ECHO_UART_PORT_NUM      (CONFIG_EXAMPLE_UART_PORT_NUM)
#define ECHO_UART_BAUD_RATE     (CONFIG_EXAMPLE_UART_BAUD_RATE)
#define ECHO_TASK_STACK_SIZE    4096

const char TAG_BLE[] = "BLE_SERVER";

#define BUF_SIZE (512)




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

    set_user_state(ble_user, DISCONNECTED, "");
}

int sendATCmd(char* AT_cmd) {
    size_t len = strlen(AT_cmd);

    const int txBytes = uart_write_bytes(ECHO_UART_PORT_NUM, AT_cmd, len);
    ESP_LOGI("Data Sent: ", "Wrote %d bytes -> %s", txBytes, AT_cmd);

    return txBytes;
}

int sendData(char* data) {
    char EOT = '\4';
    strncat(data, &EOT, 1);

    size_t len = strlen(data);

    const int txBytes = uart_write_bytes(ECHO_UART_PORT_NUM, data, len);
    ESP_LOGI("Data Sent: ", "Wrote %d bytes -> %s", txBytes, data);

    return txBytes;
}

void disconnect() {
    set_user_state(ble_user, DISCONNECTED, "");
    vTaskDelay(100 / portTICK_PERIOD_MS);
    sendATCmd("AT+DISC\r\n");
    vTaskDelay(100 / portTICK_PERIOD_MS);
    disconnect_lock();
}

bool got_first_invite_session_key = false;

_Noreturn static void echo_task(void *arg) {



    // Configure a temporary buffer for the incoming data
    char* data = (char *) malloc(BUF_SIZE);
    char* response;
    char* response_enc;
    char* response_ts;

    ESP_LOGI(TAG_BLE, "Server BLE running!");

    while (1) {

        if (strcmp(data, "PNG") == 0) {
            sendData("LOK");
            continue;
        }

        if (!isWifiConnected()) continue;


        // Read data from the UART
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, (BUF_SIZE - 1), 250 / portTICK_RATE_MS);
        // Write data back to the UART
        if (len) {
            data[len] = '\0';
            if (strncmp("AT+", (char *) data, 3) == 0 || strncmp("+", (char *) data, 1) == 0) {
                ESP_LOGI(TAG_BLE, "Received AT cmd: %s", data);
                if (strncmp("+CONNECTED", (char *) data, 10) == 0) {
                    set_user_state(ble_user, CONNECTING, "");
                    cancel_deep_sleep_timer();
                    set_current_BLE_addr(ble_user);
                } else if (strncmp("+DISCONNECT", (char *) data, 13) == 0) {
                    set_user_state(ble_user, DISCONNECTED, "");
                    set_current_BLE_addr("");
                    disconnect_lock();
                    start_deep_sleep_timer(DEFAULT_SLEEP_DELAY, DEFAULT_SLEEP_TIME);
                }

                continue;
            }

            if (strncmp("-", (char *) data, 1) == 0) {
                data[0] = '+'; // replace '-' with '+', this is used to prevent base64 string started with '+' to be handled as an AT command response
            }
            ESP_LOGI(TAG_BLE, "Received %d bytes: %s", len, data);


            esp_aes_context aes;
            aes = get_user_AES_ctx(ble_user);


            if (get_registration_status() == REGISTERED) {
                ESP_LOGI(TAG_BLE, "Not auth, First comm");

                if (!got_first_invite_session_key) {
                    if (retrieve_session_credentials(data, ble_user)) {
                        response = "ACK";

                        aes = get_user_AES_ctx(ble_user);
                        got_first_invite_session_key = true;
                    }  else {
                        ESP_LOGE(TAG_BLE, "Disconnected by server! (Error getting session key)");
                        disconnect();
                        continue;
                    }
                } else { // got_first_invite_session_key
                    got_first_invite_session_key = false;
                    aes = get_user_AES_ctx(ble_user);

                    char* cmd = decrypt_base64_AES(aes, data);

                    ESP_LOGI(TAG_BLE, "After Dec: %s", cmd);
                    response = checkCommand(cmd, ble_user);
                }

                ESP_LOGI(TAG_BLE, "resp -> %s", response);
                response_ts = addTimestampsAndNonceToMsg(response);

                response_enc = encrypt_str_AES(aes, response_ts);
                sendData(response_enc);
                continue;

            }




            user_state_t state = get_user_state(ble_user);


            ESP_LOGW(TAG_BLE, "Current state-> %d", state);
            if (state == CONNECTING) {
                if (retrieve_session_credentials(data, ble_user)) {

                    uint8_t* auth_seed = generate_random_seed(ble_user);

                    size_t base64_size;
                    char* seed_base64 = base64_encode(auth_seed, sizeof(uint8_t) * 10, &base64_size);

                    response = malloc((sizeof(char) * 5) + base64_size);


                    sprintf(response, "RAC %s", seed_base64);

//                    free(auth_seed);
//                    free(seed_base64);


                    aes = get_user_AES_ctx(ble_user);
                } else {
                    ESP_LOGE(TAG_BLE, "Disconnected by server! (Error getting session key)");
                    disconnect();
                    continue;
                }
            } else if (state >= CONNECTED) { // Connected or Authorized
                aes = get_user_AES_ctx(ble_user);
                char* cmd = decrypt_base64_AES(aes, data);
                ESP_LOGI(TAG_BLE, "After Dec: %s", cmd);
                response = checkCommand(cmd, ble_user);
            } else {
                ESP_LOGE(TAG_BLE, "Disconnected by server! (Not CONNECTED)");
                disconnect();
                continue;
            }



            response_ts = addTimestampsAndNonceToMsg(response);
            response_enc = encrypt_str_AES(aes, response_ts);
            len = (int)strlen(response_enc);
            sendData(response_enc);
        }

    }
}


void createTaskBLE() {
    xTaskCreate(echo_task, "uart_echo_task", ECHO_TASK_STACK_SIZE, (void*)NULL, 10, NULL);
}


void ble_main(void) {
    init();
    createTaskBLE();
//        xTaskCreate(stupid, "uart_echo_task", ECHO_TASK_STACK_SIZE, (void*)NULL, 10, &bleTask);
}
