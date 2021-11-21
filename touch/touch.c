/*
 * touch - update access and modification times of a file
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, July 2001.
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
#if defined (SUS)
static const char sccsid[] USED = "@(#)touch_sus.sl	1.21 (gritter) 5/29/05";
#else	/* !SUS */
static const char sccsid[] USED = "@(#)touch.sl	1.21 (gritter) 5/29/05";
#endif	/* !SUS */

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>
#include	<utime.h>
#include	<ctype.h>
#include	<time.h>

static unsigned	errcnt;			/* count of errors */
static int	aflag;			/* select access time */
static int	cflag;			/* do not create files */
static int	mflag;			/* select modification time */
static int	settime;		/* this is the settime command */
static int	date_time;		/* use date_time operand (not -t) */
static time_t	nacc = -1;		/* new access time */
static time_t	nmod = -1;		/* now modification time */
static time_t	now;			/* current time */
static char	*progname;		/* argv[0] to main() */
static int	nulltime;		/* can use settime(NULL) */

/*
 * perror()-alike.
 */
static void
pnerror(int eno, const char *string)
{
	fprintf(stderr, "%s: %s: %s\n", progname, string, strerror(eno));
	errcnt++;
}

/*
 * Touch the named file.
 */
static void
touch(const char *fn)
{
	struct stat st;
	struct utimbuf ut;

	if (stat(fn, &st) < 0) {
		if (errno == ENOENT) {
			int fd;

			if (cflag) {
#ifndef	SUS
				errcnt++;
#endif	/* !SUS */
				return;
			}
			if ((fd = creat(fn, 0666)) < 0) {
				fprintf(stderr, "%s: %s cannot create\n",
						progname, fn);
				errcnt++;
				return;
			}
			if (fstat(fd, &st) < 0) {
				fprintf(stderr, "%s: %s cannot stat\n",
						progname, fn);
				errcnt++;
				close(fd);
				return;
			}
			close(fd);
		} else {
			fprintf(stderr, "%s: %s cannot stat\n",
					progname, fn);
			errcnt++;
			return;
		}
	}
	if (aflag)
		ut.actime = nacc;
	else
		ut.actime = st.st_atime;
	if (mflag)
		ut.modtime = nmod;
	else
		ut.modtime = st.st_mtime;
	if (utime(fn, nulltime ? NULL : &ut) < 0) {
		fprintf(stderr, "%s: cannot change times on %s\n",
				progname, fn);
		errcnt++;
	}
}

static void
badtime(void)
{
	if (date_time)
		fprintf(stderr, "date: bad conversion\n");
	else
		fprintf(stderr, "%s: bad time specification\n", progname);
	exit(2);
}

/*
 * Convert s to int with maximum of m.
 */
static int
atot(const char *s, int m)
{
	char *x;
	int i;

	i = (int)strtol(s, &x, 10);
	if (*x != '\0' || i > m || i < 0 || *s == '+' || *s == '-')
		badtime();
	return i;
}

/*
 * Interpret a time argument, old style: MMDDhhmm[YY].
 */
static void
otime(char *cp)
{
	struct tm *stm;
	time_t t;

	date_time = 1;
	t = now;
	stm = localtime(&t);
	stm->tm_isdst = -1;
	stm->tm_sec = 0;
	switch (strlen(cp)) {
	case 10:
		stm->tm_year = atot(&cp[8], 99);
		if (stm->tm_year < 69)
			stm->tm_year += 100;
		cp[8] = '\0';
		/*FALLTHRU*/
	case 8:
		stm->tm_min = atot(&cp[6], 59);
		cp[6] = '\0';
		stm->tm_hour = atot(&cp[4], 23);
		cp[4] = '\0';
		if ((stm->tm_mday = atot(&cp[2], 31)) == 0)
			badtime();
		cp[2] = '\0';
		if ((stm->tm_mon = atot(cp, 12)) == 0)
			badtime();
		stm->tm_mon--;
		break;
	default:
		badtime();
	}
	if ((t = mktime(stm)) == (time_t)-1)
		badtime();
	nacc = nmod = t;
}

/*
 * Interpret a time argument, new style: [[CC]YY]MMDDhhmm[.SS].
 */
static void
ptime(char *cp)
{
	char year[5];
	struct tm *stm;
	time_t t;
	size_t sz = strlen(cp);

	t = now;
	stm = localtime(&t);
	stm->tm_isdst = -1;
	if (sz == 11 || sz == 13 || sz == 15) {
		if (cp[sz - 3] != '.')
			badtime();
		stm->tm_sec = atot(&cp[sz - 2], 61);
		cp[sz - 3] = '\0';
		sz -= 3;
	} else
		stm->tm_sec = 0;
	if (sz == 12) {
		year[0] = cp[0], year[1] = cp[1], year[2] = cp[2],
			year[3] = cp[3], year[4] = '\0';
		if ((stm->tm_year = atot(year, 30000)) < 1970)
			badtime();
		stm->tm_year -= 1900;
		cp += 4, sz -= 4;
	} else if (sz == 10) {
		year[0] = cp[0], year[1] = cp[1], year[2] = '\0';
		stm->tm_year = atot(year, 99);
		if (stm->tm_year < 69)
			stm->tm_year += 100;
		cp += 2, sz -= 2;
	}
	if (sz != 8)
		badtime();
	stm->tm_min = atot(&cp[6], 59);
	cp[6] = '\0';
	stm->tm_hour = atot(&cp[4], 23);
	cp[4] = '\0';
	if ((stm->tm_mday = atot(&cp[2], 31)) == 0)
		badtime();
	cp[2] = '\0';
	if ((stm->tm_mon = atot(cp, 12)) == 0)
		badtime();
	stm->tm_mon--;
	if ((t = mktime(stm)) == (time_t)-1)
		badtime();
	nacc = nmod = t;
}

/*
 * Get reference time from a file.
 */
static void
reffile(const char *cp)
{
	struct stat st;

	if (stat(cp, &st) < 0) {
		pnerror(errno, cp);
		exit(1);
	}
	nacc = st.st_atime;
	nmod = st.st_mtime;
}

static void
usage(void)
{
	if (settime == 0)
		fprintf(stderr, "usage: %s [-amc] [mmddhhmm[yy]] file ...\n",
       			progname);
	else
		fprintf(stderr, "usage: %s [-f file] [mmddhhmm[yy]] file ...\n",
				progname);
	exit(2);
}

int
main(int argc, char **argv)
{
	int i;

#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	progname = basename(argv[0]);
	if (strcmp(progname, "settime") == 0) {
		settime = 1;
		cflag = 1;
	}
	time(&now);
	while ((i = getopt(argc, argv, settime ? "f:" : "amcr:t:f")) != EOF) {
		switch (i) {
		case 'a':
			aflag = 1;
			break;
		case 'c':
			cflag = 1;
			break;
		case 'm':
			mflag = 1;
			break;
		case 'f':
			if (settime == 0)
				break;
			/*FALLTHRU*/
		case 'r':
			if (nacc != (time_t)-1 || nmod != (time_t)-1)
				usage();
			reffile(optarg);
			break;
		case 't':
			if (nacc != (time_t)-1 || nmod != (time_t)-1)
				usage();
			ptime(optarg);
			break;
		default:
			usage();
		}
	}
	if (nacc == (time_t)-1 && nmod == (time_t)-1 && argv[optind]
#ifdef	SUS
			&& argv[optind + 1]) {
		char	*cp;
		for (cp = argv[optind]; *cp && isdigit(*cp & 0377); cp++);
		if (*cp == '\0' && (cp - argv[optind] == 8 ||
					cp - argv[optind] == 10))
#else	/* !SUS */
			) {
		if (isdigit(argv[optind][0]))
#endif	/* !SUS */
			otime(argv[optind++]);
	}
	if (nacc == (time_t)-1 && nmod == (time_t)-1 && aflag == 0 &&
			mflag == 0)
		nulltime = 1;
	if (nacc == (time_t)-1)
		nacc = now;
	if (nmod == (time_t)-1)
		nmod = now;
	if (aflag == 0 && mflag == 0)
		aflag = mflag = 1;
#ifdef	SUS
	if (optind >= argc)
		usage();
#else	/* !SUS */
	if (optind >= argc && date_time == 0)
		usage();
#endif	/* !SUS */
	for (i = optind; i < argc; i++)
		touch(argv[i]);
	return errcnt < 0100 ? errcnt : 077;
}
