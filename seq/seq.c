/*
 * seq.c - print a sequence of numbers
 */
/* 
 * Copyright (C) 2023: Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 */

#include <float.h>
#include <pfmt.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *progname;
void main(int argc, char *argv[]);
void usage(void);

void main(int argc, char *argv[]){
	progname = argv[0];
	extern int optind;
	int option = 0;
	register float count = 0,
		 start = 0,
		 stop = 0,
		 step = 0;
	char *separator = "";
	
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

	/* If there's no separator set, defaults to a line break (\n). */
	separator = ( strncmp(separator, "", sizeof(char *)) == 0 ) ? "\n" : separator;

	/* 
	 * If argc equals 1, stop will be defined as the first command line argument
	 * and start and step will be defaulted to 1.
	 * If argc is 2, stop will be defined as the second command line argument and
	 * start as the first.
	 * If argc is 3, stop will be defined as the third, start as the first and
	 * step as the second.
	 */
	stop = ( argc == 1 )
		? atof(argv[0])
		: ( argc == 3 )
		? atof(argv[2])
		: atof(argv[1]);
	start = ( argc == 1 )
		? 1
		: atof(argv[0]);
	step = ( argc < 3 )
		? 1
		: atof(argv[1]);
	
	if (step == 0) {
		pfmt(stderr, MM_ERROR,
			"%s: increment can not be zero.\n",
			progname);
		exit(1);
	}

	for ( count = start; count <= stop; count += step ) {
		/* 
		 * If the count has come to the end or if the next sum is
		 * larger than stop, default separator back to '\n'.
		 */
		separator = (count == stop || (count + step) > stop)
				? "\n" : separator;
		printf("%g%s", count, separator);
	}
}

void usage(void) {
	pfmt(stderr, MM_NOSTD, "usage: %s: [-s separator] [start [step]] stop\n", progname);
	exit(1);
}
