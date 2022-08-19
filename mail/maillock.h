/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*
 * Copyright (c) 1999, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ifndef	_MAILLOCK_H
#define	_MAILLOCK_H

/*	from OpenSolaris "maillock.h	1.9	05/06/08 SMI"	 SVr4.0 1.6		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)maillock.h	1.3 (gritter) 6/18/05
 */

#ifdef	__cplusplus
extern "C" {
#endif

#define	MAILDIR		"/var/mail/"
#define	SAVEDIR		"/var/mail/:saved/"

#define	PATHSIZE	1024	/* maximum path length of a lock file */
#define	L_SUCCESS	0
#define	L_NAMELEN	1	/* recipient name > 13 chars */
#define	L_TMPLOCK	2	/* problem creating temp lockfile */
#define	L_TMPWRITE	3	/* problem writing pid into temp lockfile */
#define	L_MAXTRYS	4	/* cannot link to lockfile after N tries */
#define	L_ERROR		5	/* Something other than EEXIST happened */
#define	L_MANLOCK	6	/* cannot set mandatory lock on temp lockfile */

extern int maillock(char *user, int retrycnt);
extern void mailunlock(void);
extern void touchlock(void);

#ifdef	__cplusplus
}
#endif

#endif	/* _MAILLOCK_H */
