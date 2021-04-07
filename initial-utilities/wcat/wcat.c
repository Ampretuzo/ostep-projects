#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>


int main(int argc, char* argv[]) {
    // NOTE: What's the purpose of cat reading from stdin when no
    // file is specified?
    // Assertion below fails the tests/4.run
    // assert(1 < argc && "At least one file is necessary");

    bool could_not_open_file = false;

    for (int i = 1; i < argc; i++) {
        char* filename = argv[i];

        FILE* file = fopen(filename, "r");
        if (file == NULL) {
            perror(filename);
            could_not_open_file = true;
            continue;
        }

        // Sipping on the stream with 32 characters.  (Tradeoff with syscalls?)
        char buffer[32];
        while (fgets(buffer, 32, file) != NULL) {
            printf("%s", buffer);
        }

        if (ferror(file)) {
            perror(filename);
            fclose(file);
            exit(1);
        }

        fclose(file);
    }

    if (could_not_open_file) {
        return 1;
    }

    return 0;
}
