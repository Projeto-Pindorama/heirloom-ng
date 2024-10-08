/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "delete.c	1.6	05/06/08 SMI" 	 SVr4.0 2.		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)delete.c	1.4 (gritter) 6/18/05
 */
#include "mail.h"
/*
	signal catching routine --- reset signals on quits and interupts
	exit on other signals
		i	-> signal #
*/
void 
delete(int i)
{
	static char pn[] = "delete";

	if (i > SIGQUIT) {
		fprintf(stderr, "%s: ERROR signal %d\n",program,i);
		Dout(pn, 0, "caught signal %d\n", i);
	} else {
		fprintf(stderr, "\n");
	}

	if (delflg && (i==SIGINT || i==SIGQUIT)) {
		longjmp(sjbuf, i);
	}
	done(0);
}
