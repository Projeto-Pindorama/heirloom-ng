/*
 * Copyright (c) 2004 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)wcsncmp.c	1.1 (gritter) 5/1/04	*/

#include "wchar.h"

int
wcsncmp(const wchar_t *ws1, const wchar_t *ws2, size_t n)
{
	int	diff = 0;

	while (n--) {
		diff = *ws1 - *ws2;
		if (diff != 0 || *ws1++ == 0 || *ws2++ == 0)
			break;
	}
	return diff;
}
