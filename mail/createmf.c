/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*
 * Copyright 1995 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	from OpenSolaris "createmf.c	1.10	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)createmf.c	1.5 (gritter) 6/22/05
 */

#include "mail.h"
/*
	If mail file does not exist create it
*/
#ifdef OLD
void
createmf(uid_t uid, char *file)
{
	int fd;

	void (*istat)(), (*qstat)(), (*hstat)();

	if (access(file, A_EXIST) == CERROR) {
		istat = sigset(SIGINT, SIG_IGN);
		qstat = sigset(SIGQUIT, SIG_IGN);
		hstat = sigset(SIGHUP, SIG_IGN);
		umask(0);
		if ((fd = creat(file, MFMODE)) == -1)
			sav_errno = errno;
		else
			close(fd);
		umask(7);
		sigset(SIGINT, istat);
		sigset(SIGQUIT, qstat);
		sigset(SIGHUP, hstat);
	}
}
#else

#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

int
accessmf(char *path)
{

struct stat fsb,sb;
int mbfd;
tryagain:
	if (lstat(path, &sb)) {
		/* file/symlink does not exist, so create one */
		mbfd = open(path,
		    O_APPEND|O_CREAT|O_EXCL|O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
		chmod(path, 0660);
		/* if someone create a symlink/file just ahead */
		/* of us, the create will failed with EEXIST   */
		/* This is what we want, because we do not     */
		/* want someone to re-direct our "create"      */
	        /* request to a another location.              */
		if (mbfd == -1) {
			if (errno == EEXIST)
				goto tryagain;
		}

	/* file/symlink  exist, make sure it is not linked */
	} else if (sb.st_nlink != 1 || S_ISLNK(sb.st_mode)) {
		fprintf(stderr,
"%s: security violation, '%s' should not be linked to other file\n", program, path);
		sav_errno = errno;
		return -1;
	} else {
		/* if we get here, there is a pre-existing file, */
		/* and it is not a symlink...			 */
		/* open it, and make sure it is the same file    */
		/* we lstat() before...                          */
		/* this is to guard against someone deleting the */
		/* old file and creat a new symlink in its place */
		/* We are not createing a new file here, but we  */
		/* do not want append to the worng file either   */
		mbfd = open(path, O_APPEND|O_WRONLY, 0);
		if (mbfd != -1 &&
		    (fstat(mbfd, &fsb) || fsb.st_nlink != 1 ||
		    S_ISLNK(fsb.st_mode) || sb.st_dev != fsb.st_dev ||
		    sb.st_ino != fsb.st_ino)) {
			/*  file changed after open */
			fprintf(stderr, "%s: security violation, '%s' inode changed after open\n", program, path);
			close(mbfd);
			return -1;
		}
	}

	if (mbfd == -1) {
		sav_errno = errno;
		return -1;
	}

	return mbfd;
}
#endif
