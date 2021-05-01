#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>


#define fsame(stat1, stat2) (stat1.st_dev == stat2.st_dev && stat1.st_ino == stat2.st_ino)


/* TODO: Error handling where applicable
 */
int main(int argc, char *argv[]) {
	if (argc < 2 || argc > 3) {
		fprintf(stderr, "usage: reverse input [output]\n");
		return 1;
	}

	char *input_filename;
	char *output_filename;
	FILE *input_file;
	FILE *out;

	input_filename = argv[1];

	if (argc > 2) {
		struct stat input_stat;
		struct stat output_stat;
		output_filename = argv[2];
		if(stat(input_filename, &input_stat)) {
			fprintf(stderr, "reverse: cannot open file '%s'\n", input_filename);
			return 1;
		}
		if(!stat(output_filename, &output_stat)) {
			if (fsame(input_stat, output_stat)) {
				fprintf(stderr, "reverse: input and output files must differ\n");
				return 1;
			}
		}
	}

	input_file = fopen(input_filename, "r");
	if (!input_file) {
		fprintf(stderr, "reverse: cannot open file '%s'\n", input_filename);
		return 1;
	}

	out = stdout;
	if (output_filename) {
		out = fopen(output_filename, "w");
		if (!out) {
			fprintf(stderr, "reverse: cannot open file '%s'\n", output_filename);
			return 1;
		}
	}

	char *line = NULL;
	size_t line_len = 0;
	fpos_t fpos_start;

	// Actual program:

	fseek(input_file, 0, SEEK_END);

	while (true) {
		fgetpos(input_file, &fpos_start);
		getline(&line, &line_len, input_file);
		fprintf(out, "%s", line);
		fsetpos(input_file, &fpos_start);

		// Position at the start of the previous line
		fseek(input_file, -1, SEEK_CUR);
		if (ftell(input_file) <= 0) {
			return 0;
		}
		bool last_char = true;
		while (true) {
			if (ftell(input_file) <= 0) {
				break;
			}
			int tmp = fgetc(input_file);
			if (!last_char && tmp == '\n') {
				break;
			}
			last_char = false;
			fseek(input_file, -2, SEEK_CUR);
		}
	}

	return 0;
}

