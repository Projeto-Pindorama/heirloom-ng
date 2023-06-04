/*
 * seq.c - print a sequence of numbers
 */
/* 
 * Copyright (C) 2023: Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 */

//#include <pfmt.h>
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

	// FIXME: For some reason, the pointer to "separator" is now 4 when
	// it is empty with steps as argv[1] and 3 when stop as argv[1], so
	// I will be irresponsibly relying on this factor to check if the
	// user has actually set a separator or not.
	separator = ( *separator == 4 || *separator == 3 ) ? "\n" : separator; 

	stop = ( argc < 1 ) ? atoi(argv[0]) : ( argc == 3 ) ? atoi(argv[2]) : atoi(argv[1]);
	// FIXME: According to gdb(1), the program goes segmentation fault
	// when "stop" is the only argument and start isn't directly defined.
	// Why?
	start = ( argc < 1 ) ? 1 : atoi(argv[0]);
	step = ( argc < 3 ) ? 1 : atoi(argv[1]);

	for ( count = start; count <= stop; count += step ) {
		separator = (count == stop) ? "\n" : separator;
		printf("%d%s", count, separator);
	}
}

void usage(void) {
//	pfmt(stderr, MM_NOSTD, "usage: %s: [-s separator] [start] stop [step]\n", progname);
	// FIXME: For some funny reason, pfmt() causes this program to go
	// segmentation fault on the point that we set separator using the
	// ternary relying on the pointer numeric value, so I'll be using
	// fprintf() until this ca be get around.
	fprintf(stderr, "usage: %s: [-s separator] [start] stop [step]\n", progname);
	exit(1);
}
