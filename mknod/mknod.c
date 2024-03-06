/*
 * mknod - build special file
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, August 2003.
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
static const char sccsid[] USED = "@(#)mknod.sl	1.9 (gritter) 12/16/07";

#if defined (__GLIBC__) || defined (_AIX)
#include	<sys/sysmacros.h>
#endif	/* __GLIBC__ || _AIX */
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<libgen.h>
#include	<errno.h>
#include	<string.h>
#include	<limits.h>
#include	<unistd.h>
#ifndef	major
#include	<sys/mkdev.h>
#endif	/* If "major" still not defined. */

#ifndef	S_IFNAM
#define	S_IFNAM		0x5000
#endif
#ifndef	S_INSEM
#define	S_INSEM		0x1
#endif
#ifndef	S_INSHD
#define	S_INSHD		0x2
#endif

static char	*progname;
static long	max_major;
static long	max_minor;

static void
usage(void)
{
	fprintf(stderr, "usage: %s name [ b/c major minor ] [ p ]\n",
			progname);
	exit(2);
}

static int
mnode(const char *name, mode_t type, dev_t dev)
{
	if (mknod(name, type|0666, dev) == 0)
		return 0;
	switch (errno) {
	case EPERM:
		fprintf(stderr, "%s: must be super-user\n", progname);
		break;
	default:
		fprintf(stderr, "%s: %s\n", progname, strerror(errno));
	}
	return 1;
}

static dev_t
nu(const char *s, const char *msg, dev_t max)
{
	int	n;

	n = atoi(s);
	if (n < 0 || n > max) {
		fprintf(stderr, "%s: invalid %s number '%s' - "
				"valid range is 0-%d\n",
				progname, msg, s, (int)max);
		exit(2);
	}
	return n;
}

static int
mspec(int ac, char **av, mode_t type)
{
	dev_t	major, minor;

	if (ac != 5)
		usage();
	major = nu(av[3], "major", max_major);
	minor = nu(av[4], "minor", max_minor);
	return mnode(av[1], type, makedev(major, minor));
}

static int
mfifo(int ac, char **av)
{
	if (ac != 3)
		usage();
	return mnode(av[1], S_IFIFO, 0);
}

static int
mnam(int ac, char **av, mode_t type)
{
	if (ac != 3)
		usage();
	return mnode(av[1], S_IFNAM, type);
}

static void
getlimits(void)
{
#ifdef	__linux__
#ifndef	WORD_BIT
	int	WORD_BIT;
	for (WORD_BIT = 2; (1<<WORD_BIT) != 1; WORD_BIT++);
#endif	/* !WORD_BIT */
	max_major = (1<<(WORD_BIT-20)) - 1;
	max_minor = (1<<20) - 1;
#else	/* !__linux__ */
	max_major = (1<<14) - 1;
	max_minor = (1<<18) - 1;
#endif	/* !__linux__ */
}

int
main(int argc, char **argv)
{
	progname = basename(argv[0]);
	if (argc < 3)
		usage();
	if (argv[2][1] != '\0')
		usage();
	getlimits();
	switch (argv[2][0]) {
	case 'b':
		return mspec(argc, argv, S_IFBLK);
	case 'c':
		return mspec(argc, argv, S_IFCHR);
	case 'p':
		return mfifo(argc, argv);
	case 'm':
		return mnam(argc, argv, S_INSHD);
	case 's':
		return mnam(argc, argv, S_INSEM);
	default:
		usage();
	}
	/*NOTREACHED*/
	return 0;
}
