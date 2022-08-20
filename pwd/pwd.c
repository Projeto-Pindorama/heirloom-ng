/*
 * pwd - print working directory
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, June 2005.
 */
/*
 * Copyright (c) 2005 Gunnar Ritter
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
static const char sccsid[] USED = "@(#)pwd.sl	1.1 (gritter) 6/29/05";

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

int
main(int argc, char **argv)
{
	size_t	size = 0;
	char	*buf = NULL, *cwd = NULL;

	do {
		if ((buf = realloc(buf, size += 256)) == NULL)
			break;
	} while ((cwd = getcwd(buf, size)) == NULL && errno == ERANGE);
	if (cwd == NULL) {
		fputs("cannot determine current directory\n", stderr);
		return 1;
	}
	puts(cwd);
	return 0;
}
