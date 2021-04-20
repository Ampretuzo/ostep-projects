#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>


char *mystrstr(const char *haystack, const char *needle) {
    /*
     * Reimplement strstr.
     * For fun!
     */
    long h_len = (long) strlen(haystack);
    long n_len = (long) strlen(needle);

    for (int i = 0; i < h_len - n_len; i++) {
        bool match = true;
        for (int j = 0; j < n_len; j++) {
            if (haystack[i + j] != needle[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            return (char *) &haystack[i];
        }
    }
    
    return NULL;
}


void print_matches(FILE *input, char *searchterm) {
    char *line = NULL;
    size_t len = 0;
    bool read_successful = true;

    while (getline(&line, &len, input) != -1) {
        if (mystrstr(line, searchterm)) {
            printf(line);
        }
    }
    if (ferror(input)) {
        /*
         * NOTE: How to test this?
         * One way would be to call wgrep in a way that instead of file
         * stdin would be acquired.  But what to feed to it to cause some
         * errors?
         */
        perror("print_matches");
    }

    free(line);
    if (!read_successful) {
        fclose(input);
        exit(1);
    }
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: wgrep searchterm [file ...]\n");
        return 1;
    }

    char *searchterm;
    char **file_paths;
    int num_files;

    searchterm = argv[1];
    if (argc == 2) {
        file_paths = (char*[1]){"/dev/stdin"};
        num_files = 1;
    } else {
        file_paths = &argv[2];
        num_files = argc - 2;
    }

    for (int i = 0; i < num_files; i++) {
        char *file_path;
        FILE *file;

        file_path = file_paths[i];
        file = fopen(file_path, "r");
        if (file == NULL) {
            perror(file_path);
            return 1;
        }

        print_matches(file, searchterm);

        fclose(file);
    }

    return 0;
}

