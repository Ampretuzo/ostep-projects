#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

struct tokens {
	char **list;
	size_t len;
};

struct command {
	char *line;
	struct tokens tokens;
};

char *BUILTIN_EXIT = "exit";
char *BUILTIN_PWD = "pwd";
char *BUILTIN_CD = "cd";
char *BUILTIN_PATH = "path";

// Why not char*?  See: https://stackoverflow.com/a/164258
char *DEFAULT_PATH[2] = {"/bin", "/usr/bin"};
char **path;  // Null-terminated, for fun


char *path_concat(char *base, char *tail) {
	char *path;

	bool needs_slash = base[strlen(base) - 1] != '/';
	size_t total_len = strlen(base) + strlen(tail) + 1;
	if (needs_slash) {
		total_len++;
	}
	path = malloc(total_len);
	strcpy(path, base);
	if (needs_slash) {
		strcat(path, "/");
	}
	strcat(path, tail);

	return path;
}

void change_path(char **tokens, int tokens_len) {
	// Mutates global `path`
	path = malloc(sizeof(char*) * tokens_len + 1);
	path[tokens_len - 1] = NULL;
	fprintf(stderr, "Changing path:");
	for (int i = 0; i < tokens_len; i++) {
		path[i] = strdup(tokens[i]);
		fprintf(stderr, " \"%s\"", path[i]);
	}
	fprintf(stderr, "\n");
}

struct tokens tokenize(char *line) {
	char **tokens = NULL;
	size_t tokens_len = 0;
	char *token;

	while ((token = strsep(&line, " \n")) != NULL) {
		if (*token == '\0') {
			continue;
		}

		tokens_len ++;
		tokens = realloc(tokens, tokens_len * sizeof(char*));
		tokens[tokens_len - 1] = token;
	}

	return (struct tokens){tokens, tokens_len};
}

void command_init(struct command *command, char *line) {
	command->line = strdup(line);
	command->tokens = tokenize(command->line);
}

void handle(struct command *command) {
	// noop

	if (!command->tokens.len) {
		return;
	}

	// builtins
	
	char *cmd = command->tokens.list[0];
	
	if (!strcmp(cmd, BUILTIN_EXIT)) {
		if (command->tokens.len > 1) {
			fprintf(stderr, "\"exit\" expects no arguments\n");
			exit(1);
		}
		exit(0);
	}

	if (!strcmp(cmd, BUILTIN_PWD)) {
		char *cwd = getcwd(NULL, 0);  // TODO: Error handling
		printf("%s\n", cwd);
		return;
	}

	if (!strcmp(cmd, BUILTIN_CD)) {
		if (command->tokens.len != 2) {
			fprintf(stderr, "\"%s\" expects exactly one argument\n", BUILTIN_CD);
		} else {
			if (chdir(command->tokens.list[1]) < 0) {
				// Just mimicking bash
				fprintf(stderr, "cd: %s: ", command->tokens.list[1]);
				perror("");
			}
		}
		return;
	}

	if (!strcmp(cmd, BUILTIN_PATH)) {
		change_path(&command->tokens.list[1], command->tokens.len - 1);
	}

	// TODO: binary command

	for (int i = 0; i < 1; i++) {
		char *exec_path = path_concat(path[i], cmd);
		if (access(exec_path, X_OK) < 0) {
			perror("X_OK nono");
			continue;
		}

		int cmd_pid;
		int cmd_wstatus;
		if (!(cmd_pid = fork())) {
			char **args = malloc((command->tokens.len + 1) * sizeof(char*));
			memcpy(
				args,
				command->tokens.list,
				command->tokens.len * sizeof(char*)
			);
			args[command->tokens.len] = (char*) NULL;
			execv(exec_path, args);
			perror("TODO: execv");
			exit(1);
		}

		if (cmd_pid < 0) {
			perror("TODO: fork");
			exit(1);
		}

		do {
			wait(&cmd_wstatus);
		} while (!WIFEXITED(cmd_wstatus) && !WIFSIGNALED(cmd_wstatus));
		break;
	}
}

/*
*/

/* TODO: Error handlings, cleanup and resource closing etc...  Final check with valgrind
 * TODO: Sigint handler.  (So that the shell won't quit on C-c.)
 */
int main(int argc, char *argv[]) {
	char *line = NULL;
	struct command command;
	size_t line_len = 0;
	
	change_path(DEFAULT_PATH, 2);

	while (true) {
		fprintf(stdout, "wish> ");

		if (getline(&line, &line_len, stdin) < 0) {
			if (feof(stdin)) {
				return 0;
			}
			perror("Couln't read command line (getline)");
			return 1;
		}

		command_init(&command, line);
		handle(&command);

	}

	return 0;
}

