/*
 * Copyright (c) 2004 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)wcwidth.c	1.1 (gritter) 5/1/04	*/

#include "wchar.h"
#include "wctype.h"

int
wcwidth(wchar_t wc)
{
	if (wc == 0)
		return 0;
	else if (iswprint(wc))
		return 1;
	else
		return -1;
}
