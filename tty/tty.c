/*
 * tty - get the name of the terminal
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, March 2003.
 */
/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)tty.sl	1.14 (gritter) 1/22/06";

#include <unistd.h>
#include <stdio.h>
#include <libgen.h>
#include <errno.h>
#include <stdlib.h>
#if !defined (__linux) && !defined (__FreeBSD__) && !defined (__hpux) && \
	!defined (_AIX) && !defined (__NetBSD__) && !defined (__OpenBSD__) && \
	!defined (__DragonFly__) && !defined (__APPLE__)
#include <sys/stermio.h>
#endif	/* !__linux__, !__FreeBSD__, !__hpux, !_AIX, !__NetBSD__,
	   !__OpenBSD__, !__DragonFly__, !__APPLE__ */

static int	lflag;
static int	sflag;
static int	status;
static char	*progname;

static void
usage(void)
{
	fprintf(stderr, "Usage: %s [-l] [-s]\n", progname);
	exit(2);
}

int
main(int argc, char **argv)
{
	const char	optstring[] = "ls";
	char	*tty;
	int	i;

#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	progname = basename(argv[0]);
	while ((i = getopt(argc, argv, optstring)) != EOF) {
		switch (i) {
		case 's':
			sflag = 1;
			break;
		case 'l':
			lflag = 1;
			break;
		default:
			usage();
		}
	}
	if (sflag)
		status = isatty(0) == 0;
	else {
		if ((tty = ttyname(0)) != NULL)
			printf("%s\n", tty);
		else {
			printf("not a tty\n");
			status = 1;
		}
	}
	if (lflag)
#ifdef	STWLINE
		if ((i = ioctl(0, STWLINE)) >= 0)
			printf("synchronous line %d\n", i);
		else
#endif	/* !STWLINE */
		/*
		 * There are no synchronous terminals on a Linux system.
		 */
			printf("not on an active synchronous line\n");
	return status;
}
