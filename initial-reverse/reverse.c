#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>


#define fsame(stat1, stat2) (stat1.st_dev == stat2.st_dev && stat1.st_ino == stat2.st_ino)


void reverse_seekable(FILE *in, FILE *out, FILE *err) {
	char *line = NULL;
	size_t line_len = 0;
	fpos_t fpos_start;

	fseek(in, 0, SEEK_END);

	while (true) {
		fgetpos(in, &fpos_start);
		getline(&line, &line_len, in);
		fprintf(out, "%s", line);
		fsetpos(in, &fpos_start);

		fseek(in, -1, SEEK_CUR);
		if (ftell(in) <= 0) {
			return;
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


void reverse(FILE *in, FILE *out, FILE *err) {
	char **lines = NULL;
	size_t lines_size = 0;
	size_t num_lines = 0;

	while (true) {
		char *new_line = NULL;
		size_t line_len = 0;
		if (getline(&new_line, &line_len, in) < 0) {
			break;
		}

		if (num_lines + 1 > lines_size) {
			lines_size = lines_size == 0 ? 1 : lines_size * 2;
			void *new_lines;
			if ((new_lines = realloc(lines, lines_size * sizeof(char *))) ) {
				lines = new_lines;
			} else {
				// TODO: Handle
			}
		}

		lines[num_lines] = new_line;
		num_lines ++;
	}

	for (int i = 0; i < num_lines; i++) {
		fprintf(out, lines[num_lines - i - 1]);
	}
}


/* TODO: Error handling where applicable (+ file closing and memory freeing)
 */
int main(int argc, char *argv[]) {
	if (argc > 3) {
		fprintf(stderr, "usage: reverse [input] [output]\n");
		return 1;
	}

	if (argc == 1) {
		reverse(stdin, stdout, stderr);
		return 0;
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
		reverse_seekable(input_file, stdout, stderr);
		return 0;
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
		reverse_seekable(input_file, out, stderr);
		return 0;
	}

}

