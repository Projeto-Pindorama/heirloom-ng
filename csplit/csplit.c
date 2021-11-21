/*
 * csplit - context split
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, May 2003.
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
#if defined (SU3)
static const char sccsid[] USED = "@(#)csplit_su3.sl	1.10 (gritter) 5/29/05";
#elif defined (SUS)
static const char sccsid[] USED = "@(#)csplit_sus.sl	1.10 (gritter) 5/29/05";
#else
static const char sccsid[] USED = "@(#)csplit.sl	1.10 (gritter) 5/29/05";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <stdarg.h>
#include <math.h>
#include <locale.h>
#include <signal.h>
#include "sigset.h"
#include "atoll.h"

#if defined (__GLIBC__) 
#if defined (_IO_getc_unlocked)
#undef	getc
#define	getc(f)		_IO_getc_unlocked(f)
#endif
#if defined (_IO_putc_unlocked)
#undef	putc
#define	putc(c, f)	_IO_putc_unlocked(c, f)
#endif
#endif

#if defined (SUS) || defined (SU3)
#include <regex.h>
#else
#include <regexpr.h>
#endif

#include <iblok.h>

static struct	arg {
	long long	a_no;
	long long	a_nx;
	long long	a_ro;
	long long	a_once;
	const char	*a_op;
	const char	*a_rp;
#if defined (SUS) || defined (SU3)
	regex_t		*a_re;
#else
	char		*a_re;
#endif
} **args;

static const char	*progname;
static const char	*prefix = "xx";
static const char	*curarg;
static int		filec;
static int		suffixlength = 2;
static int		status;
static int		kflag;
static int		sflag;

static void	usage(void);
static void	msg(int, const char *, ...);
static void	scan(char *);
static void	csplit(const char *);
static int 	match(const char *, long long, long long *, int *);
static FILE	*nextfile(void);
static const char	*makename(int, int);
static void	delfiles(void);
static void	onint(int);
static struct iblok	*copytemp(struct iblok *);
static void	*smalloc(size_t);
static void	*srealloc(void *, size_t);
static void	*scalloc(size_t, size_t);

int
main(int argc, char **argv)
{
	int	i, illegal = 0;
	char	*fn;

	progname = basename(argv[0]);
#if defined (SUS) || defined (SU3)
	setlocale(LC_COLLATE, "");
#endif
	setlocale(LC_CTYPE, "");
	while ((i = getopt(argc, argv, "ksf:n:")) != EOF) {
		switch (i) {
		case 'k':
			kflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 'f':
			prefix = optarg;
			break;
		case 'n':
			suffixlength = atoi(optarg);
			break;
		default:
			illegal++;
		}
	}
	if (illegal || optind + 2 > argc)
		usage();
	fn = argv[optind++];
	args = scalloc(argc - optind + 1, sizeof *args);
	for (i = 0; i + optind < argc; i++)
		scan(argv[i+optind]);
	if (sigset(SIGINT, SIG_IGN) != SIG_IGN)
		sigset(SIGINT, onint);
	csplit(fn);
	return status;
}

static void
usage(void)
{
	fprintf(stderr, "%s: Usage: %s [-s] [-k] [-f prefix] file args ...\n",
		progname, progname);
	exit(2);
}

static void
msg(int term, const char *fmt, ...)
{
	va_list	ap;

	fprintf(stderr, "%s: ", progname);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	putc('\n', stderr);
	delfiles();
	if (term)
		exit(1);
	else
		status |= 1;
}

static void
scan(char *s)
{
	static int	ncur = -1;
	long long	c;
	char	*sp, *x;
#if defined (SUS) || defined (SU3)
	int	reflags;
#endif

	if (*s != '{') {
		args[++ncur] = scalloc(1, sizeof *args[ncur]);
		args[ncur]->a_op = s;
	}
	switch (*s) {
	case '/':
	case '%':
		for (sp = &s[1]; *sp; sp++) {
			if (sp[0] == '\\' && sp[1])
				sp++;
			else if (*sp == *s)
				break;
		}
		if (*sp == '\0')
			msg(1, "%s: missing delimiter", s);
		*sp = '\0';
		args[ncur]->a_nx = strtoll(&sp[1], &x, 10);
		if (*x != '\0')
			msg(1, "%s: illegal offset", s);
#if defined (SUS) || defined (SU3)
		args[ncur]->a_re = smalloc(sizeof *args[ncur]->a_re);
		reflags = REG_ANGLES|REG_NOSUB;
#if defined (SU3)
		reflags |= REG_AVOIDNULL;
#endif	/* SU3 */
		if (regcomp(args[ncur]->a_re, &s[1], reflags) != 0)
#else	/* !SUS, !SU3 */
		if ((args[ncur]->a_re = compile(&s[1], 0, 0)) == 0)
#endif	/* !SUS, !SU3 */
			msg(1, "%s: Illegal Regular Expression", s);
		break;
	default:
		args[ncur]->a_nx = args[ncur]->a_no = strtoll(s, &x, 10);
		if (*x != '\0')
			msg(1, "%s: bad line number", s);
		break;
	case '{':
		c = strtoll(&s[1], &x, 10);
		if (x > &s[1] && x[0] != '}')
			msg(1, "%s: missing '}'", s);
		if (s[1] == '-' || x[0] != '}')
			msg(1, "Illegal repeat count: %s", s);
		if (ncur < 0 || args[ncur] == 0 || args[ncur]->a_rp)
			msg(1, "No operation for %s", s);
		args[ncur]->a_ro = c;
		args[ncur]->a_rp = s;
	}
}

static void
csplit(const char *fn)
{
	struct iblok	*ip;
	int	skip = 0, osk = 0, gotcha;
	FILE	*op = NULL;
	char	*line = 0, c = 0;
	size_t	linesize = 0, linelen;
	long long	lineno = 0, oln = 1, xln = 1,
			bytes = 0, noffs = 0, on, brks = 0;

	if ((ip = fn[0] == '-' && fn[1] == '\0' ? ib_alloc(0, 0) :
			ib_open(fn, 0)) == NULL)
		msg(1, "Cannot open %s", fn);
	if (ib_seek(ip, 0, SEEK_CUR) != 0)
		ip = copytemp(ip);
	do {
		if ((linelen=ib_getlin(ip, &line, &linesize, srealloc)) != 0) {
			lineno++;
			if ((c = line[linelen-1]) == '\n')
				line[linelen-1] = '\0';
		} else
			lineno = -1;
		while (on = noffs, osk = skip,
		    (gotcha=match(line, lineno, &noffs, &skip)) != 0) {
			if (on) {
				if (osk == 0)
					op = nextfile();
				ib_seek(ip, brks, SEEK_SET);
				on = xln - on + (lineno <= 0);
				lineno = oln - 1;
				if (lineno > on)
					msg(1, "%s - out of range", curarg);
				while ((linelen = ib_getlin(ip, &line,
							&linesize,
							srealloc)) != 0 &&
						lineno++ < on) {
					if (osk == 0) {
						fwrite(line, sizeof *line,
								linelen, op);
						bytes += linelen;
					}
				}
				if (linelen != 0 && (c=line[linelen-1]) == '\n')
					line[linelen-1] = '\0';
			}
			if (lineno==1 && op==NULL && *args[0]->a_op != '%' &&
					(args[0]->a_re==0 || args[0]->a_nx>=0))
				op = nextfile();
			if (op) {
				if (!sflag)
					printf("%lld\n", bytes);
				bytes = 0;
				fclose(op);
			}
			if (lineno <= 0)
				break;
			if (!skip) {
				if (noffs) {
					op = NULL;
					brks = ib_seek(ip, 0, SEEK_CUR) -
						linelen;
					oln = lineno;
				} else
					op = nextfile();
			} else
				op = NULL;
			if (gotcha < 2)
				break;
		}
		if (!skip) {
			if (lineno == 1 && op == NULL && noffs == 0)
				op = nextfile();
			if (op && linelen != 0) {
				if (c == '\n')
					line[linelen-1] = '\n';
				fwrite(line, sizeof *line, linelen, op);
				bytes += linelen;
			}
		}
		xln = lineno;
	} while (linelen != 0);
}

static int
match(const char *line, long long lineno, long long *noffp, int *skip)
{
	static int	ncur = -1;

	if (ncur == -1) {
		ncur = 0;
		curarg = args[ncur]->a_op;
		if (args[ncur]->a_re)
			*skip = *args[ncur]->a_op == '%';
		if (args[ncur]->a_re && args[ncur]->a_nx < 0)
			*noffp = -args[ncur]->a_nx;
		else
			*noffp = 0;
	}
	if (args[ncur] == NULL)
		return lineno <= 0;
	if (args[ncur]->a_once == lineno)
		return 0;
	args[ncur]->a_once = 0;
	*skip = *args[ncur]->a_op == '%';
	if (args[ncur]->a_re != NULL && lineno > 0) {
		if (args[ncur]->a_no) {
			if (--args[ncur]->a_no > 0)
				return 0;
		} else {
#if defined (SUS) || defined (SU3)
			if (regexec(args[ncur]->a_re, line, 0, NULL, 0) != 0)
#else
			if (step(line, args[ncur]->a_re) == 0)
#endif
				return 0;
			if (args[ncur]->a_nx > 0) {
				args[ncur]->a_no = args[ncur]->a_nx;
				return 0;
			}
			if (args[ncur]->a_nx < 0 &&
					args[ncur]->a_nx+lineno < 0)
				msg(1, "%s - out of range", curarg);
		}
	} else {
		if (lineno > 0 && lineno < args[ncur]->a_no)
			return 0;
		else if (lineno < 0 && args[ncur]->a_nx < 0)
			/*EMPTY*/;
		else if (lineno < 0 || lineno > args[ncur]->a_no)
			msg(kflag==0||lineno>0, "%s - out of range", curarg);
	}
	if (args[ncur]->a_re && args[ncur]->a_nx >= 0)
		args[ncur]->a_once = lineno;
	if (args[ncur]->a_ro) {
		args[ncur]->a_ro--;
		curarg = args[ncur]->a_rp;
		if (args[ncur]->a_re == 0)
			args[ncur]->a_no += args[ncur]->a_nx;
	} else {
		if (args[++ncur]) {
			curarg = args[ncur]->a_op;
			if (args[ncur]->a_re)
				*skip = *args[ncur]->a_op == '%';
			else
				*skip = 0;
			if (args[ncur]->a_re && args[ncur]->a_nx < 0)
				*noffp = -args[ncur]->a_nx;
			else
				*noffp = 0;
		} else {
			*noffp = 0;
			*skip = 0;
		}
	}
	return 1 + ((lineno==1&&ncur==0) || args[ncur]==0 ||
			args[ncur]->a_re==0 || args[ncur]->a_nx < 0);
}

static FILE *
nextfile(void)
{
	const char	*name;
	FILE	*fp;

	name = makename(filec++, 1);
	if ((fp = fopen(name, "w")) == NULL)
		msg(1, "Cannot create %s", name);
	return fp;
}

static const char *
makename(int n, int prnt)
{
	static char		*name, *sufp;
	int	c;
	const char	*cp;
	char	*sp;

	if (name == 0) {
		name = smalloc(strlen(prefix)+suffixlength+1);
		sufp = name;
		for (cp = prefix; *cp; cp++)
			*sufp++ = *cp;
	}
	c = n;
	sp = &sufp[suffixlength];
	*sp = '\0';
	while (--sp >= sufp) {
		*sp = (c % 10) + '0';
		c /= 10;
	}
	if (c) {
		if (prnt)
			msg(1, "%0.0f file limit reached at arg %s",
				pow(10, suffixlength), curarg);
		return 0;
	}
	return name;
}

static void
delfiles(void)
{
	const char	*np;
	int	i;

	if (!kflag)
		for (i = 0; i < filec && (np = makename(i, 0)); i++)
			unlink(np);
}

static struct iblok *
copytemp(struct iblok *ip)
{
	char	tfname[] = "/tmp/csplitXXXXXX";
	char	buf[4096];
	struct iblok	*op = NULL;
	int	fd;
	ssize_t	rd;

	if ((fd = mkstemp(tfname)) < 0 || unlink(tfname) < 0)
		goto err;
	while ((rd = read(ip->ib_fd, buf, sizeof buf)) > 0)
		if (write(fd, buf, rd) != rd)
			goto err;
	if (ip->ib_fd)
		ib_close(ip);
	else
		ib_free(ip);
	if (lseek(fd, 0, SEEK_SET) != 0 || (op = ib_alloc(fd, 0)) == NULL)
	err:	msg(1, "Bad write to temporary file");
	return op;
}

/*ARGSUSED*/
static void
onint(int signo)
{
	msg(1, "Interrupt - program aborted at arg '%s'", curarg);
}

static void *
smalloc(size_t size)
{
	return srealloc(0, size);
}

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
scalloc(size_t nmemb, size_t size)
{
	void	*vp;

	if ((vp = calloc(nmemb, size)) == NULL) {
		write(2, "no memory\n", 10);
		_exit(077);
	}
	return vp;
}
