/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "new_recipl.c	1.6	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)new_recipl.c	1.3 (gritter) 6/18/05
 */
	 	/* SVr4.0 1.	*/
#include "mail.h"


/*
    NAME
	new_reciplist - initialize a recipient list

    SYNOPSIS
	new_reciplist (reciplist *list)

    DESCRIPTION
	Initialize a recipient list to have no recipients.
*/

void 
new_reciplist(reciplist *plist)
{
	static char	pn[] = "new_reciplist";
	Dout(pn, 0, "entered\n");
	plist->recip_list.next = 0;
	plist->recip_list.name = 0;
	plist->last_recip = &plist->recip_list;
}
