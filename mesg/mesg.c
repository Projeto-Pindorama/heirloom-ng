/*
 * mesg - permit or deny messages
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, June 2005.
 */
/*
 * Copyright (c) 2005 Gunnar Ritter
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
static const char sccsid[] USED = "@(#)mesg.c	1.2 (gritter) 7/29/05";

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>

static const char	*progname;
static int		status;

static void
usage(void)
{
	fprintf(stderr, "%s: usage: %s [-y] [-n]\n", progname, progname);
	exit(2);
}

static void
smode(const char *tp, mode_t mode)
{
	if (chmod(tp, mode) < 0) {
		fprintf(stderr, "%s: cannot change mode\n", progname);
		exit(2);
	}
}

int
main(int argc, char **argv)
{
	int	f = 0;
	char	*tp;
	struct stat	st;

	progname = basename(argv[0]);
	if (argc > 1 && argv[1][0] == '-' && argv[1][1] == '-' &&
			argv[1][2] == '\0') {
		argv++;
		argc--;
	}
	if (argc > 1) {
		switch (argv[1][0]) {
		case 'y':
		case 'n':
			f = argv[1][0];
			break;
		case '-':
			switch (argv[1][1]) {
			case 'y':
			case 'n':
				f = argv[1][1];
				break;
			default:
				while (argv[1][1]) {
					fprintf(stderr,
						"%s: illegal option -- %c\n",
						progname, argv[1][1]);
					argv[1]++;
				}
				usage();
			}
			break;
		default:
			usage();
		}
	}
	if ((tp = ttyname(0)) == NULL &&
			(tp = ttyname(1)) == NULL &&
			(tp = ttyname(2)) == NULL) {
		fprintf(stderr, "%s: no tty\n", progname);
		return 2;
	}
	if (stat(tp, &st) < 0) {
		fprintf(stderr, "%s: cannot stat\n", progname);
		return 2;
	}
	switch (f) {
	case 0:
		printf("is %c\n", st.st_mode & 0020 ? 'y' : 'n');
		status = (st.st_mode & 0020) == 0;
		break;
	case 'y':
		smode(tp, st.st_mode | 0020);
		break;
	case 'n':
		smode(tp, st.st_mode & ~(mode_t)0020);
		status = 1;
		break;
	}
	return status;
}
