#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kv.h"

// Keys might have no commas, otherwise all good.
int main(int argc, char *argv[]) {
    char *command;
    char *key;
    char *value;
    for (int argidx = 1; argidx < argc; argidx ++) {
        command = strsep(&argv[argidx], ",");
        if (!strcmp(command, "p")) {
            key = strsep(&argv[argidx], ",");
            value = strsep(&argv[argidx], ",");
            if (!key || !value) {
                printf("bad command");
            } else {
                kv_put(key, value);
            }
        } else if (!strcmp(command, "g")) {
            exit(1);
        } else if (!strcmp(command, "d")) {
            exit(1);
        } else if (!strcmp(command, "c")) {
            exit(1);
        } else if (!strcmp(command, "a")) {
            exit(1);
        } else {
            exit(1);
        }
    }
}
