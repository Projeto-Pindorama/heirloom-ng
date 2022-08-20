/*
 * line - read one line
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, December 2000.
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
static const char sccsid[] USED = "@(#)line.sl	1.11 (gritter) 5/29/05";

#include	<stdio.h>
#include	<unistd.h>

static int	status;		/* exit status */

static void
doline(int fd)
{
	char c;

	for (;;) {
		if (read(fd, &c, 1) <= 0) {
			status = 1;
			break;
		}
		if (c == '\n')
			break;
		putchar(c);
	}
	putchar('\n');
}

int
main(int argc, char **argv)
{
	doline(0);
	return status;
}
