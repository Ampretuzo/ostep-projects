#include <stdio.h>
#include <stdbool.h>


bool debug = false;


/*
 * Cannot argv be null terminated?
 * See: https://retrocomputing.stackexchange.com/a/5180
 */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "wzip: file1 [file2 ...]\n");
        return 1;
    }

    char *filename;
    FILE* file;
    char prev_char;
    char curr_char;
    /*
     * NOTE: unsigned int would not waste that one bit :)
     * I'll stick to int to pass assignment tests.
     */
    int char_count = 0;
    bool eof;

    for (int i = 1; i < argc; i++) {
        filename = argv[i];
        file = fopen(filename, "r");  // TODO: Handle file problems
        eof = false;

        while(!eof) {
            /*
             * NOTE: I don't care about read optimization using buffers
             * right now.  So, single char buffer it is.
             */
            if (fread(&curr_char, 1, 1, file) < 1) {
                if (ferror(file)) {
                    perror("fread");
                    return 1;
                }
                if (feof(file)) {
                    eof = true;
                }
            }
            bool write_prev_char = (
                (eof && i >= argc - 1) || 
                (char_count > 0 && curr_char != prev_char)
                // TODO: char_count overflow must be protected too
            );
            if (write_prev_char) {
                if (debug) {
                    printf("%d%c", char_count, prev_char);
                } else {
                    fwrite(&char_count, sizeof(char_count), 1, stdout);
                    if (ferror(stdout)) {
                        perror("fwrite");
                        return 1;
                    }
                    fwrite(&prev_char, 1, 1, stdout);
                    if (ferror(stdout)) {
                        perror("fwrite");
                        return 1;
                    }
                }
                char_count = 0;
            }

            prev_char = curr_char;
            if (!eof) {
                char_count++;
            }
        }

        fclose(file);  // TODO: Handle file problems
    }

    return 0;
}
