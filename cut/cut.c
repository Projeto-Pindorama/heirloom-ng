/*
 * cut - cut out fields of lines of files
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, December 2002.
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
static const char sccsid[] USED = "@(#)cut.sl	1.20 (gritter) 5/29/05";

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>
#include	<limits.h>
#include	<wchar.h>
#include	<ctype.h>
#include	<locale.h>

#include	"iblok.h"

#if defined (__GLIBC__) && defined (_IO_putc_unlocked)
#undef	putc
#define	putc(c, f)	_IO_putc_unlocked(c, f)
#endif

struct	range {
	struct range	*r_nxt;
	long	r_min;
	long	r_max;
};

static unsigned	errcnt;			/* count of errors */
static int	method;			/* one of b, c, f */
static int	nflag;			/* character boundary bytes */
static int	sflag;			/* suppress lines w/o delimiters */
static char	*progname;		/* argv[0] to main() */
static wchar_t	wcdelim = '\t';		/* delimiter character */
static const char	*mbdelim = "\t";/* delimiter character */
struct range	*fields;		/* range list */
static int	multibyte;		/* multibyte LC_CTYPE */

#define	next(wc, s)	(multibyte ? mbtowc(&(wc), s, MB_LEN_MAX) :\
				((wc) = *(s) & 0377, (wc) != 0))

void *
lrealloc(void *vp, size_t nbytes)
{
	void	*p;

	if ((p = realloc(vp, nbytes)) == NULL) {
		write(2, "line too long\n", 14);
		exit(076);
	}
	return p;
}

void *
smalloc(size_t nbytes)
{
	void	*p;

	if ((p = malloc(nbytes)) == NULL) {
		write(2, "no memory\n", 11);
		exit(077);
	}
	return p;
}

static void
error(const char *s)
{
	fprintf(stderr, "%s: ERROR: %s\n", progname, s);
	exit(2);
}

static void
usage(void)
{
	error("Usage: cut [-s] [-d<char>] {-c<list> | -f<list>} file ...");
}

static void
badlist(void)
{
	error(method == 'b' ? "bad list for b/c/f option" : 
			"bad list for c/f option");
}

static void
setdelim(const char *s)
{
	int	n;

	if ((n = next(wcdelim, s)) < 0 || (n > 0 && s[n] != '\0'))
		error("no delimiter");
	mbdelim = s;
}

static void
addrange(long m, long n)
{
	struct range	*rp, *rq;

	rp = smalloc(sizeof *rp);
	rp->r_nxt = NULL;
	rp->r_min = m;
	rp->r_max = n ? n : m;
	if (fields) {
		for (rq = fields; rq->r_nxt; rq = rq->r_nxt);
		rq->r_nxt = rp;
	} else
		fields = rp;
}

static int
have(long i)
{
	struct range	*rp;

	for (rp = fields; rp; rp = rp->r_nxt)
		if (i >= rp->r_min && i <= rp->r_max)
			return 1;
	return 0;
}

#define	mnreset()	m = 0, n = 0, lp = &m

static void
setlist(const char *s)
{
	char	*cbuf, *cp;
	long	m, n;
	long	*lp;

	fields = NULL;
	cbuf = smalloc(strlen(s) + 1);
	mnreset();
	for (;;) {
		if (*s == '-') {
			if (m == 0)
				m = 1;
			n = LONG_MAX;
			lp = &n;
			s++;
		} else if (*s == ',' || *s == ' ' || *s == '\t' || *s == '\0') {
			if (m)
				addrange(m, n);
			mnreset();
			if (*s == '\0')
				break;
			s++;
		} else if (isdigit(*s & 0377)) {
			cp = cbuf;
			do
				*cp++ = *s++;
			while (isdigit(*s & 0377));
			*cp = '\0';
			*lp = strtol(cbuf, NULL, 10);
		} else
			badlist();
	}
	if (fields == NULL)
		error("no fields");
	free(cbuf);
}

static void
cutb(struct iblok *ip)
{
	int	c, i;

	i = 1;
	while ((c = ib_get(ip)) != EOF) {
		if (c == '\n') {
			i = 1;
			putc(c, stdout);
		} else if (have(i++))
			putc(c, stdout);
	}
}

static void
cutbn(struct iblok *ip)
{
	char	*cp;
	int	i, m, n;
	wint_t	wc;

	i = 1;
	while ((cp = ib_getw(ip, &wc, &n)) != NULL) {
		if (wc == '\n') {
			i = 1;
			putc('\n', stdout);
		} else {
			if (have(i + n - 1))
				for (m = 0; m < n; m++)
					putc(cp[m], stdout);
			i += n;
		}
	}
}

static void
cutc(struct iblok *ip)
{
	char	*cp;
	int	i, n, m;
	wint_t	wc;

	i = 1;
	while ((cp = ib_getw(ip, &wc, &n)) != NULL) {
		if (wc == '\n') {
			i = 1;
			putc('\n', stdout);
		} else if (wc != WEOF && have(i++)) {
			for (m = 0; m < n; m++)
				putc(cp[m], stdout);
		}
	}
}

static void
cutf(struct iblok *ip)
{
	static char	*line;
	static size_t	linesize;
	char	*cp, *lp, *lq;
	int	c, i, n, m, gotcha;
	char	b;
	wint_t	wc;
	const int	incr = 128;

	if (linesize == 0)
		line = smalloc(linesize = incr);
	lp = line;
	gotcha = 0;
	i = 1;
	do {
		if (multibyte)
			cp = ib_getw(ip, &wc, &n);
		else {
			if ((c = ib_get(ip)) != EOF) {
				wc = c;
				b = (char)c;
				cp = &b;
			} else {
				wc = WEOF;
				cp = NULL;
			}
			n = 1;
		}
		if (cp == NULL || wc == '\n' || wc == wcdelim) {
			if (have(i) && (!sflag || gotcha || wc == wcdelim) ||
					(!sflag && i == 1 &&
						(cp == NULL || wc == '\n'))) {
				if (gotcha)
					for (m = 0; mbdelim[m]; m++)
						putc(mbdelim[m], stdout);
				for (lq = line; lq < lp; lq++)
					putc(*lq, stdout);
				gotcha = 1;
			}
			if (wc == '\n') {
				if (gotcha)
					putc('\n', stdout);
				i = 1;
				gotcha = 0;
			} else
				i++;
			lp = line;
		} else {
			for (m = 0; m < n; m++) {
				if (lp >= &line[linesize]) {
					size_t	diff = lp - line;
					line = lrealloc(line, linesize += incr);
					lp = &line[diff];
				}
				*lp++ = cp[m];
			}
		}
	} while (cp != NULL);
}

static int
fdcut(int fd)
{
	struct iblok	*ip;

	ip = ib_alloc(fd, 0);
	switch (method) {
	case 'b':
		if (nflag && multibyte)
			cutbn(ip);
		else
			cutb(ip);
		break;
	case 'c':
		if (multibyte)
			cutc(ip);
		else
			cutb(ip);
		break;
	case 'f':
		cutf(ip);
		break;
	}
	ib_free(ip);
	return 0;
}

static int
fncut(const char *fn)
{
	int	fd, res;

	if (fn[0] == '-' && fn[1] == '\0')
		fd = 0;
	else if ((fd = open(fn, O_RDONLY)) < 0) {
		fprintf(stderr, "%s: WARNING: cannot open %s\n", progname, fn);
		return 1;
	}
	res = fdcut(fd);
	if (fd)
		close(fd);
	return res;
}

int
main(int argc, char **argv)
{
	const char	optstring[] = "b:c:d:f:ns";
	int	i;

	progname = basename(argv[0]);
	setlocale(LC_CTYPE, "");
	multibyte = MB_CUR_MAX > 1;
#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	while ((i = getopt(argc, argv, optstring)) != EOF) {
		switch (i) {
		case 'b':
		case 'c':
		case 'f':
			if (method && method != i)
				usage();
			method = i;
			setlist(optarg);
			break;
		case 'd':
			setdelim(optarg);
			break;
		case 'n':
			nflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		default:
			usage();
		}
	}
	/*if ((sflag && method != 'f') || (nflag && method != 'b'))
		usage();*/
	if (method == 0)
		badlist();
	if (argv[optind]) {
		for (i = optind; argv[i]; i++)
			errcnt |= fncut(argv[i]);
	} else
		errcnt |= fdcut(0);
	return errcnt;
}
