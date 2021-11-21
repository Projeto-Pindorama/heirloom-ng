/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute
 * it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include	<sys/types.h>
#include	<sys/wait.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<libgen.h>
#include	<errno.h>
#include	<signal.h>

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)setpgrp.sl	1.11 (gritter) 5/29/05";

static char	*progname;
static const char	cookie[] = "cookie";
static char	box[sizeof cookie];

static pid_t	procreate(int *notify);

int
main(int argc, char **argv)
{
	pid_t	pid, ppid, pgid, sid;
	pid_t	child = -1;
	int	notify = -1, i;

	progname = basename(argv[0]);
	if (argc > 1 && argv[1][0] == '-' && argv[1][1] == '-' &&
			argv[1][2] == '\0')
		argv++, argc--;
	if (argc <= 1) {
		fprintf(stderr, "Usage:  %s command [ arg ... ]\n", progname);
		exit(2);
	}
	/*
	 * We need to emulate the SVR4 setpgrp() behaviour, that is:
	 * - If we are a session leader or there is already another
	 *   process group with our process id, do nothing. The kernel
	 *   will detect these conditions.
	 * - If we are a process group leader, but not a session
	 *   leader, become a session leader. setsid() will fail
	 *   with EPERM in this case, so we have to enter another
	 *   process group before. Getting rid of leadership can
	 *   be difficult, see below.
	 * - If we are not a process group leader, become a session
	 *   leader. This is the same as a simple setsid() call.
	 */
	pid = getpid();
	pgid = getpgid(0);
	sid = getsid(0);
	if (pid == pgid && pid != sid) {
		/*
		 * If our parent is member of another process group,
		 * try to join there.
		 */
		ppid = getppid();
		if (getpgid(ppid) == pgid || setpgid(0, ppid) < 0) {
			/*
			 * This happens if our parent process is not
			 * member of our session (most likely if our
			 * parent had died and we've been adopted by
			 * init). Create a child process and change
			 * to its process group.
			 */
			if ((child = procreate(&notify)) != -1) {
				/*
				 * This should really succeed now.
				 */
				if (setpgid(0, child) < 0)
					perror("setpgid");
			}
		}
	}
	/*
	 * Don't report setsid() failure since it will only occur
	 * now in cases where System V setpgrp() would be a no-op
	 * (or a previous error was already reported).
	 */
	setsid();
	if (child != -1) {
		/*
		 * Don't need the child process anymore; tell it
		 * to exit.
		 */
		struct sigaction	act, oact;

		act.sa_handler = SIG_IGN;
		sigemptyset(&act.sa_mask);
		act.sa_flags = 0;
		sigaction(SIGPIPE, &act, &oact);
		write(notify, cookie, sizeof cookie);
		close(notify);
		sigaction(SIGPIPE, &oact, NULL);
		waitpid(child, NULL, 0);
	}
	execvp(argv[1], &argv[1]);
	i = errno;
	fprintf(stderr, "%s: %s not executed.  %s\n",
			progname, argv[1], strerror(i));
	return i == ENOENT ? 0177 : 0176;
}

/*
 * Create a child process and wait until it lives in its own process
 * group. Return the child process id, or -1 on error.
 */
static pid_t
procreate(int *notify)
{
	int	fd1[2], fd2[2];
	pid_t	pid;

	if (pipe(fd1) < 0) {
		perror("pipe");
		return -1;
	}
	if (pipe(fd2) < 0) {
		perror("pipe");
		close(fd1[0]);
		close(fd1[1]);
		return -1;
	}
	switch (pid = fork()) {
	case 0:
		close(fd1[0]);
		close(fd2[1]);
		setpgid(0, 0);
		write(fd1[1], cookie, sizeof cookie);
		/*
		 * Now wait until we are notified by the parent that it
		 * has joined our process group. This might not actually
		 * be necessary; at least Linux 2.4.20 allows to change
		 * to a zombie's process group. I'm not sure that the
		 * standards guarantee it, though, and even if I were,
		 * implementations might behave differently in this
		 * obscure matter.
		 */
		read(fd2[0], box, sizeof box);
		_exit(0);
		/*NOTREACHED*/
	default:
		close(fd1[1]);
		close(fd2[0]);
		/*
		 * Wait until the child process is ready.
		 */
		read(fd1[0], box, sizeof box);
		*notify = fd2[1];
		break;
	case -1:
		perror("fork");
		close(fd1[1]);
		close(fd2[0]);
		close(fd2[1]);
	}
	close(fd1[0]);
	return pid;
}
