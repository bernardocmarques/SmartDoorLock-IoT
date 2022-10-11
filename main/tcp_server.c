/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include <lwip/apps/sntp.h>

#include "tcp_server_handler.c"
#include "utils/utils.h"
#include "utils/aes_util.h"
#include "utils/base64_util.h"
#include "utils/nvs_util.h"
#include "utils/user_info.h"
#include "utils/pushingbox_util.h"



#include "time_util.h"
#include "power_util.h"

#define PORT                        CONFIG_EXAMPLE_PORT
#define KEEPALIVE_IDLE              CONFIG_EXAMPLE_KEEPALIVE_IDLE
#define KEEPALIVE_INTERVAL          CONFIG_EXAMPLE_KEEPALIVE_INTERVAL
#define KEEPALIVE_COUNT             CONFIG_EXAMPLE_KEEPALIVE_COUNT

const static char* TAG_TCP = "TAG_TCP";


char addr_str[45];

void createTaskBLE();

static void disconnect_sock(const int sock) {
//    createTaskBLE();
    set_user_state(addr_str, DISCONNECTED, "");
    free_AES(get_user_AES_ctx(addr_str));
    close(sock);
    disconnect_lock();
    start_deep_sleep_timer(DEFAULT_SLEEP_DELAY, DEFAULT_SLEEP_TIME);
}

static void do_retransmit(const int sock) {
    int len;
    char rx_buffer[512];

//    vTaskDelete(bleTask); // fixme maybe suspend anyway

    do {
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) {
            ESP_LOGE(TAG_TCP, "Error occurred during receiving: errno %d", errno);
        } else if (len == 0) {
            ESP_LOGW(TAG_TCP, "Connection closed");
        } else {
            rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string
            ESP_LOGI(TAG_TCP, "Received %d bytes: %s", len, rx_buffer);

            user_state_t state = get_user_state(addr_str);
            esp_aes_context aes;
            aes = get_user_AES_ctx(addr_str);

            char* response;
            if (state == CONNECTING) {
                if (retrieve_session_credentials(rx_buffer, addr_str)) {

                    uint8_t* auth_seed = generate_random_seed(addr_str);
                    
                    size_t base64_size;
                    char* seed_base64 = base64_encode(auth_seed, sizeof(uint8_t) * 10, &base64_size);

                    response = malloc((sizeof(char) * 5) + base64_size);


                    sprintf(response, "RAC %s", seed_base64);

//                    free(auth_seed);
//                    free(seed_base64);


                    aes = get_user_AES_ctx(addr_str);
                } else {
                    ESP_LOGE(TAG_TCP, "Disconnected by server! (Error getting session key)");
                    disconnect_sock(sock);
                    return;
                }
            } else if (state >= CONNECTED) { // Connected or Authorized
                aes = get_user_AES_ctx(addr_str);
                char* cmd = decrypt_base64_AES(aes, rx_buffer);
                ESP_LOGI(TAG_TCP, "After Dec: %s", cmd);
                response = checkCommand(cmd, addr_str);
            } else {
                ESP_LOGE(TAG_TCP, "Disconnected by server! (Not CONNECTED)");
                disconnect_sock(sock);
                return;
            }


            char* response_ts = addTimestampsAndNonceToMsg(response);
            char* response_enc = encrypt_str_AES(aes, response_ts);
            len = (int)strlen(response_enc);


            int to_write = len;
            while (to_write > 0) {
                int written = send(sock, response_enc + (len - to_write), to_write, 0);
                if (written < 0) {
                    ESP_LOGE(TAG_TCP, "Error occurred during sending: errno %d", errno);
                }
                to_write -= written;
            }
            ESP_LOGI(TAG_TCP, "Wrote %d bytes: %s", len, response_enc);

            if (strcmp(response, NAK_MESSAGE) == 0) {
                disconnect_sock(sock);
                return;
            }

        }
    } while (len > 0);
}

static void tcp_server_task(void *pvParameters) {
    int addr_family = (int)pvParameters;
    int ip_protocol = 0;
    int keepAlive = 1;
    int keepIdle = KEEPALIVE_IDLE;
    int keepInterval = KEEPALIVE_INTERVAL;
    int keepCount = KEEPALIVE_COUNT;
    struct sockaddr_storage dest_addr;

    if (addr_family == AF_INET) {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(PORT);
        ip_protocol = IPPROTO_IP;
    }
#ifdef CONFIG_EXAMPLE_IPV6
    else if (addr_family == AF_INET6) {
        struct sockaddr_in6 *dest_addr_ip6 = (struct sockaddr_in6 *)&dest_addr;
        bzero(&dest_addr_ip6->sin6_addr.un, sizeof(dest_addr_ip6->sin6_addr.un));
        dest_addr_ip6->sin6_family = AF_INET6;
        dest_addr_ip6->sin6_port = htons(PORT);
        ip_protocol = IPPROTO_IPV6;
    }
#endif

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG_TCP, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#if defined(CONFIG_EXAMPLE_IPV4) && defined(CONFIG_EXAMPLE_IPV6)
    // Note that by default IPV6 binds to both protocols, it is must be disabled
    // if both protocols used at the same time (used in CI)
    setsockopt(listen_sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
#endif

    ESP_LOGI(TAG_TCP, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG_TCP, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TAG_TCP, "IPPROTO: %d", addr_family);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG_TCP, "Socket bound, port %d", PORT);

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG_TCP, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1) {
        ESP_LOGI(TAG_TCP, "Socket listening");

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG_TCP, "Unable to accept connection: errno %d", errno);
            break;
        }

        // Set tcp keepalive option
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
        // Convert ip address to string
        if (source_addr.ss_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }
#ifdef CONFIG_EXAMPLE_IPV6
        else if (source_addr.ss_family == PF_INET6) {
            inet6_ntoa_r(((struct sockaddr_in6 *)&source_addr)->sin6_addr, addr_str, sizeof(addr_str) - 1);
        }
#endif
        ESP_LOGI(TAG_TCP, "Socket accepted ip address: %s", addr_str);

        cancel_deep_sleep_timer();

        set_user_state(addr_str, CONNECTING, "");

        do_retransmit(sock);

        // free_AES(AES_ctx); //FIXME check this
        shutdown(sock, 0);
        start_deep_sleep_timer(DEFAULT_SLEEP_DELAY, DEFAULT_SLEEP_TIME);
        close(sock);
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}


void tcp_main(void) {
    #ifdef CONFIG_EXAMPLE_IPV4
    xTaskCreate(tcp_server_task, "tcp_server", 8192, (void*)AF_INET, 5, NULL);
    #endif
    #ifdef CONFIG_EXAMPLE_IPV6
     xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET6, 5, NULL);
    #endif
}
