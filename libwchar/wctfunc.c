/*
 * Copyright (c) 2004 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)wctfunc.c	1.1 (gritter) 5/1/04	*/

#include "wchar.h"
#include <ctype.h>

#define	imake(s)	int \
			isw ##s(wint_t c) { \
				return is ##s(c); \
			}

#define	tmake(s)	wint_t \
			tow ##s(wint_t c) { \
				return to ##s(c); \
			}

imake(alnum);
imake(alpha);
imake(blank);
imake(cntrl);
imake(digit);
imake(graph);
imake(lower);
imake(print);
imake(punct);
imake(space);
imake(upper);
imake(xdigit);

tmake(lower);
tmake(upper);
