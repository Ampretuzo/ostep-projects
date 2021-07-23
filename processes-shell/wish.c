#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define DEBUG false
#define NELEMS(x) (sizeof x / sizeof x[0])
#define PRINTDEBUG(...) if (DEBUG) { fprintf(stderr, __VA_ARGS__); }
#define GENERIC_ERROR_MESSAGE "An error has occured\n"

struct tokens {
	char **list;
	size_t len;
};

struct command {
	char *line;  // echo Hello>  hello.text
	struct tokens tokens;  // ["echo", "Hello"]
	char *redir_path;  // "hello.txt"
};

struct paths {
	char **list;
	size_t len;
};

char *BUILTIN_EXIT = "exit";
char *BUILTIN_PWD = "pwd";
char *BUILTIN_CD = "cd";
char *BUILTIN_PATH = "path";

// Why not char*?  See: https://stackoverflow.com/a/164258
char *DEFAULT_PATH[1] = {"/bin"};
struct paths paths;  // Global obj for convenience


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

void paths_init(struct paths *paths) {
	paths->len = 0;
	paths->list = malloc(0);
}

void paths_free(struct paths *paths) {
	for (int i = 0; i < paths->len; i++) {
		free(paths->list[i]);
	}
	free(paths->list);
	paths->len = 0;
}

void paths_update(struct paths *paths, char **tokens, int tokens_len) {
	paths_free(paths);
	paths->len = tokens_len;
	paths->list = malloc(sizeof(char*) * paths->len);
	PRINTDEBUG("New `paths->list` (%ld dir(s)):", paths->len);
	for (int i = 0; i < tokens_len; i++) {
		paths->list[i] = strdup(tokens[i]);
		PRINTDEBUG(" \"%s\"", paths->list[i]);
	}
	PRINTDEBUG("\n");
}

struct tokens tokenize(char *line) {
	PRINTDEBUG("Tokenizing \"%s\"\n", line);

	char **tokens = NULL;
	size_t tokens_len = 0;
	char *token;

	while ((token = strsep(&line, " \t\n")) != NULL) {
		if (*token == '\0') {
			continue;
		}

		tokens_len ++;
		tokens = realloc(tokens, tokens_len * sizeof(char*));
		tokens[tokens_len - 1] = strdup(token);
	}

	return (struct tokens){tokens, tokens_len};
}

void trim(char **str) {
	// NOTE: Modifies pointer!
	char *c = *(str) - 1;
	while (*(++c) == ' ' || *c == '\t') {
		(*str)++;
	}
	while (*(++c) != '\0') {  // Finding string end
	}
	while(*(--c) == '\n' || *c == ' ' || *c == '\t') {
		*c = '\0';
	}
}

void command_init(struct command *command, char *line) {
	command->line = strdup(line);
	command->tokens.list = NULL;
	command->tokens.len = 0;
	command->redir_path = NULL;
}

void command_free(struct command *command) {
	// TODO
	free(command->redir_path);
}

void command_parse(struct command *command) {
	char *line = strdup(command->line);

	char *after_redir = line;
	strsep(&after_redir, ">");
	if (after_redir) {
		trim(&after_redir);
		command->redir_path = strdup(after_redir);
		PRINTDEBUG("Input includes redir to \"%s\"\n", command->redir_path);
	} else {
		command->redir_path = NULL;
	}
	command->tokens = tokenize(line);

	free(line);
}

void handle(struct command *command) {
	if (command->tokens.len) {
		PRINTDEBUG("Handling executable \"%s\"\n", command->tokens.list[0]);
	} else {
		PRINTDEBUG("\"%s\" has no executable\n", command->line);
	}
	PRINTDEBUG("Redirect output to \"%s\"\n", command->redir_path);

	FILE *out;
	if (command->redir_path) {
		out = fopen(command->redir_path, "w");
		if (!out) {
			perror(command->redir_path);
			return;
		}
	} else {
		out = stdout;
	}

	// noop

	if (!command->tokens.len) {
		return;
	}

	// builtins
	
	char *cmd = command->tokens.list[0];
	
	if (!strcmp(cmd, BUILTIN_EXIT)) {
		if (command->tokens.len > 1) {
			fprintf(stderr, "\"exit\" expects no arguments\n");
			return;
		}
		exit(0);
	}

	if (!strcmp(cmd, BUILTIN_PWD)) {
		char *cwd = getcwd(NULL, 0);  // TODO: Error handling
		fprintf(out, "%s\n", cwd);
		fflush(out);  fsync(fileno(out));
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
		paths_update(&paths, &command->tokens.list[1], command->tokens.len - 1);
		return;
	}

	// TODO: WIP: binary command
	
	char *candidate_exec_path;
	char *exec_path = NULL;

	for (int i = 0; i < paths.len; i++) {
		candidate_exec_path = path_concat(paths.list[i], cmd);
		PRINTDEBUG("Testing %s exec access\n", candidate_exec_path);

		if (access(candidate_exec_path, F_OK)) {
			continue;
		}

		if (!access(candidate_exec_path, X_OK)) {
			exec_path = candidate_exec_path;
		}
		break;
	}

	if (!exec_path) {
		if (paths.len) {
			perror(candidate_exec_path);
		} else {
			fprintf(stderr, "Path is empty: executable was not found\n");
		}
		return;
	}

	int cmd_pid;
	int cmd_wstatus;

	if (!(cmd_pid = fork())) {
		if (command->redir_path) {
			PRINTDEBUG("Closing stdout and duping %s to it\n", command->redir_path);
			close(STDOUT_FILENO);
			if (dup(fileno(out)) < 0) {
				fprintf(stderr, GENERIC_ERROR_MESSAGE);
				exit(1);
			}
		}
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

	if (command->redir_path) {
		fclose(out);
	}
}

void shell(FILE *input, bool interactive) {
	char *line = NULL;
	size_t line_len = 0;
	struct command command;
	
	paths_init(&paths);
	paths_update(&paths, DEFAULT_PATH, NELEMS(DEFAULT_PATH));

	while (true) {
		if (interactive) {
			fprintf(stdout, "wish> ");
		}

		if (getline(&line, &line_len, input) < 0) {
			if (feof(input)) {
				return;
			}
			perror("Couln't read command line (getline)");
			return;
		}

		command_init(&command, line);
		command_parse(&command);
		handle(&command);
		command_free(&command);

	}
}

void shell_interactive() {
	shell(stdin, true);
}

void shell_script(char *file_path) {
	FILE *script;
	if ((script = fopen(file_path, "r")) == NULL) {
		perror(file_path);
		exit(1);
	};
	shell(script, false);
}

/* TODO: Error handlings, cleanup and resource closing etc...  Final check with valgrind
 * TODO: Sigint handler.  (So that the shell won't quit on C-c.)
 */
int main(int argc, char *argv[]) {
	if (argc == 1) {
		shell_interactive();
	} else if (argc == 2) {
		shell_script(argv[1]);
	} else {
		fprintf(stderr, "Usage: wish [script]\n");
		exit(1);
	}

	exit(0);
}

