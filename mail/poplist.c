/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "poplist.c	1.6	05/06/08 SMI" 	 SVr4.0 2.		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)poplist.c	1.3 (gritter) 6/18/05
 */
#include "mail.h"
/*
 * Remove an entry from its linked list and free any malloc'd memory..
 */
void 
poplist(register int hdrtype, register int where)
{
	struct	hdrs	*hdr2rm, *cont2rm, *nextcont;

	/* Remove first/last entry from list */

	hdr2rm = (where == HEAD ?
			hdrlines[hdrtype].head : hdrlines[hdrtype].tail);

	if (hdr2rm == (struct hdrs *)NULL) {
		return;
	}
	if (where == HEAD) {
		if (hdr2rm->next == (struct hdrs *)NULL) {
			/* Only 1 entry in list */
			hdrlines[hdrtype].head = hdrlines[hdrtype].tail =
							(struct hdrs *)NULL;
		} else {
			hdrlines[hdrtype].head = hdr2rm->next;
			hdr2rm->next->prev = (struct hdrs *)NULL;
		}
	} else {
		if (hdr2rm->prev == (struct hdrs *)NULL) {
			/* Only 1 entry in list */
			hdrlines[hdrtype].head = hdrlines[hdrtype].tail =
							(struct hdrs *)NULL;
		} else {
			hdrlines[hdrtype].tail = hdr2rm->prev;
			hdr2rm->prev->next = (struct hdrs *)NULL;
		}
	}
	/* Keep track of total bytes added to message due to    */
	/* selected lines in case non-delivery                  */
	/* notification needs to be sent. (See also copylet())  */
	if (hdrtype == H_AFWDFROM) {
	    affbytecnt -=
		(strlen(header[H_AFWDFROM].tag) + strlen(hdr2rm->value) + 2);
	    affcnt--;
	}
	if (hdrtype == H_RECEIVED) {
	    rcvbytecnt -=
		(strlen(header[H_RECEIVED].tag) + strlen(hdr2rm->value) + 2);
	}

	cont2rm = hdr2rm->cont;
	while (cont2rm != (struct hdrs *)NULL) {
		nextcont = cont2rm->next;
		if (hdrtype == H_AFWDFROM) {
		    affbytecnt -= (strlen(cont2rm->value) + 1);
		    affcnt--;
		}
		if (hdrtype == H_RECEIVED) {
		    rcvbytecnt -= (strlen(cont2rm->value) + 1);
		}
		free ((char *)cont2rm);
		cont2rm = nextcont;
	}
	free ((char *)hdr2rm);
}
