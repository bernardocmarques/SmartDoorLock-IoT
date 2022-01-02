#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <esp_system.h>
#include "esp_log.h"
#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"
#include "base64.h"
#include "rsa_util.h"
#include "mbedtls/entropy_poll.h"

static const char TAG[] = "RSA_UTIL";

static const char privkey_2048_buf[] = "-----BEGIN RSA PRIVATE KEY-----\r\n"
"MIIEpAIBAAKCAQEAl4iRt8ORglI2tv0U3Dp23Zyoc4bY0l414bNCK6TN1AXKXx6i\r\n"
"QaiugnsFK84BhVtd6uNX/hMxsat+aZoJvPdMaY48U1DgAqBtFhSbXakyfghdk6VD\r\n"
"VV6chQzrYzyvZ1eR7q0qfmf5w3Z02fSfI66Ea8BAT1UpAEWdSU+xFlbRb9qsZYGV\r\n"
"99+JjPC4PGhbHMOSsO+We4ZsP8UosNyF8A62FheFXimCujiPmOBIOablN9TuWXAU\r\n"
"tNHhWf4EyYDQvEo/NfY2mleiYjKqHoJpkIu+sMcJJ3ry5Z4HEZi+SUbCjL7I5ZYF\r\n"
"8aZq3YRxS4n2ZO7/w7n7B5621HMsahRNUi761wIDAQABAoIBAGn2cg9SjnnXC7PC\r\n"
"HcgyidRGK/U9InlYr8z4ERl70QKmWfFR9px7XCyZ4e/TynR6g54xA+MDgQiAp5Eo\r\n"
"yg59z80wTTblov+zNxTtrAc+vbQsHWOVeRRFaKYRdriaQv282qtQJBroklsAho2y\r\n"
"5WWKL8c5VL8lCdrK00XkmCzK4QGZkNDE52Bh44OsR3slNzD6tKOEy5g2bTxEprYH\r\n"
"CLkSznOAddPcf2F9UBXKw1vXSNK78r/LubQpO+GpMJr9L4g892r63Zih8CTWFyQi\r\n"
"SzbmSyRIYaB5wtJtI2dkiNQ5G1ri4+xfPT06xFQ6Ax0kgpCIWGhECpGQV/FYHEa8\r\n"
"ZId2MuECgYEAyfPciS+M3X3EZP1rUkgjXyDKMJ3Izi+3v+7GsSd0mAUFwK4KJLLY\r\n"
"vJEulNKZHzjufwebpPogoqNvJUCtSQaU6Y7RAHDCDrmVlFQaFi5WXocsji++rEW9\r\n"
"lKmu1jYgIkJ+McBBbAxIQlmpaG4hXkKiagkQZ01DhWOzk1yUN601od0CgYEAwBZn\r\n"
"IfrtDN5w7WGNJJeo3Kzwpc9t0ULUqPu/GN3PO5fVrT80BIz3vDit9n0A6/KVhHV6\r\n"
"SmorRIGpsjgQRT/iKM7WsKhHnN8TqsVTy3pr4nX0/KZiAiNI1G2LelCWVAdRd5PI\r\n"
"mrhYAT078jCkxPkxB2oE1pLQC8fYv6Xisc91NkMCgYEAw7TU5QT1h7dXWV7UYBqU\r\n"
"XJ2UEuT1MgrAEPm+BvNrY96KIp0GK2Y97w7qi0JDLSSoyuV+ibPzaGjlTr6MrxX3\r\n"
"vRavp7Od+1MRh3qxBQnGnTh1jxzptFypSaXeTqyJG2pAjMn5HFISvGnTZ+ZB4+zD\r\n"
"I+rAwLr5Ugy6e3XeFM6ACOkCgYEAu/vwuG+CV+rFZ3rlj18gsb5J0Gt81KNrzWh5\r\n"
"7xL8AR0pz1+gP6fZtoldrnFNWpvQOY5ivLrEV0nx0elN+wd3BGrP7pjxZJNoAuMU\r\n"
"i3jmZfz8YdlO5zqyxrniGzUMuXVkA/tMAibQcX4E0ZNLXT1l/xSBYaDSHAVbmMr2\r\n"
"XP9jfVMCgYBB9p2DhRKZ+7FJfdPgFuW7YaV+5QT5ptVyafVy4wlCTRjd/2ZrJUPM\r\n"
"mk/ULjn7izTspDbdRrd3aC9uZ+Q2EqEISNJBBCqElUI0uVfoBjs9V9kRJu3gf0bi\r\n"
"U2jiN//oWef+CapGob3FagxapVJmmYnVHTMMClKcjOAKITbsW1RtQQ==\r\n"
"-----END RSA PRIVATE KEY-----\r\n";


static int myrand(void *rng_state, unsigned char *output, size_t len) {
    size_t olen;
    return mbedtls_hardware_poll(rng_state, output, len, &olen);
}


void print_array_RSA(unsigned char* a, int n) {
    for (int i = 0; i < n; i++) {
        ESP_LOGI("TEST", "%x, ", a[i]);
    }
}

void encrypt() {
    mbedtls_pk_context clientkey;
    mbedtls_rsa_context rsa;

    
    const unsigned char orig_buf[6] = "Ola !";
    unsigned char encrypted_buf[KEY_SIZE_BYTES_RSA];
    unsigned char decrypted_buf[KEY_SIZE_BYTES_RSA];

    int res = 0;

    memset(encrypted_buf, '#', sizeof(encrypted_buf));
    memset(decrypted_buf, '#', sizeof(decrypted_buf));

    // memset(decrypted_buf, 'A', sizeof(orig_buf));


    ESP_LOGI("Orig", "%x | %s", res*-1, orig_buf);
    // print_array_RSA(orig_buf, S);


    mbedtls_pk_init(&clientkey);
    res = mbedtls_pk_parse_key(&clientkey, (const uint8_t *)privkey_2048_buf, sizeof(privkey_2048_buf), NULL, 0);


    ESP_LOGI("PRIV", "%x | %s", res*-1, privkey_2048_buf);

    memcpy(&rsa, mbedtls_pk_rsa(clientkey), sizeof(mbedtls_rsa_context));
    mbedtls_rsa_set_padding(&rsa, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);


    ESP_LOGI("CAN PUB", "%d", mbedtls_rsa_check_pubkey(&rsa));
    ESP_LOGI("HASH_ID", "%d", mbedtls_md_info_from_type( (mbedtls_md_type_t) rsa.hash_id ) == NULL);


    res = mbedtls_rsa_rsaes_oaep_encrypt(&rsa, myrand, NULL, MBEDTLS_RSA_PUBLIC, NULL, 0, sizeof(unsigned char) * 6, orig_buf, encrypted_buf);


    ESP_LOGI("Enc", "%x | %s", res*-1, encrypted_buf);
    // print_array_RSA(encrypted_buf, S);

    
    size_t base64_size;
    char* encrypted_base64 = base64_encode(encrypted_buf, KEY_SIZE_BYTES_RSA, &base64_size);
    ESP_LOGI("Enc B64", "%x | %s", res*-1, encrypted_base64);


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

    return rsa_decrypted;
}

char* decrypt_base64_RSA(char* chipertext_base64) {
    int base64_size = strlen(chipertext_base64);

    size_t size;
    uint8_t* chipertext = base64_decode(chipertext_base64, base64_size, &size);

    RSA_Decrypted* rsa_decrypted = decrypt_RSA(chipertext);

    // Convert to char*
    char* decrypted_str = (char *) malloc(sizeof(char) * rsa_decrypted->dataLength);
    memcpy(decrypted_str, rsa_decrypted->data, rsa_decrypted->dataLength);
    decrypted_str[rsa_decrypted->dataLength] = '\0';

    free(chipertext);
    free_RSA_Decrypted(rsa_decrypted);

    return decrypted_str;
}