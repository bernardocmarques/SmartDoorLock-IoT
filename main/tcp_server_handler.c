#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "utils/aes_util.h"
#include "utils/authorization.h"
#include "utils/utils.h"
#include "utils/user_info.h"
#include "utils/time_util.h"
#include "utils/nonce.h"
#include "pushingbox_util.h"
#include "database_util.h"
#include "nvs_util.h"

#define SEP " "

#define ERROR_VERIFYING_TIMESTAMP_AND_NONCE "Message denied due to invalid timestamp or nonce!"
#define ERROR_AUTH_CODE_NOT_VALID "Authorization code not valid!"
#define ERROR_FEW_ARGUMENTS "Message expecting %d arguments but only got %d!"

#define ERROR_NO_PERMISSIONS "User don't have permissions to do this operation"


static char* getCommand(char* str) {
    char* cmd = malloc(sizeof(char) * strlen(str)+1);
    strcpy(cmd, str);
    char *pt;
    pt = strtok(cmd, SEP);
    return pt;
}

bool verifyTimestampsAndNonce(char** args, int first_args_n) {
    int ts1 = strtol(args[first_args_n + 0], NULL, 10);
    int ts2 = strtol(args[first_args_n + 1], NULL, 10);
    long nonce = strtol(args[first_args_n + 2], NULL, 10);

    int now_ts = getNowTimestamp();

    bool valid = ts1 <= now_ts && ts2 >= now_ts && checkNonce(nonce);

    if (valid) {
        addNonceSorted(nonce, ts2 + 10);
    }

    return valid;
}

bool canUseLock(char* user_ip) {
    if (get_user_state(user_ip) != AUTHORIZED) return false;
    authorization* auth = malloc(sizeof(authorization));

    char* phone_id = get_phone_id(user_ip);
    if (phone_id == NULL) return false;

    esp_err_t res = get_authorization(phone_id, auth);

    if (res != ESP_OK) return false;
    int today_ts;
    int weekday;
    switch (auth->user_type) {

        case admin:
        case owner:
            return true;
        case periodic_user:
            weekday = getTodayWeekday();

            if (!auth->weekdays[weekday]) return false;
            // fall through
        case tenant:
            today_ts = getTodayTimestamp();

            if (auth->valid_from_ts < today_ts) return false;
            if (auth->valid_until_ts > today_ts) return false;

            return true;
        case one_time_user:
            today_ts = getTodayTimestamp();
            if (auth->one_day_ts != today_ts) return false;

            return true;
    }
    return false;
}

bool canCreateInvite(char* user_ip, enum userType userType, int valid_from, int valid_until, int one_day) {

    if (get_user_state(user_ip) != AUTHORIZED) return false;
    authorization* auth = malloc(sizeof(authorization));

    char* phone_id = get_phone_id(user_ip);
    if (phone_id == NULL) return false;

    esp_err_t res = get_authorization(phone_id, auth);

    if (res != ESP_OK) return false;

    if (auth->user_type > userType) return false;

    switch (auth->user_type) {
        case admin:
        case owner:
            return true;
        case tenant:
            if (valid_from < auth->valid_from_ts) return false;
            if (valid_from > auth->valid_until_ts) return false;

            if (valid_until < auth->valid_from_ts) return false;
            if (valid_until > auth->valid_until_ts) return false;

            if (one_day < auth->valid_from_ts) return false;
            if (one_day > auth->valid_until_ts) return false;
            return true;
        case periodic_user:
        case one_time_user:
            return false;
    }
    return false;
}


static char** getArgs(char* str, int n) {
    heap_caps_check_integrity_all(true);
    char* cmd = malloc(sizeof(char) * strlen(str)+1);
    strcpy(cmd, str);
    heap_caps_check_integrity_all(true);
    char** args = (char**)malloc(n * sizeof(char*));
    heap_caps_check_integrity_all(true);
    char *pt;

    pt = strtok(cmd, SEP);
    if (pt != NULL) pt = strtok(NULL, SEP);

    int i = 0;
    while (pt != NULL && i<n) {
        args[i] = malloc(sizeof(char) * (strlen(pt)+1));
        strcpy(args[i++], pt);
        ESP_LOGI("example", "pt = %s", pt);
        pt = strtok(NULL, SEP);
    }

    if (i < n) {
        int a = i;
        while (i--) {
            free(args[i-1]);
        }
        free(args);
        ESP_LOGE("ERROR", ERROR_FEW_ARGUMENTS, n, a);
        return NULL;
    }

    return args;
}

static void free_args(char** args, int n) {
    for (int i = 0; i < n; i++) {
        free(args[i]);
    }
    free(args);
}

static char* checkCommand(char* cmd, char* user_ip) {

    ESP_LOGW("example", "Received cmd %s", cmd);

    char* c = getCommand(cmd);

    if (strcmp(c, "SAC") == 0) { // Send Auth Credentials
        char** args =  getArgs(cmd, 5);

        if (args == NULL) {
            free(c);
            return NAK_MESSAGE;
        }

        char* phone_id = args[0];
        char* auth_code = args[1];


        int is_valid = 0;
        if (verifyTimestampsAndNonce(args, 2)) {
            uint8_t* auth_seed = get_user_seed(user_ip);
            is_valid = check_authorization_code(phone_id, auth_code, auth_seed);
            if (is_valid) {
                set_user_state(user_ip, AUTHORIZED, phone_id);
            } else {
                ESP_LOGE("Error", ERROR_AUTH_CODE_NOT_VALID);
            }
        } else {
            ESP_LOGE("Error", ERROR_VERIFYING_TIMESTAMP_AND_NONCE);
        }

        free(c);
        free_args(args, 5);

        return is_valid ? ACK_MESSAGE : NAK_MESSAGE;
    } else if (strcmp(c, "RUD") == 0) { // Request Unlock Door
        char **args = getArgs(cmd, 3);

        if (args == NULL) {
            free(c);
            return NAK_MESSAGE;
        }

        bool ack = false;
        if (verifyTimestampsAndNonce(args, 0)) {
            if (canUseLock(user_ip)) {
                unlock_lock();
                ack = true;
            } else {
                ESP_LOGE("Error", ERROR_NO_PERMISSIONS);
            }
        } else {
            ESP_LOGE("Error", ERROR_VERIFYING_TIMESTAMP_AND_NONCE);
        }
        free_args(args, 3);
        free(c);
        set_BLE_user_state_to_connecting();
        return ack ? ACK_MESSAGE : NAK_MESSAGE;
    } else if (strcmp(c, "RLD") == 0) { // Request Lock Door
        char **args = getArgs(cmd, 3);

        if (args == NULL) {
            free(c);
            return NAK_MESSAGE;
        }

        bool ack = false;
        if (verifyTimestampsAndNonce(args, 0)) {
            if (canUseLock(user_ip)) {
                lock_lock();
                ack = true;
            } else {
                ESP_LOGE("Error", ERROR_NO_PERMISSIONS);
            }
        } else {
            ESP_LOGE("Error", ERROR_VERIFYING_TIMESTAMP_AND_NONCE);

        }
        free_args(args, 3);
        free(c);
        set_BLE_user_state_to_connecting();
        return ack ? ACK_MESSAGE : NAK_MESSAGE;
    } else if (strcmp(c, "RNI") == 0) { // Request New Invite
        int n_args = 0;

        if (strncmp("RNI 0", (char *) cmd, 5) == 0) {
            n_args = 4;
        } else if (strncmp("RNI 1", (char *) cmd, 5) == 0) {
            n_args = 4;
        } else if (strncmp("RNI 2", (char *) cmd, 5) == 0) {
            n_args = 6;
        } else if (strncmp("RNI 3", (char *) cmd, 5) == 0) {
            n_args = 7;
        } else if (strncmp("RNI 4", (char *) cmd, 5) == 0) {
            n_args = 5;
        }

        char **args = getArgs(cmd, n_args);

        if (args == NULL) {
            free(c);
            return NAK_MESSAGE;
        }
        enum userType user_type = strtol(args[0], NULL, 10);
        int valid_from = -1;
        int valid_until = -1;
        char* weekdays_str = NULL;
        int one_day = -1;

        switch (user_type) {
            case admin:
            case owner:
                break;
            case tenant:
                valid_from = strtol(args[1],NULL, 10);
                valid_until = strtol(args[2],NULL, 10);
                break;
            case periodic_user:
                valid_from = strtol(args[1],NULL, 10);
                valid_until = strtol(args[2],NULL, 10);
                weekdays_str = args[3];
                break;
            case one_time_user:
                one_day = strtol(args[1],NULL, 10);
                break;
            default:
                break;
        }

        char* response = NAK_MESSAGE;

        if (verifyTimestampsAndNonce(args, n_args - 3)) {
            if (canUseLock(user_ip)) {
                if (!canCreateInvite(user_ip,  user_type, valid_from, valid_until, one_day)) {
                    ESP_LOGE("Error", "User does not have enough permissions to create this invite.");
                    response = NAK_MESSAGE;
                } else {
                    char* invite_id = create_invite(1649787416/*fixme change*/, user_type, valid_from, valid_until, weekdays_str, one_day, NULL);

                    response = malloc(strlen("XXX ") + strlen(invite_id) + 1);

                    sprintf(response, "SNI %s", invite_id);
                    if (invite_id == NULL) {
                        ESP_LOGE("Error", "Could not create invite");
                        response = NAK_MESSAGE;
                    }
                }
            } else {
                ESP_LOGE("Error", ERROR_NO_PERMISSIONS);
            }
        } else {
            ESP_LOGE("Error", ERROR_VERIFYING_TIMESTAMP_AND_NONCE);
        }

        free_args(args, n_args);
        free(c);
        set_BLE_user_state_to_connecting();
        return response;
    }  else if (strcmp(c, "RFI") == 0) {

        if (get_registration_status() != REGISTERED) {
            free(c);
            return NAK_MESSAGE;
        }
        int n_args = 4;

        char **args = getArgs(cmd, n_args);

        if (args == NULL) {
            free(c);
            return NAK_MESSAGE;
        }

        enum userType user_type = 0;

        int valid_from = -1;
        int valid_until = -1;
        char* weekdays_str = NULL;
        int one_day = -1;

        char* response = NAK_MESSAGE;

        if (verifyTimestampsAndNonce(args, n_args - 3)) {

            char* invite_id = create_invite(1649787416/*fixme change*/, user_type, valid_from, valid_until, weekdays_str, one_day, NULL);

            response = malloc(strlen("XXX ") + strlen(invite_id) + 1);

            sprintf(response, "SFI %s", invite_id);
            if (invite_id == NULL) {
                ESP_LOGE("Error", "Could not create invite");
                response = NAK_MESSAGE;
            }

        } else {
            ESP_LOGE("Error", ERROR_VERIFYING_TIMESTAMP_AND_NONCE);
        }
        free_args(args, n_args);
        free(c);
        return response;
    } else if (strcmp(c, "RLD") == 0) { // Request Lock Door
        char **args = getArgs(cmd, 3);

        if (args == NULL) {
            free(c);
            return NAK_MESSAGE;
        }

        bool ack = false;
        if (verifyTimestampsAndNonce(args, 0)) {
            if (canUseLock(user_ip)) {
                lock_lock();
                ack = true;
            } else {
                ESP_LOGE("Error", ERROR_NO_PERMISSIONS);
            }
        } else {
            ESP_LOGE("Error", ERROR_VERIFYING_TIMESTAMP_AND_NONCE);

        }
        free_args(args, 3);
        free(c);
        set_BLE_user_state_to_connecting();
        return ack ? ACK_MESSAGE : NAK_MESSAGE;
    } else if (strcmp(c, "RUI") == 0) { // Request User Invite
        char **args = getArgs(cmd, 4);

        if (args == NULL) {
            free(c);
            return NAK_MESSAGE;
        }

        if (get_user_state(user_ip) != AUTHORIZED) return false;
        authorization* auth = malloc(sizeof(authorization));

        char* phone_id = get_phone_id(user_ip);
        if (phone_id == NULL) return false;

        esp_err_t res = get_authorization(phone_id, auth);

        if (res != ESP_OK) return false;

        char* email = args[0];


        enum userType user_type = auth->user_type;

        int valid_from = -1;
        int valid_until = -1;
        char* weekdays_str = NULL;
        int one_day = -1;

        switch (user_type) {
            case admin:
            case owner:
                break;
            case periodic_user:

                weekdays_str = malloc(7 * sizeof(char) + 1);

                char str[12];
                for (int i = 0; i < 7; i++) {
                    if (auth->weekdays[i]) {
                        sprintf(str, "%d", i+1);
                        strcat(weekdays_str, str);
                    }
                }

            case tenant:
                valid_from = auth->valid_from_ts;
                valid_until = auth->valid_until_ts;
                break;
            case one_time_user:
                one_day = auth->one_day_ts;
                break;
            default:
                break;
        }

        char* response = NAK_MESSAGE;

        if (verifyTimestampsAndNonce(args, 1)) {
            if (canUseLock(user_ip)) {
                if (!canCreateInvite(user_ip,  user_type, valid_from, valid_until, one_day)) {
                    ESP_LOGE("Error", "User does not have enough permissions to create this invite.");
                    response = NAK_MESSAGE;
                } else {
                    char* invite_id = create_invite(1649787416/*fixme change*/, user_type, valid_from, valid_until, weekdays_str, one_day, email);

                    response = malloc(strlen("XXX ") + strlen(invite_id) + 1);

                    sprintf(response, "SUI %s", invite_id);
                    if (invite_id == NULL) {
                        ESP_LOGE("Error", "Could not create invite");
                        response = NAK_MESSAGE;
                    }
                }
            } else {
                ESP_LOGE("Error", ERROR_NO_PERMISSIONS);
            }
        } else {
            ESP_LOGE("Error", ERROR_VERIFYING_TIMESTAMP_AND_NONCE);
        }

        free_args(args, 4);
        free(c);
        set_BLE_user_state_to_connecting();
        return response;
    } else if (strcmp(c, "SNT") == 0) {
        char **args = getArgs(cmd, 4);

        if (args == NULL) {
            free(c);
            return NAK_MESSAGE;
        }

        bool ack = false;
        if (verifyTimestampsAndNonce(args, 1)) {
            if (get_user_state(user_ip) == AUTHORIZED) {
                send_notification(args[0]);
                ack = true;
            }
        } else {
            ESP_LOGE("Error", ERROR_VERIFYING_TIMESTAMP_AND_NONCE);

        }
        free_args(args, 4);
        free(c);
        return ack ? ACK_MESSAGE : NAK_MESSAGE;
    } else if (strcmp(c, "RDS") == 0) {
        char **args = getArgs(cmd, 3);

        if (args == NULL) {
            free(c);
            set_BLE_user_state_to_connecting();
            return NAK_MESSAGE;
        }

        char* response = NAK_MESSAGE;
        if (verifyTimestampsAndNonce(args, 0)) {
            if (get_user_state(user_ip) == AUTHORIZED) {
                response = malloc(strlen("XXX X") + 1);

                lock_state_t lock_state = get_lock_state();

                ESP_LOGE("STATE -> ", "SDS %d", lock_state);

                sprintf(response, "SDS %d", lock_state);
            }
        } else {
            ESP_LOGE("Error", ERROR_VERIFYING_TIMESTAMP_AND_NONCE);
        }

        free_args(args, 3);
        free(c);
        set_BLE_user_state_to_connecting();
        return response;
    } else {
        set_BLE_user_state_to_connecting();
        return NAK_MESSAGE;
    }
	return NAK_MESSAGE;
}

// int main() {
// 	char s[20];
// 	printf("> ");
//     scanf("%[^\n]%*c", s);

// 	printf("-> %d\n", checkCommand(s));

//     return 0;
// }
  