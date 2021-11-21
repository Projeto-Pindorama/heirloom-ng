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

/*	from OpenSolaris "cksaved.c	1.9	05/06/11 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)cksaved.c	1.7 (gritter) 6/22/05
 */

/*
 *  NAME
 *	cksaved - check for an orphaned save file
 *
 *  SYNOPSIS
 *	void cksaved(char *user)
 *
 *  DESCRIPTION
 *	cksaved() looks to see if there is a saved-mail file sitting
 *	around which should be reinstated. These files should be sitting
 *	around only in the case of a crash during rewriting a mail message.
 *
 *	The strategy is simple: if the file exists it is appended to
 *	the end of $MAIL.  It is better that a user potentially sees the
 *	mail twice than to lose it.
 *
 *	If $MAIL doesn't exist, then a simple rename() will suffice.
 */

#define	Return	free(save); free(mail); return

#include "mail.h"
void 
cksaved(char *user)
{
	struct stat stbuf;
	char command[512];
	char *save = NULL, *mail = NULL;
	size_t	savesize = 0, mailsize = 0;

	cat(&mail, &mailsize, maildir, user);
	cat(&save, &savesize, mailsave, user);

	/*
	 *	If no save file, or size is 0, return.
	 */
	if ((stat(save, &stbuf) != 0) || (stbuf.st_size == 0)) {
		Return;
	}

	/*
	 *	Ok, we have a savefile. If no mailfile exists,
	 *	then we want to restore to the mailfile,
	 *	else we append to the mailfile.
	 */
	lock(user);
	if (stat(mail, &stbuf) != 0) {
		/*
		 *	Restore from the save file by linking
		 *	it to $MAIL then unlinking save file
		 */
		chmod(save, MFMODE);
		if (rename(save, mail) != 0) {
			unlock();
			perror("Cannot rename saved file");
			Return;
		}

		snprintf(command, sizeof (command),
		    "echo \"Your mailfile was just restored by the mail "
		    "program.\nPermissions of your mailfile are set "
		    "to 0660.\"| mail %s", user);
	}

	else {
		FILE *Istream, *Ostream;
		if ((Ostream = fopen(mail, "a")) == NULL) {
			fprintf(stderr,
			    "%s: Cannot open file '%s' for output\n",
			program, mail);
			unlock();
			Return;
		}
		if ((Istream = fopen(save, "r")) == NULL) {
			fprintf(stderr, "%s: Cannot open saved "
			    "file '%s' for reading\n", program, save);
			fclose(Ostream);
			unlock();
			Return;
		}
		copystream(Istream, Ostream);
		fclose(Istream);
		fclose(Ostream);

		if (unlink(save) != 0) {
			perror("Unlink of save file failed");
			Return;
		}

		snprintf(command, sizeof (command),
		    "echo \"Your mail save file has just been appended "
		    "to your mail box by the mail program.\" | mail %s", user);
	}

	/*
	 *	Try to send mail to the user whose file
	 *	is being restored.
	 */
	unlock();
	systm(command);
	Return;
}
