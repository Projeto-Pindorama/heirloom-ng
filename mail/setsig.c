/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "setsig.c	1.6	05/06/08 SMI" 	 SVr4.0 2.		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)setsig.c	1.5 (gritter) 6/18/05
 */
#include "mail.h"
/*
	Signal reset
	signals that are not being ignored will be 
	caught by function f
		i	-> signal number
		f	-> signal routine
	return
		rc	-> former signal
 */
void (*
setsig(int i, void (*f)(int)))(int)
{
	register void (*rc)(int);

	if ((rc = sigset(i, SIG_IGN)) != SIG_IGN)
		sigset(i, f);
	return(rc);
}
