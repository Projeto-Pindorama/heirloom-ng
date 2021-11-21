/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
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
