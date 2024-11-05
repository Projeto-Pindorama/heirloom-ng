/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "dumprcv.c	1.7	05/06/08 SMI"         SVr4.0 1.6 	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)dumprcv.c	1.3 (gritter) 6/18/05
 */

#include "mail.h"
/*
 * Put out H_RECEIVED lines if necessary, or
 * suppress their printing from the calling routine.
 */
void
dumprcv(
	int	type,
	int	htype,
	int	*didrcvlines,
	int	*suppress,
	FILE	*f
)
{
	int		rcvspot;	/* Place to put H_RECEIVED lines */
	struct hdrs	*hptr;
	char		*pn = "dumprcv";

	Dout(pn, 15, "type=%d, htype=%d/%s, *didrcvlines=%d, *suppress=%d\n", type, htype, htype >= 0 ? header[htype].tag : "None", *didrcvlines, *suppress);

	rcvspot = pckrcvspot();
	if (rcvspot == -1) {
		Dout(pn, 15, "\trcvspot==-1\n");
		return;
	}

	if (htype == H_RECEIVED) {
		*suppress = TRUE;
	}

	if (*didrcvlines == TRUE) {
		Dout(pn, 15, "\tdidrcvlines == TRUE\n");
		return;
	}
	if ((htype >= 0) && (rcvspot != htype)) {
		Dout(pn, 15, "\thtype < 0 || rcvspot != htype, *suppress=%d\n", *suppress);
		return;
	}

	*didrcvlines = TRUE;
	for (hptr = hdrlines[H_RECEIVED].head;
	     hptr != (struct hdrs *)NULL;
	     hptr = hptr->next) {
		printhdr(type, H_RECEIVED, hptr, f);
	}
	Dout(pn, 15, "\t*didrcvlines=%d, *suppress=%d\n", *didrcvlines, *suppress);
}
