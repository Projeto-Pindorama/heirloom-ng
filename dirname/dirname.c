/*
 * dirname - return the directory portion of a pathname
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, March 2002.
 */
/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute
 * it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
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
