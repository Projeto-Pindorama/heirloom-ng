/*
 * dirname - return the directory portion of a pathname
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, March 2002.
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
static const char sccsid[] USED = "@(#)dirname.sl	1.5 (gritter) 5/29/05";

#include <stdio.h>
#include <libgen.h>
#include <string.h>
#include <stdlib.h>

static char	*progname;

static void
usage(void)
{
	fprintf(stderr, "Usage: %s [ path ]\n", progname);
	exit(2);
}

int
main(int argc, char **argv)
{
	char	*path = 0, *cp;

	progname = basename(argv[0]);
	if (argc > 1 && argv[1][0] == '-' && argv[1][1] == '-' &&
			argv[1][2] == '\0')
		argv++, argc--;
	if (argc == 1) {
		path = ".";
		goto out;
	} else if (argc == 2)
		path = argv[1];
	else
		usage();
	if (strcmp(path, "//")) {
		for (cp = path; *cp && *cp == '/'; cp++);
		if (cp > path && *cp == '\0') {
			path = "/";
			goto out;
		}
		for (cp = path; *cp; cp++);
		while (*--cp == '/' && cp >= path);
		*cp = '\0';
	}
	for (cp = path; *cp && *cp != '/'; cp++);
	if (*cp == '\0') {
		path = ".";
		goto out;
	}
	for (cp = path; *cp; cp++);
	while (*--cp != '/');
	while (*--cp == '/');
	*++cp = '\0';
	if (*path == '\0')
		path = "/";
out:	puts(path);
	return 0;
}
