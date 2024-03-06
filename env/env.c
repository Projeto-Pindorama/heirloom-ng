/*
 * env - set environment for command
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, August 2002.
 */
/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)env.sl	1.10 (gritter) 5/29/05";

#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>

static int	iflag;			/* ignore initial environment */
static char	*progname;		/* argv[0] to main() */

extern char	**environ;

int
main(int argc, char **argv)
{
	int	i;

	progname = basename(argv[0]);
	while ((i = getopt(argc, argv, ":i")) != EOF) {
		switch (i) {
		case 'i':
			iflag = 1;
			break;
		default:
			optind--;
			goto done;
		}
	}
done:	if (optind < argc && strcmp(argv[optind], "-") == 0 &&
			strcmp(argv[optind - 1], "--")) {
		iflag = 1;
		optind++;
	}
	if (iflag)
		environ = NULL;
	while (optind < argc && strchr(argv[optind], '=') != NULL)
		putenv(argv[optind++]);
	if (optind >= argc) {
		char	**cp;

		if (environ) {
			for (cp = environ; *cp; cp++)
				puts(*cp);
		}
		return 0;
	}
	execvp(argv[optind], &argv[optind]);
	i = errno;
	fprintf(stderr, "%s: %s\n", strerror(i), argv[optind]);
	return i == ENOENT ? 0177 : 0176;
}
