#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>


#define fsame(stat1, stat2) (stat1.st_dev == stat2.st_dev && stat1.st_ino == stat2.st_ino)


int reverse_seekable(FILE *in, FILE *out, FILE *err) {
	char *line = NULL;
	size_t line_len = 0;
	fpos_t fpos_start;

	if(fseek(in, 0, SEEK_END) < 0) {
		perror("fseek to the end of file in reverse_seekable");
		return 1;
	}

	/* NOTE: From this point on I'm just assuming
	 * that syscalls won't error out.
	 * Otherwise it gets too verbose.
	 * That's why exceptions are so great btw...
	 */

	while (true) {
		fgetpos(in, &fpos_start);
		getline(&line, &line_len, in);
		fprintf(out, "%s", line);
		fsetpos(in, &fpos_start);

		fseek(in, -1, SEEK_CUR);
		if (ftell(in) <= 0) {
			return 0;
		}
		bool last_char = true;
		while (true) {
			if (ftell(in) <= 0) {
				break;
			}
			int c = fgetc(in);
			if (!last_char && c == '\n') {
				break;
			}
			last_char = false;
			fseek(in, -2, SEEK_CUR);
		}
	}
}


int reverse(FILE *in, FILE *out, FILE *err) {
	char **lines = NULL;
	size_t lines_size = 0;
	size_t num_lines = 0;
	char *new_line;
	size_t line_len;
	int ret_val = 0;

	while (true) {
		new_line = NULL;
		line_len = 0;
		if (getline(&new_line, &line_len, in) < 0) {
			if (ferror(in)) {
				perror("getline in reverse");
				free(new_line);
				ret_val = 1;
			}
			break;
		}

		if (num_lines + 1 > lines_size) {
			lines_size = lines_size == 0 ? 1 : lines_size * 2;
			void *new_lines;
			if ((
				new_lines = realloc(lines, lines_size * sizeof(char *) )
			)) {
				lines = new_lines;
			} else {
				fprintf(err, "realloc failed in reverse.  Possibly out-of-memory\n");
				ret_val = 1;
				break;
			}
		}

		lines[num_lines] = new_line;
		num_lines ++;
	}

	for (int i = 0; i < num_lines; i++) {
		int line_idx = num_lines - i - 1;

		fprintf(out, lines[line_idx]);  // NOTE: fprintf errors?
		free(lines[line_idx]);
	}

	return ret_val;
}


int main(int argc, char *argv[]) {
	if (argc > 3) {
		fprintf(stderr, "usage: reverse [input] [output]\n");
		return 1;
	}

	if (argc == 1) {
		return reverse(stdin, stdout, stderr);
	}

	char *input_filename;
	FILE *input_file;

	input_filename = argv[1];
	input_file = fopen(input_filename, "r");
	if (!input_file) {
		fprintf(stderr, "reverse: cannot open file '%s'\n", input_filename);
		return 1;
	}

	if (argc == 2) {
		return reverse_seekable(input_file, stdout, stderr);
	}

	if (argc == 3) {
		char *output_filename;
		FILE *out;
		struct stat input_stat;
		struct stat output_stat;

		output_filename = argv[2];

		if(stat(input_filename, &input_stat)) {
			fprintf(stderr, "reverse: cannot stat file '%s'\n", input_filename);
			return 1;
		}
		if(!stat(output_filename, &output_stat)) {
			if (fsame(input_stat, output_stat)) {
				fprintf(stderr, "reverse: input and output files must differ\n");
				return 1;
			}
		}
		out = fopen(output_filename, "w");
		if (!out) {
			fprintf(stderr, "reverse: cannot open file '%s'\n", output_filename);
			return 1;
		}
		return reverse_seekable(input_file, out, stderr);
	}

}

