/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "Tout.c	1.7	05/06/08 SMI"	 SVr4.0 2.		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)Tout.c	1.4 (gritter) 6/18/05
 */
/*
    NAME
	Tout - Print surrogate debug output

    SYNOPSIS
	void Tout(char *subname, char *msg, ...)

    DESCRIPTION
	Tout prints debugging output if surrogate tracing
	has been turned on (-T specified). The message will
	also go to the debug output if debugging is turned
	on (-x specified). The subroutine name is printed
	if it is not a null string.
*/
#include "mail.h"
#include <stdarg.h>

/* VARARGS2 PRINTFLIKE2 */
void
Tout(char *subname, char *fmt, ...)
{
        va_list args;

        if (debug > 0) {
                va_start(args, fmt);
                if (subname && *subname) {
                        fprintf(dbgfp,"%s(): ", subname);
                }
                vfprintf(dbgfp, fmt, args);
                va_end(args);
        }

        if (flgT) {
                va_start(args, fmt);
                vfprintf(stdout, fmt, args);
                va_end(args);
        }
}
