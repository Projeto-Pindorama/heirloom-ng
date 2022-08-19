/*
 * SPDX-Licence-Identifier: CDDL-1.0
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
