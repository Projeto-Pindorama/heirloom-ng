/*
 * rev.c - reverse lines of a file 
 *
 * A little bit based on Luiz' implementation
 */
/*
 * Copyright (C) 2024: Luiz Antônio Rangel (takusuman)
 * Copyright (C) 2024: Samuel Brederodes (callsamu)
 *
 * SPDX-Licence-Identifier: Zlib
 */

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(__linux__) || defined(__dietlibc__)
#define _XOPEN_SOURCE 600L
#endif

#define NULL_BYTE '\0'

static char *progname;

int copy_line(FILE *from, char *to_buffer, size_t to_buffer_size) {
	for (int i = 0; i < to_buffer_size; i++) {
		int c = fgetc(from);

		switch (c) {
			case EOF:
				to_buffer[i] = NULL_BYTE;
				return EOF;
			case '\n':
				to_buffer[i] = NULL_BYTE;
				return 0;
			default:
				to_buffer[i] = c;
		}
	}

	return 0;
}

bool isutf8chr(char c) {
	if ((c & 0x80) == 0) {
		return false;
	} else if ((c & 0xc0) == 0xc0) {
		return true;
	}
}

void reverse_string(char *string) {
	int len = strlen(string);

	for (int i = 0; i < len / 2; i++) { 
		char tmp = string[i];
		if (isutf8chr(string[len - i - 1])) {
			/* New temporary character for the string[i + 1]. */
			char ntmp = string[i + 1];

			/* Interpret "i - 2" as "(i - 1) - 1" because, for some
			 * reason, this code breaks when trying to format this
			 * equation better. Much obliged, compiler. 
			 * This move a multi-byte cluster. */
			string[i] = string[len - i - 2];
			string[i + 1] = string[len - i - 1];
			string[len - i - 1] = tmp;
			string[len - (i + 1) - 1] = ntmp;
			i++;
		} else {
			string[i] = string[len - i - 1];
			string[len - i - 1] = tmp;
		}
	}
}

int main(int argc, char *argv[]) {
	progname = argv[0];

	int fdinput = STDIN_FILENO;
	if (argc > 1) {
		fdinput = open(argv[1], O_RDONLY);

		if (fdinput < 0) {
			fprintf(stderr, "%s: %s\n",
					progname, strerror(errno));
			return EXIT_FAILURE;
		}
	}

	FILE *input = fdopen(fdinput, "r");
	if (!input) {
		fprintf(stderr, "%s: cannot open stream at descriptor %d: %s\n",
				progname, fdinput, strerror(errno));
		return EXIT_FAILURE;
	}

	char buffer[BUFSIZ];

	while (true) {
		int eof = copy_line(input, buffer, BUFSIZ);
		reverse_string(buffer);
		puts(buffer);
		strncpy(buffer, "", BUFSIZ);

		if (eof == EOF) {
			if (fdinput) {
				fclose(input);
			}
			return EXIT_SUCCESS;
		}
	}
}
