/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/


/*
 * Copyright 2001-2002 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	from OpenSolaris "sendlist.c	1.20	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)sendlist.c	1.9 (gritter) 6/22/05
 */

#include "mail.h"
/*
 *  NAME
 *	sendlist - send copy to specified users
 *
 *  SYNOPSIS
 *	int sendlist(reciplist *list, int letnum, int level)
 *
 *  DESCRIPTION
 *	sendlist() will traverse the current recipient list and
 *	send a copy of the given letter to each user specified,
 *	invoking send() to do the sending. It returns
 *	1 if the sending fails, 0 otherwise.
 */


/*
 * mailx and mailtool read the SENDMAIL from an environment, since few
 *  people use /bin/mail as their user agent and since /bin/mail is often
 *  called as root or made setuid it's safer to leave this hardwired.
 */

static char *sendmail_prog = SENDMAIL;

static int	send_mbox(char *, int);
static void	notifybiff(char *);

int
sendlist(reciplist *list, int letnum, int level)
{
	recip *to;
	int rc = 0;
	FILE *fp;
	int nargs = 4;			/* "sendmail", "-oi", "--", .. NULL */
	char **argv;
	char **p;

	/* Deliver mail directly to a mailbox */
	if (deliverflag) {
		/*
		 * Note failure to deliver to any one of the recipients
		 * should be considered a failure, so that the user
		 * get's an indication of that failure.
		 */
		for (to = &(list->recip_list); to; to = to->next) {
			if (to->name)
				if (!send_mbox(to->name, letnum))
					rc = 1;
		}
		return (rc);
	}

	/*
	 * build argv list, allowing for arbitrarily long deliver lists
	 * and then  hand the message off to sendmail
	 */

	if (!ismail)
		nargs += 2;	/* for "-f", "Rpath" */

	for (to = &(list->recip_list); to; to = to->next)
		if (to->name)
			nargs++;

	argv = malloc(nargs * sizeof (char *));

	if (argv == NULL)
		return (1);

	p = argv;

	*p++ = sendmail_prog;

	/* If we're rmail add "-f", "Rpath" to the the command line */
	if (!ismail) {
		*p++ = "-f";
		*p++ = Rpath ? Rpath : "";
	}

	*p++ = "-oi";
	*p++ = "--";		/* extra protection: end of argument list */

	for (to = &(list->recip_list); to; to = to->next)
		if (to->name)
			*p++ = to->name;

	*p = NULL;

	fp = popenvp(sendmail_prog, argv, "w", 0);

	free(argv);

	if (fp == NULL)
		return (1);

	copylet(letnum, fp, ORDINARY);
	rc = pclosevp(fp);
	if (!rc)
		return (0);
	else
		return (1);
}

/*
 * send_mbox(user, letnum)  Sends the letter specified by letnum to the
 *	"user"'s mailbox. It returns 1 if the sending fails;
 *	0 on success.
 */

#define	Return	free(file); return

static int
send_mbox(char *mbox, int letnum)
{
	char *file = NULL;
	size_t	filesize = 0;
	char biffmsg[PATH_MAX];
	int mbfd;
	FILE *malf;
	int rc = 0;
	uid_t useruid, saved_uid;
	void (*istat)(int), (*qstat)(int), (*hstat)(int);

	if (!islocal(mbox, &useruid)) {
		Return (1);
	}
	cat(&file, &filesize, maildir, mbox);

	/*
	 * We need to setgid and seteuid here since the users's mail box
	 * might be NFS mounted and since root can't write across NFS.
	 * Note this won't work with Secure NFS/RPC's.  Since delivering to
	 * NFS mounted directories isn't really supported that's OK for now.
	 */
	setgid(mailgrp);
	saved_uid = geteuid();
	seteuid(useruid);
	lock(mbox);

	/* ignore signals */
	istat = sigset(SIGINT, SIG_IGN);
	qstat = sigset(SIGQUIT, SIG_IGN);
	hstat = sigset(SIGHUP, SIG_IGN);
	/* now access mail box */
	mbfd = accessmf(file);
	if (mbfd == -1) {	/* mail box access failed, bail out */
		unlock();
		rc = FALSE;
		sav_errno = EACCES;
		goto done;
	} else {
				/* mail box is ok, now do append */
		if ((malf = fdopen(mbfd, "a")) != NULL) {
			snprintf(biffmsg, sizeof (biffmsg),
			    "%s@%ld\n", mbox, ftell(malf));
			rc = copylet(letnum, malf, ORDINARY);
			fclose(malf);
		}
	}

	if (rc == FALSE)
		fprintf(stderr, "%s: Cannot append to %s\n", program, file);
	else
		notifybiff(biffmsg);

done:
	/* restore signal */
	sigset(SIGINT, istat);
	sigset(SIGQUIT, qstat);
	sigset(SIGHUP, hstat);
	unlock();
	seteuid(saved_uid);
	Return (rc);
}

#include <sys/socket.h>
#include <netinet/in.h>

#ifndef	IPPORT_BIFFUDP
#define	IPPORT_BIFFUDP	512
#endif

static void
notifybiff(char *msg)
{
	static struct sockaddr_in addr;
	static int f = -1;

	if (addr.sin_family == 0) {
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_LOOPBACK;
		addr.sin_port = htons(IPPORT_BIFFUDP);
	}
	if (f < 0)
		f = socket(AF_INET, SOCK_DGRAM, 0);
	sendto(f, msg, strlen(msg)+1, 0, (struct sockaddr *)&addr,
		sizeof (addr));
}
