/*
 * who - who is on the system
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, August 2002.
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
#ifdef	SUS
static const char sccsid[] USED = "@(#)who_sus.sl	1.19 (gritter) 1/22/06";
#else
static const char sccsid[] USED = "@(#)who.sl	1.19 (gritter) 1/22/06";
#endif

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/time.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<time.h>
#include	<libgen.h>
#include	<utmpx.h>
#include	<limits.h>

enum okay {
	OKAY,
	STOP
};

static enum {
	FL_b = 00001,
	FL_d = 00002,
	FL_l = 00004,
	FL_p = 00010,
	FL_r = 00020,
	FL_t = 00040,
	FL_A = 00100,
	FL_USER = 01000
} flags;

static unsigned	errcnt;			/* count of errors */
static int	Hflag;			/* print headers */
static int	mflag;			/* select who am i */
static int	qflag;			/* quick who */
static int	Rflag;			/* display host name */
static int	sflag;			/* short listing */
static int	Tflag;			/* show terminal state */
static int	uflag;			/* write idle time */
static int	number = 8;		/* for -q */
static time_t	now;
static time_t	boottime;
static char	*whoami;		/* current terminal only */
static char	*progname;		/* argv[0] to main() */

static void
usage(void)
{
	fprintf(stderr, "\n\
Usage:\t%s[-abdHlmnpqrRstTu] [am i] [utmp_like_file]\n\
a\tAll options (same as -bdlprtTu)\n\
b\tboot record\n\
d\tdead processes\n\
H\tprint column headings\n\
l\tlogin lines\n\
m\trestrict to current terminal\n\
n #\tnumber of columns for -q option\n\
p\tother active processes\n\
q\tquick format\n\
r\trun-level record\n\
R\tprint host name\n\
s\tshort who (default)\n\
t\tsystem time changes\n\
T\tterminal status (+ writable, - not writable, ? indeterminable)\n\
u\tadds idle time\n\
",
		progname);
	exit(2);
}

static void
header(void)
{
	printf("NAME       LINE         TIME");
	if (uflag)
		printf("          IDLE    PID");
	if (flags & (FL_d|FL_r))
		printf("%s COMMENTS", uflag ? " " : "         ");
	putchar('\n');
}

static void
print(const struct utmpx *u)
{
	struct tm	*tp;
	char	buf[LINE_MAX];
	const char	*cp;
	int	c;
	time_t	t;

	if (u->ut_type == LOGIN_PROCESS)
		cp = "LOGIN";
	else if (u->ut_type == BOOT_TIME || u->ut_type == RUN_LVL ||
			u->ut_user[0] == '\0')
		cp = "   .";
	else
		cp = u->ut_user;
	printf("%-8.*s ", (int)sizeof u->ut_user, cp);
	if (Tflag && u->ut_type == USER_PROCESS) {
		struct stat	st;

		snprintf(buf, sizeof buf, "/dev/%.*s",
				(int)sizeof u->ut_line, u->ut_line);
		if (stat(buf, &st) == 0)
			c = (st.st_mode & 0022) ? '+' : '-';
		else
			c = '?';
	} else
		c = ' ';
	printf("%c ", c);
	if (u->ut_type == RUN_LVL) {
		snprintf(buf, sizeof buf, "run-level %c",
				(int)(u->ut_pid & 0377));
		cp = buf;
	} else if (u->ut_type == BOOT_TIME)
		cp = "system boot";
	else if (u->ut_line[0] == '\0')
		cp = "     .";
	else
		cp = u->ut_line;
	printf("%-12.*s ", (int)sizeof u->ut_line, cp);
	t = u->ut_tv.tv_sec;
	tp = localtime(&t);
	strftime(buf, sizeof buf, "%b %e %H:%M", tp);
	printf("%s", buf);
	if (uflag && !sflag) {
		struct stat	st;

		snprintf(buf, sizeof buf, "/dev/%.*s",
				(int)sizeof u->ut_line, u->ut_line);
		if (stat(buf, &st) == 0) {
			time_t	idle;
			int	hours, minutes;

			if ((idle = now - st.st_mtime) < 0)
				idle = 0;
			hours = idle / 3600;
			minutes = (idle % 3600) / 60;
			/*
			 * Boottime might not be particularly accurate;
			 * add 10 seconds grace time.
			 */
			if (hours >= 24 || st.st_mtime <= boottime + 10)
				cp = " old ";
			else if (hours > 0 || minutes > 0) {
				snprintf(buf, sizeof buf, "%2d:%02d",
						hours, minutes);
				cp = buf;
			} else
				cp = "  .  ";
			printf(" %s", cp);
		}
		if (u->ut_type != RUN_LVL && u->ut_type != BOOT_TIME
#ifdef	ACCOUNTING
				&& u->ut_type != ACCOUNTING
#endif	/* ACCOUNTING */
				)
			printf(" %6d", (int)u->ut_pid);
	}
	if (u->ut_type == DEAD_PROCESS && !sflag)
#ifdef	__hpux
#define	e_termination	__e_termination
#define	e_exit		__e_exit
#endif	/* __hpux */
		printf("  id=%4.4s term=%-3d exit=%d",
				u->ut_id,
#if !defined (_AIX) || !defined (__APPLE__)
				u->ut_exit.e_termination,
				u->ut_exit.e_exit
#else	/* _AIX, __APPLE__ */
				0,
				0
#endif	/* _AIX, __APPLE__ */
				);
	else if (u->ut_type == INIT_PROCESS && !sflag)
		printf("  id=%4.4s", u->ut_id);
	else if (u->ut_type == RUN_LVL)
		printf("    %c    %-4ld %c", (int)(u->ut_pid & 0377),
				0L, (int)((u->ut_pid & 0177777) / 0400));
	if (Rflag && u->ut_host[0])
		printf("\t(%.*s)", (int)sizeof u->ut_host, u->ut_host);
	putchar('\n');
}

static enum okay
selected(const struct utmpx *u)
{
	enum okay	val = STOP;

	switch (u->ut_type) {
	case RUN_LVL:
		if (flags & FL_r)
			val = OKAY;
		break;
	case BOOT_TIME:
		if (flags & FL_b)
			val = OKAY;
		break;
	case NEW_TIME:
	case OLD_TIME:
		if (flags & FL_t)
			val = OKAY;
		break;
	case INIT_PROCESS:
		if (flags & FL_p)
			val = OKAY;
		break;
	case LOGIN_PROCESS:
		if (flags & FL_l)
			val = OKAY;
		break;
	case USER_PROCESS:
		if (flags & FL_USER)
			val = OKAY;
		break;
	case DEAD_PROCESS:
		if (flags & FL_d)
			val = OKAY;
		break;
#ifdef	ACCOUNTING
	case ACCOUNTING:
		if (flags & FL_A)
			val = OKAY;
#endif
	}
	if (val == OKAY && whoami != NULL && strncmp(whoami, u->ut_line,
				sizeof u->ut_line))
		val = STOP;
	return val;
}

static void
who(void)
{
	struct utmpx	*u;
	int	users = 0, printed = 0;

	if (Hflag)
		header();
	if (uflag) {
		struct utmpx	id;
		setutxent();
		id.ut_type = BOOT_TIME;
		if ((u = getutxid(&id)) != NULL)
			boottime = u->ut_tv.tv_sec;
	}
	setutxent();
	time(&now);
	while ((u = getutxent()) != NULL) {
		if (qflag) {
			if (u->ut_type == USER_PROCESS) {
				users++;
				printf("%-8.*s%c", (int)sizeof u->ut_user,
						u->ut_user,
						++printed < number ?
						' ' : '\n');
				if (printed == number)
					printed = 0;
			}
		} else {
			if (selected(u) == OKAY)
				print(u);
		}
	}
	endutxent();
	if (qflag) {
		if (printed > 0)
			putchar('\n');
		printf("# users=%u\n", users);
	}
}

int
main(int argc, char **argv)
{
	const char	optstring[] = "AabdHlmn:pqRrstTu";
	int	i;

#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	progname = basename(argv[0]);
	while ((i = getopt(argc, argv, optstring)) != EOF) {
		switch (i) {
		case 'A':
			flags |= FL_A;
			break;
		case 'a':
			flags |= FL_b|FL_d|FL_l|FL_p|FL_r|FL_t|FL_USER;
			uflag = Tflag = 1;
			break;
		case 'b':
			flags |= FL_b;
			break;
		case 'd':
			flags |= FL_d;
			break;
		case 'H':
			Hflag = 1;
			break;
		case 'l':
			flags |= FL_l;
			break;
		case 'm':
			mflag = 1;
			break;
		case 'n':
			if ((number = atoi(optarg)) < 1)
				number = 1;
			break;
		case 'p':
			flags |= FL_p;
			break;
		case 'q':
			qflag = 1;
			break;
		case 'R':
			Rflag = 1;
			break;
		case 'r':
			flags |= FL_r;
			break;
		case 's':
			sflag = 1;
			break;
		case 't':
			flags |= FL_t;
			break;
		case 'T':
			Tflag = 1;
#ifdef	SUS
			break;
#else
			/*FALLTHRU*/
#endif
		case 'u':
			uflag = 1;
			break;
		default:
			usage();
		}
	}
	if (flags == 0)
		flags |= FL_USER;
	if (argc - optind == 1) {
		struct stat	st;
		int	fd;

		if (stat(argv[optind], &st) < 0) {
			fprintf(stderr, "%s: Cannot stat file '%s': %s\n",
					progname,
					argv[optind], strerror(errno));
			exit(1);
		}
		if ((fd = open(argv[optind], O_RDONLY)) < 0) {
			fprintf(stderr, "%s: Cannot open file '%s': %s\n",
					progname,
					argv[optind], strerror(errno));
			exit(1);
		}
		close(fd);
		if (st.st_size % sizeof (struct utmpx)) {
			fprintf(stderr, "%s: File '%s' is not a utmp file\n",
					progname, argv[optind]);
			exit(1);
		}
#if !defined (__hpux) && !defined (_AIX)
		utmpxname(argv[optind]);
#else	/* __hpux, _AIX */
		fprintf(stderr, "%s: %s\n", progname, strerror(errno));
		exit(1);
#endif	/* __hpux, _AIX */
	}
	if (mflag || (argc - optind == 2 && strcmp(argv[optind], "am") == 0 &&
			(argv[optind+1][0] == 'i' ||
			 argv[optind+1][0] == 'I') &&
			argv[optind+1][1] == '\0')) {
		if ((whoami = ttyname(0)) == NULL &&
				(whoami = ttyname(1)) == NULL &&
				(whoami = ttyname(2)) == NULL) {
			fprintf(stderr, "Must be attached to a terminal "
					"for the '%s' option\n",
					mflag ? "-m" : "am I");
			exit(4);
		}
		if (strncmp(whoami, "/dev/", 5) == 0)
			whoami = &whoami[5];
	}
	who();
	return errcnt;
}
