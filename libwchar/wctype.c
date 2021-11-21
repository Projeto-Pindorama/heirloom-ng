/*
 * Copyright (c) 2004 Gunnar Ritter
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
