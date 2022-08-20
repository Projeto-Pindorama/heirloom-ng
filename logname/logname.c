/*
 * logname - get login name
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
static const char sccsid[] USED = "@(#)logname.sl	1.3 (gritter) 5/29/05";

#include <unistd.h>
#include <stdio.h>

int
main(int argc, char **argv)
{
	char	*lp;

	if ((lp = getlogin()) == NULL) {
		fputs("Cannot get login name\n", stderr);
		return 1;
	}
	puts(lp);
	return 0;
}
