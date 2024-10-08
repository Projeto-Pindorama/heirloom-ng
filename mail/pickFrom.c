/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "pickFrom.c	1.6	05/06/08 SMI" 	 SVr4.0 2.		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)pickFrom.c	1.5 (gritter) 7/3/05
 */
#include "mail.h"
#include "asciitype.h"
/*
 * pickFrom (line) - scans line, ASSUMED TO BE of the form
 *	[>]From <fromU> <date> [remote from <fromS>]
 * and fills fromU and fromS global strings appropriately.
 */

void 
pickFrom(char *lineptr)
{
	char *p;
	static char rf[] = "remote from ";
	int rfl;

	if (*lineptr == '>')
		lineptr++;
	lineptr += 5;
	cpy(&fromU, &fromUsize, lineptr);
	for (p = fromU; *p; p++)
		if (spacechar(*p & 0377)) {
			*p = '\0';
			break;
		}
	rfl = strlen (rf);
	while (*lineptr && strncmp (lineptr, rf, rfl))
		lineptr++;
	if (*lineptr == '\0') {
		cpy(&fromS, &fromSsize, "");
	} else {
		lineptr += rfl;
		cpy(&fromS, &fromSsize, lineptr);
		for (p = fromS; *p; p++)
			if (spacechar(*p & 0377)) {
				*p = '\0';
				break;
			}
	}
}
