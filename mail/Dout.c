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


/*	from OpenSolaris "Dout.c	1.5	05/06/08 SMI" 	 SVr4.0 1.		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)Dout.c	1.4 (gritter) 6/18/05
 */
/*
    NAME
	Dout - Print debug output

    SYNOPSIS
	void Dout(char *subname, int level, char *msg, ...)

    DESCRIPTION
	Dout prints debugging output if debugging is turned
	on (-x specified) and the level of this message is
	lower than the value of the global variable debug.
	The subroutine name is printed if it is not a null
	string.
*/
#include "mail.h"
#include <stdarg.h>

/* VARARGS3 PRINTFLIKE3 */
void
Dout(char *subname, int level, char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);

	if (debug > level) {
		if (subname && *subname) {
			fprintf(dbgfp,"%s(): ", subname);
		}
		vfprintf(dbgfp, fmt, args);
	}
	va_end(args);
}
