/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "pckrcvspot.c	1.7	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)pckrcvspot.c	1.4 (gritter) 6/18/05
 */
	 	/* SVr4.0 2.	*/
#include "mail.h"
/*
 * If any H_RECEIVED lines in msg, decide where to put them.
 * Returns :
 *	-1 ==> No H_RECEIVED lines to be printed.
 *	> 0 ==> Header line type after (before) which to place H_RECEIVED lines
 */
int
pckrcvspot(void)
{
	static char pn[] = "pckrcvspot";
	int	rc = 0;

	if (hdrlines[H_RECEIVED].head == (struct hdrs *)NULL) {
		rc = -1;
	} else if (orig_rcv) {
		rc = H_RECEIVED;
	} else if (orig_aff) {
		rc = H_AFWDFROM;
	} else if (fnuhdrtype == H_RVERS) {
		if (hdrlines[H_EOH].head != (struct hdrs *)NULL) {
			if (hdrlines[H_DATE].head != (struct hdrs *)NULL) {
				rc = H_DATE;
			} else {
				rc = H_EOH;
			}
		}
	} else if ((fnuhdrtype == H_MVERS) &&
	    (hdrlines[H_EOH].head != (struct hdrs *)NULL)) {
		rc = H_EOH;
	} else {
		rc = H_CTYPE;
	}
	Dout(pn, 3, "'%s'\n", (rc == -1 ? "No Received: lines": header[rc].tag));
	return (rc);
}
