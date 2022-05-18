#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include "esp_log.h"
#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"
#include "base64_util.h"
#include "rsa_util.h"
#include "mbedtls/entropy_poll.h"
#include "sha256_util.h"


const char TAG[] = "RSA_UTIL";

// TODO remove hardcoded private key
//static const char privkey_2048_buf[] = "-----BEGIN RSA PRIVATE KEY-----\r\n"
//"MIIEowIBAAKCAQEAxLHVaTHF5D1Jm/+5n+YH/Ci0QLAg/2nmyyW+QwNHZYNWc93r\r\n"
//"QSRumjgv4Imi1kUGtpqu7PsmStGtFWlXUsaBHIgWEhBkXY1tyHv4l3r0NpFWephU\r\n"
//"ER+ED8Uo90+sjrF4VycZAh7AS7SuMRTSJh7przV7N3Htl4GHfGUD3Kdii90uYIKP\r\n"
//"Sq2nFOWoBzcSK6QmtnkQb3yw4zb6cVUjoBD7qPYezx9VKY/gYy7qGNq9vooKXOdH\r\n"
//"se7oLCZ+5O0QbeaPY53aLuJbs2KJtTr0iwEmyA2ELJKs3fQiVEKeitbAybOk/2mv\r\n"
//"gCXQcQLrN1H2rl/cpEYN6f6qJxjgPTktt+dnkQIDAQABAoIBAHEaZUIxGb7tswce\r\n"
//"HGoixxKrgUL1RHQ6PDkygd5c41AvHqZPxLhXr7XEe1tdKaKWXI7iEZY5sMIzIZj/\r\n"
//"UvRJKvLyGebXQC8/ZRJ0nvTUAdvi5Nxn/Wc/PRwoXi8fxHTk/fL3i3zZm++sfMHC\r\n"
//"XDkJa4yRb0HppBqLpBHWsErQgW00hlVjM6Wlrs1pvWFPACJ5/GO65f2JJ+UtbKsS\r\n"
//"IPeb6DN1SssE7kdSI7RVUCQTfEwRtTKXmHYtlE6/Z/EPJKE7pPwBfTHYEtcs+SOW\r\n"
//"HAz5fntX5IE4A2NLTu4P+dRCUqjF46zmSJd4/3pPT0MkfOJyjDWRP/TxNE+Jxq3U\r\n"
//"XdpbPkECgYEA8vNuNP6/oNceASk5gyGWADdsoCHZ3ODLqh4Wm9Jk74uOZEEKd6tY\r\n"
//"23hAY9AGDTgrEj8mh5uVMDJvGfQ2GVsLDU8dCIZUmMciRBwIiOlexvWnyRGqW3o8\r\n"
//"E9qCBChPwp2kT1Qttilaxhvroi5xzPB+d50TIK8o6XpypGbGSRFMpvkCgYEAz0Jh\r\n"
//"glYjPSgSc+Cd6HtDKkE8WI0r4keT/7+Kxs4NpxPrKTTh6h6ql0lc/dkgcbtkOazH\r\n"
//"A2U5ufTzFMAwzW3X1OCe5BKILDxE1bDlhj8TU+hkHXnb0jn06/NE2JvK5dTi7LyJ\r\n"
//"ZiWJ6+kaBOA6I2CBb9eP7Z2gcDDdKWWe+eGb81kCgYBtm3OqBxBvQP3xaibfSUTC\r\n"
//"Pj8Mk5kVtHlN+5sZm7cb92s7Qbi2OqCxCzSJk21Xg3KzHbiFT6TkBKzpGatajx+S\r\n"
//"VpHzqZ76+kQ0VC1pj1fKDUQwS37/HEuEbX1g4MrzM2nQvFqPJ2Mjo68QEUIYQpvb\r\n"
//"3QqnIT8k7rBQCWoFxv89CQKBgQCQ44/1JLB31Wao+VKKrnjyti4wnWgbRPyyoj2q\r\n"
//"42tp7KPN57kzCQMqxc+rajmjKGRVaXKq7f3gANxaGk1Dn1Ft8SVCva3SdsOMO6EJ\r\n"
//"K1kgpGowrPq+SWPt+t+bKbY624tUAi1vajiz4f4dgH9EMffqruBgNXxuUcqaYP81\r\n"
//"IsH56QKBgHJMqUAcAxrzTMOXkcWD0nySn8RIysDEK1uhTI2OdtmI5/HDo1cn6M1F\r\n"
//"wrwcScYdf7NrZoVuncVT+SbZAWPo8MM8cF9IS7RBfanEGdL7ZEsPRUtJ5aQqKJdd\r\n"
//"MfmAKSUHB7O9aWbdBsjO5b3tXHbk6p7J9nSEsXQ7Pn8KXqpVotuK\r\n"
//"-----END RSA PRIVATE KEY-----\r\n";

static char privkey_2048_buf[2048];


void set_rsa_private_key(const char* key) {
    strcpy(privkey_2048_buf, key);
}

static int myrand(void *rng_state, unsigned char *output, size_t len) {
    size_t olen;
    return mbedtls_hardware_poll(rng_state, output, len, &olen);
}


void print_array_RSA(unsigned char* a, int n) {
    for (int i = 0; i < n; i++) {
        ESP_LOGI("TEST", "%x, ", a[i]);
    }
}

char* sign_RSA(char* to_sign) {
    mbedtls_pk_context clientkey;
    mbedtls_rsa_context rsa;


    unsigned char * signature_buf = (unsigned char *) malloc(sizeof(unsigned char) * KEY_SIZE_BYTES_RSA);

    int res = 0;

    mbedtls_pk_init(&clientkey);
    res = mbedtls_pk_parse_key(&clientkey, (const uint8_t *)privkey_2048_buf, sizeof(privkey_2048_buf), NULL, 0);

    if (res != 0) {
        ESP_LOGE(TAG, "Failed while parsing key. Code: %X", -res);
        return NULL;
    }

    rsa = *mbedtls_pk_rsa(clientkey);
    mbedtls_rsa_set_padding(&rsa, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);

    uint8_t* hash = (uint8_t*) malloc(sizeof(uint8_t) * 32);

    hash_sha256(to_sign, hash);


    res = mbedtls_rsa_rsassa_pss_sign(
            &rsa,
            myrand,
            NULL,
            MBEDTLS_RSA_PRIVATE,
            MBEDTLS_MD_SHA256,
            0,
            (unsigned char*) hash,
            signature_buf);
    if (res != 0) {
        ESP_LOGE(TAG, "Failed while sign. Code: %X", -res);
        return NULL;
    }

    size_t base64_size;
    char* signature_base64 = base64_encode(signature_buf, KEY_SIZE_BYTES_RSA, &base64_size);

    mbedtls_rsa_free(&rsa);

    return signature_base64;
}


void free_RSA_Decrypted(RSA_Decrypted* rsa_decrypted) {
    free(rsa_decrypted->data);
    free(rsa_decrypted);
}

RSA_Decrypted* decrypt_RSA(uint8_t* chipertext) {
    mbedtls_pk_context clientkey;
    mbedtls_rsa_context rsa;

    unsigned char * decrypted_buf = (unsigned char *) malloc(sizeof(unsigned char) * KEY_SIZE_BYTES_RSA);
    RSA_Decrypted* rsa_decrypted = (RSA_Decrypted*) malloc(sizeof(RSA_Decrypted));
    rsa_decrypted->data = decrypted_buf;

    int res = 0;

    mbedtls_pk_init(&clientkey);
    res = mbedtls_pk_parse_key(&clientkey, (const uint8_t *)privkey_2048_buf, sizeof(privkey_2048_buf), NULL, 0);

    if (res != 0) {
        ESP_LOGE(TAG, "Failed while parsing key. Code: %X", -res);
        return NULL;
    }

    rsa = *mbedtls_pk_rsa(clientkey);
    mbedtls_rsa_set_padding(&rsa, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);

    size_t len_decrypted;
    res = mbedtls_rsa_rsaes_oaep_decrypt(&rsa, myrand, NULL, MBEDTLS_RSA_PRIVATE, NULL, 0, &len_decrypted, chipertext, decrypted_buf, sizeof(unsigned char) * KEY_SIZE_BYTES_RSA);
    if (res != 0) {
        ESP_LOGE(TAG, "Failed while decrypting. Code: %X", -res);
        return NULL;
    }

    rsa_decrypted->dataLength = (int)(len_decrypted / sizeof(uint8_t));

    mbedtls_rsa_free(&rsa);
//    mbedtls_pk_free(&clientkey);


    return rsa_decrypted;
}

char* decrypt_base64_RSA(char* chipertext_base64) {

    size_t base64_size = strlen(chipertext_base64);

    size_t size;

    uint8_t* chipertext = base64_decode(chipertext_base64, base64_size, &size);

    RSA_Decrypted* rsa_decrypted = decrypt_RSA(chipertext);

    // Convert to char*
    char* decrypted_str = (char *) malloc((sizeof(char) * rsa_decrypted->dataLength)+1);
    memcpy(decrypted_str, rsa_decrypted->data, rsa_decrypted->dataLength);
    decrypted_str[rsa_decrypted->dataLength] = '\0';

    free(chipertext);
    free_RSA_Decrypted(rsa_decrypted);

    return decrypted_str;
}