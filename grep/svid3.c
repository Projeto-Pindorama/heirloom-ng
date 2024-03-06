/*
 * grep - search a file for a pattern
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, April 2001.
 */
/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */

/*	Sccsid @(#)svid3.c	1.7 (gritter) 4/17/03>	*/

/*
 * Code for SVID3 grep version only.
 */

#include	<sys/types.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	"grep.h"

int		sus = 0;

/*
 * Usage message.
 */
void
usage(void)
{
	fprintf(stderr, usagemsg, progname);
	exit(2);
}
