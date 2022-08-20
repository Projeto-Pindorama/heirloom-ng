/*
 * Copyright (c) 2004 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)mbstowcs.c	1.2 (gritter) 1/15/05	*/

#include <errno.h>
#include "wchar.h"

size_t
mbstowcs(wchar_t *pwcs, const char *s, size_t n)
{
	int	r = 0, m;

	while (n) {
		if ((m = mbtowc(pwcs, s, n)) < 0)
			return (size_t)-1;
		if (m == 0) {
			n++;
			break;
		}
		n -= m;
		r += m;
	}
	return r;
}
