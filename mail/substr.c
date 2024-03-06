/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*
 * Copyright (c) 1999, by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*	from OpenSolaris "substr.c	1.9	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)substr.c	1.3 (gritter) 6/18/05
 */
/*LINTLIBRARY*/

#include <sys/types.h>
#include "libmail.h"

/*
 *	This routine looks for string2 in string1.
 *	If found, it returns the position string2 is found at,
 *	otherwise it returns a -1.
 */
int
substr(char *string1, char *string2)
{
	int i, j, len1, len2;

	/* the size of the substring will always fit into an int */
	/*LINTED*/
	len1 = (int)strlen(string1);
	/*LINTED*/
	len2 = (int)strlen(string2);
	for (i = 0; i < len1 - len2 + 1; i++) {
		for (j = 0; j < len2 && string1[i+j] == string2[j]; j++);
		if (j == len2)
			return (i);
	}
	return (-1);
}
