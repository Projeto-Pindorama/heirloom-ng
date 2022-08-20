/*
 * hostname - set or print name of current host system
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, October 2003.
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
static const char sccsid[] USED = "@(#)hostname.sl	1.3 (gritter) 5/29/05";

#include <sys/utsname.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <libgen.h>

static const char	*progname;

int
main(int argc, char **argv)
{
	progname = basename(argv[0]);
	if (argc > 1 && argv[1][0]=='-' && argv[1][1]=='-' && argv[1][2]=='\0')
		argc--, argv++;
	if (argc == 1) {
		struct utsname	u;

		if (uname(&u) < 0) {
			perror("uname");
			return 1;
		}
		puts(u.nodename);
	} else if (argc == 2) {
		if (sethostname(argv[1], strlen(argv[1]))) {
			perror("sethostname");
			return 1;
		}
	} else {
		fprintf(stderr, "Usage: %s [name]\n", progname);
		return 2;
	}
	return 0;
}
