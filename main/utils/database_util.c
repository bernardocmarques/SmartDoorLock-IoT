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
#include "nvs_util.h"


const char* base_url = "http://192.168.1.7:5000";

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
//            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
//            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
//            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
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
//            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
//            int mbedtls_err = 0;
//            esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
//            if (err != 0) {
//                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
//                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
//            }
//            if (output_buffer != NULL) {
//                free(output_buffer);
//                output_buffer = NULL;
//            }
//            output_len = 0;
            break;
//        case HTTP_EVENT_REDIRECT:
//            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
//            esp_http_client_set_header(evt->client, "From", "user@example.com");
//            esp_http_client_set_header(evt->client, "Accept", "text/html");
//            break;
    }
    return ESP_OK;
}

char* create_invite(int expiration, enum userType user_type, int valid_from, int valid_until, char* weekdays_str, int one_day) {
    cJSON* invite_json  = cJSON_CreateObject();

    uint8_t mac_array[6] = {0};
    char mac[18];
    esp_err_t err = esp_read_mac(mac_array, ESP_MAC_WIFI_STA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error: Could not get MAC Address");
    }
    sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x", mac_array[0], mac_array[1], mac_array[2], mac_array[3], mac_array[4], mac_array[5]);


    cJSON_AddItemToObjectCS(invite_json, "smart_lock_MAC", cJSON_CreateString(mac));
    cJSON_AddItemToObjectCS(invite_json, "expiration", cJSON_CreateNumber(expiration));
    cJSON_AddItemToObjectCS(invite_json, "type", cJSON_CreateNumber(user_type));
    if (valid_from != -1 ) cJSON_AddItemToObjectCS(invite_json, "valid_from", cJSON_CreateNumber(valid_from));
    if (valid_from != -1 ) cJSON_AddItemToObjectCS(invite_json, "valid_until", cJSON_CreateNumber(valid_until));
    if (weekdays_str != NULL ) cJSON_AddItemToObjectCS(invite_json, "weekdays_str", cJSON_CreateString(weekdays_str));
    if (one_day != -1 ) cJSON_AddItemToObjectCS(invite_json, "one_day", cJSON_CreateNumber(one_day));

    char* invite = cJSON_PrintUnformatted(invite_json);

    char* signature = sign_RSA(invite);

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
    err = esp_http_client_perform(client);
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
            ESP_LOGE(TAG, "Create invite: Error Code %d: %s", code, message);
        }

    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }


    return NULL;
}

esp_err_t get_authorization_db(char* username, authorization* auth) {
    cJSON* authorization_request_json  = cJSON_CreateObject();

    uint8_t mac_array[6] = {0};
    char mac[18];
    esp_err_t err = esp_read_mac(mac_array, ESP_MAC_WIFI_STA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error: Could not get MAC Address");
    }
    sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x", mac_array[0], mac_array[1], mac_array[2], mac_array[3], mac_array[4], mac_array[5]);


    cJSON_AddItemToObjectCS(authorization_request_json, "smart_lock_MAC", cJSON_CreateString(mac));
    cJSON_AddItemToObjectCS(authorization_request_json, "username", cJSON_CreateString(username));

    char* authorization_request = cJSON_PrintUnformatted(authorization_request_json);

    char* signature = sign_RSA(authorization_request);

    char local_response_buffer[1000] = {0};

    esp_http_client_config_t config = {
            .host = base_url,
            .path = "/",
            .event_handler = _http_event_handler,
            .user_data = local_response_buffer
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);


    // POST
    char* path = "/request-authorization";
    char* url = malloc(sizeof(char) * (strlen(base_url) + strlen(path)));


    sprintf(url, "%s%s", base_url, path);
    esp_http_client_set_url(client, url);


    cJSON* post_data_json  = cJSON_CreateObject();

    cJSON_AddItemToObjectCS(post_data_json, "data", cJSON_CreateString(authorization_request));
    cJSON_AddItemToObjectCS(post_data_json, "signature", cJSON_CreateString(signature));

    char* post_data = cJSON_PrintUnformatted(post_data_json);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, (int)strlen(post_data));
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        cJSON* result_json = cJSON_Parse(local_response_buffer);

        bool success = cJSON_IsTrue(cJSON_GetObjectItem(result_json, "success"));

        if (success) {
            cJSON* data_json = cJSON_GetObjectItem(result_json, "data");

            strcpy(auth->username, cJSON_GetStringValue(cJSON_GetObjectItem(data_json, "username")));
            auth->user_type = (enum userType) cJSON_GetNumberValue(cJSON_GetObjectItem(data_json, "type"));

            char* master_key_base64 = decrypt_base64_RSA(cJSON_GetStringValue(cJSON_GetObjectItem(data_json, "master_key_encrypted_lock")));

            size_t size;
            int iv_base64_size = (int)(strlen(master_key_base64)/sizeof(char));
            uint8_t* master_key = base64_decode(master_key_base64, iv_base64_size, &size);
            memcpy(auth->master_key, master_key, size);

            err = set_authorization(auth);

            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Error setting authorization!");
                ESP_LOGE(TAG, "%s", esp_err_to_name(err));
            }


            return err;
        } else {
            int code = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(result_json, "code"));
            char* message = cJSON_GetStringValue(cJSON_GetObjectItem(result_json, "msg"));
            ESP_LOGE(TAG, "Get Authorization: Error Code %d: %s", code, message);
            return ERR_ARG; //fixme maybe fix error type
        }

    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    return err;
}

//
//void register_lock(char* certificate) {
//    cJSON* post_data_json  = cJSON_CreateObject();
//
//    uint8_t mac_array[6] = {0};
//    char mac[18];
//    esp_err_t err = esp_read_mac(mac_array, ESP_MAC_WIFI_STA);
//    if (err != ESP_OK) {
//        ESP_LOGE(TAG, "Error: Could not get MAC Address");
//    }
//    sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x", mac_array[0], mac_array[1], mac_array[2], mac_array[3], mac_array[4], mac_array[5]);
//
//
//    cJSON_AddItemToObjectCS(post_data_json, "MAC", cJSON_CreateString(mac));
//    cJSON_AddItemToObjectCS(post_data_json, "certificate", cJSON_CreateString(certificate));
//    free(certificate);
//    char local_response_buffer[500] = {0};
//
//    esp_http_client_config_t config = {
//            .host = base_url,
//            .path = "/",
//            .event_handler = _http_event_handler,
//            .user_data = local_response_buffer
//    };
//    esp_http_client_handle_t client = esp_http_client_init(&config);
//
//
//    // POST
//    char* path = "/register-door-lock";
//    char* url = malloc(sizeof(char) * (strlen(base_url) + strlen(path)));
//
//
//    sprintf(url, "%s%s", base_url, path);
//    esp_http_client_set_url(client, url);
//
//
//
//
//    char* post_data = cJSON_PrintUnformatted(post_data_json);
//
//    ESP_LOGE(TAG, "%s", post_data);
//
//    esp_http_client_set_method(client, HTTP_METHOD_POST);
//    esp_http_client_set_header(client, "Content-Type", "application/json");
//    esp_http_client_set_post_field(client, post_data, (int)strlen(post_data));
//    err = esp_http_client_perform(client);
//
//    free(post_data);
////    cJSON_free(post_data_json);
//    if (err == ESP_OK) {
//        cJSON* result_json = cJSON_Parse(local_response_buffer);
//
//
//        bool success = cJSON_IsTrue(cJSON_GetObjectItem(result_json, "success"));
//
//        if (success) {
//            ESP_LOGI(TAG, "Lock registered in database.");
//        } else {
//            int code = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(result_json, "code"));
//            char* message = cJSON_GetStringValue(cJSON_GetObjectItem(result_json, "msg"));
//            ESP_LOGE(TAG, "Error Code %d: %s", code, message);
//        }
////        cJSON_free(result_json);
//
//
//    } else {
//        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
//    }
//
//}
