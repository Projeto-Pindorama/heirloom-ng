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

/*	Sccsid @(#)fgrep.c	1.12 (gritter) 12/17/04>	*/

/*
 * Code for traditional fgrep command only.
 */

#include	<sys/types.h>
#include	"grep.h"

char *usagemsg =
"usage: %s [ -bchilnvx ] [ -e exp ] [ -f file ] [ strings ] [ file ] ...\n";
char *stdinmsg;

void
init(void)
{
	Fflag = 1;
	ac_select();
	options = "bce:f:hilnrRvxyz";
}

void
misop(void)
{
	usage();
}

/*
 * Dummies.
 */
void
eg_select(void)
{
}

void
st_select(void)
{
}

void
rc_select(void)
{
}
