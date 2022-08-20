/*
 * sleep - suspend execution time for an interval
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
static const char sccsid[] USED = "@(#)sleep.sl	1.8 (gritter) 5/29/05";

#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>

static char	*progname;		/* argv[0] to main() */

static void
usage(void)
{
	fprintf(stderr, "usage: %s time\n", progname);
	exit(2);
}

int
main(int argc, char **argv)
{
	unsigned	seconds;
	char	*x;

	progname = basename(argv[0]);
	if (argc > 1 && strcmp(argv[1], "--") == 0)
		argv++, argc--;
	if (argc <= 1)
		usage();
	seconds = strtoul(argv[1], &x, 10);
	if (*x != '\0' || argv[1][0] == '-' || argv[1][0] == '+') {
		fprintf(stderr, "%s: bad character in argument\n", progname);
		return 2;
	}
	do
		seconds = sleep(seconds);
	while (seconds > 0);
	return 0;
}
