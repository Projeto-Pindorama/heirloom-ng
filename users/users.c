/*
 * users - display a compact list of users logged in
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
static const char sccsid[] USED = "@(#)users.sl	1.5 (gritter) 5/29/05";

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include <utmpx.h>
#include <errno.h>

int
main(int argc, char **argv)
{
	struct utmpx	*u;
	int	count = 0;

	if (argc > 1 && argv[1][0]=='-' && argv[1][1]=='-' && argv[1][2]=='\0')
		argc--, argv++;
	if (argc == 2) {
		int	fd;

		if ((fd = open(argv[1], O_RDONLY)) < 0) {
			perror(argv[1]);
			return 1;
		}
		close(fd);
#if !defined (__hpux) && !defined (_AIX)
		utmpxname(argv[1]);
#else	/* __hpux, _AIX */
		errno = ENOSYS;
		perror(argv[1]);
		return 1;
#endif	/* __hpux, _AIX */
	}
	while ((u = getutxent()) != NULL) {
		if (u->ut_type == USER_PROCESS) {
			if (count++)
				putchar(' ');
			fputs(u->ut_user, stdout);
		}
	}
	endutxent();
	if (count)
		putchar('\n');
	return 0;
}
