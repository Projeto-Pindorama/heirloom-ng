/*
 * rmdir - remove directories
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, July 2002.
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
static const char sccsid[] USED = "@(#)rmdir.sl	1.10 (gritter) 5/29/05";

#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>

static unsigned	errcnt;			/* count of errors */
static int	pflag;			/* remove parent directories */
static int	sflag;			/* silent if -p */
static char	*progname;		/* argv[0] to main() */

static void *
smalloc(size_t n)
{
	void	*s;

	if ((s = malloc(n)) == NULL) {
		write(2, "no memory\n", 10);
		exit(077);
	}
	return s;
}

static void
usage(void)
{
	fprintf(stderr, "%s: usage: %s [-ps] dirname ...\n",
			progname, progname);
	exit(2);
}

static void
doit(const char *path)
{
	if (pflag) {
		char *cp, *cq;

		cp = smalloc(strlen(path) + 1);
		strcpy(cp, path);
		for (cq = cp; *cq; cq++);
		while (cp > cq && cq[-1] == '/')
			cq--;
		*cq = '\0';
		while (*cp) {
			if (rmdir(cp) < 0) {
				if (!sflag) {
					fprintf(stderr,
				"%s: %s: %s not removed; %s\n",
					progname, path, cp, strerror(errno));
					errcnt |= 1;
				}
				break;
			}
			while (cq > cp && cq[-1] != '/')
				cq--;
			while (cq > cp && cq[-1] == '/')
				cq--;
			if (cp == cq && cp[0] == '/' && cp[1] != '\0')
				cq[1] = '\0';
			else
				cq[0] = '\0';
		}
#ifndef	SUS
		if (*cp == '\0')
			printf("%s: %s: Whole path removed.\n", progname, path);
#endif	/* !SUS */
		free(cp);
	} else {
		if (rmdir(path) < 0) {
			fprintf(stderr, "%s: %s: %s\n",
				progname, path, strerror(errno));
			errcnt |= 1;
		}
	}
}

int
main(int argc, char **argv)
{
	int i;

#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	progname = basename(argv[0]);
	while ((i = getopt(argc, argv, "ps")) != EOF) {
		switch (i) {
		case 'p':
			pflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		default:
			usage();
		}
	}
	if (optind >= argc)
		usage();
	while (optind < argc)
		doit(argv[optind++]);
	return errcnt;
}
