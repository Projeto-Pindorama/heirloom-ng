/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*
 * Copyright 2002 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	from OpenSolaris "lock.c	1.7	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)lock.c	1.4 (gritter) 6/22/05
 */

#include "mail.h"

void
lock(char *user)
{
	char	tbuf[80];

	switch (maillock(user, 10)) {
	case L_SUCCESS:
	    return;
	case L_NAMELEN:
	    snprintf(tbuf, sizeof (tbuf),
		"%s: Cannot create lock file. Username '%s' is > 13 chars\n",
		program, user);
	    break;
	case L_TMPLOCK:
	    strcpy(tbuf, "Cannot create temp lock file\n");
	    break;
	case L_TMPWRITE:
	    strcpy(tbuf, "Error writing pid to lock file\n");
	    break;
	case L_MAXTRYS:
	    strcpy(tbuf, "Creation of lockfile failed after 10 tries");
	    break;
	case L_ERROR:
	    strcpy(tbuf, "Cannot link temp lockfile to lockfile\n");
	    break;
	case L_MANLOCK:
	    strcpy(tbuf, "Cannot set mandatory file lock on temp lockfile\n");
	    break;
	}
	errmsg(E_LOCK, tbuf);
	if (sending) {
		goback(0);
	}
	done(0);
}

void 
unlock(void) {
	mailunlock();
}
