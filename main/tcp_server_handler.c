#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "utils/aes_util.h"
#include "utils/authorization.h"
#include "utils/utils.h"
#include "utils/user_info.h"
#include "utils/lock_status.h"
#include "utils/time_util.h"
#include "utils/nonce.h"
#include "pushingbox_util.h"
#include "database_util.h"

#define SEP " "

#define ERROR_VERIFYING_TIMESTAMP_AND_NONCE "Message denied due to invalid timestamp or nonce!"
#define ERROR_FEW_ARGUMENTS "Message expecting %d arguments but only got %d!"

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

static char* checkCommand(char* cmd, char* user_ip, long t1) { //FIXME remove t1 after testing

    ESP_LOGW("example", "Received cmd %s", cmd);

    char* c = getCommand(cmd);

    if (strcmp(c, "SAC") == 0) {
        char** args =  getArgs(cmd, 5);

        if (args == NULL) {
            free(c);
            return NAK_MESSAGE;
        }

        char* username = args[0];
        char* auth_code = args[1];


        int is_valid = 0;
        if (verifyTimestampsAndNonce(args, 2)) {
            uint8_t* auth_seed = get_user_seed(user_ip);
            is_valid = check_authorization_code(username, auth_code, auth_seed);
            if (is_valid) set_user_state(user_ip, AUTHORIZED, username);
        } else {
            ESP_LOGE("Error", ERROR_VERIFYING_TIMESTAMP_AND_NONCE);
        }

        free(c);
        free_args(args, 5);

        return is_valid ? ACK_MESSAGE : NAK_MESSAGE;
    } else if (strcmp(c, "RUD") == 0) {
        char **args = getArgs(cmd, 3);

        if (args == NULL) {
            free(c);
            return NAK_MESSAGE;
        }

        bool ack = false;
        if (verifyTimestampsAndNonce(args, 0)) {
            if (get_user_state(user_ip) == AUTHORIZED) {
                unlock_lock();
                ack = true;
            }
        } else {
            ESP_LOGE("Error", ERROR_VERIFYING_TIMESTAMP_AND_NONCE);
        }
        free_args(args, 3);
        free(c);
        set_BLE_user_state_to_connecting();
        return ack ? ACK_MESSAGE : NAK_MESSAGE;
    } else if (strcmp(c, "RLD") == 0) {
        char **args = getArgs(cmd, 3);

        if (args == NULL) {
            free(c);
            return NAK_MESSAGE;
        }

        bool ack = false;
        if (verifyTimestampsAndNonce(args, 0)) {
            if (get_user_state(user_ip) == AUTHORIZED) {
                lock_lock();
                ack = true;
            }
        } else {
            ESP_LOGE("Error", ERROR_VERIFYING_TIMESTAMP_AND_NONCE);

        }
        free_args(args, 3);
        free(c);
        set_BLE_user_state_to_connecting();
        return ack ? ACK_MESSAGE : NAK_MESSAGE;
    } else if (strcmp(c, "RNI") == 0) {
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
            if (get_user_state(user_ip) == AUTHORIZED) {
                if (get_user_type(get_username(user_ip)) > user_type) { // fixme change permissions verification
                    ESP_LOGE("Error", "User does not have enough permissions to create this invite.");
                    response = NAK_MESSAGE;
                }
                char* invite_id = create_invite(1649787416/*fixme change*/, user_type, valid_from, valid_until, weekdays_str, one_day);

                response = malloc(strlen("XXX ") + strlen(invite_id));

                sprintf(response, "SNI %s", invite_id);
                if (invite_id == NULL) {
                    ESP_LOGE("Error", "Could not create invite");
                    response = NAK_MESSAGE;
                }

            }
        } else {
            ESP_LOGE("Error", ERROR_VERIFYING_TIMESTAMP_AND_NONCE);
        }

        free_args(args, n_args);
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
    }else {
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
  