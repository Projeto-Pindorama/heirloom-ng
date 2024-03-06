/*
 * Copyright (c) 2005 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)mblen.c	1.2 (gritter) 1/15/05	*/

#include "wchar.h"

int
mblen(const char *s, size_t n)
{
	if (s != NULL) {
		if (n < 1)
			return 0;
		else if (*s == '\0')
			return 0;
		else
			return 1;
	} else
		return 0;
}
