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

/*	from OpenSolaris "copyback.c	1.10	05/06/11 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)copyback.c	1.8 (gritter) 6/22/05
 */

/*
 *  NAME
 *	copyback - copy temp or whatever back to /var/mail
 *
 *  SYNOPSIS
 *	void copyback()
 *
 *  DESCRIPTION
 *	Copy the reduced contents of lettmp back to
 *	the mail file. First copy any new mail from
 *	the mail file to the end of lettmp.
 */

#include "mail.h"
void 
copyback(void)
{
	register int	i, n;
	int		new = 0;
	mode_t		mailmode, omask;
	struct stat	stbuf;
	void		(*istat)(int), (*qstat)(int), (*hstat)(int);

	istat = sigset(SIGINT, SIG_IGN);
	qstat = sigset(SIGQUIT, SIG_IGN);
	hstat = sigset(SIGHUP, SIG_IGN);
	lock(my_name);
	stat(mailfile, &stbuf);
	mailmode = stbuf.st_mode;

	/*
	 *	Has new mail arrived?
	 */
	if (stbuf.st_size != let[nlet].adr) {
		malf = doopen(mailfile, "r", E_FILE);
		fseek(malf, let[nlet].adr, 0);
		fclose(tmpf);
		tmpf = doopen(lettmp, "a", E_TMP);
		/*
		 *	Append new mail assume only one new letter
		 */
		if (!copystream(malf, tmpf)) {
			fclose(malf);
			tmperr();
			done(0);
		}
		fclose(malf);
		fclose(tmpf);
		tmpf = doopen(lettmp, "r+", E_TMP);
		if (nlet == (MAXLET-2)) {
			errmsg(E_SPACE, "");
			done(0);
		}
		let[++nlet].adr = stbuf.st_size;
		new = 1;
	}

	/*
	 *	Copy mail back to mail file
	 */
	omask = umask(0117);

	/*
	 *	The invoker must own the mailfile being copied to
	 */
	if ((stbuf.st_uid != my_euid) && (stbuf.st_uid != my_uid)) {
		errmsg(E_OWNR, "");
		done(0);
	}

	/*
	 *	If user specified the '-f' option we dont do
	 *	the routines to handle :saved files.
	 *	As we would(incorrectly) restore to the user's
	 *	mailfile upon next execution!
	 */
	if (flgf) {
		cpy(&savefile, &savefilesize, mailfile);
	} else {
		cat(&savefile, &savefilesize, mailsave, my_name);
	}

	if ((malf = fopen(savefile, "w")) == NULL) {
		if (!flgf) {
			cat(&savefile, &savefilesize, mailfile, "XXXXXX");
			close(mkstemp(savefile));
			if ((malf = fopen(savefile, "w")) != NULL)
				goto success;
			errmsg(E_FILE, "Cannot open savefile");
		} else {
			errmsg(E_FILE, "Cannot re-write the alternate file");
		}
		done(0);
	}
success:
	if (chown(savefile, mf_uid, mf_gid) == -1) {
		/*EMPTY*/;
		/*
		errmsg(E_FILE, "Cannot chown savefile");
		done(0);
		*/
	}
	umask(omask);
	n = 0;

	for (i = 0; i < nlet; i++) {
		/*
		 *	Note: any action other than an undelete, or a
		 *	plain read causes the letter acted upon to be
		 *	deleted
		 */
		if (let[i].change == ' ') {
			if (copylet(i, malf, ORDINARY) == FALSE) {
				errmsg(E_FILE, "Cannot copy mail to savefile");
				fprintf(stderr, "%s: A copy of your "
				    "mailfile is in '%s'\n", program, lettmp);
				done(1);	/* keep temp file */
			}
			n++;
		}
	}
	fclose(malf);

	if (!flgf) {
		if (unlink(mailfile) != 0) {
			errmsg(E_FILE, "Cannot unlink mailfile");
			done(0);
		}
		chmod(savefile, mailmode);
		if (rename(savefile, mailfile) != 0) {
			errmsg(E_FILE, "Cannot rename savefile to mailfile");
			done(0);
		}
	}

	/*
	 *	Empty mailbox?
	 */
	if (n == 0) {
		delempty(stbuf.st_mode, mailfile);
	}

	if (new && !flgf) {
		printf("New mail arrived\n");
	}

	unlock();
	sigset(SIGINT, istat);
	sigset(SIGQUIT, qstat);
	sigset(SIGHUP, hstat);
}
