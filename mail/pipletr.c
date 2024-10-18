/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "pipletr.c	1.7	05/06/08 SMI" 	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)pipletr.c	1.6 (gritter) 6/18/05
 */

#include "mail.h"
#include <sys/wait.h>

int
dowait(pid_t pidval)
{
	pid_t w;
	int status;
	void (*istat)(int), (*qstat)(int);

	/*
		Parent temporarily ignores signals so it will remain
		around for command to finish
	*/
	istat = sigset(SIGINT, SIG_IGN);
	qstat = sigset(SIGQUIT, SIG_IGN);

	while ((w = wait(&status)) != pidval && w != CERROR);
	if (w == CERROR) {
		status = -errno;
		sigset(SIGINT, istat);
		sigset(SIGQUIT, qstat);
		return (status);
	}

	sigset(SIGINT, istat);
	sigset(SIGQUIT, qstat);
	status = ((status>>8)&0xFF);  		/* extract 8 high order bits */
	return (status);
}

/*
	invoke shell to execute command waiting for command to terminate
		s	-> command string
	return:
		status	-> command exit status
*/
int
systm(char *s)
{
	pid_t	pid;

	/*
		Spawn the shell to execute command, however, since the
		mail command runs setgid mode reset the effective group
		id to the real group id so that the command does not
		acquire any special privileges
	*/
	if ((pid = fork()) == CHILD) {
		setuid(my_uid);
		setgid(my_gid);
		execl(SHELL, "sh", "-c", s, (char*)NULL);
		exit(127);
	}
	return (dowait(pid));
}
