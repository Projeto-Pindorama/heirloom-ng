/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)ib_popen.c	1.2 (gritter) 4/17/03	*/

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/wait.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<string.h>
#include	<errno.h>
#include	<stdlib.h>
#include	<signal.h>

#include	"iblok.h"

struct iblok *
ib_popen(const char *cmd, unsigned blksize)
{
	struct iblok	*ip;
	int	fd[2], err;
	pid_t	pid;
	char	*shell;

	if (pipe(fd) < 0)
		return NULL;
	switch (pid = fork()) {
	case -1:
		return NULL;
	case 0:
		close(fd[0]);
		dup2(fd[1], 1);
		close(fd[1]);
		if ((shell = getenv("SHELL")) == NULL)
			shell = "/bin/sh";
		execl(shell, shell, "-c", cmd, NULL);
		_exit(0177);
		/*NOTREACHED*/
	}
	close(fd[1]);
	if ((ip = ib_alloc(fd[0], blksize)) == NULL) {
		err = errno;
		close(fd[0]);
		errno = err;
	}
	ip->ib_pid = pid;
	return ip;
}

int
ib_pclose(struct iblok *ip)
{
	struct sigaction	oldhup, oldint, oldquit, act;
	int	status;

	close(ip->ib_fd);
	act.sa_handler = SIG_IGN;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGHUP, &act, &oldhup);
	sigaction(SIGINT, &act, &oldint);
	sigaction(SIGQUIT, &act, &oldquit);
	while (waitpid(ip->ib_pid, &status, 0) < 0 && errno == EINTR);
	sigaction(SIGHUP, &oldhup, NULL);
	sigaction(SIGINT, &oldint, NULL);
	sigaction(SIGQUIT, &oldquit, NULL);
	return status;
}
