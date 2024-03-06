/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "del_recipl.c	1.6	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)del_recipl.c	1.3 (gritter) 6/18/05
 */
	 	/* SVr4.0 1.	*/
#include "mail.h"

/*
    NAME
	del_reciplist - delete a recipient list

    SYNOPSIS
	del_reciplist (reciplist *list)

    DESCRIPTION
	Free the space used by a recipient list.
*/

void 
del_reciplist(reciplist *plist)
{
	static char	pn[] = "del_reciplist";
	recip		*r = &plist->recip_list;
	Dout(pn, 0, "entered\n");
	if (r->next != (struct recip *)NULL) {
		for (r = r->next; r != (struct recip *)NULL; ) {
			recip *old = r;
			r = old->next;
			free(old->name);
			free((char*)old);
		}
	}
}
