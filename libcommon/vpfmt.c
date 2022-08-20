/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)vpfmt.c	1.2 (gritter) 9/21/03	*/

#include <stdio.h>
#include <stdarg.h>

#include "pfmt.h"

extern char	*pfmt_label__;

/*
 * Strip catalog and msgnum from s, but only if they actually appear.
 */
static const char *
begin(const char *s, long flags)
{
	const char	*sp;

	if (flags & MM_NOGET)
		return s;
	sp = s;
	if (*sp && *sp != ':') {
		sp++;
		while (*sp && *sp != '/' && *sp != ':' && sp - s < 14)
			sp++;
	}
	if (*sp++ != ':')
		return s;
	while (*sp >= '0' && *sp <= '9')
		sp++;
	if (*sp++ != ':' || *sp == '\0')
		return s;
	return sp;
}

int
vpfmt(FILE *stream, long flags, const char *fmt, va_list ap)
{
	int	n = 0;
	const char	*severity = NULL;
	char	sevbuf[25];

	if ((flags&MM_NOSTD) == 0) {
		if (flags & MM_ACTION)
			severity = "TO FIX";
		else switch (flags & 0377) {
		case MM_HALT:
			severity = "HALT";
			break;
		case MM_WARNING:
			severity = "WARNING";
			break;
		case MM_INFO:
			severity = "INFO";
			break;
		case MM_ERROR:
			severity = "ERROR";
			break;
		default:
			snprintf(sevbuf, sizeof sevbuf, "SEV=%ld", flags&0377);
			severity = sevbuf;
		}
		if (pfmt_label__)
			n = fprintf(stream, "%s: ", pfmt_label__);
		if (severity)
			n += fprintf(stream, "%s: ", severity);
	}
	n += vfprintf(stream, begin(fmt, flags), ap);
	return n;
}
