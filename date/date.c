/*
 * date - write the date and time
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, June 2002.
 */
/*
 * Copyright (c) 2003 Gunnar Ritter
 * Copyright (c) 2023 - 2024: Luiz Antônio (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 */

#define USED
#if defined (SUS)
static const char sccsid[] USED = "@(#)date_sus.sl	1.27 (gritter) 12/16/07";
#else
static const char sccsid[] USED = "@(#)date.sl	1.27 (gritter) 12/16/07";
#endif

#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>
#include	<time.h>
#include	<locale.h>
#include	<utmpx.h>
#include	<langinfo.h>

static unsigned	errcnt;			/* count of errors */
static int	bflag;			/* boot flag: don't write wtmp */
static int	uflag;			/* use UTC */
static char	*progname;		/* argv[0] to main() */
static time_t	now;			/* current time */

static void
usage(void)
{
	fprintf(stderr,
	"usage: %s [-u] [+format] [hhmm | mmddhhmm[[cc]yy]] [-a sss.fff]\n",
		progname);
	exit(1);
}

static void
badconv(void)
{
	fprintf(stderr, "%s: bad conversion\n", progname);
	exit(1);
}

static int
atot(char *s, int m)
{
	char *x;
	int i;

	i = (int)strtol(s, &x, 10);
	if (*x != '\0' || i > m || i < 0 || *s == '-' || *s == '+')
		return -1;
	return i;
}

static time_t
timeop(char *op)
{
	struct tm *tp;

	tp = uflag ? gmtime(&now) : localtime(&now);
	tp->tm_isdst = -1;
	tp->tm_sec = 0;
	switch(strlen(op)) {
	case 12:
		if ((tp->tm_year = atot(&op[8], 30000)) < 1970)
			goto badtime;
		tp->tm_year -= 1900;
		goto bypass;
	case 10:
		if ((tp->tm_year = atot(&op[8], 99)) < 0)
			goto badtime;
		if (tp->tm_year < 69)
			tp->tm_year += 100;
bypass:
		op[8] = '\0';
		/*FALLTHRU*/
	case 8:
		if ((tp->tm_min = atot(&op[6], 59)) < 0)
			goto badtime;
		op[6] = '\0';
		if ((tp->tm_hour = atot(&op[4], 23)) < 0)
			goto badtime;
		op[4] = '\0';
		if ((tp->tm_mday = atot(&op[2], 31)) <= 0)
			goto badtime;
		op[2] = '\0';
		if ((tp->tm_mon = atot(op, 12)) <= 0)
			goto badtime;
		tp->tm_mon--;
		break;
	case 4:
		if ((tp->tm_min = atot(&op[2], 59)) < 0)
			goto badtime;
		op[2] = '\0';
		if ((tp->tm_hour = atot(op, 23)) < 0)
			goto badtime;
		break;
	default:
		goto badtime;
	}
	return mktime(tp);
badtime:
	return (time_t)-1;
}

#if defined (__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) || \
	defined (__DragonFly__) || defined (__APPLE__) || __GLIBC__ >= 2 && \
	__GLIBC_MINOR__ >= 31
static int
stime(time_t *t)
{
	struct timeval	tv;

	tv.tv_sec = *t;
	tv.tv_usec = 0;

	return settimeofday(&tv, NULL);
}
#endif	/* __FreeBSD__, __NetBSD__, __OpenBSD__, __DragonFly__, __APPLE__, GLIBC above 2.35*/

static void
settime(char *op)
{
	/*
	 * Since it's not guaranteed that (utmpx).ut_tv
	 * is a struct of the type timeval, we will be
	 * recording the time from gettimeofday(2) on
	 * a temporary struct of the type timeval and
	 * then copying the values to the ".ut_tv".
	 */
	struct utmpx before, after;
	struct timeval tv_tmp;
	const char wtmpxfile[] = "/var/log/wtmp";
	time_t newtime;

	memset(&before, 0, sizeof before);
	memset(&after, 0, sizeof after);
	before.ut_type = OLD_TIME;
	after.ut_type = NEW_TIME;
	strcpy(before.ut_line, "old time");
	strcpy(after.ut_line, "new time");
	if ((newtime = timeop(op)) == (time_t)-1)
		badconv();
	gettimeofday(&tv_tmp, NULL);
	before.ut_tv.tv_sec = tv_tmp.tv_sec;
	before.ut_tv.tv_usec = tv_tmp.tv_usec;
	/* Clean it for (struct utmpx)after. */
	memset(&tv_tmp, 0, sizeof tv_tmp);
	if (stime(&newtime) < 0) {
		fprintf(stderr, "%s: no permission\n", progname);
		exit(1);
	}
	gettimeofday(&tv_tmp, NULL);
	after.ut_tv.tv_sec = tv_tmp.tv_sec;
	after.ut_tv.tv_usec = tv_tmp.tv_usec;
#ifdef	__linux__
	system("/sbin/hwclock -w >/dev/null 2>&1");
#endif	/* __linux__ */
	if (bflag == 0) {
		pututxline(&before);
		pututxline(&after);
#if !defined (__hpux) && !defined (_AIX) && !defined(__APPLE__)
		updwtmpx(wtmpxfile, &before);
		updwtmpx(wtmpxfile, &after);
#endif	/* !__hpux, !__AIX, !__APPLE__ */
	}
	exit(0);
}

static void
printtime(const char *cp)
{
	struct tm *tp;
	char fmt[256];
	char *fp;
	char buf[256];
	int mod;
	char	*date_fmt;

	tp = uflag ? gmtime(&now) : localtime(&now);
	while (*cp != '\0') {
		if (*cp == '%') {
			const char	valid[] = "aAbBc"
#ifdef	SUS
					"C"
#endif	/* SUS */
					"dDeFgGhHIjmMnprRStTuUVwWxXyYZ%";
			fp = fmt;
			*fp++ = *cp++;
			if (*cp == 'E' || *cp == 'O')
				mod = *cp++;
			else
				mod = '\0';
			if (*cp == '\0') {
				putchar('%');
				goto end;
			}
			if (!strchr(valid, *cp)) {
				switch (*cp) {
				case 'N':
#ifndef	SUS
				case 'C':
#endif	/* !SUS */
#ifdef	_DATE_FMT
					if ((date_fmt = nl_langinfo(_DATE_FMT))
							== NULL ||
							*date_fmt == '\0')
#endif	/* _DATE_FMT */
						date_fmt =
						"%a %b %e %H:%M:%S %Z %Y";
					strcpy(fmt, date_fmt);
					break;
				case 'E':
				case 'O':
					goto next;
				case 'z': {
						  int hour, min;
#if defined (__FreeBSD__) || defined (__OpenBSD__) || defined (__DragonFly__) \
	|| defined (__APPLE__)
						  long	timezone;
						  timezone = -tp->tm_gmtoff;
#endif	/* __FreeBSD__, __OpenBSD__, __DragonFly__, __APPLE__ */

						  hour = -timezone / 3600;
						  min = (timezone / 60) % 60;
						  if (tp->tm_isdst > 0)
							  hour++;
						  fp = fmt;
						  if (hour <= 0) {
							  *fp++ = '-';
							  hour = -hour;
						  }
						  sprintf(fp, "%02d%02d",
								  hour, min);
					  }
					break;
				default:
					*fp++ = '%';
					if (mod)
						*fp++ = mod;
					*fp++ = *cp;
					*fp++ = '\0';
				}
			} else {
				if ((mod == 'E' && strchr("cCxXyY", *cp)) ||
						(mod == 'O' &&
						 strchr("deHImMSuUVwWy", *cp)))
					*fp++ = mod;
				*fp++ = *cp;
				*fp = '\0';
			}
			strftime(buf, sizeof buf, fmt, tp);
			fputs(buf, stdout);
		} else
			putchar(*cp);
next:
		cp++;
	}
end:
	putchar('\n');
}

static void
adjust(char *op)
{
	struct timeval tv;
	char *cp;

	if ((cp = strchr(op, '.')) != NULL) {
		if ((tv.tv_usec = atot(&cp[1], 999)) < 0)
			usage();
		tv.tv_usec *= 1000;
		*cp = '\0';
	}
	if ((tv.tv_sec = atot(*op == '-' ? &op[1] : op, 999)) < 0)
		usage();
	if (*op == '-') {
		tv.tv_sec = -tv.tv_sec;
		tv.tv_usec = -tv.tv_usec;
	}
	if (adjtime(&tv, 0) < 0) {
		fprintf(stderr, "%s: Failed to adjust date: %s\n", progname,
				strerror(errno));
		exit(1);
	}
	return;
}

int
main(int argc, char **argv)
{
	const char optstring[] = ":a:bu";
	char *adjustment = NULL;
	int i;

	time(&now);
	progname = basename(argv[0]);
	setlocale(LC_TIME, "");
#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	while ((i = getopt(argc, argv, optstring)) != EOF) {
		switch (i) {
		case 'a':
			adjustment = optarg;
			break;
		case 'b':
			bflag = 1;
			break;
		case 'u':
			uflag = 1;
			putenv("TZ=GMT");
			tzset();
			break;
		case ':':
			usage();
			/*NOTREACHED*/
		default:
			/*fprintf(stderr, "%s: no TOY clock\n", progname);
			exit(2);*/
			badconv();
		}
	}
	if (adjustment) {
		if (uflag)
			badconv();
		adjust(adjustment);
	} else if (argv[optind] && argv[optind][0] == '+') {
		if (argv[optind][1] == '\0')
			goto dfl;
		printtime(&argv[optind][1]);
	} else if (argv[optind])
		settime(argv[optind]);
	else
	dfl:	printtime("%N");
	return errcnt;
}
