/*
 * Simple Regular Expression functions. Derived from Unix 7th Edition,
 * /usr/src/cmd/expr.y
 *
 * Modified by Gunnar Ritter, Freiburg i. Br., Germany, January 2003.
 *
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * SPDX-Licence-Identifier: Caldera
 */

/*	Sccsid @(#)regexpr.c	1.8 (gritter) 10/13/04	*/

#include	<stdlib.h>
#include	"regexpr.h"

int	regerrno, reglength;
static int	circf;

static char	*regexpr_compile(char *, char *, const char *, int);

char *
compile(const char *instring, char *ep, char *endbuf)
{
	char	*cp;
	int	sz = 0;

	if (ep == 0) {
		for (cp = (char *)instring; *cp != '\0'; cp++)
			if (*cp == '[')
				sz += 32;
		sz += 2 * (cp - instring) + 5;
		if ((ep = malloc(sz)) == 0) {
			regerrno = 11;
			return 0;
		}
		endbuf = &ep[sz];
		ep[1] = '\0';
	}
	if ((cp=regexpr_compile((char *)instring, &ep[1], endbuf, '\0')) == 0) {
		if (sz)
			free(ep);
		return 0;
	}
	ep[0] = circf;
	reglength = cp - ep;
	return sz ? ep : cp;
}

#define	INIT			char *sp = instring;
#define	GETC()			(*sp++)
#define	PEEKC()			(*sp)
#define	UNGETC(c)		(--sp)
#define	RETURN(c)		return (c);
#define	ERROR(c)		{ regerrno = c; return 0; }

#define	compile(a, b, c, d)	regexpr_compile(a, b, c, d)
#define	regexp_h_static		static
#define	REGEXP_H_STEP_INIT	circf = *p2++;
#define	REGEXP_H_ADVANCE_INIT	circf = *ep++;

#include	"regexp.h"
