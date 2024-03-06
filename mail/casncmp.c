/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*
 * Copyright (c) 1999, by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*	from OpenSolaris "casncmp.c	1.9	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)casncmp.c	1.4 (gritter) 7/3/05
 */
/*LINTLIBRARY*/

/*
 *  NAME
 *	casncmp - compare strings ignoring case
 *
 *  SYNOPSIS
 *	int casncmp(char *s1, char *s2, ssize_t n)
 *
 *  DESCRIPTION
 *	Compare two strings ignoring case differences.
 *	Stop after n bytes or the trailing NUL.
 */

#include "libmail.h"
#include <sys/types.h>
#include "asciitype.h"

int
casncmp(char *s1, char *s2, ssize_t n)
{
	if (s1 == s2)
		return (0);
	while ((--n >= 0) && (lowerconv(*s1&0377) == lowerconv(*s2&0377))) {
		s2++;
		if (*s1++ == '\0')
			return (0);
	}
	return ((n < 0)? 0: (*s1 - *s2));
}
