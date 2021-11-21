/*
 * sdiff - print file differences side-by-side
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, April 2003
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
static const char sccsid[] USED = "@(#)sdiff.sl	1.8 (gritter) 5/29/05";

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <stdarg.h>
#include <limits.h>
#include <signal.h>
#include "sigset.h"
#include <locale.h>
#include <wchar.h>
#include "atoll.h"

#if defined (__GLIBC__) && defined(_IO_putc_unlocked)
#undef	putchar
#define	putchar(c)	_IO_putc_unlocked(c, stdout)
#endif

#ifndef	LLONG_MAX
#define	LLONG_MAX	9223372036854775807LL
#endif

#define	next(wc, s, n) (mb_cur_max > 1 && *(s) & 0200 ? \
			((n) = mbtowi(&(wc), (s), mb_cur_max), \
			 (n) = ((n) > 0 ? (n) : (n) < 0 ? (wc=WEOF, 1) : 1)) :\
		((wc) = *(s) & 0377, (n) = 1))

#include <iblok.h>
#include <mbtowi.h>

struct difference {
	long long	d_lbot;
	long long	d_ltop;
	long long	d_rbot;
	long long	d_rtop;
	enum {
		D_A,
		D_C,
		D_D
	}		d_type;
};

static char	*progname;
static int	lflag;		/* print left side of identical lines only */
static int	sflag;		/* do not print identical lines */
static int	wflag = 130;	/* output line width */
static int	width;		/* output part width */
static int	mb_cur_max;	/* MB_CUR_MAX acceleration */
static FILE	*oflag;		/* output to file */
static FILE	*ltemp;		/* left temporary file */
static FILE	*rtemp;		/* right temporary file */

static char	ltname[] = "/tmp/sdifflXXXXXX";
static char	rtname[] = "/tmp/sdiffrXXXXXX";
static char	ttname[20];

static void	usage(void);
static void	fatal(int, const char *, ...);
static void	*smalloc(size_t);
static void	*srealloc(void *, size_t);
static struct iblok	*openfile(int, const char *, const char *,
				const char **);
static struct iblok	*copyin(const char **);
static struct iblok	*diff(const char *, const char *, int);
static void	sdiff(struct iblok *, struct iblok *, struct iblok *);
static int	dget(struct iblok *, struct difference *);
static int	pass(struct iblok *, struct difference *, char **, size_t *);
static long long	dput(struct iblok *, struct iblok *,
				struct difference *);
static void	iput(struct iblok *, int, int, FILE *);
static void	gput(int);
static long long	cput(struct iblok *, struct iblok *,
				struct difference *, long long);
static void	done(int);
static void	ask(void);
static void	iflush(char);
static void	tcopy(FILE *, FILE *);
static void	edit(const char *, FILE *, FILE *);

int
main(int argc, char **argv)
{
	const char	optstring[] = ":w:lso:";
	const char	*fl, *fr;
	static char	illegal[20];
	struct iblok	*il, *ir, *id;
	int	i, j = 0;

	progname = basename(argv[0]);
	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	while ((i = getopt(argc, argv, optstring)) != EOF) {
		switch (i) {
		case 'l':
			lflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 'w':
			if ((wflag = atoi(optarg)) < 20)
				fatal(2, "Wrong line length %s", optarg);
			break;
		case 'o':
			if ((oflag = fopen(optarg, "w")) == NULL)
				fatal(2, "Cannot open output %s", optarg);
			break;
		default:
			if (j < sizeof illegal - 1)
				illegal[j++] = optopt;
		}
	}
	if (optind != argc - 2)
		usage();
	if (illegal[0])
		fatal(2, "Illegal argument: %s", illegal);
	width = (wflag - 5) >> 1;
	if (oflag) {
		if (sigset(SIGHUP, SIG_IGN) != SIG_IGN)
			sigset(SIGHUP, done);
		if (sigset(SIGINT, SIG_IGN) != SIG_IGN)
			sigset(SIGINT, done);
		if (sigset(SIGPIPE, SIG_IGN) != SIG_IGN)
			sigset(SIGPIPE, done);
		if (sigset(SIGTERM, SIG_IGN) != SIG_IGN)
			sigset(SIGTERM, done);
		if ((i = mkstemp(ltname)) < 0 ||
				(ltemp = fdopen(i, "r+")) == NULL)
			fatal(1, "Cannot open temp %s", ltname);
		if ((i = mkstemp(rtname)) < 0 ||
				(rtemp = fdopen(i, "r+")) == NULL)
			fatal(1, "Cannot open temp %s", rtname);
	}
	il = openfile(0, argv[optind], argv[optind+1], &fl);
	ir = openfile(1, argv[optind+1], argv[optind], &fr);
	id = diff(fl, fr, ir->ib_fd);
	if (fr[0] == '-' && fr[1] == '\0')
		lseek(ir->ib_fd, 0, SEEK_SET);
	sdiff(il, ir, id);
	done(0);
	/*NOTREACHED*/
	return 0;
}

static void
usage(void)
{
	fprintf(stderr, "Usage: %s [-l] [-s] [-o output] [-w #] file1 file2\n",
			progname);
	exit(2);
}

static void
fatal(int i, const char *fmt, ...)
{
	va_list	ap;

	fprintf(stderr, "%s: ", progname);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
	done(i);
}

static void *
smalloc(size_t size)
{
	return srealloc(NULL, size);
}

static void *
srealloc(void *op, size_t size)
{
	void	*np;

	if ((np = realloc(op, size)) == NULL) {
		write(2, "no memory\n", 10);
		done(0);
	}
	return np;
}

static struct iblok *
openfile(int no, const char *fn, const char *ot, const char **path)
{
	struct stat	st;
	char	*pp;
	const char	*cp;
	struct iblok	*ip;

	if (fn[0] == '-' && fn[1] == '\0') {
		if (no == 0)
			fatal(2, "Illegal argument: ");
		return copyin(path);
	} else if (stat(fn, &st) == 0 && (st.st_mode&S_IFMT) == S_IFDIR) {
		for (cp = ot; *cp; cp++);
		while (cp > ot && cp[-1] == '/')
			cp--;
		while (cp > ot && cp[-1] != '/')
			cp--;
		ot = cp;
		*path = pp = smalloc(strlen(fn) + strlen(ot) + 1);
		for (cp = fn; *cp; cp++)
			*pp++ = *cp;
		if (pp > *path && pp[-1] != '/')
			*pp++ = '/';
		for (cp = ot; *cp; cp++)
			*pp++ = *cp;
		*pp = '\0';
	} else
		*path = fn;
	if ((ip = ib_open(*path, 0)) == NULL)
		fatal(1, "Cannot open: %s", *path);
	return ip;
}

static struct iblok *
copyin(const char **path)
{
	static char	templ[] = "/tmp/sdiffXXXXXX";
	char	buf[4096];
	struct iblok	*ip = NULL;
	int	fd;
	ssize_t	rd;

	if ((fd = mkstemp(templ)) < 0 || unlink(templ) < 0 ||
			(ip = ib_alloc(fd, 0)) == NULL)
		fatal(1, "Cannot open temp %s", templ);
	while ((rd = read(0, buf, sizeof buf)) > 0)
		write(fd, buf, rd);
	lseek(fd, 0, SEEK_SET);
	*path = "-";
	return ip;
}

static struct iblok *
diff(const char *f1, const char *f2, int d2)
{
	static char	templ[] = "/tmp/sdiffXXXXXX";
	struct iblok	*ip = NULL;
	pid_t	pid;
	int	fd;

	if ((fd = mkstemp(templ)) < 0 || unlink(templ) < 0 ||
			(ip = ib_alloc(fd, 0)) == NULL)
		fatal(1, "Cannot open temp %s", templ);
	switch (pid = fork()) {
	case -1:
		fatal(1, "Cannot fork");
		/*NOTREACHED*/
	case 0:
		if (f2[0] == '-' && f2[1] == '\0') {
			dup2(d2, 0);
			close(d2);
		}
		dup2(fd, 1);
		close(fd);
		execlp("diff", "diff", "-b", f1, f2, NULL);
		_exit(0177);
	}
	while (waitpid(pid, NULL, 0) != pid);
	lseek(fd, 0, SEEK_SET);
	return ip;
}

static void
sdiff(struct iblok *il, struct iblok *ir, struct iblok *id)
{
	long long	cur = 1;
	struct difference	di;

	while (dget(id, &di) == 0) {
		cur += cput(il, ir, &di, cur);
		cur += dput(il, ir, &di);
	}
	di.d_lbot = di.d_ltop = di.d_rbot = di.d_rtop = LLONG_MAX;
	cput(il, ir, &di, cur);
}

static int
dget(struct iblok *ip, struct difference *dp)
{
	static char	*line;
	static size_t size;
	char	*x;

	if (ib_getlin(ip, &line, &size, srealloc) == 0)
		return -1;
	dp->d_lbot = strtoll(line, &x, 10);
	if (*x == ',')
		dp->d_ltop = strtoll(&x[1], &x, 10);
	else
		dp->d_ltop = dp->d_lbot;
	switch (*x++) {
	case 'a':
		dp->d_type = D_A;
		break;
	case 'c':
		dp->d_type = D_C;
		break;
	case 'd':
		dp->d_type = D_D;
		break;
	default:
		fprintf(stderr, "%s: cmd not found%c\n", progname, x[-1]);
		return -1;
	}
	dp->d_rbot = strtoll(x, &x, 10);
	if (*x == ',')
		dp->d_rtop = strtoll(&x[1], &x, 10);
	else
		dp->d_rtop = dp->d_rbot;
	if (*x != '\n')
		return -1;
	if (sflag)
		printf("%s", line);
	return pass(ip, dp, &line, &size);
}

static int
pass(struct iblok *ip, struct difference *dp, char **lp, size_t *sp)
{
	long long	cnt = -1;

	switch (dp->d_type) {
	case D_A:
		cnt = dp->d_rtop - dp->d_rbot + 1;
		break;
	case D_C:
		cnt = (dp->d_ltop - dp->d_lbot + 1) +
			(dp->d_rtop - dp->d_rbot + 1) + 1;
		break;
	case D_D:
		cnt = dp->d_ltop - dp->d_lbot + 1;
		break;
	}
	if (cnt <= 0)
		return -1;
	while (cnt--)
		if (ib_getlin(ip, lp, sp, srealloc) == 0)
			return -1;
	return 0;
}

static long long
dput(struct iblok *il, struct iblok *ir, struct difference *dp)
{
	long long	cnt = 0, c, c2;

	switch (dp->d_type) {
	case D_A:
		c = dp->d_rtop - dp->d_rbot + 1;
		while (c--) {
			iput(NULL, 1, 0, ltemp);
			gput('>');
			iput(ir, 0, 0, rtemp);
			putchar('\n');
		}
		break;
	case D_C:
		c = cnt = dp->d_ltop - dp->d_lbot + 1;
		c2 = dp->d_rtop - dp->d_rbot + 1;
		while (c && c2) {
			c--, c2--;
			iput(il, 1, 0, ltemp);
			gput('|');
			iput(ir, 0, 0, rtemp);
			putchar('\n');
		}
		while (c2--) {
			iput(NULL, 1, 0, ltemp);
			gput('>');
			iput(ir, 0, 0, rtemp);
			putchar('\n');
		}
		while (c--) {
			iput(il, 1, 0, ltemp);
			gput('<');
			putchar('\n');
		}
		break;
	case D_D:
		c = cnt = dp->d_ltop - dp->d_lbot + 1;
		while (c--) {
			iput(il, 1, 0, ltemp);
			gput('<');
			putchar('\n');
		}
		break;
	}
	if (oflag)
		ask();
	return cnt;
}

static void
iput(struct iblok *ip, int fill, int discard, FILE *temp)
{
	static char	*line;
	static size_t	size;
	size_t	len;
	int	i = 0, j = 0;

	if (ip) {
		if ((len = ib_getlin(ip, &line, &size, srealloc)) == 0)
			done(0);
		if (temp)
			fwrite(line, sizeof *line, len, temp);
		if (line[len-1] == '\n')
			line[--len] = '\0';
		if (discard == 0) {
			for (i = 0, j = 0; j < width && i < len; i++) {
				if (line[i] == '\t') {
					while (j++ < width) {
						putchar(' ');
						if ((j & 07) == 0)
							break;
					}
				} else {
					if (mb_cur_max > 1) {
						wint_t	wc;
						int	n, m;
						next(wc, &line[i], n);
						if (wc != WEOF) {
							m = wcwidth(wc);
							if (j + m >= width)
								break;
							j += m - 1;
						}
						while (--n) {
							putchar(line[i]);
							i++;
						}
					}
					putchar(line[i]);
					j++;
				}
			}
		}
	}
	if (fill && discard == 0)
		while (j++ < width)
			putchar(' ');
}

static void
gput(int c)
{
	printf("  %c  ", c);
}

static long long
cput(struct iblok *il, struct iblok *ir,
		struct difference *dp, long long cur)
{
	long long	cnt, c;

	c = cnt = dp->d_lbot - cur + (dp->d_type == D_A);
	while (c-- > 0) {
		iput(il, lflag == 0, sflag, oflag);
		if (lflag || sflag)
			iput(ir, 0, 1, NULL);
		else {
			gput(' ');
			iput(ir, 0, 0, NULL);
		}
		if (sflag == 0)
			putchar('\n');
	}
	return cnt;
}

static void
done(int signo)
{
	if (ltemp)
		unlink(ltname);
	if (rtemp)
		unlink(rtname);
	if (ttname[0])
		unlink(ttname);
	exit(signo ? signo | 0200 : 0);
}

static void
ask(void)
{
	char	c;

	for (;;) {
		putchar('%');
		fflush(stdout);
		if (read(0, &c, 1) != 1)
			goto illre;
		switch (c) {
		default:
		illre:	fprintf(stderr, "Illegal command reenter\n");
			iflush(c);
			continue;
		case 'l':
			tcopy(ltemp, oflag);
			break;
		case 'r':
			tcopy(rtemp, oflag);
			break;
		case 's':
			sflag = 1;
			iflush(c);
			continue;
		case 'v':
			sflag = 0;
			iflush(c);
			continue;
		case 'e':
			while (read(0, &c, 1) == 1 && (c == ' ' || c == '\t'));
			switch (c) {
			case 'l':
			case '<':
				iflush(c);
				edit(ltname, ltemp, NULL);
				break;
			case 'r':
			case '>':
				iflush(c);
				edit(rtname, rtemp, NULL);
				break;
			case 'b':
			case '|':
				iflush(c);
				edit(NULL, ltemp, rtemp);
				break;
			case '\n':
				iflush(c);
				edit(NULL, NULL, NULL);
				break;
			default:
				goto illre;
			}
			break;
		case 'q':
			iflush(c);
			done(0);
		}
		break;
	}
	iflush(c);
	fflush(ltemp);
	rewind(ltemp);
	ftruncate(fileno(ltemp), 0);
	fflush(rtemp);
	rewind(rtemp);
	ftruncate(fileno(rtemp), 0);
}

static void
iflush(char c)
{
	while (c != '\n' && read(0, &c, 1) == 1);
}

static void
tcopy(FILE *temp, FILE *outf)
{
	char	b[4096];
	size_t	rd;

	fflush(temp);
	rewind(temp);
	while ((rd = fread(b, sizeof *b, sizeof b, temp)) != 0)
		fwrite(b, sizeof *b, rd, outf);
}

static void
edit(const char *name, FILE *file1, FILE *file2)
{
	FILE	*efile = NULL;
	const char	*efname;
	int	fd;
	pid_t	pid;

	if (file1 == NULL || file2) {
		strcpy(ttname, "/tmp/sdiffXXXXXX");
		if ((fd = mkstemp(ttname)) < 0 ||
				(efile = fdopen(fd, "r+")) == NULL)
			fatal(1, "Cannot open temp %s", ttname);
		if (file2) {
			tcopy(file1, efile);
			tcopy(file2, efile);
		}
		efname = ttname;
	} else {
		efile = file1;
		efname = name;
	}
	fflush(efile);
	switch (pid = fork()) {
	case -1:
		fatal(1, "Cannot fork");
		/*NOTREACHED*/
	case 0:
		execlp("ed", "ed", efname, NULL);
		_exit(0177);
	}
	while (waitpid(pid, NULL, 0) != pid);
	if (ttname[0]) {
		unlink(ttname);
		ttname[0] = '\0';
	}
	tcopy(efile, oflag);
}
