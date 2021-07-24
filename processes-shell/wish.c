#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#ifndef DEBUG
#define DEBUG false
#endif
#define NELEMS(x) (sizeof x / sizeof x[0])
#define PRINTDEBUG(...) if (DEBUG) { fprintf(stderr, __VA_ARGS__); }
#define GENERIC_ERROR_MESSAGE "An error has occurred\n"

struct tokens {
	char **list;
	size_t len;
};

struct command {
	char *line;			// echo Hello>  hello.text
	struct tokens tokens;		// ["echo", "Hello"]
	char *redir_path;		// "hello.txt"
	int builtin_status;		// Exit status for builtins.  (< 0 if not yet run.)
	int fork_pid;			// Process pid if not a builtin
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
}

void paths_update(struct paths *paths, char **tokens, int tokens_len) {
	paths_free(paths);
	paths->len = tokens_len;
	paths->list = malloc(sizeof(char*) * paths->len);
	PRINTDEBUG("paths_update: New `paths->list` (%ld dir(s)):", paths->len);
	for (int i = 0; i < tokens_len; i++) {
		paths->list[i] = strdup(tokens[i]);
		PRINTDEBUG(" \"%s\"", paths->list[i]);
	}
	PRINTDEBUG("\n");
}

/**
 * line - Will be butchered
 * skip_empty - Skips empty tokens, that is - repeated delimiters
 */
struct tokens tokenize(char *line, char *delim, bool skip_empty) {
	PRINTDEBUG("tokenize: Tokenizing \"%s\" with \"%s\"\n", line, delim);

	char **tokens = NULL;
	size_t tokens_len = 0;
	char *token;

	while ((token = strsep(&line, delim)) != NULL) {
		if (skip_empty && *token == '\0') {
			continue;
		}

		tokens_len ++;
		tokens = realloc(tokens, tokens_len * sizeof(char*));
		tokens[tokens_len - 1] = strdup(token);
	}
	PRINTDEBUG("tokenize: Found %ld tokens\n", tokens_len);

	return (struct tokens){tokens, tokens_len};
}

void tokens_free(struct tokens *tokens) {
	for (size_t i = 0; i < tokens->len; i++) {
		free(tokens->list[i]);
	}
	free(tokens->list);
}

struct tokens tokenize_as_words(char *line) {
	return tokenize(line, " \t\n", true);
}

struct tokens tokenize_as_commands(char *line) {
	return tokenize(line, "&", false);
}

void command_init(struct command *command, char *line) {
	command->line = strdup(line);
	command->tokens.list = NULL;  // NOTE: tokens_init...
	command->tokens.len = 0;
	command->redir_path = NULL;
}

void command_free(struct command *command) {
	free(command->line);
	tokens_free(&command->tokens);
	free(command->redir_path);
}

bool command_parse(struct command *command) {
	char *line = strdup(command->line);

	char *redir_part = line;
	strsep(&redir_part, ">");
	if (redir_part) {
		struct tokens redir_tokens = tokenize_as_words(redir_part);
		if (redir_tokens.len != 1) {
			return true;
		}
		command->redir_path = redir_tokens.list[0];
	}
	command->tokens = tokenize_as_words(line);

	free(line);

	return false;
}

void command_execute_builtin(struct command *command, bool parallel) {
	PRINTDEBUG("command_execute_builtin: Called with parallel=%d\n", parallel);

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

	command->builtin_status = 0;

	if (!command->tokens.len) {
		return;
	}

	char *cmd = command->tokens.list[0];
	
	if (!strcmp(cmd, BUILTIN_EXIT)) {
		if (parallel) {
			return;
		}
		if (command->tokens.len > 1) {
			fprintf(stderr, "\"exit\" expects no arguments\n");
			command->builtin_status = 1;
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
		if (parallel) {
			return;
		}
		if (command->tokens.len != 2) {
			command->builtin_status = 1;
			fprintf(stderr, "\"%s\" expects exactly one argument\n", BUILTIN_CD);
		} else {
			if (chdir(command->tokens.list[1]) < 0) {
				command->builtin_status = 1;
				// Just mimicking bash
				fprintf(stderr, "cd: %s: ", command->tokens.list[1]);
				perror("");
			}
		}
		return;
	}

	if (!strcmp(cmd, BUILTIN_PATH)) {
		if (parallel) {
			return;
		}
		paths_update(&paths, &command->tokens.list[1], command->tokens.len - 1);
		return;
	}

	if (command->redir_path) {
		fclose(out);
	}

	command->builtin_status = -1;
}

void command_execute(struct command *command) {
	if (command->tokens.len) {
		PRINTDEBUG(
			"command_execute: Handling executable \"%s\"\n",
			command->tokens.list[0]
		);
	} else {
		PRINTDEBUG("command_execute: \"%s\" has no executable\n", command->line);
	}
	PRINTDEBUG("command_execute: Redirect output to \"%s\"\n", command->redir_path);

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

	char *candidate_exec_path;
	char *exec_path = NULL;

	for (int i = 0; i < paths.len; i++) {
		candidate_exec_path = path_concat(paths.list[i], command->tokens.list[0]);
		PRINTDEBUG("command_execute: Testing %s exec access\n", candidate_exec_path);

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
	if (!(cmd_pid = fork())) {
		if (command->redir_path) {
			PRINTDEBUG(
				"command_execute: Closing stdout and duping %s to it\n",
				command->redir_path
			);
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

	command->fork_pid = cmd_pid;

	if (cmd_pid < 0) {
		perror("Failed to fork a new process for command");
	}

	if (command->redir_path) {
		fclose(out);
	}
}

/* *******************************************************************
 * Shell related code below
 */

bool parallel_command_lines(struct tokens *tokens, char *line) {
	struct tokens ampersand_separated = tokenize_as_commands(line);
	for (int i = 0; i < ampersand_separated.len; i++) {
		if (
			!strcmp(ampersand_separated.list[i], "") &&
			i != 0 &&
			i != ampersand_separated.len - 1
		) {
			tokens_free(&ampersand_separated);
			return true;
		}
	}
	*tokens = ampersand_separated;
	return false;
}

void handle(char *line) {
	struct tokens lines;
	struct command *commands;

	if (parallel_command_lines(&lines, line)) {
		fprintf(stderr, "Only single &'s might be used\n");
		return;
	}

	commands = malloc(sizeof(struct command) * lines.len);

	for (size_t i = 0; i < lines.len; i++) {
		struct command command = commands[i];

		command_init(&command, lines.list[i]);
		if (command_parse(&command)) {
			fprintf(stderr, GENERIC_ERROR_MESSAGE);
		} else {
			command_execute_builtin(&command, lines.len > 1);
			if (command.builtin_status < 0) {
				command_execute(&command);
			}
		}
		command_free(&command);
	}

	int wstatus;

	for (size_t i = 0; i < lines.len; i++) {
		wait(&wstatus);
		PRINTDEBUG("handle: A process exited with %d\n", WEXITSTATUS(wstatus));
	}

	free(commands);
	tokens_free(&lines);
}

void shell(FILE *input, bool interactive) {
	char *line = NULL;
	size_t line_len = 0;
	
	paths_init(&paths);
	paths_update(&paths, DEFAULT_PATH, NELEMS(DEFAULT_PATH));

	while (true) {
		if (interactive) {
			fprintf(stdout, "wish> ");
		}

		if (getline(&line, &line_len, input) < 0) {
			if (!feof(input)) {
				perror("Couln't read command line (getline)");
			}
			free(line);
			return;
		}

		handle(line);
	}

	paths_free(&paths);
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

	fclose(script);
}

/* TODO: Error handlings, cleanup and resource closing etc...  Final check with valgrind
 * TODO: Sigint command_executer.  (So that the shell won't quit on C-c.)
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

