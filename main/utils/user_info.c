#include <esp_log.h>
#include "uthash.h"
#include "user_info.h"
#define TAG "USER_INFO"


user_info_hash_t* users_info = NULL;
char current_BLE_addr[18] = "";



/* Setters */

void set_user_state(char* user_ip, user_state_t state, char* phone_id) {
    user_info_hash_t* user_info;

    HASH_FIND_INT(users_info, user_ip, user_info);
    if (user_info == NULL) {
        user_info = malloc(sizeof(user_info_hash_t));

        strcpy(user_info->user_ip, user_ip);
        HASH_ADD_INT(users_info, user_ip, user_info);  /* id: name of key field */
    }

    user_info->state = state;
    strcpy(user_info->phone_id, phone_id);

}

uint8_t* generate_random_seed(char* user_ip) {
    user_info_hash_t* user_info;

    HASH_FIND_INT(users_info, user_ip, user_info);
    if (user_info == NULL) {
        user_info = malloc(sizeof(user_info_hash_t));

        strcpy(user_info->user_ip, user_ip);
        HASH_ADD_INT(users_info, user_ip, user_info);  /* id: name of key field */
    }

    user_info->seed = get_random_array(10);
    return user_info->seed;
}

void set_AES_ctx(char* user_ip, esp_aes_context AES_ctx) {
    user_info_hash_t* user_info;

    HASH_FIND_INT(users_info, user_ip, user_info);
    if (user_info == NULL) {
        user_info = malloc(sizeof(user_info_hash_t));

        strcpy(user_info->user_ip, user_ip);
        HASH_ADD_INT(users_info, user_ip, user_info);  /* id: name of key field */
    }

    user_info->AES_ctx = AES_ctx;
}

void set_BLE_user_state_to_connecting() {
    if (strcmp(current_BLE_addr, "") != 0 && get_user_info(current_BLE_addr) != DISCONNECTED) {
        set_user_state(current_BLE_addr, CONNECTING, "");
    }
}

void set_current_BLE_addr(char addr[18]) {
    strcpy(current_BLE_addr, addr);
}




/* Getters */

user_info_hash_t* get_user_info(char* user_ip) {
    user_info_hash_t* user_info;

    HASH_FIND_INT(users_info, user_ip, user_info);
    return user_info;
}

user_state_t get_user_state(char* user_ip) {
    user_info_hash_t* user_info;

    HASH_FIND_INT(users_info, user_ip, user_info);
    return user_info->state;
}

char* get_phone_id(char* user_ip) {
    user_info_hash_t* user_info;

    HASH_FIND_INT(users_info, user_ip, user_info);
    return strlen(user_info->phone_id) != 0 ? user_info->phone_id : NULL;
}

uint8_t* get_user_seed(char* user_ip) {
    user_info_hash_t* user_info;

    HASH_FIND_INT(users_info, user_ip, user_info);
    return user_info->seed;
}

esp_aes_context get_user_AES_ctx(char* user_ip) {
    user_info_hash_t* user_info;

    HASH_FIND_INT(users_info, user_ip, user_info);
    return user_info->AES_ctx;
}

char* get_current_BLE_addr() {
    return current_BLE_addr;
}




/* Delete */

void delete_user(user_info_hash_t* user_info) {
    HASH_DEL( users_info, user_info);
}
