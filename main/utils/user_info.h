#ifndef USER_INFO_H
#define USER_INFO_H

#include "uthash.h"
#include "utils.h"
#include "aes_util.h"


typedef enum {
    DISCONNECTED = 0,
    CONNECTED = 1,
    AUTHORIZED = 2
} user_state_t;

typedef struct {
    char user_ip[45];
    user_state_t state;   
    uint8_t* seed;   
    esp_aes_context* AES_ctx_pt;
    UT_hash_handle hh;
} user_info_hash_t;


/* Setters */

void set_user_state(char* user_ip, user_state_t state);
uint8_t* generate_random_seed(char* user_ip);
void set_AES_ctx(char* user_ip, esp_aes_context* AES_ctx_pt);



/* Getters */

user_info_hash_t* get_user_info(char* user_ip);
user_state_t get_user_state(char* user_ip);
uint8_t* get_user_seed(char* user_ip);
esp_aes_context* get_user_AES_ctx_pt(char* user_ip);



/* Delete */

void delete_user(user_info_hash_t* user_state);

#endif // USER_INFO_H