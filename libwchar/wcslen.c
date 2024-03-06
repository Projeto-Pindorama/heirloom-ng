/*
 * Copyright (c) 2004 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)wcslen.c	1.1 (gritter) 5/1/04	*/

#include "wchar.h"

size_t
wcslen(const wchar_t *ws)
{
	size_t	n;

	for (n = 0; *ws; n++)
		ws++;
	return n;
}
