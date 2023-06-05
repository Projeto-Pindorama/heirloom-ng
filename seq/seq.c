/*
 * seq.c - print a sequence of numbers
 */
/* 
 * Copyright (C) 2023: Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 */

// Read lines 64 to 67.
// #include <pfmt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

char *progname;
int main(int argc, char *argv[]);
void usage(void);

int main(int argc, char *argv[]){
	progname = argv[0];
	int count, start, stop, step = 0;
	char *separator = "\0";

	extern int optind; int option;
	while ( (option = getopt(argc, argv, ":s:")) != -1 ){
		switch (option) {
			case 's':
				separator = optarg;
				break;
			default:
				usage();
		}
	}
	argc -= optind;
	argv += optind;
	
	if ( argc < 1 ) {
		usage();
	}

	// If there's no separator set, defaults to a line break (\n)
	separator = ( strncmp(separator, "\0", sizeof("\0")) == 0 ) ? "\n" : separator;

	// If argc equals 1, stop will be defined as the first command line argument
	// and start and step will be defaulted to 1.
	// If argc is 2, stop will be defined as the second command line argument and
	// start as the first.
	// If argc is 3, stop will be defined as the third, start as the first and
	// step as the second.
	stop = ( argc == 1 ) ? atoi(argv[0]) : ( argc == 3 ) ? atoi(argv[2]) : atoi(argv[1]);
	start = ( argc == 1 ) ? 1 : atoi(argv[0]);
	step = ( argc < 3 ) ? 1 : atoi(argv[1]);

	for ( count = start; count <= stop; count += step ) {
		separator = (count == stop) ? "\n" : separator;
		printf("%d%s", count, separator);
	}
}

void usage(void) {
//	pfmt(stderr, MM_NOSTD, "usage: %s: [-s separator] [start] stop [step]\n", progname);
	// FIXME: For some funny reason, pfmt()/libcommon in general cause
	// "separator" to be printed as some funky "AWL=G+"/"AWL=+"/"AWL=)"/
	// "AWL&" strings. I don't know the reason yet, but I'll be sticking
	// with fprintf() until this can be get around.
	fprintf(stderr, "usage: %s: [-s separator] [start] stop [step]\n", progname);
	exit(1);
}
