/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	from OpenSolaris "pushlist.c	1.8	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)pushlist.c	1.4 (gritter) 6/18/05
 */

#include "mail.h"
/*
 * link new entry into list of headerlines encountered of this type.
 * If contflg == TRUE, link this line to the end of the continuation lines
 * for the headerline specified (head or tail of type hdrtype).
 */
void 
pushlist(register int hdrtype, register int where, register char *s, int contflg)
{
	static char pn[] = "pushlist";
	char		*p;
	struct	hdrs	*nhp, *ohp, *nextcont;

	/* Keep track of total bytes added to message due to    */
	/* certain lines in case non-delivery                   */
	/* notification needs to be sent. (See also copylet())  */
	if (hdrtype == H_AFWDFROM) {
		affbytecnt += (strlen(s) + ((contflg == TRUE) ?
			1 :
			(strlen(header[H_AFWDFROM].tag) + 2)) );
		if (contflg == FALSE) {
			affcnt++;
		}
	}
	if (hdrtype == H_RECEIVED) {
		rcvbytecnt += (strlen(s) + ((contflg == TRUE) ?
			1 :
			(strlen(header[H_RECEIVED].tag) + 2)) );
	}
	if ((p = malloc(sizeof(struct hdrs))) == (char *)NULL) {
		errmsg(E_MEM,"malloc failed in pushlist()");
		done(1);
	}
	memset(p, 0, sizeof(struct hdrs));

	ohp = (where == HEAD ? hdrlines[hdrtype].head : hdrlines[hdrtype].tail);
	nhp = (struct hdrs *)p;

	snprintf(nhp->value, sizeof nhp->value, "%s", s);

	Dout(pn, 0, "hdrtype = %d/%s, contflg = %d, saved value = '%s'\n",
		hdrtype, header[hdrtype].tag, contflg, s);

	if (contflg) {
		if (ohp == (struct hdrs *)NULL) {
			/* This shouldn't happen.....? */
			/* No headline of this type found so far. How */
			/* did we think this is a continuation of something? */
			if (debug > 0) {
				Dout(pn, 0, "H_CONT with no hdr yet\n");
				abort();
			}
			/* Throw it on the floor... (!) */
			/**/
			/* Subtract anything that might have been added above */
			if (hdrtype == H_AFWDFROM) {
			    affbytecnt -= (strlen(s) + ((contflg == TRUE) ?
				1 :
				(strlen(header[H_AFWDFROM].tag) + 2)) );
			}
			if (hdrtype == H_RECEIVED) {
			    rcvbytecnt -= (strlen(s) + ((contflg == TRUE) ?
				1 :
				(strlen(header[H_RECEIVED].tag) + 2)) );
			}
			free ((char *)nhp);
			return;
		}
		/* Since we ONLY walk down 'cont' chains, */
		/* we only need forward links */
		nextcont = ohp;
		while (nextcont->cont != (struct hdrs *)NULL) {
			nextcont = nextcont->cont;
		}
		/* Add this one to end of list... */
		nextcont->cont = nhp;
		return;
	}

	/* link value from this header line to end of list for */
	/* all header lines of the same type */

	if (ohp == (struct hdrs *)NULL) {
		/* Empty list so far. New element goes first */
		hdrlines[hdrtype].head = hdrlines[hdrtype].tail = nhp;
	} else {
		if (where == HEAD) {
			/* Add new element to head of list */
			nhp->next = ohp;
			hdrlines[hdrtype].head = ohp->prev = nhp;
		} else {
			/* Add new element to tail of list */
			nhp->prev = ohp;
			hdrlines[hdrtype].tail = ohp->next = nhp;
		}
	}
}
