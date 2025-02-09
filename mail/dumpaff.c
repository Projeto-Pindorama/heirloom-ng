/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "dumpaff.c	1.7	05/06/08 SMI"         SVr4.0 1.6 	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)dumpaff.c	1.3 (gritter) 6/18/05
 */

#include "mail.h"
/*
 * Put out H_AFWDFROM and H_AFWDCNT lines if necessary, or
 * suppress their printing from the calling routine.
 */
void
dumpaff(
	int	type,
	int	htype,
	int	*didafflines,
	int	*suppress,
	FILE	*f
)
{
	int		affspot;	/* Place to put H_AFWDFROM lines */
	struct hdrs	*hptr;
	char		*pn = "dumpaff";

	Dout(pn, 15, "type=%d, htype=%d/%s, *didafflines=%d, *suppress=%d\n", type, htype, htype >= 0 ? header[htype].tag : "None", *didafflines, *suppress);

	affspot = pckaffspot();
	if (affspot == -1) {
		Dout(pn, 15, "\taffspot==-1\n");
		return;
	}

	switch (htype) {
	case H_AFWDCNT:
		*suppress = TRUE;
		Dout(pn, 15, "\tAuto-Forward-Count found\n");
		return;
	case H_AFWDFROM:
		*suppress = TRUE;
		break;
	}

	if (*didafflines == TRUE) {
		Dout(pn, 15, "\tdidafflines == TRUE\n");
		return;
	}

	if ((htype >= 0) && (affspot != htype)) {
		Dout(pn, 15, "\thtype < 0 || affspot != htype, *suppress=%d\n", *suppress);
		return;
	}

	*didafflines = TRUE;
	for (hptr = hdrlines[H_AFWDFROM].head;
	     hptr != (struct hdrs *)NULL;
	     hptr = hptr->next) {
		printhdr(type, H_AFWDFROM, hptr, f);
	}
	fprintf(f,"%s %d\n", header[H_AFWDCNT].tag, affcnt);
	Dout(pn, 15, "\t*didafflines=%d, *suppress=%d\n", *didafflines, *suppress);
}
