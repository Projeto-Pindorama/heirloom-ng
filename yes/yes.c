/*
 * yes - print string repeatedly
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, August 2003.
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
static const char sccsid[] USED = "@(#)yes.sl	1.4 (gritter) 5/29/05";

#include <stdio.h>

int
main(int argc, char **argv)
{
	while (puts(argc>1?argv[1]:"y") != EOF);
	return 1;
}
