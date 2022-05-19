/*
#include <stdio.h>

int main() {
    char *null = 0;

    // Results in SIGSEGV
    printf("Char at NULL: %c\n", *null);
    return 0;
}
*/

#include "user/user.h"

int main() {
    int *null = 0;

    printf("NOTE: main() is at address %p\n", &main);

    printf("Dereferencing null address: %d\n", *null);

    *null = 99;

    printf("New value at null after writing to it: %d\n", *null);

    exit(0);
}
