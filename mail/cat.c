/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "cat.c	1.6	05/06/08 SMI" 	 SVr4.0 1.3		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)cat.c	1.4 (gritter) 6/18/05
 */
/*
    NAME
	cat - concatenate two strings

    SYNOPSIS
	void cat(char *to, char *from1, char *from2)

    DESCRIPTION
	cat() concatenates "from1" and "from2" to "to"
		to	-> destination string
		from1	-> source string
		from2	-> source string
*/
#include "mail.h"
#define	next(to, tosize, i) \
		if ((i) >= *(tosize)) \
			*(to) = srealloc(*(to), *(tosize) += 32); \
		(*(to))[(i)++]
	
void 
cat(char **to, size_t *tosize,
		register const char *from1, register const char *from2)
{
	size_t	i = 0;

	while (from1 && *from1) {
		next(to, tosize, i) = *from1++;
	}
	while (from2 && *from2) {
		next(to, tosize, i) = *from2++;
	}
	next(to, tosize, i) = '\0';
}

void
concat(char **to, size_t *tosize, const char *from)
{
	size_t	i = 0;

	if (*to)
		while ((*to)[i] != '\0')
			i++;
	while (from && *from) {
		next(to, tosize, i) = *from++;
	}
	next(to, tosize, i) = '\0';
}
