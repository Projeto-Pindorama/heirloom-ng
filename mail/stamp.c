/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "stamp.c	1.7	05/06/08 SMI"         SVr4.0 1.5 	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)stamp.c	1.3 (gritter) 6/18/05
 */

#include "mail.h"
/*
	If the mailfile still exists (it may have been deleted),
	time-stamp it; so that our re-writing of mail back to the
	mailfile does not make shell think that NEW mail has arrived
	(by having the file times change).
*/
void
stamp(void)
{
	if ((access(mailfile, A_EXIST) == A_OK) && (utimep->modtime != -1))
		if (utime(mailfile, utimep) != A_OK)
			errmsg(E_FILE,"Cannot time-stamp mailfile");
}
