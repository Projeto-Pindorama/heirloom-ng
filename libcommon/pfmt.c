/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)pfmt.c	1.2 (gritter) 9/21/03	*/

#include <stdio.h>
#include <stdarg.h>

#include "pfmt.h"

int
pfmt(FILE *stream, long flags, const char *fmt, ...)
{
	va_list	ap;
	int	i;

	va_start(ap, fmt);
	i = vpfmt(stream, flags, fmt, ap);
	va_end(ap);
	return i;
}
