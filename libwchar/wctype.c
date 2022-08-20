/*
 * Copyright (c) 2004 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)wctype.c	1.2 (gritter) 9/28/04	*/

#include "wctype.h"
#include <string.h>
#include <blank.h>

#define	thistype(s)	if (strcmp((#s), property) == 0) { \
				func = is ##s; \
				goto found; \
			}

wctype_t
wctype(const char *property)
{
	int	(*func)(int) = NULL;

	thistype(alnum);
	thistype(alpha);
	thistype(blank);
	thistype(cntrl);
	thistype(digit);
	thistype(graph);
	thistype(lower);
	thistype(print);
	thistype(punct);
	thistype(space);
	thistype(upper);
	thistype(xdigit);
found:	return (wctype_t)func;
}

int
iswctype(wint_t wc, wctype_t charclass)
{
	if (charclass == 0)
		return 0;
	return ((int (*)(int))charclass)(wc);
}
