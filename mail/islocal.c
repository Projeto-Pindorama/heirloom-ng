/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	from OpenSolaris "islocal.c	1.6	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)islocal.c	1.4 (gritter) 6/22/05
 */

#include <sys/param.h>
#include "mail.h"

/*
 * islocal (char *user, uid_t *puid) - see if user exists on this system
 */
int
islocal(char *user, uid_t *puid)
{
	char	fname[MAXPATHLEN];
	struct stat statb;
	struct passwd *pwd_ptr;

	/* Check for existing mailfile first */
	snprintf(fname, sizeof (fname), "%s%s", maildir, user);
	if (stat(fname, &statb) == 0) {
		*puid = statb.st_uid;
		return (TRUE);
	}

	/* If no existing mailfile, check passwd file */
	setpwent();
	if ((pwd_ptr = getpwnam(user)) == NULL) {
		return (FALSE);
	}
	*puid = pwd_ptr->pw_uid;
	return (TRUE);
}
