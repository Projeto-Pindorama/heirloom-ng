/*
 * printenv - print out the environment
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, October 2003.
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
static const char sccsid[] USED = "@(#)printenv.sl	1.3 (gritter) 5/29/05";

#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char **argv)
{
	extern char	**environ;
	char	*cp, **ep;

	if (argc > 1 && argv[1][0]=='-' && argv[1][1]=='-' && argv[1][2]=='\0')
		argc--, argv++;
	if (argc > 1) {
		if ((cp = getenv(argv[1])) == NULL)
			return 1;
		puts(cp);
	} else {
		for (ep = environ; *ep; ep++)
			puts(*ep);
	}
	return 0;
}
