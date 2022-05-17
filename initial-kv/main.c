#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kv.h"

// Keys might have no commas, otherwise all good.
int main(int argc, char *argv[]) {
    char *command;
    char *key;
    char *value;
    struct kv kv;
    int ec = 0;

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
            ec = 1;
        } else if (!strcmp(command, "d")) {
            ec = 1;
        } else if (!strcmp(command, "c")) {
            ec = 1;
        } else if (!strcmp(command, "a")) {
            ec = 1;
        } else {
            ec = 1;
        }
    }
    kv_free(&kv);

    return ec;
}
