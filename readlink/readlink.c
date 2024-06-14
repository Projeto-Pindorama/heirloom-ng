/*
 * readlink.c - displays the target of a symbolic link 
 */
/* 
 * Copyright (C) 2023: Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 *
 * Heavly based (almost 1:1, although there's not so much to change) on OpenBSD's
 * readlink(1) implementation, but rewritten from scratch to work more like a
 * traditional UNIX(R) utility, with SVID4-style error reporting.
 * As per the copyright reader of OpenBSD's "usr.bin/readlink/readlink.c":
 * Copyright (c) 1997 Kenneth Stailey (hereinafter referred to as the author)
 *
 * The non-UCB variant differs in the fact that it can handle one or more files.
 */

#include <limits.h>
#include <pfmt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *progname;
char buffer[PATH_MAX];
void usage(void);
int main(int argc, char *argv[]);
char *resolve(char *path, int name_size);

int fFollow_link = 0;
#if !defined(UCB)
char personality[] = "file ...";
#else /* BSD variant */
char personality[] = "file";
#endif

int main(int argc, char *argv[]) {
	progname = argv[0];
	
	extern int optind;
	int option = 0,
	    fNo_newline = 0,
	    name_size = 0;
	while ((option = getopt(argc, argv, "fn")) != -1) {
		switch (option) {
			case 'f':
				fFollow_link = 1;
				break;
			case 'n':
				fNo_newline = 1;
				break;
			default:
				usage();
		}
	}
	argc -= optind;
	argv += optind;

#if !defined(UCB)
	if (argc < 1) {
#else /* BSD variant */
	if (argc != 1) {
#endif
		usage();
	}

#if !defined(UCB)
	register int file;
	for (file=0; file < argc; file++){
		name_size = strlen(argv[file]);
#else /* BSD variant */
	name_size = strlen(argv[0]);
#endif

	if (name_size > (PATH_MAX - 1)) {
		pfmt(stderr, MM_ERROR, "%s: file name exceeds PATH_MAX - 1 (%d).",
				progname, (PATH_MAX - 1));
		exit(2);
	}

#if !defined(UCB)
		printf("%s", resolve(argv[file], name_size));
		
		/* GNU readlink says: 'ignoring --no-newline with multiple arguments',
		 * so it will print with new lines anyway. */
		if (!fNo_newline || ( argc > 1 ))  {
#else /* BSD variant */
	printf("%s", resolve(argv[0], name_size));

	if (!fNo_newline) {
#endif	
		printf("%c", '\n');
	}

# if !defined(UCB) 
	} /* End for() */
#endif

	exit(0);
}

char *resolve(char *path, int name_size) {
	/* readlink is meant to read links, so, unlike realpath, it won't be
	 * printing anything and will exit with non-zero status unless we're
	 * using the -f flag (flags.follow_link = 1).
	 */
	if (fFollow_link) {
		if (realpath(path, buffer) == NULL) {
			exit(1);
		}
	} else {
		if ( (name_size = readlink(path, buffer, sizeof(buffer - 1))) < 0 ) {
			exit(1);
		}
		/*
		 * Close buffer string, since readlink(2)
		 * apparently doesn't do it as realpath(3).
		 */
		buffer[name_size] = '\0';
	}

	return buffer;
}

void usage(void) {
	pfmt(stderr, MM_NOSTD, "usage: %s [-fn] [%s]\n", progname, personality);
	exit(1);
}
