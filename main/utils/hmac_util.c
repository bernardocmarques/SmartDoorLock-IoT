#include <stdio.h>    
#include <string.h>     
#include "mbedtls/md.h" 
#include "esp_err.h" 
#include "hmac_util.h"



esp_err_t gen_hmac_32(uint8_t* key, uint8_t* message, size_t message_size, uint8_t* hmac) { 
    int KEY_SIZE = 32;
    int ret;
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    
    mbedtls_md_init(&ctx);

    ret = mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
    if(ret != ESP_OK) {        
        printf("mbedtls_md_setup ret : %X\n", ret);
        mbedtls_md_free(&ctx);
        return ret;
    }
    ret = mbedtls_md_hmac_starts(&ctx, (const unsigned char *) key, KEY_SIZE);
    if(ret != ESP_OK) {        
        printf("mbedtls_md_hmac_starts ret : %X\n", ret);
        mbedtls_md_free(&ctx);
        return ret;
    }

    ret = mbedtls_md_hmac_update(&ctx, (const unsigned char *) message, message_size);
    if(ret != ESP_OK) {        
        printf("mbedtls_md_hmac_update ret : %X\n", ret);
        mbedtls_md_free(&ctx);
        return ret;
    }

    ret = mbedtls_md_hmac_finish(&ctx, hmac);
    if(ret != ESP_OK) {        
        printf("mbedtls_md_hmac_finish ret : %X\n", ret);
    }

    mbedtls_md_free(&ctx);

    return ret;
}