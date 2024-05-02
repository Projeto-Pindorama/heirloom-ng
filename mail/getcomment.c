/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "getcomment.c	1.7	05/06/08 SMI" 	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)getcomment.c	1.3 (gritter) 6/18/05
 */
		/* SVr4.0 2.	*/
#include "mail.h"
/*
 * Get comment field, if any, from line.
 *	1 ==> found comment.
 *	0 ==> no comment found.
 *     -1 ==> no closing (terminating) paren found for comment.
 */

int 
getcomment (
    register char *s,
    register char *q	/* Copy comment, if found, to here */
)
{
	register char	*p, *sav_q;
	register int	depth = 0;
	
	if ((p = strchr(s, '(')) == (char *)NULL) {
		/* no comment found */
		return (0);
	}
	sav_q = q;
	while (*p) {
		*q++ = *p;
		if (*p == ')') {
			/* account for nested parens within comment */
			depth--;
			if (depth == 0) {
				break;
			}
		} else if (*p == '(') {
			depth++;
		}
		p++;
	}
	*q = '\0';
	if (*p != ')') {
		/* closing paren not found */
		*sav_q = '\0';
		return (-1);
	}
	/* found comment */
	return (1);
}
