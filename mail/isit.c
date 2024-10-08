/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*
 * Copyright 1995 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	from OpenSolaris "isit.c	1.6	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)isit.c	1.5 (gritter) 7/3/05
 */

#include "mail.h"
#include "asciitype.h"

/*
 * isit(lp, type) --  match "name" portion of 
 *		"name: value" pair
 *	lp	->	pointer to line to check
 *	type	->	type of header line to match
 * returns
 *	TRUE	-> 	lp matches header type (case independent)
 *	FALSE	->	no match
 *
 *  Execpt for H_FORM type, matching is case insensitive (bug 1173101)
 */
int 
isit(char *lp, int type)
{
	char	*p;

	switch (type) {
	case H_FROM:
		for (p = header[type].tag; *lp && *p; lp++, p++) {
			if (*p != *lp)  {
				return(FALSE);
			}
		}
		break;
	default:
		for (p = header[type].tag; *lp && *p; lp++, p++) {
			if (upperconv(*p&0377) != upperconv(*lp&0377))  {
				return(FALSE);
			}
		}
		break;
	}
	if (*p == '\0') {
		return(TRUE);
	}
	return(FALSE);
}
