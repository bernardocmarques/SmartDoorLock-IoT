#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include <esp_http_client.h>
#include <esp_tls.h>
#include "database_util.h"
#include "authorization.h"
#include "rsa_util.h"
#include "cJSON.h"
#include "base64_util.h"


const char* base_url = "http://192.168.1.154:5000";

static const char *TAG = "Database_Util";

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                if (evt->user_data) {
                    memcpy(evt->user_data + output_len, evt->data, evt->data_len);
                } else {
                    if (output_buffer == NULL) {
                        output_buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    memcpy(output_buffer + output_len, evt->data, evt->data_len);
                }
                output_len += evt->data_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
//        case HTTP_EVENT_REDIRECT:
//            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
//            esp_http_client_set_header(evt->client, "From", "user@example.com");
//            esp_http_client_set_header(evt->client, "Accept", "text/html");
//            break;
    }
    return ESP_OK;
}

char* create_invite(int expiration, enum user_type userType, int validFrom, int validUntil) {
    cJSON* invite_json  = cJSON_CreateObject();

    cJSON_AddItemToObjectCS(invite_json, "doorMAC", cJSON_CreateString("aa:bb:ee:ff:dd:aa"));
    cJSON_AddItemToObjectCS(invite_json, "expiration", cJSON_CreateNumber(expiration));
    cJSON_AddItemToObjectCS(invite_json, "type", cJSON_CreateNumber(userType));
    cJSON_AddItemToObjectCS(invite_json, "valid_from", cJSON_CreateNumber(validFrom));
    cJSON_AddItemToObjectCS(invite_json, "valid_until", cJSON_CreateNumber(validUntil));

    char* invite = cJSON_PrintUnformatted(invite_json);

//    char* signature = sign_RSA(invite);
    char* signature = sign_RSA("invite");

    char local_response_buffer[500] = {0};



    esp_http_client_config_t config = {
            .host = base_url,
            .path = "/",
            .event_handler = _http_event_handler,
            .user_data = local_response_buffer
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);


    // POST
    char* path = "/register-invite";
    char* url = malloc(sizeof(char) * (strlen(base_url) + strlen(path)));


    sprintf(url, "%s%s", base_url, path);
    esp_http_client_set_url(client, url);


    cJSON* post_data_json  = cJSON_CreateObject();

    cJSON_AddItemToObjectCS(post_data_json, "data", cJSON_CreateString(invite));
    cJSON_AddItemToObjectCS(post_data_json, "signature", cJSON_CreateString(signature));

    char* post_data = cJSON_PrintUnformatted(post_data_json);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, (int)strlen(post_data));
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        cJSON* result_json = cJSON_Parse(local_response_buffer);


        bool success = cJSON_IsTrue(cJSON_GetObjectItem(result_json, "success"));

        if (success) {
            char* inviteID = cJSON_GetStringValue(cJSON_GetObjectItem(result_json, "inviteID"));
            ESP_LOGI(TAG, "Invite registered with id: %s", inviteID);
            return inviteID;


        } else {
            int code = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(result_json, "code"));
            char* message = cJSON_GetStringValue(cJSON_GetObjectItem(result_json, "msg"));
            ESP_LOGE(TAG, "Error Code %d: %s", code, message);
        }

    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }


    return NULL;
}
