/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "ble_s3_server.h"

#include <sys/cdefs.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sdkconfig.h"
#include "utils/utils.h"
#include "base64_util.h"

#define BLE_S3_TAG  "BLE_S3_SERVER"

#define SPP_PROFILE_NUM                           1
#define SPP_PROFILE_APP_IDX                       0
#define ESP_SPP_APP_ID                            0x55
#define SPP_SVC_INST_ID                    0
#define EXT_ADV_HANDLE                            0
#define NUM_EXT_ADV_SET                           1
#define EXT_ADV_DURATION                          0
#define EXT_ADV_MAX_EVENTS                        0

#define SAMPLE_DEVICE_NAME          "SmartLockBLE-S3"    //The Device Name Characteristics in GAP


#define VERBOSE_LEVEL               2 // fixme change to config

/// SPP Service
static const uint16_t spp_service_uuid = 0xffe0;
/// Characteristic UUID
#define ESP_GATT_UUID_SPP_DATA_RECEIVE      0xffe1
#define ESP_GATT_UUID_SPP_DATA_NOTIFY       0xffe2
#define ESP_GATT_UUID_SPP_COMMAND_RECEIVE   0xffe3
#define ESP_GATT_UUID_SPP_COMMAND_NOTIFY    0xffe4

static uint16_t spp_handle_table[SPP_IDX_NB];

static uint8_t ext_adv_raw_data[] = {
        0x02, 0x01, 0x06,
        0x02, 0x0a, 0xeb, 0x03, 0x03, 0xab, 0xcd,
        0x11, 0X09, 'E', 'S', 'P', '_', 'B', 'L', 'E', '5', '0', '_', 'S', 'E', 'R', 'V', 'E', 'R',
};


static esp_ble_gap_ext_adv_t ext_adv[1] = {
        [0] = {EXT_ADV_HANDLE, EXT_ADV_DURATION, EXT_ADV_MAX_EVENTS},
};

static uint16_t spp_mtu_size = 123;
static uint16_t spp_conn_id = 0xffff;
static esp_gatt_if_t spp_gatts_if = 0xff;
static xQueueHandle cmd_cmd_queue = NULL;

static bool enable_data_ntf = false;
static bool is_connected = false;
static esp_bd_addr_t spp_remote_bda = {0x0,};
static char ble_user_addr[18];

esp_ble_gap_ext_adv_params_t spp_adv_params = {
        .type = ESP_BLE_GAP_SET_EXT_ADV_PROP_CONNECTABLE,
        .interval_min = 0x20,
        .interval_max = 0x20,
        .channel_map = ADV_CHNL_ALL,
        .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
        .primary_phy = ESP_BLE_GAP_PHY_1M,
        .max_skip = 0,
        .secondary_phy = ESP_BLE_GAP_PHY_2M,
        .sid = 0,
        .scan_req_notif = false,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                        esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst heart_rate_profile_tab[SPP_PROFILE_NUM] = {
        [SPP_PROFILE_APP_IDX] = {
                .gatts_cb = gatts_profile_event_handler,
                .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
        },

};

/*
 *  SPP PROFILE ATTRIBUTES
 ****************************************************************************************
 */

#define CHAR_DECLARATION_SIZE   (sizeof(uint8_t))
static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

static const uint8_t char_prop_read_notify = ESP_GATT_CHAR_PROP_BIT_READ|ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t char_prop_read_write = ESP_GATT_CHAR_PROP_BIT_WRITE_NR|ESP_GATT_CHAR_PROP_BIT_READ;


///SPP Service - data receive characteristic, read&write without response
static const uint16_t spp_data_receive_uuid = ESP_GATT_UUID_SPP_DATA_RECEIVE;
static const uint8_t  spp_data_receive_val[120] = {0x00};

///SPP Service - data notify characteristic, notify&read
static const uint16_t spp_data_notify_uuid = ESP_GATT_UUID_SPP_DATA_NOTIFY;
static const uint8_t  spp_data_notify_val[120] = {0x00};
static const uint8_t  spp_data_notify_ccc[2] = {0x00, 0x00};

///SPP Service - command characteristic, read&write without response
static const uint16_t spp_command_uuid = ESP_GATT_UUID_SPP_COMMAND_RECEIVE;
static const uint8_t  spp_command_val[10] = {0x00};

///SPP Service - status characteristic, notify&read
static const uint16_t spp_status_uuid = ESP_GATT_UUID_SPP_COMMAND_NOTIFY;
static const uint8_t  spp_status_val[10] = {0x00};
static const uint8_t  spp_status_ccc[2] = {0x00, 0x00};

/* Full Database Description - Used to add attributes into the database */
static const esp_gatts_attr_db_t gatt_db[SPP_IDX_NB] =
        {
                //SPP -  Service Declaration
                [SPP_IDX_SVC]                      	=
                        {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
                                sizeof(spp_service_uuid), sizeof(spp_service_uuid), (uint8_t *)&spp_service_uuid}},
                //SPP -  data receive characteristic Declaration
                [SPP_IDX_SPP_DATA_RECV_CHAR]            =
                        {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
                                CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

                //SPP -  data receive characteristic Value
                [SPP_IDX_SPP_DATA_RECV_VAL]             	=
                        {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&spp_data_receive_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
                                SPP_DATA_MAX_LEN,sizeof(spp_data_receive_val), (uint8_t *)spp_data_receive_val}},

                //SPP -  data notify characteristic Declaration
                [SPP_IDX_SPP_DATA_NOTIFY_CHAR]  =
                        {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
                                CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},

                //SPP -  data notify characteristic Value
                [SPP_IDX_SPP_DATA_NTY_VAL]   =
                        {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&spp_data_notify_uuid, ESP_GATT_PERM_READ,
                                SPP_DATA_MAX_LEN, sizeof(spp_data_notify_val), (uint8_t *)spp_data_notify_val}},

                //SPP -  data notify characteristic - Client Characteristic Configuration Descriptor
                [SPP_IDX_SPP_DATA_NTF_CFG]         =
                        {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
                                sizeof(uint16_t),sizeof(spp_data_notify_ccc), (uint8_t *)spp_data_notify_ccc}},

                //SPP -  command characteristic Declaration
                [SPP_IDX_SPP_COMMAND_CHAR]            =
                        {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
                                CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

                //SPP -  command characteristic Value
                [SPP_IDX_SPP_COMMAND_VAL]                 =
                        {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&spp_command_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
                                SPP_CMD_MAX_LEN,sizeof(spp_command_val), (uint8_t *)spp_command_val}},

                //SPP -  status characteristic Declaration
                [SPP_IDX_SPP_STATUS_CHAR]            =
                        {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
                                CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},

                //SPP -  status characteristic Value
                [SPP_IDX_SPP_STATUS_VAL]                 =
                        {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&spp_status_uuid, ESP_GATT_PERM_READ,
                                SPP_STATUS_MAX_LEN,sizeof(spp_status_val), (uint8_t *)spp_status_val}},

                //SPP -  status characteristic - Client Characteristic Configuration Descriptor
                [SPP_IDX_SPP_STATUS_CFG]         =
                        {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
                                sizeof(uint16_t),sizeof(spp_status_ccc), (uint8_t *)spp_status_ccc}},

        };


//static void show_bonded_devices(void)
//{
//    int dev_num = esp_ble_get_bond_device_num();
//
//    esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
//    esp_ble_get_bond_device_list(&dev_num, dev_list);
//    ESP_LOGI(BLE_S3_TAG, "Bonded devices number : %d", dev_num);
//    for (int i = 0; i < dev_num; i++) {
//                esp_log_buffer_hex(BLE_S3_TAG, (void *)dev_list[i].bd_addr, sizeof(esp_bd_addr_t));
//    }
//
//    free(dev_list);
//}

//static void __attribute__((unused)) remove_all_bonded_devices(void)
//{
//    int dev_num = esp_ble_get_bond_device_num();
//
//    esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
//    esp_ble_get_bond_device_list(&dev_num, dev_list);
//    for (int i = 0; i < dev_num; i++) {
//        esp_ble_remove_bond_device(dev_list[i].bd_addr);
//    }
//
//    free(dev_list);
//}

static uint8_t find_char_and_desr_index(uint16_t handle) {
    uint8_t error = 0xff;

    for(int i = 0; i < SPP_IDX_NB ; i++){
        if(handle == spp_handle_table[i]){
            return i;
        }
    }

    return error;
}

void send_data_ble_s3(char* data) {

    uint8_t total_num = 0;
    uint8_t current_num = 0;
    uint8_t * data_frag = NULL;

    char EOT = '\4';
    strncat(data, &EOT, 1);

    size_t len = strlen(data);

    if(len <= (spp_mtu_size - 3)){
        ESP_LOGE(BLE_S3_TAG, "ainda cabe... %d", spp_mtu_size);
        esp_ble_gatts_send_indicate(spp_gatts_if, spp_conn_id, spp_handle_table[SPP_IDX_SPP_DATA_NTY_VAL], len, (uint8_t*) data, false);
    }else if(len > (spp_mtu_size - 3)){
        if((len%(spp_mtu_size - 3)) == 0){
            total_num = len/(spp_mtu_size - 3);
        }else{
            total_num = len/(spp_mtu_size - 3) + 1;
        }
        current_num = 1;
        data_frag = (uint8_t *)malloc((spp_mtu_size-3)*sizeof(uint8_t));

        while(current_num <= total_num){
            if(current_num < total_num) {
                memcpy(data_frag, data + (current_num - 1)*(spp_mtu_size-3),(spp_mtu_size-3));
                esp_ble_gatts_send_indicate(spp_gatts_if, spp_conn_id, spp_handle_table[SPP_IDX_SPP_DATA_NTY_VAL],(spp_mtu_size-3), data_frag, false);
            }else if(current_num == total_num){

                memcpy(data_frag,data + (current_num - 1)*(spp_mtu_size-3),(len - (current_num - 1)*(spp_mtu_size - 3)));
                esp_ble_gatts_send_indicate(spp_gatts_if, spp_conn_id, spp_handle_table[SPP_IDX_SPP_DATA_NTY_VAL],(len - (current_num - 1)*(spp_mtu_size - 3)), data_frag, false);
            }
            vTaskDelay(20 / portTICK_PERIOD_MS);
            current_num++;
        }

        free(data_frag);
    }
}

void disconnect_ble_s3() {
    esp_ble_gap_disconnect(spp_remote_bda);
}


_Noreturn void spp_cmd_task(void * arg) {
    uint8_t * cmd_id;

    for(;;) {
        vTaskDelay(50 / portTICK_PERIOD_MS);
        if(xQueueReceive(cmd_cmd_queue, &cmd_id, portMAX_DELAY)) {
                    esp_log_buffer_char(BLE_S3_TAG, (char *)(cmd_id), strlen((char *)cmd_id));

            ESP_LOGE(BLE_S3_TAG, "test---> %s\n", (char *)(cmd_id));
            free(cmd_id);
        }
    }
    vTaskDelete(NULL);
}

static void spp_task_init(void) {
    cmd_cmd_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(spp_cmd_task, "spp_cmd_task", 2048, NULL, 10, NULL);
}

static void process_normal_data(char* data) {

    ESP_LOGI(BLE_S3_TAG, "DATA ----> %s", data);

    char* response;
    char* response_enc;
    char* response_ts;

    user_state_t state = get_user_state(ble_user_addr);
    esp_aes_context aes;
    aes = get_user_AES_ctx(ble_user_addr);

    ESP_LOGW(BLE_S3_TAG, "State = %d", state);


    if (state == CONNECTING) {
        if (retrieve_session_credentials(data, ble_user_addr)) {


            uint8_t* auth_seed = generate_random_seed(ble_user_addr);

            uint freeRAM = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
            ESP_LOGI(BLE_S3_TAG, "free RAM is %d.", freeRAM);

            if ( !heap_caps_check_integrity_all(true) )
            {
                ESP_LOGI(BLE_S3_TAG, "Algo mal");

            } else {
                ESP_LOGI(BLE_S3_TAG, "Well... Algo mal à mesma... so n sei o q.... :(");

            }
            size_t base64_size;
            char* seed_base64 = base64_encode(auth_seed, sizeof(uint8_t) * 10, &base64_size);

            response = malloc((sizeof(char) * 5) + base64_size);

            ESP_LOGI(TAG_BLE, "RAC %s", seed_base64); //FIXME remove

            sprintf(response, "RAC %s", seed_base64);

            aes = get_user_AES_ctx(ble_user_addr);
        } else {
            ESP_LOGE(TAG_BLE, "Disconnected by server! (Error getting session key)");
            disconnect_ble_s3();
            return;
        }
    } else if (state >= CONNECTED) { // Connected or Authorized
        aes = get_user_AES_ctx(ble_user_addr);
        char* cmd = decrypt_base64_AES(aes, data);
        ESP_LOGI(TAG_BLE, "After Dec: %s", cmd);
        response = checkCommand(cmd, ble_user_addr, 0); // FIXME remove t1 after
    } else {
        ESP_LOGE(TAG_BLE, "Disconnected by server! (Not CONNECTED)");
        disconnect_ble_s3();
        return;
    }

    response_ts = addTimestampsAndNonceToMsg(response);
    response_enc = encrypt_str_AES(aes, response_ts);
    send_data_ble_s3(response_enc);

}


static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT:
            if (VERBOSE_LEVEL >= 2) ESP_LOGI(BLE_S3_TAG, "ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT status %d", param->ext_adv_set_params.status);
            esp_ble_gap_config_ext_adv_data_raw(EXT_ADV_HANDLE,  sizeof(ext_adv_raw_data), &ext_adv_raw_data[0]);
            break;
        case ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT:
            if (VERBOSE_LEVEL >= 2) ESP_LOGI(BLE_S3_TAG, "ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT status %d", param->ext_adv_data_set.status);
            esp_ble_gap_ext_adv_start(NUM_EXT_ADV_SET, &ext_adv[0]);
            break;
        case ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT:
            if (VERBOSE_LEVEL >= 2) ESP_LOGI(BLE_S3_TAG, "ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT, status = %d", param->ext_adv_data_set.status);
            break;
        case ESP_GAP_BLE_ADV_TERMINATED_EVT:
            if (VERBOSE_LEVEL >= 2) ESP_LOGI(BLE_S3_TAG, "ESP_GAP_BLE_ADV_TERMINATED_EVT, status = %d", param->adv_terminate.status);
            if(param->adv_terminate.status == 0x00) {
                if (VERBOSE_LEVEL >= 2) ESP_LOGI(BLE_S3_TAG, "ADV successfully ended with a connection being created");
            }
            break;
        case ESP_GAP_BLE_PASSKEY_REQ_EVT:                           /* passkey request event */
            /* Call the following function to input the passkey which is displayed on the remote device */
            //esp_ble_passkey_reply(heart_rate_profile_tab[SPP_PROFILE_APP_IDX].remote_bda, true, 0x00);
            break;
        case ESP_GAP_BLE_OOB_REQ_EVT: {
            if (VERBOSE_LEVEL >= 2) ESP_LOGI(BLE_S3_TAG, "ESP_GAP_BLE_OOB_REQ_EVT");
            uint8_t tk[16] = {1}; //If you paired with OOB, both devices need to use the same tk
            esp_ble_oob_req_reply(param->ble_security.ble_req.bd_addr, tk, sizeof(tk));
            break;
        }
        case ESP_GAP_BLE_LOCAL_IR_EVT:                               /* BLE local IR event */
            if (VERBOSE_LEVEL >= 2) ESP_LOGI(BLE_S3_TAG, "ESP_GAP_BLE_LOCAL_IR_EVT");
            break;
        case ESP_GAP_BLE_LOCAL_ER_EVT:                               /* BLE local ER event */
            if (VERBOSE_LEVEL >= 2) ESP_LOGI(BLE_S3_TAG, "ESP_GAP_BLE_LOCAL_ER_EVT");
            break;
        case ESP_GAP_BLE_NC_REQ_EVT:
            /* The app will receive this evt when the IO has DisplayYesNO capability and the peer device IO also has DisplayYesNo capability.
            show the passkey number to the user to confirm it with the number displayed by peer device. */
            esp_ble_confirm_reply(param->ble_security.ble_req.bd_addr, true);
            if (VERBOSE_LEVEL >= 2) ESP_LOGI(BLE_S3_TAG, "ESP_GAP_BLE_NC_REQ_EVT, the passkey Notify number:%d", param->ble_security.key_notif.passkey);
            break;
        case ESP_GAP_BLE_SEC_REQ_EVT:
            /* send the positive(true) security response to the peer device to accept the security request.
            If not accept the security request, should send the security response with negative(false) accept value*/
            esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
            break;
        case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:  ///the app will receive this evt when the IO  has Output capability and the peer device IO has Input capability.
            ///show the passkey number to the user to input it in the peer device.
            if (VERBOSE_LEVEL >= 2) ESP_LOGI(BLE_S3_TAG, "The passkey Notify number:%06d", param->ble_security.key_notif.passkey);
            break;
        case ESP_GAP_BLE_AUTH_CMPL_EVT: {
            esp_bd_addr_t bd_addr;
            memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
            if (VERBOSE_LEVEL >= 2) ESP_LOGI(BLE_S3_TAG, "remote BD_ADDR: %08x%04x",\
                (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
                     (bd_addr[4] << 8) + bd_addr[5]);
            if (VERBOSE_LEVEL >= 2) ESP_LOGI(BLE_S3_TAG, "address type = %d", param->ble_security.auth_cmpl.addr_type);
            if (VERBOSE_LEVEL >= 2) ESP_LOGI(BLE_S3_TAG, "pair status = %s", param->ble_security.auth_cmpl.success ? "success" : "fail");
            if(!param->ble_security.auth_cmpl.success) {
                if (VERBOSE_LEVEL >= 2) ESP_LOGI(BLE_S3_TAG, "fail reason = 0x%x", param->ble_security.auth_cmpl.fail_reason);
            } else {
                if (VERBOSE_LEVEL >= 2) ESP_LOGI(BLE_S3_TAG, "auth mode = %hhu", param->ble_security.auth_cmpl.auth_mode);
            }
            break;
        }
        case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT: {
            if (VERBOSE_LEVEL >= 2) ESP_LOGD(BLE_S3_TAG, "ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT status = %d", param->remove_bond_dev_cmpl.status);
            if (VERBOSE_LEVEL >= 2) ESP_LOGI(BLE_S3_TAG, "ESP_GAP_BLE_REMOVE_BOND_DEV");
            if (VERBOSE_LEVEL >= 2) ESP_LOGI(BLE_S3_TAG, "-----ESP_GAP_BLE_REMOVE_BOND_DEV----");
                    esp_log_buffer_hex(BLE_S3_TAG, (void *)param->remove_bond_dev_cmpl.bd_addr, sizeof(esp_bd_addr_t));
            if (VERBOSE_LEVEL >= 2) ESP_LOGI(BLE_S3_TAG, "------------------------------------");
            break;
        }
        case ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT:
            if (VERBOSE_LEVEL >= 2) ESP_LOGI(BLE_S3_TAG, "ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT, tatus = %x", param->local_privacy_cmpl.status);
            esp_ble_gap_ext_adv_set_params(EXT_ADV_HANDLE, &spp_adv_params);
            break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            if (VERBOSE_LEVEL >= 2) ESP_LOGI(BLE_S3_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                     param->update_conn_params.status,
                     param->update_conn_params.min_int,
                     param->update_conn_params.max_int,
                     param->update_conn_params.conn_int,
                     param->update_conn_params.latency,
                     param->update_conn_params.timeout);
            break;
        default:
            break;
    }
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                        esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    esp_ble_gatts_cb_param_t *p_data = (esp_ble_gatts_cb_param_t *) param;
    uint8_t res = 0xff;
    if (VERBOSE_LEVEL >= 2) ESP_LOGI(BLE_S3_TAG, "event = %x\n", event);

    switch (event) {
        case ESP_GATTS_REG_EVT:
            esp_ble_gap_set_device_name(SAMPLE_DEVICE_NAME);
            //generate a resolvable random address
            esp_ble_gap_config_local_privacy(true);
            esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, SPP_IDX_NB, SPP_SVC_INST_ID);
            break;
        case ESP_GATTS_READ_EVT:
            res = find_char_and_desr_index(p_data->read.handle);
            if(res == SPP_IDX_SPP_STATUS_VAL){
                //TODO:client read the status characteristic
            }
            break;
        case ESP_GATTS_WRITE_EVT:
            res = find_char_and_desr_index(p_data->write.handle);
            if(p_data->write.is_prep == false){
                if (VERBOSE_LEVEL >= 2) ESP_LOGI(BLE_S3_TAG, "ESP_GATTS_WRITE_EVT : handle = %d\n", res);
                if(res == SPP_IDX_SPP_COMMAND_VAL) { // if data recieved is a command
                    uint8_t * spp_cmd_buff = NULL;
                    spp_cmd_buff = (uint8_t *)malloc((spp_mtu_size - 3) * sizeof(uint8_t));
                    if(spp_cmd_buff == NULL){
                        ESP_LOGE(BLE_S3_TAG, "%s malloc failed\n", __func__);
                        break;
                    }
                    memset(spp_cmd_buff,0x0,(spp_mtu_size - 3));
                    memcpy(spp_cmd_buff,p_data->write.value,p_data->write.len);
                    xQueueSend(cmd_cmd_queue,&spp_cmd_buff,10/portTICK_PERIOD_MS);
                } else if(res == SPP_IDX_SPP_DATA_NTF_CFG) { // if data recieved is data notify
                    if((p_data->write.len == 2)&&(p_data->write.value[0] == 0x01)&&(p_data->write.value[1] == 0x00)){
                        enable_data_ntf = true;
                    }else if((p_data->write.len == 2)&&(p_data->write.value[0] == 0x00)&&(p_data->write.value[1] == 0x00)){
                        enable_data_ntf = false;
                    }
                }
                else if(res == SPP_IDX_SPP_DATA_RECV_VAL) { // if data recieved is normal data
                    char* data = (char *)(p_data->write.value);

                    //TODO check for EOT char

                    process_normal_data(data);
                }else {
                    //TODO:
                }
            }else if((p_data->write.is_prep == true) && (res == SPP_IDX_SPP_DATA_RECV_VAL)){
                if (VERBOSE_LEVEL >= 2) ESP_LOGI(BLE_S3_TAG, "ESP_GATTS_PREP_WRITE_EVT : handle = %d\n", res);
                ESP_LOGE(BLE_S3_TAG, "Expected error, but not handled. Check github to get funtion 'static bool store_wr_buffer(esp_ble_gatts_cb_param_t *p_data)'");
            }
            break;
        case ESP_GATTS_EXEC_WRITE_EVT:
            break;
        case ESP_GATTS_MTU_EVT:
            spp_mtu_size = p_data->mtu.mtu;
            break;
        case ESP_GATTS_CONF_EVT:
            break;
        case ESP_GATTS_UNREG_EVT:
            break;
        case ESP_GATTS_DELETE_EVT:
            break;
        case ESP_GATTS_START_EVT:
            break;
        case ESP_GATTS_STOP_EVT:
            break;
        case ESP_GATTS_CONNECT_EVT:
            /* start security connect with peer device when receive the connect event sent by the master */
            esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);

            spp_conn_id = p_data->connect.conn_id;
            spp_gatts_if = gatts_if;
            is_connected = true;
            memcpy(&spp_remote_bda,&p_data->connect.remote_bda,sizeof(esp_bd_addr_t));
            sprintf(ble_user_addr, "%02x:%02x:%02x:%02x:%02x:%02x",
                    spp_remote_bda[0],
                    spp_remote_bda[1],
                    spp_remote_bda[2],
                    spp_remote_bda[3],
                    spp_remote_bda[4],
                    spp_remote_bda[5]);

            ESP_LOGI(BLE_S3_TAG, "ADDR: %s", ble_user_addr);

            set_current_BLE_addr(ble_user_addr);
            set_user_state(ble_user_addr, CONNECTING, "");

            break;
        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(BLE_S3_TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);
            /* start advertising again when missing the connect */
            is_connected = false;
            enable_data_ntf = false;
            esp_ble_gap_ext_adv_start(NUM_EXT_ADV_SET, &ext_adv[0]);

            set_current_BLE_addr(NULL);
            set_user_state(ble_user_addr, DISCONNECTED, "");
            disconnect_lock();
            break;
        case ESP_GATTS_OPEN_EVT:
            break;
        case ESP_GATTS_CANCEL_OPEN_EVT:
            break;
        case ESP_GATTS_CLOSE_EVT:
            break;
        case ESP_GATTS_LISTEN_EVT:
            break;
        case ESP_GATTS_CONGEST_EVT:
            break;
        case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
            ESP_LOGI(BLE_S3_TAG, "The number handle = %x", param->add_attr_tab.num_handle);
            if (param->create.status == ESP_GATT_OK){
                if(param->add_attr_tab.num_handle == SPP_IDX_NB) {
                    memcpy(spp_handle_table, param->add_attr_tab.handles,
                           sizeof(spp_handle_table));
                    esp_ble_gatts_start_service(spp_handle_table[SPP_IDX_SVC]);
                }else{
                    ESP_LOGE(BLE_S3_TAG, "Create attribute table abnormally, num_handle (%d) doesn't equal to HRS_IDX_NB(%d)",
                             param->add_attr_tab.num_handle, SPP_IDX_NB);
                }
            }else{
                ESP_LOGE(BLE_S3_TAG, " Create attribute table failed, error code = %x", param->create.status);
            }
            break;
        }

        default:
            break;
    }
}


static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            heart_rate_profile_tab[SPP_PROFILE_APP_IDX].gatts_if = gatts_if;
        } else {
            ESP_LOGI(BLE_S3_TAG, "Reg app failed, app_id %04x, status %d\n",
                     param->reg.app_id,
                     param->reg.status);
            return;
        }
    }

    do {
        int idx;
        for (idx = 0; idx < SPP_PROFILE_NUM; idx++) {
            if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                gatts_if == heart_rate_profile_tab[idx].gatts_if) {
                if (heart_rate_profile_tab[idx].gatts_cb) {
                    heart_rate_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}



void ble_s3_main(void)
{
    esp_err_t ret;

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(BLE_S3_TAG, "%s init controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(BLE_S3_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(BLE_S3_TAG, "%s init bluetooth", __func__);
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(BLE_S3_TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(BLE_S3_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret){
        ESP_LOGE(BLE_S3_TAG, "gatts register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret){
        ESP_LOGE(BLE_S3_TAG, "gap register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gatts_app_register(ESP_SPP_APP_ID);
    if (ret){
        ESP_LOGE(BLE_S3_TAG, "gatts app register error, error code = %x", ret);
        return;
    }

    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;     //bonding with peer device after authentication
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;           //set the IO capability to No output No input
    uint8_t key_size = 16;      //the key size should be 7~16 bytes
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    //set static passkey
    uint32_t passkey = 123456;
    uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;
    uint8_t oob_support = ESP_BLE_OOB_DISABLE;
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT, &oob_support, sizeof(uint8_t));
    /* If your BLE device acts as a Slave, the init_key means you hope which types of key of the master should distribute to you,
    and the response key means which key you can distribute to the master;
    If your BLE device acts as a master, the response key means you hope which types of key of the slave should distribute to you,
    and the init key means which key you can distribute to the slave. */
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

    spp_task_init();
}