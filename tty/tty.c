/*
 * tty - get the name of the terminal
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, March 2003.
 */
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
