/*
 * Copyright (c) 2004 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)wctomb.c	1.1 (gritter) 5/1/04	*/

#include "wchar.h"

int
wctomb(char *s, wchar_t wchar)
{
	if (s != NULL) {
		*s = wchar;
		return 1;
	} else
		return 0;
}
