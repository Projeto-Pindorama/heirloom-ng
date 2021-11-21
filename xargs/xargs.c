/*
 * xargs - construct argument list(s) and execute command
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, March 2003.
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
static const char sccsid[] USED = "@(#)xargs.sl	1.15 (gritter) 6/21/05";

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/wait.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<libgen.h>
#include	<locale.h>
#include	<wchar.h>
#include	<wctype.h>
#include	<ctype.h>
#include	<errno.h>

#include	<iblok.h>
#include	<blank.h>
#include	<mbtowi.h>

#if defined (__GLIBC__) && defined(_IO_putc_unlocked)
#undef	putc
#define	putc(c, f)	_IO_putc_unlocked(c, f)
#endif

#define	peek(wc, s, n)	(mb_cur_max > 1 && *(s) & 0200 ? \
			((n) = mbtowi(&(wc), (s), mb_cur_max), \
		 	(n) = ((n) > 0 ? (n) : (n) < 0 ? (wc=WEOF, 1) : 1)) :\
		((wc) = *(s) & 0377, (n) = 1))

/*
 * Wording of SUSv2 and SUSv3 seems to require some static limits. This
 * implementation can go beyond, but if you want to have a bone-headed
 * standards-compliant utility, #define WEIRD_LIMITS.
 */
#undef	WEIRD_LIMITS

static const char	*progname;
static const char	*eflag = "_";
static const char	*iflag;
static size_t		iflen;
static long		lflag;
static long		nflag;
static int		pflag;
static const char	*sflag;
static int		tflag;
static int		xflag;
static int		errcnt;
static int		mb_cur_max;

static long		a_cnt;		/* count of arguments */
static long		a_agg;		/* aggregate commands */
static long		a_cur;		/* current position in aggregate */
static long		a_asz;		/* aggregate command length */
static long		a_csz;		/* aggregate current length */
static long		a_maxsize;	/* aggregate maximum length */
static const char	**a_vec;	/* arguments */
static char		*a_spc;		/* aggregate space */
static long		a_maxarg;	/* maximum arguments in e_vec */

static long		*replpos;	/* position of arguments with replstr */
static long		replsiz;	/* number of members in replpos */
static int		replcnt;	/* count of arguments with replstr */

static void *
srealloc(void *op, size_t size)
{
	void	*np;

	if ((np = realloc(op, size)) == NULL) {
		write(2, "no memory\n", 10);
		_exit(077);
	}
	return np;
}

static void *
smalloc(size_t size)
{
	return srealloc(NULL, size);
}

static const char *
contains(const char *haystack, const char *needle)
{
	size_t	ln = strlen(needle);
	wint_t	wc;
	int	n;

	while (*haystack) {
		if (*haystack == *needle && strncmp(haystack, needle, ln) == 0)
			return haystack;
		peek(wc, haystack, n);
		haystack += n;
	}
	return NULL;
}

static long
envsz(void)
{
	extern char	**environ;
	long	val = 0;
	int	i;

	for (i = 0; environ[i]; i++)
		val += strlen(environ[i]) + 1;
	return val;
}

static const char *
validate(const char *s, int optc)
{
	const char	*sp = s;
	wint_t	wc;
	int	n;

	if (mb_cur_max > 1) {
		while (*sp) {
			peek(wc, sp, n);
			if (wc == WEOF) {
				fprintf(stderr,
			"%s: illegal byte sequence in argument to -%c\n",
					progname, optc);
				exit(1);
			}
			sp += n;
		}
	}
	return s;
}

static void
overflow(void)
{
	if (iflag)
		fprintf(stderr,
			"%s: max arg size with insertion via {}'s exceeded\n",
			progname);
	else
		fprintf(stderr, "%s: arg list too long\n", progname);
	exit(1);
}

static void
unknown(const char *s)
{
	fprintf(stderr, "%s: unknown option: -%s\n", progname, s);
	exit(1);
}

static void
missing(int c)
{
	fprintf(stderr, "%s: missing argument: -%c\n", progname, c);
	exit(1);
}

static void
mustbepos(int c, const char *s)
{
	const char	*type = 0;

	switch (c) {
	case 'l':
	case 'L':
		type = "#lines";
		break;
	case 'n':
		type = "#args";
		break;
	}
	fprintf(stderr, "%s: %s must be positive int: -%c%s\n",
			progname, type, c, s);
	exit(1);
}

static long
number(int c, const char *s)
{
	char	*x;
	long	val;

	val = strtol(s, &x, 10);
	if (val <= 0)
		mustbepos(c, s);
	return val;
}

static int
flags(int ac, char **av)
{
	int	i;

	for (i = 1; i < ac && av[i][0] == '-'; i++) {
	nxt:	switch (av[i][1]) {
		case 'e':
			eflag = validate(&av[i][2], 'e');
			continue;
		case 'E':
			if (av[i][2])
				eflag = validate(&av[i][2], 'E');
			else if (++i < ac)
				eflag = validate(av[i], 'E');
			else
				missing('E');
			continue;
		case 'i':
			if (av[i][2])
				iflag = validate(&av[i][2], 'i');
			else
				iflag = "{}";
			continue;
		case 'I':
			if (av[i][2])
				iflag = validate(&av[i][2], 'i');
			else if (++i < ac)
				iflag = validate(av[i], 'i');
			else
				missing('I');
			continue;
		case 'l':
			if (av[i][2])
				lflag = number('l', &av[i][2]);
			else
				lflag = 1;
			xflag = 1;
			iflag = NULL;
			continue;
		case 'L':
			if (av[i][2])
				lflag = number('L', &av[i][2]);
			else if (++i < ac)
				lflag = number('L', av[i]);
			else
				mustbepos('L', "");
			xflag = 1;
			iflag = NULL;
			nflag = 0;
			continue;
		case 'n':
			if (av[i][2])
				nflag = number('n', &av[i][2]);
			else if (++i < ac)
				nflag = number('n', av[i]);
			else
				mustbepos('n', "");
			lflag = 0;
			continue;
		case 'p':
			pflag = open("/dev/tty", O_RDONLY);
			fcntl(pflag, F_SETFD, FD_CLOEXEC);
			tflag = 1;
			break;
		case 's':
			if (av[i][2])
				sflag = &av[i][2];
			else if (++i < ac)
				sflag = av[i];
			else
				sflag = "0";
			continue;
		case 't':
			tflag = 1;
			break;
		case 'x':
			xflag = 1;
			break;
		case '-':
			return ++i;
		default:
			unknown(&av[i][1]);
		}
		if (av[i][2]) {
			(av[i])++;
			goto nxt;
		}
	}
	return i;
}

static void
run(const char **args)
{
	pid_t	pid;
	int	status;

	if (tflag) {
		int	i;
		const char	*cp;

		for (i = 0; args[i]; i++) {
			if (i > 0)
				putc(' ', stderr);
			for (cp = args[i]; *cp; cp++)
				putc(*cp & 0377, stderr);
		}
		if (pflag) {
			char	ch, ch2;

			for (cp = " ?..."; *cp; cp++)
				putc(*cp, stderr);
			fflush(stderr);
			switch (read(pflag, &ch, 1)) {
			case -1:
				fprintf(stderr,
					"%s: can't read from tty for -p\n",
					progname);
				exit(1);
				/*NOTREACHED*/
			case 0:
				exit(errcnt);
			}
			if (ch != '\n')
				while (read(pflag, &ch2, 1) == 1 &&
						ch2 != '\n');
			if (ch != 'y')
				return;
		} else
			putc('\n', stderr);
	}
	switch (pid = fork()) {
	case -1:
		fprintf(stderr, "%s: cannot fork(): %s\n", progname,
				strerror(errno));
		exit(126);
		/*NOTREACHED*/
	case 0:
		execvp(args[0], (char **)args);
		_exit(errno == ENOENT ? 127 : 126);
	default:
		while (wait(&status) != pid);
		if (status && ((WIFEXITED(status)&&WEXITSTATUS(status)==255) ||
				WIFSIGNALED(status))) {
			fprintf(stderr, "%s: %s not executed or returned -1\n",
					progname, args[0]);
			exit(125);
		}
		if (status)
			errcnt |= WIFEXITED(status) ? WEXITSTATUS(status) : 1;
	}
}

static void
addcmd(const char *s)
{
	if (a_cnt >= a_maxarg - 4096) {
		a_maxarg += 8192;
		a_vec = srealloc(a_vec, a_maxarg * sizeof *a_vec);
	}
	if (iflag && contains(s, iflag)) {
		if (replcnt >= replsiz) {
#ifdef	WEIRD_LIMITS
			if (replcnt >= 5) {
				fprintf(stderr, "%s: too many args with %s\n",
					progname, iflag);
				exit(1);
#endif	/* WEIRD_LIMITS */
			replpos = srealloc(replpos,
					(replsiz += 5) * sizeof *replpos);
		}
		replpos[replcnt++] = a_cnt;
	}
	a_vec[a_cnt++] = s;
	a_asz += strlen(s) + 1;
}

static void
endcmd(void)
{
	a_agg = a_cnt;
	a_maxsize = sysconf(_SC_ARG_MAX) - envsz() - 2048 - a_asz;
	if (nflag || sflag) {
		long	newsize = sflag ? atol(sflag) :
#ifdef	WEIRD_LIMITS
			LINE_MAX;
#else	/* !WEIRD_LIMITS */
			a_maxsize;
#endif	/* !WEIRD_LIMITS */
		if (sflag && (newsize <= a_asz || newsize > a_maxsize))
			fprintf(stderr,
				"%s: 0 < max-cmd-line-size <= %ld: -s%s\n",
				progname, a_maxsize, sflag);
		if (newsize < a_maxsize)
			a_maxsize = newsize;
	}
	if (sflag)
		a_spc = smalloc(a_maxsize);
	else {
		while ((a_spc = malloc(a_maxsize)) == NULL) {
			a_maxsize /= 2;
			if (a_maxsize < LINE_MAX) {
				a_spc = smalloc(a_maxsize);
				break;
			}
		}
	}
	a_cur = a_agg;
	if (nflag && nflag < a_maxarg + a_cnt) {
		nflag += a_cnt;
		a_maxarg = nflag;
	}
}

static void
addarg(const char *s, int always)
{
	size_t	sz;
	int	toolong = 0;
	static int	didexec;

	do {
		if (s) {
			sz = strlen(s) + 1;
			if (a_csz == 0 && sz > a_maxsize) {
				fprintf(stderr,
		"%s: a single arg was greater than the max arglist size\n",
					progname);
				exit(1);
			}
			if (a_csz + sz <= a_maxsize && a_cur < a_maxarg) {
				strcpy((char *)(a_vec[a_cur++]=&a_spc[a_csz]),
						s);
				a_csz += sz;
				if (always)
					goto doit;
				return;
			} else {
				if (a_agg == a_cur)
					a_vec[a_cur++] = s;
				else
					toolong = 1;
			doit:	if (nflag && xflag && a_cnt > nflag)
					overflow();
				if (lflag && xflag && !always)
					overflow();
				always = 0;
			}
		} else {
			if ((didexec || lflag) && a_agg == a_cur)
				return;
		}
		didexec = 1;
		a_vec[a_cur] = NULL;
		a_cur = a_agg;
		a_csz = 0;
		run(a_vec);
	} while (toolong != 0);
}

static char *
replace(const char *arg, const char *s)
{
	wint_t	wc;
	int	n;
	size_t	sz = strlen(s);
	char	*bot = &a_spc[a_csz];

	while (*arg != '\0') {
		if (*arg == *iflag && strncmp(arg, iflag, iflen) == 0) {
			if (a_csz + sz > a_maxsize)
				overflow();
			strcpy(&a_spc[a_csz], s);
			a_csz += sz;
			arg += iflen;
		} else {
			peek(wc, arg, n);
			if (a_csz + n > a_maxsize)
				overflow();
			while (n--)
				a_spc[a_csz++] = *arg++;
		}
	}
	if (a_csz + 1 > a_maxsize
#ifdef	WEIRD_LIMITS
			|| &a_spc[a_csz] - bot > 255
#endif	/* WEIRD_LIMITS */
			)
		overflow();
	a_spc[a_csz++] = '\0';
	return bot;
}

static void
insert(const char *s)
{
	const char	**args;
	int	i, j;

	args = smalloc((a_cnt + 1) * sizeof *args);
	a_csz = 0;
	for (i = 0, j = 0; i < a_cnt; i++) {
		if (j < replcnt && replpos[j] == i) {
			j++;
			args[i] = replace(a_vec[i], s);
		} else {
			a_csz += strlen(a_vec[i]) + 1;
			if (a_csz > a_maxsize)
				overflow();
			args[i] = a_vec[i];
		}
	}
	args[i] = NULL;
	run(args);
	free(args);
}

#define	nextc()		(cp = (mb_cur_max > 1 ? ib_getw(ip, &c, &n) : \
		(c = ib_get(ip)) == (wint_t)EOF ? NULL : (b = c, n = 1, &b)))
			
#define	blankc(c)	(mb_cur_max > 1 ? iswblank(c):isblank(c))

static char *
nextarg(struct iblok *ip, long *linecnt)
{
	static char	*buf;
	static size_t	size;
	static int	eof;
	wint_t	c, quote = WEOF, lastc = WEOF;
	char	*cp;
	char	b;
	int	content = 0, i = 0, n;

	if (eof)
		return NULL;
	if (buf == NULL)
		buf = smalloc(size = 128);
	for (;;) {
		nextc();
		if (cp) {
			if (c != WEOF && c == quote) {
				quote = WEOF;
				continue;
			}
			lastc = c;
			if (quote == WEOF && c == '\\') {
				content = 1;
				nextc();
			} else {
				if (quote == WEOF && (c == '"' || c == '\'')) {
					quote = c;
					content = 1;
					continue;
				}
				if (c == '\n' || quote == WEOF &&
						blankc(c) &&
						(iflag == NULL ||
						 content == 0)) {
					if (content) {
						if (c == '\n' && !blankc(lastc))
							(*linecnt)++;
						break;
					} else
						continue;
				}
			}
		}
		if (cp == NULL)
			break;
		content = 1;
		while (n--) {
			buf[i] = *cp++;
			if (++i >= size)
				buf = srealloc(buf, size += 128);
		}
	}
	buf[i] = '\0';
	if (quote != WEOF) {
		fprintf(stderr, "%s: missing quote?: %s\n", progname, buf);
		exit(1);
	}
	if (cp == NULL)
		eof = 1;
	return content ? buf : NULL;
}

static void
process(void)
{
	struct iblok	*ip;
	char	*arg;
	long	linecnt = 0;

	ip = ib_alloc(0, 0);
	if (iflag) {
		iflen = strlen(iflag);
		while ((arg = nextarg(ip, &linecnt)) != NULL) {
			if (eflag && *eflag && strcmp(arg, eflag) == 0)
				break;
			insert(arg);
		}
	} else {
		do {
			arg = nextarg(ip, &linecnt);
			if (arg && eflag && *eflag && strcmp(arg, eflag) == 0)
				arg = NULL;
			addarg(arg, lflag && lflag == linecnt ?
					linecnt = 0, 1 : 0);
		} while (arg != NULL);
	}
}

int
main(int argc, char **argv)
{
	static char	errbuf[256];
	int	i;

	setvbuf(stderr, errbuf, _IOLBF, sizeof errbuf);
	progname = basename(argv[0]);
	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	i = flags(argc, argv);
	argc -= i, argv += i;
	if (argc > 0)
		for (i = 0; i < argc; i++)
			addcmd(argv[i]);
	else
		addcmd("echo");
	endcmd();
	process();
	return errcnt;
}
