#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

char *BUILTIN_EXIT = "exit";
char *BUILTIN_PWD = "pwd";
char *BUILTIN_CD = "cd";
char *BUILTIN_PATH = "path";

// Why not char*?  See: https://stackoverflow.com/a/164258
char DEFAULT_PATH[] = "/bin /usr/bin";
char **paths = NULL;
size_t paths_len = 0;

size_t words_len;  // Ain't nobody got time for...  Just go global.


char **_tokenize(char *line) {
	/* NOTE: Free what you get, don't mess with
	 * words_len and all's gonna be fine!
	 */

	char **words = NULL;
	char *word;

	words_len = 0;

	while ((word = strsep(&line, " \n")) != NULL) {
		if (*word == '\0') {
			continue;
		}

		words_len ++;
		words = realloc(words, words_len * sizeof(char*));
		words[words_len - 1] = word;
	}

	return words;
}


char *_path_concat(char *base, char *argv0) {
	bool needs_slash = base[strlen(base) - 1] != '/';
	size_t total_len = strlen(base) + strlen(argv0) + 1;
	if (needs_slash) {
		total_len++;
	}
	char *path = malloc(total_len);
	strcpy(path, base);
	if (needs_slash) {
		strcat(path, "/");
	}
	strcat(path, argv0);
	return path;
}


/* TODO: Error handlings, resource closing etc...  Final check with valgrind
 * TODO: Sigint handler.  (So that the shell won't quit on C-c.)
 */
int main(int argc, char *argv[]) {
	char *line = NULL;
	size_t line_len = 0;
	char **words;
	int cmd_pid;
	int cmd_wstatus;
	char *cwd = NULL;

	paths = _tokenize(DEFAULT_PATH);
	paths_len = words_len;

	while (true) {
		fprintf(stdout, "wish> ");

		if (getline(&line, &line_len, stdin) < 0) {
			// TODO: Consistent cleanups when exiting
			if (feof(stdin)) {
				return 0;
			}
			perror("Couln't read input (getline)");
			return 1;
		}

		words = _tokenize(line);

		if (!words_len) {
			continue;
		}

		if (!strcmp(words[0], BUILTIN_EXIT)) {
			if (words_len > 1) {
				fprintf(stderr, "\"exit\" expects no arguments\n");
				return 1;
			}
			return 0;  // TODO: Cleanup
		}

		if (!strcmp(words[0], BUILTIN_PWD)) {
			cwd = getcwd(NULL, 0);  // TODO: Error handling
			printf("%s\n", cwd);
			continue;
		}

		if (!strcmp(words[0], BUILTIN_CD)) {
			if (words_len != 2) {
				fprintf(stderr, "\"%s\" expects one argument\n", BUILTIN_CD);
			} else {
				if (chdir(words[1]) < 0) {
					// Just mimicking bash
					fprintf(stderr, "cd: %s: ", words[1]);
					perror("");
				}
			}
			continue;
		}

		if (!strcmp(words[0], BUILTIN_PATH)) {
			if (words_len < 2) {
				free(paths);
				paths_len = 0;
				continue;
			}
			paths = &words[1];  // TODO: paths needs its own malloc
			paths_len = words_len - 1;
			fprintf(stderr, "DEBUG: paths[0]: \"%s\"\n", paths[0]);
			fprintf(stderr, "DEBUG: paths_len: %ld\n", paths_len);
			continue;
		}

		// TODO: WIP
		for (int i = 0; i < 1; i++) {
			char *path = _path_concat(paths[i], words[0]);
			if (access(path, X_OK) < 0) {
				perror("X_OK nono");
				continue;
			}
			// printf("Trying to run %s\n", path);
			if (!(cmd_pid = fork())) {
				words = realloc(words, (words_len + 1) * sizeof(char*));
				words[words_len] = NULL;
				execv(path, words);
				perror("TODO: execv");
				return 1;
			}

			if (cmd_pid < 0) {
				perror("TODO: fork");
				return 1;
			}

			do {
				wait(&cmd_wstatus);
			} while (!WIFEXITED(cmd_wstatus) && !WIFSIGNALED(cmd_wstatus));
			break;
		}

		free(words);
	}

	free(line);

	return 0;
}

