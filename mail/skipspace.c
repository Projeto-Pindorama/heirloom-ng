/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*
 * Copyright (c) 1999, by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*	from OpenSolaris "skipspace.c	1.9	05/06/08 SMI"		SVr4.0 1.5		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)skipspace.c	1.4 (gritter) 7/3/05
 */
/*LINTLIBRARY*/

#include <sys/types.h>
#include "libmail.h"
#include "asciitype.h"

/*
 * Return pointer to first non-blank character in p
 */
char *
skipspace(char *p)
{
	while (*p && spacechar(*p&0377)) {
		p++;
	}
	return (p);
}
