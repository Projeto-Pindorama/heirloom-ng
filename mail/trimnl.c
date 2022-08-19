/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*
 * Copyright (c) 1999, by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*	from OpenSolaris "trimnl.c	1.6	05/06/08 SMI" 	 SVr4.0 1.3		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)trimnl.c	1.3 (gritter) 6/18/05
 */
/*LINTLIBRARY*/

#include <sys/types.h>
#include "libmail.h"
#include <string.h>

/*
 * Trim trailing newlines from string.
 */
void
trimnl(char *s)
{
	char	*p;

	p = s + strlen(s) - 1;
	while ((*p == '\n') && (p >= s)) {
		*p-- = '\0';
	}
}
