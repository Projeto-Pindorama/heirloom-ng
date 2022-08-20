/*
 * Copyright (c) 2004 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)mbtowc.c	1.2 (gritter) 1/15/05	*/

#include <errno.h>
#include "wchar.h"

int
mbtowc(wchar_t *pwc, const char *s, size_t n)
{
	if (s != NULL) {
		if (n < 1) {
			errno = EILSEQ;
			return -1;
		}
		if (pwc != NULL)
			*pwc = *s & 0377;
		return *s != '\0';
	} else
		return 0;
}
