#ifndef USER_INFO_H
#define USER_INFO_H

#include "uthash.h"
#include "utils.h"
#include "aes_util.h"


typedef enum {  // order of permissions. 3 is higher than 2, AUTHORIZED is higher than CONNECTED
    DISCONNECTED = 0,
    CONNECTING = 1,
    CONNECTED = 2,
    AUTHORIZED = 3
} user_state_t;

typedef struct {
    char user_ip[45];
    char username[16];
    user_state_t state;   
    uint8_t* seed;   
    esp_aes_context AES_ctx;
    UT_hash_handle hh;
} user_info_hash_t;


/* Setters */

void set_user_state(char* user_ip, user_state_t state, char* used_id);
uint8_t* generate_random_seed(char* user_ip);
void set_AES_ctx(char* user_ip, esp_aes_context AES_ctx);

void set_BLE_user_state_to_connecting();
void set_current_BLE_addr(char addr[18]);


/* Getters */

user_info_hash_t* get_user_info(char* user_ip);
user_state_t get_user_state(char* user_ip);
uint8_t* get_user_seed(char* user_ip);
esp_aes_context get_user_AES_ctx(char* user_ip);
char* get_username(char* user_ip);
char* get_current_BLE_addr();


/* Delete */

void delete_user(user_info_hash_t* user_state);

#endif // USER_INFO_H