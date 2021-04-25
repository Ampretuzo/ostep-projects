#include <stdio.h>
#include <stdbool.h>


#define DEV_PRINT false


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "wunzip: file1 [file2 ...]\n");
        return 1;
    }

    char *filename;
    FILE *file;
    /*
     * NOTE: sizeof(int) makes this program non-portable.  How would
     * one design it so that it works on ancient devices as well?  Just
     * by using the smallest integer type like char instead of an int?
     * See: https://stackoverflow.com/a/11438838
     */
    char buffer[sizeof(int) + 1];
    int character_count;
    char character;

    for (int fi = 1; fi < argc; fi++) {
        filename = argv[fi];
        if (!(file = fopen(filename, "r"))) {
            perror(filename);
            return 1;
        }

        while (fread(&buffer, sizeof(int) + 1, 1, file) == 1) {
            character = buffer[sizeof(int)];
            /*
             * NOTE: How to make this endianness-portable?  I guess we'd need
             * to use program-specific int formatting in wzip.c
             */
            character_count = *(int *)buffer;
            if (DEV_PRINT) {
                printf(
                    "Character '%c' repeated %d times.\n",
                    character,
                    character_count
                );
            } else {
                /*
                 * NOTE: Would it make sense to do buffering or just
                 * malloc a large chunk before printf?
                 * Does syscall for each char suck?
                 */
                for (int i = 0; i < character_count; i++) {
                    printf("%c", character);
                }
            }
        }

        if (ferror(file)) {
            perror(filename);
            return 1;
        }

        if (fclose(file)) {
            perror(filename);
            return 1;
        }
    }

    return 0;
}
