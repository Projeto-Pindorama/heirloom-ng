/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*
 * Copyright (c) 1999, by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*	from OpenSolaris "maillock.c	1.13	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)maillock.c	1.5 (gritter) 6/22/05
 */
/*LINTLIBRARY*/

#include "maillock.h"
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <utime.h>
#include <time.h>

#include <sys/stat.h>

static	char	*lockext = ".lock";	/* Lock suffix for mailname */
static	char	curlock[PATHSIZE];	/* Last used name of lock */
static	int	locked;			/* To note that we locked it */
static	time_t	locktime;		/* time lock file was touched */
static time_t	lock1(char *, char *);

/*
 * Lock the specified mail file by setting the file mailfile.lock.
 * We must, of course, be careful to remove the lock file by a call
 * to unlock before we stop.  The algorithm used here is to see if
 * the lock exists, and if it does, to check its modify time.  If it
 * is older than 5 minutes, we assume error and set our own file.
 * Otherwise, we wait for 5 seconds and try again.
 */

/*ARGSUSED*/
int
maillock(char *user, int retrycnt)
{
	time_t t;
	struct stat sbuf;
	int statfailed;
	char locktmp[PATHSIZE];	/* Usable lock temporary */
	char _file[PATHSIZE];
	char *file;
	extern char	*lockfile;

	if (locked)
		return (0);
	if (lockfile)
		file = lockfile;
	else {
		snprintf(_file, sizeof _file, "%s%s", MAILDIR, user);
		file = _file;
	}
	snprintf(curlock, sizeof curlock, "%s%s", file, lockext);
	snprintf(locktmp, sizeof locktmp, "%sXXXXXX", file);
	close(mkstemp(locktmp));
	remove(locktmp);
	statfailed = 0;
	for (;;) {
		t = lock1(locktmp, curlock);
		if (t == (time_t)0) {
			locked = 1;
			locktime = time(0);
			return (0);
		}
		if (stat(curlock, &sbuf) < 0) {
			if (statfailed++ > 5)
				return (-1);
			sleep(5);
			continue;
		}
		statfailed = 0;

		/*
		 * Compare the time of the temp file with the time
		 * of the lock file, rather than with the current
		 * time of day, since the files may reside on
		 * another machine whose time of day differs from
		 * ours.  If the lock file is less than 5 minutes
		 * old, keep trying.
		 */
		if (t < sbuf.st_ctime + 300) {
			sleep(5);
			continue;
		}
		remove(curlock);
	}
}

/*
 * Remove the mail lock, and note that we no longer
 * have it locked.
 */
void
mailunlock(void)
{
	remove(curlock);
	locked = 0;
}

/*
 * Attempt to set the lock by creating the temporary file,
 * then doing a link/unlink.  If it succeeds, return 0,
 * else return a guess of the current time on the machine
 * holding the file.
 */
static time_t
lock1(char tempfile[], char name[])
{
	int fd;
	struct stat sbuf;

	fd = open(tempfile, O_RDWR|O_CREAT|O_EXCL, 0600);
	if (fd < 0)
		return (time(0));
	fstat(fd, &sbuf);
	/*
	 * Write the string "0" into the lock file to give us some
	 * interoperability with SVR4 mailers.  SVR4 mailers expect
	 * a process ID to be written into the lock file and then
	 * use kill() to see if the process is alive or not.  We write
	 * 0 into it so that SVR4 mailers will always think our lock file
	 * is valid.
	 */
	write(fd, "0", 2);
	close(fd);
	if (link(tempfile, name) < 0) {
		remove(tempfile);
		return (sbuf.st_ctime);
	}
	remove(tempfile);
	return ((time_t)0);
}

/*
 * Update the change time on the lock file so
 * others will know we're still using it.
 */
void
touchlock(void)
{
	struct stat sbuf;
	time_t t;
	struct utimbuf tp;

	if (!locked)
		return;

	/* if it hasn't been at least 3 minutes, don't bother */
	if (time(&t) < locktime + 180)
		return;
	locktime = t;

	if (stat(curlock, &sbuf) < 0)
		return;
	/*
	 * Don't actually change the times, we just want the
	 * side effect that utime causes st_ctime to be set
	 * to the current time.
	 */
	tp.actime = sbuf.st_atime;
	tp.modtime = sbuf.st_mtime;
	utime(curlock, &tp);
}
