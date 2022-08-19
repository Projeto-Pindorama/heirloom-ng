/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "printhdr.c	1.8	05/06/08 SMI" 	 SVr4.0 2.		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)printhdr.c	1.4 (gritter) 6/22/05
 */


/*
 * Copyright 2002 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include "mail.h"

int
printhdr(int type, int hdrtype, struct hdrs *hptr, FILE *fp)
{
	char		buf[1024];
	register int	n;
	struct hdrs 	*contptr;


	if (hptr == NULL)
		return (0);
	if (sel_disp(type, hdrtype, header[hdrtype].tag) < 0) {
		return (0);
	}

	snprintf(buf, sizeof (buf), "%s %s\n",
	    header[hdrtype].tag, hptr->value);
	n = strlen(buf);
	if (fwrite(buf, 1, n, fp) != n)  {
		sav_errno = errno;
		return (-1);
	}

	/* Print continuation lines, if any... */
	contptr = hptr;
	while (contptr->cont != (struct hdrs *)NULL) {
		contptr = contptr->cont;
		snprintf(buf, sizeof (buf), "%s\n", contptr->value);
		n = strlen(buf);
		if (fwrite(buf, 1, n, fp) != n)  {
			sav_errno = errno;
			return (-1);
		}
	}
	return (0);
}
