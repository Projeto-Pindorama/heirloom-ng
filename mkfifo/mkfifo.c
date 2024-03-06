/*
 * mkfifo - make FIFO special file
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, August 2003.
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
static const char sccsid[] USED = "@(#)mkfifo.sl	1.4 (gritter) 5/29/05";

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<libgen.h>
#include	<errno.h>

static char	*progname;		/* argv[0] to main() */
static int	errcnt;			/* count of errors */
static char	*mflag;			/* set a mode */
static mode_t	um;			/* umask value */

static mode_t	newmode(const char *ms, const mode_t pm, const char *fn);

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-m mode] path ...\n", progname);
	exit(2);
}

static void
mfifo(const char *path)
{
	mode_t	mode;

	mode = mflag ? newmode(mflag, 0666, path) : 0666;
	if (mkfifo(path, mode) < 0) {
		fprintf(stderr, "%s: %s\n", progname, strerror(errno));
		errcnt++;
		return;
	}
	if (mflag)
		chmod(path, mode);
}

int
main(int argc, char **argv)
{
	const char	optstring[] = "m:";
	int	i;

	progname = basename(argv[0]);
	umask(um = umask(0));
	while ((i = getopt(argc, argv, optstring)) != EOF) {
		switch (i) {
		case 'm':
			mflag = optarg;
			newmode(mflag, 0666, NULL);
			break;
		default:
			usage();
		}
	}
	if (optind == argc)
		usage();
	for (i = optind; i < argc; i++)
		mfifo(argv[i]);
	return errcnt > 255 ? 255 : errcnt;
}

/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, March 2003.
 */
/*	from Unix 7th Edition /usr/src/cmd/chmod.c	*/
/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * SPDX-Licence-Identifier: Caldera
 */

#define	USER	05700	/* user's bits */
#define	GROUP	02070	/* group's bits */
#define	OTHER	00007	/* other's bits */
#define	ALL	07777	/* all */

#define	READ	00444	/* read permit */
#define	WRITE	00222	/* write permit */
#define	EXEC	00111	/* exec permit */
#define	SETID	06000	/* set[ug]id */
#define	STICKY	01000	/* sticky bit */

#ifndef	S_ENFMT
#define	S_ENFMT	02000	/* mandatory locking bit */
#endif

static mode_t	absol(const char **);
static mode_t	who(const char **, mode_t *);
static int	what(const char **);
static mode_t	where(const char **, mode_t, int *, int *, const mode_t);

static mode_t
newmode(const char *ms, const mode_t pm, const char *fn)
{
	register mode_t	o, m, b;
	int	lock, setsgid = 0, cleared = 0, copy = 0;
	mode_t	nm, om, mm;

	nm = om = pm;
	m = absol(&ms);
	if (!*ms) {
		nm = m;
		goto out;
	}
	if ((lock = (nm&S_IFMT) != S_IFDIR && (nm&(S_ENFMT|S_IXGRP)) == S_ENFMT)
			== 01)
		nm &= ~(mode_t)S_ENFMT;
	do {
		m = who(&ms, &mm);
		while (o = what(&ms)) {
			b = where(&ms, nm, &lock, &copy, pm);
			switch (o) {
			case '+':
				nm |= b & m & ~mm;
				if (b & S_ISGID)
					setsgid = 1;
				if (lock & 04)
					lock |= 02;
				break;
			case '-':
				nm &= ~(b & m & ~mm);
				if (b & S_ISGID)
					setsgid = 1;
				if (lock & 04)
					lock = 0;
				break;
			case '=':
				nm &= ~m;
				nm |= b & m & ~mm;
				lock &= ~01;
				if (lock & 04)
					lock |= 02;
				om = 0;
				if (copy == 0)
					cleared = 1;
				break;
			}
			lock &= ~04;
		}
	} while (*ms++ == ',');
	if (*--ms) {
		fprintf(stderr, "Invalid mode.\n");
		exit(2);
	}
out:	if ((pm & S_ISGID) && setsgid == 0)
		nm |= S_ISGID;
	else if ((nm & S_ISGID) && setsgid == 0)
		nm &= ~(mode_t)S_ISGID;
	return(nm);
}

static mode_t
absol(const char **ms)
{
	register int c, i;

	i = 0;
	while ((c = *(*ms)++) >= '0' && c <= '7')
		i = (i << 3) + (c - '0');
	(*ms)--;
	return(i);
}

static mode_t
who(const char **ms, mode_t *mp)
{
	register int m;

	m = 0;
	*mp = 0;
	for (;;) switch (*(*ms)++) {
	case 'u':
		m |= USER;
		continue;
	case 'g':
		m |= GROUP;
		continue;
	case 'o':
		m |= OTHER;
		continue;
	case 'a':
		m |= ALL;
		continue;
	default:
		(*ms)--;
		if (m == 0) {
			m = ALL;
			*mp = um;
		}
		return m;
	}
}

static int
what(const char **ms)
{
	switch (**ms) {
	case '+':
	case '-':
	case '=':
		return *(*ms)++;
	}
	return(0);
}

static mode_t
where(const char **ms, mode_t om, int *lock, int *copy, const mode_t pm)
{
	register mode_t m;

	m = 0;
	*copy = 0;
	switch (**ms) {
	case 'u':
		m = (om & USER) >> 6;
		goto dup;
	case 'g':
		m = (om & GROUP) >> 3;
		goto dup;
	case 'o':
		m = (om & OTHER);
	dup:
		*copy = 1;
		m &= (READ|WRITE|EXEC);
		m |= (m << 3) | (m << 6);
		++(*ms);
		return m;
	}
	for (;;) switch (*(*ms)++) {
	case 'r':
		m |= READ;
		continue;
	case 'w':
		m |= WRITE;
		continue;
	case 'x':
		m |= EXEC;
		continue;
	case 'X':
		if ((pm&S_IFMT) == S_IFDIR || (pm & EXEC))
			m |= EXEC;
		continue;
	case 'l':
		if ((pm&S_IFMT) != S_IFDIR)
			*lock |= 04;
		continue;
	case 's':
		m |= SETID;
		continue;
	case 't':
		m |= STICKY;
		continue;
	default:
		(*ms)--;
		return m;
	}
}
