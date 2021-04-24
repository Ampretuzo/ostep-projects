#include <stdio.h>
#include <stdbool.h>


bool debug = false;


bool compress(FILE *file, FILE *out) {
    char prev_char;
    char curr_char;
    /*
     * NOTE: unsigned int would not waste that one bit :)
     * I'll stick to int to pass assignment tests.
     */
    int char_count = 0;
    bool eof = false;

    while(!eof) {
        /*
         * NOTE: I don't care about read optimization using buffers right now.
         * So, single char buffer it is.
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
            eof || 
            (char_count > 0 && curr_char != prev_char)
            // TODO: char_count overflow must be protected too
        );
        if (write_prev_char) {
            if (debug) {
                printf("%d%c", char_count, prev_char);
            } else {
                fwrite(&char_count, sizeof(char_count), 1, out);
                if (ferror(out)) {
                    perror("fwrite");
                    return 1;
                }
                fwrite(&prev_char, 1, 1, out);
                if (ferror(out)) {
                    perror("fwrite");
                    return 1;
                }
            }
            char_count = 0;
        }

        prev_char = curr_char;
        char_count++;
    }

    return true;
}


/*
 * Cannot argv be null terminated?
 * See: https://retrocomputing.stackexchange.com/a/5180
 */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "wzip: file1 [file2 ...]");
        return 1;
    }

    int ret_val = 0;
    char *filename;
    FILE* file;

    for (int i = 1; i < argc; i++) {
        filename = argv[i];
        file = fopen(filename, "r");  // TODO: Handle file problems

        if (!compress(file, stdout)) {
            ret_val = 1;
            break;
        }

        fclose(file);  // TODO: Handle file problems
    }

    return ret_val;
}
