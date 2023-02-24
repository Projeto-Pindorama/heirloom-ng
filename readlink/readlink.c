/*
 * readlink.c - displays the target of a symbolic link 
 */
/* Copyright (C) 2023: Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 *
 * Heavly based (almost 1:1) to OpenBSD's readlink(1) implementation, originally
 * copyright 1997 by Kenneth Stailey and the OpenBSD project, but rewritten from
 * scratch to look more like a traditional UNIX(R) utility, with SVID4-style
 * error reporting.
 */

#include <limits.h>
#include <pfmt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *progname;
void usage(void);
int main(int argc, char *argv[]);

struct Flag {
	/* Per default, readlink(1) doesn't follow links nor prints without
	 * newline, so we will use the default int value which is zero. */
	int follow_link;
	int no_newline;
};
struct Flag flags;

int main(int argc, char *argv[]) {
	progname = argv[0];

	extern int optind;
	int name_size, option;
	char buffer[PATH_MAX];
	
	while ( (option = getopt(argc, argv, "fn")) != -1 ){
		switch (option) {
			case 'f':
				flags.follow_link = 1;
				break;
			case 'n':
				flags.no_newline = 1;
				break;
			default:
				usage();
		}
	}
	argc -= optind;
	argv += optind;
	name_size = strlen(argv[0]);

	if ( argc != 1 ) {
		usage();
	}

	if ( name_size > (PATH_MAX - 1) ) {
		pfmt(stderr, MM_ERROR, "%s: file name exceeds PATH_MAX - 1 (%d).",
				progname, (PATH_MAX - 1));
		exit(2);
	}
	
	/* readlink is meant to read links, so, unlike realpath, it won't be
	 * printing anything and will exit with non-zero status unless we're
	 * using the -f flag (flags.follow_link = 1).
	 */
	if (flags.follow_link){
		if ( realpath(argv[0], buffer) == NULL ) {
			exit(1);
		}
	} else if ( (name_size = readlink(argv[0], buffer, sizeof(buffer - 1))) < 0 ) {
		exit(1);
		buffer[name_size] = '\0';
	}
	
	printf("%s", buffer);
	if ( !flags.no_newline ) {
		printf("%c", '\n');
	}
	
	exit(0);
}

void usage(void) {
	pfmt(stderr, MM_NOSTD, "usage: %s [-fn] [file]\n", progname);
	exit(1);
}
