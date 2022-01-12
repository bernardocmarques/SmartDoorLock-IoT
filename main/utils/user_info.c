#include "uthash.h"
#include "user_info.h"

user_info_hash_t* users_info = NULL;


/* Setters */

void set_user_state(char* user_ip, user_state_t state) {
    user_info_hash_t* user_info;

    HASH_FIND_INT(users_info, user_ip, user_info);
    if (user_info == NULL) {
        user_info = malloc(sizeof(user_info_hash_t));

        strcpy(user_info->user_ip, user_ip);
        HASH_ADD_INT(users_info, user_ip, user_info);  /* id: name of key field */
    }

    users_info->state = state;
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

    users_info->AES_ctx = AES_ctx;
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



/* Delete */

void delete_user(user_info_hash_t* user_info) {
    HASH_DEL( users_info, user_info);
}
