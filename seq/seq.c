/*
 * seq.c - print a sequence of numbers
 */
/* 
 * Copyright (C) 2023: Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 */

#include <pfmt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char *progname;
int main(int argc, char *argv[]);
void usage(void);

int main(int argc, char *argv[]){
	progname = argv[0];
	int count, start, stop, step = 0;
	char *separator;

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

	// For some reason, the pointer to "separator" is 3 when it is empty, so
	// I will be relying on this factor to check if the user has actually
	// setted a separator or not.
	separator = ( *separator == 3 ) ? "\n" : separator; 
	stop = ( argc < 1 )  ? atoi(argv[0]) : atoi(argv[1]);
	start = ( argc < 1 ) ? 1 : atoi(argv[0]);
	step = ( argc < 3 ) ? 1 : atoi(argv[2]);

	for ( count = start; count <= stop; count += step ) {
		separator = (count == stop) ? "\n" : separator;
		printf("%d%s", count, separator);
	}
}

void usage(void) {
	pfmt(stderr, MM_NOSTD, "usage: %s: [-s separator] [start] stop [step]\n", progname);
	exit(1);
}
