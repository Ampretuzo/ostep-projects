#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kv.h"

void printkv(char *key, char *value) {
    printf("%s,%s\n", key, value);
}

// Keys might have no commas, otherwise all good.
int main(int argc, char *argv[]) {
    char *command;
    char *key;
    char *value;
    struct kv kv;

    kv_init(&kv, NULL);
    for (int argidx = 1; argidx < argc; argidx ++) {
        command = strsep(&argv[argidx], ",");
        if (!strcmp(command, "p")) {
            key = strsep(&argv[argidx], ",");
            value = strsep(&argv[argidx], ",");
            if (!key || !value) {
                printf("bad command");
            } else {
                kv_put(&kv, key, value);
            }
        } else if (!strcmp(command, "g")) {
            key = strsep(&argv[argidx], ",");
            value = kv_get(&kv, key);
            if (value) {
                printf("%s,%s\n", key, value);
            } else {
                printf("%s not found\n", key);
            }
        } else if (!strcmp(command, "d")) {
            key = strsep(&argv[argidx], ",");
            if (kv_delete(&kv, key)) {
                printf("%s not found\n", key);
            }
        } else if (!strcmp(command, "c")) {
            kv_clear(&kv);
        } else if (!strcmp(command, "a")) {
            kv_callforeach(&kv, &printkv);
        } else {
            printf("bad command");
        }
    }
    kv_free(&kv);

    return 0;
}
