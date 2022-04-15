idf_component_register(
    SRCS
        "main.c"
        "utils/utils.c"
        "utils/aes_util.c"
        "utils/sha256_util.c"
        "utils/PKCS7.c"
        "utils/base64_util.c"
        "utils/rsa_util.c"
        "utils/nvs_util.c"
        "utils/hmac_util.c"
        "utils/authorization.c"
        "utils/user_info.c"
        "utils/lock_status.c"
        "utils/time_util.c"
        "utils/nonce.c"
        "utils/pushingbox_util.c"
        "utils/esp_touch_util.c"
        "utils/wifi_connect_util.c"
        "utils/database_util.c"


    INCLUDE_DIRS "utils"
)