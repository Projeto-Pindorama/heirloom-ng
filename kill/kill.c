/*
 * kill - terminate a process
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, June 2005.
 */
/*
 * Copyright (c) 2005 Gunnar Ritter
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

/*
 * Sccsid @(#)kill.c	1.2 (gritter) 6/30/05
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <libgen.h>
#include <ctype.h>
#include <errno.h>

#ifdef	SIGRTMIN
#undef	NSIG
#define	NSIG	SIGRTMIN
#endif
#ifndef	NSIG
#define	NSIG	32
#endif

static const char	*progname;
static int		lflag;
static int		sflag = SIGTERM;
static int		status;

extern int	sig_2_str(int, char *);
extern int	str_2_sig(const char *, int *);

static void
usage(void)
{
	fprintf(stderr, "usage: %s [ [ -sig ] id ... | -l ]\n", progname);
	exit(1);
}

static void
badsig(void)
{
	fprintf(stderr, "%s: bad signal\n", progname);
	exit(1);
}

static int
getsig(const char *cp)
{
	int	n;

	if (str_2_sig(cp, &n) < 0)
		badsig();
	return n;
}

static void
listsigs(void)
{
	int	i, j = 0;
	char	b[20];

	for (i = 1; i < NSIG; i++) {
		if (sig_2_str(i, b) < 0)
			continue;
		if (j++ % 10)
			putchar('\t');
		else if (j > 1)
			putchar('\n');
		fputs(b, stdout);
	}
	putchar('\n');
}

static void
printsig(const char *cp)
{
	char	*xp;
	int	n;
	char	b[20];

	n = strtol(cp, &xp, 10);
	if (*xp != '\0' || *cp == '-' || *cp == '+')
		badsig();
	if (n >= 128)
		n -= 128;
	if (sig_2_str(n, b) < 0)
		badsig();
	puts(b);
}

static void
sendsig(const char *pp)
{
	pid_t	pid;
	char	*xp;

	pid = strtol(pp, &xp, 10);
	if (*xp != '\0' || *pp == '+')
		goto esrch;
	if (kill(pid, sflag) < 0) {
		switch (errno) {
		case EINVAL:
			badsig();
			/*NOTREACHED*/
		case EPERM:
			fprintf(stderr, "%s: permission denied\n", progname);
			status |= 1;
			break;
		case ESRCH:
		esrch:	fprintf(stderr, "%s: %s\n", progname, pid >= 0 ?
				"no such process" : "no such process group");
			status |= 1;
			break;
		default:
			perror(progname);
			status |= 1;
		}
	}
}

int
main(int argc, char **argv)
{
	int	i = 0;

	progname = basename(argv[0]);
	if (argc >= 2 && argv[1][0] == '-') {
		i++;
		if (argv[1][1] == 's') {
			if (argv[1][2])
				sflag = getsig(&argv[1][2]);
			else if (argc >= 3) {
				i++;
				sflag = getsig(&argv[2][0]);
			} else
				usage();
		} else if (argv[1][1] == 'l')
			lflag = 1;
		else
			sflag = getsig(&argv[1][1]);
	}
	argv += i;
	argc -= i;
	if (lflag) {
		if (argc == 1)
			listsigs();
		else if (argc == 2)
			printsig(argv[1]);
		else
			usage();
	} else {
		if (argc <= 1)
			usage();
		for (i = 1; i < argc; i++)
			sendsig(argv[i]);
	}
	return status;
}
