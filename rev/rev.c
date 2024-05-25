/*
 * rev.c - reverse lines of a file 
 */
/* 
 * Copyright (C) 2024: Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 */

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *progname;

void main(int argc, char *argv[]) {
	progname = argv[0];
	register size_t i = 0,
		 j = 0;
	register unsigned int magia = 0;
	register char c = '\0',
		 swc = '\0';
	unsigned int slen = 0;
	int fdinput = STDIN_FILENO;
	char string[BUFSIZ] = "";
	FILE *input = NULL;

	if (argc > 1) {
		if ((fdinput = open(argv[1], O_RDONLY)) < 0) {
			fprintf(stderr, "%s: cannot open %s: %s\n", 
				progname, argv[1], strerror(errno));
			exit(1);
		}
	}

	if ((input = fdopen(fdinput, "r")) == NULL) { 
		fprintf(stderr, "%s: cannot open stream at descriptor %d: %s\n",
			progname, fdinput, strerror(errno));
		exit(1);
	}

	for (;;) {
		for (i = 0; i < BUFSIZ; i++) {
			c = fgetc(input);
			switch (c) {
				case '\n':
					break;
				case EOF:
					string[i] = '\0';
					goto eof;
				default:
					string[i] = c;
					continue;
			}
			/* Just a simple formality. */ 
			break;
		}

		slen = strlen(string);	  
		for (j = 0; j < (slen / 2); j++) {
			/*
			 * Number that will indicate the second
			 * simmetric element of a duo on the string.
			 * The subtraction of one is related to the
			 * string closer character, which would
			 * prevent the string from even being printed.
			 */
			magia = (slen - j - 1);

			swc = string[j];
			string[j] = string[magia];
			string[magia] = swc;
		}
		printf("%s\n", string);

		/* Wash the string. */ 
		strncpy(string, "", BUFSIZ);
	}

eof:
	if (fdinput) fclose(input);
	exit(0);
}
