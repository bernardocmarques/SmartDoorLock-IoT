#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "utils/aes_util.h"
#include "utils/authorization.h"
#include "utils/utils.h"
#include "utils/user_info.h"
#include "ccomp_timer.h" //FIXME remove
#define SEP " "

static char* getCommand(char* str) {
    char* cmd = malloc(sizeof(char) * strlen(str));
    strcpy(cmd, str);
    char *pt;
    pt = strtok(cmd, SEP);
    return pt;
}


static char** getArgs(char* str, int n) {
    char* cmd = malloc(sizeof(char) * strlen(str));
    strcpy(cmd, str);
    char** args = (char**)malloc(n * sizeof(char*));
    char *pt;

    pt = strtok(cmd, SEP);
    if (pt != NULL) pt = strtok(NULL, SEP);

    int i = 0;
    while (pt != NULL) {
        args[i] = malloc(sizeof(char) * (strlen(pt)+1));
        strcpy(args[i++], pt);
        ESP_LOGI("example", "pt = %s", pt);
        pt = strtok(NULL, SEP);
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

    ESP_LOGI("example", "Received cmd %s", cmd);

    char* c = getCommand(cmd);

    if (strcmp(c, "SAC") == 0) {
        char** args =  getArgs(cmd, 2);
        char* user_id = args[0];
        char* auth_code = args[1];
        uint8_t* auth_seed = get_user_seed(user_ip);
        int is_valid = check_authorization_code(user_id, auth_code, auth_seed);
        if (is_valid) set_user_state(user_ip, AUTHORIZED);
        free(c);
        free_args(args, 2);

        return is_valid ? ACK_MESSAGE : NAK_MESSAGE;
    } else if (strcmp(c, "RUD") == 0) {
        free(c);
        ESP_LOGI("Handler", "Door unlocked!"); //FIXME remove

        if (get_user_state(user_ip) == AUTHORIZED) {
            ESP_LOGI("Handler", "Door unlocked!"); //FIXME remove
            ESP_LOGE("Handler", "After unlock: %ld", (long) ccomp_timer_stop()); //FIXME remove
            
            return ACK_MESSAGE;
        } else {
            return NAK_MESSAGE;
        }
    } else {
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
  