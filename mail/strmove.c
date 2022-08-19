/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*
 * Copyright (c) 1999, by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*	from OpenSolaris "strmove.c	1.8	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)strmove.c	1.3 (gritter) 6/18/05
 */
/*LINTLIBRARY*/

#include <sys/types.h>
#include "libmail.h"

/*
 *  NAME
 *	strmove - copy a string, permitting overlaps
 *
 *  SYNOPSIS
 *	void strmove(char *to, char *from)
 *
 *  DESCRIPTION
 *	strmove() acts exactly like strcpy() with the additional
 *	guarantee that it will work with overlapping strings.
 *	It does it left-to-right, a byte at a time.
 */

void
strmove(char *to, char *from)
{
	while ((*to++ = *from++) != 0)
		;
}
