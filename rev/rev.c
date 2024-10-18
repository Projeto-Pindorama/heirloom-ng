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
 *
 * UTF-8 support borrowed from suckless sbase implementation.
 * Copyright (C) 2016 Mattias Andrée (maandree@kth.se)
 * SPDX-Licence-Identifier: MIT
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
				to_buffer[i] = '\n';
				to_buffer[(i + 1)] = NULL_BYTE;
				return 0;
			default:
				to_buffer[i] = c;
		}
	}

	return 0;
}

bool isutf8chr(char c) {
	if ((c & 0xC0) == 0x80) {
		return true;
	} else {
		return false;
	}
}

void reverse_string(char *string) {
	int len = strlen(string),
	    i = 0,
	    lf = 0;

	lf = len && string[(len - 1)] == '\n';
	len -= lf;
	i = len;
	for (len = 0; i--;) {
		switch (isutf8chr(string[i])) {
			case true:
				len++;
				break;
			default: /* "Else", "false", call it what you like. */
				fwrite((string + i), 1,
					(len + 1), stdout);
				len = 0;
				break;
		}
	}

	if (len) fwrite(string, 1, len, stdout);
	if (lf) fputc('\n', stdout);
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

		if (eof == EOF) {
			if (fdinput) {
				fclose(input);
			}
			return EXIT_SUCCESS;
		}
	}
}
