/*
 * grep - search a file for a pattern
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, April 2001.
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
