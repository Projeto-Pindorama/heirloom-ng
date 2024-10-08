/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "clr_hinfo.c	1.7	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)clr_hinfo.c	1.3 (gritter) 6/18/05
 */
	 	/* SVr4.0 2.	*/
/*
    NAME
	clr_hinfo, clrhdr - clean out mail header information

    SYNOPSIS
	void clr_hinfo()
	void clrhdr(int hdrtype)

    DESCRIPTION
	Clr_hinfo() cleans out hdrlines[] and other associated data
	in preparation for the next message.

	Clrhdr() does a single hdrlines[].
*/

#include "mail.h"

void 
clr_hinfo(void)
{
	int	i;
	static		int	firsttime = 1;
	static char		pn[] = "clr_hinfo";

	Dout(pn, 0, "\n");
	if (firsttime) {
		firsttime = 0;
		return;
	}
	fnuhdrtype = 0;
	orig_aff = orig_rcv = 0;
	for (i = 0; i < H_CONT; i++) {
		clrhdr(i);
	}
	return;
}

void 
clrhdr(int hdrtype)
{
	while (hdrlines[hdrtype].head != (struct hdrs *)NULL) {
		poplist (hdrtype, HEAD);
	}
}
