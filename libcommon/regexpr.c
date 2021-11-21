/*
 * Simple Regular Expression functions. Derived from Unix 7th Edition,
 * /usr/src/cmd/expr.y
 *
 * Modified by Gunnar Ritter, Freiburg i. Br., Germany, January 2003.
 *
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   Redistributions of source code and documentation must retain the
 *    above copyright notice, this list of conditions and the following
 *    disclaimer.
 *   Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *   All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed or owned by Caldera
 *      International, Inc.
 *   Neither the name of Caldera International, Inc. nor the names of
 *    other contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE
 * LIABLE FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

#define	INIT			register char *sp = instring;
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
