/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute
 * it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
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
