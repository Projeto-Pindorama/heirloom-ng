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
pickFrom(register char *lineptr)
{
	register char *p;
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
