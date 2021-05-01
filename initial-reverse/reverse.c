#include <stdio.h>
#include <stdbool.h>


/* TODO: Error handling where applicable
 */
int main(int argc, char *argv[]) {
	char *input_filename;
	FILE *input_file;

	input_filename = argv[1];
	input_file = fopen(input_filename, "r");

	char *line = NULL;
	size_t line_len = 0;
	fpos_t fpos_start;

	fseek(input_file, 0, SEEK_END);

	while (true) {
		fgetpos(input_file, &fpos_start);
		getline(&line, &line_len, input_file);
		fprintf(stdout, "%s", line);
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

