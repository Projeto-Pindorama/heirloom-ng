/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "getarg.c	1.6	05/06/08 SMI" 	 SVr4.0 2.		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)getarg.c	1.3 (gritter) 6/18/05
 */
#include "mail.h"
/*
	get next token
		p	-> string to be searched
		s	-> area to return token
	returns:
		p	-> updated string pointer
		s	-> token
		NULL	-> no token
*/
char *
getarg(register char *s, register char *p)
{
	while (*p == ' ' || *p == '\t') p++;
	if (*p == '\n' || *p == '\0') return(NULL);
	while (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\0') *s++ = *p++;
	*s = '\0';
	return(p);
}
