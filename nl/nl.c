/*
 * nl - line numbering filter
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, January 2003.
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
#if defined (S42)
static const char sccsid[] USED = "@(#)nl_s42.sl	1.18 (gritter) 5/29/05";
#elif defined (SU3)
static const char sccsid[] USED = "@(#)nl_su3.sl	1.18 (gritter) 5/29/05";
#elif defined (SUS)
static const char sccsid[] USED = "@(#)nl_sus.sl	1.18 (gritter) 5/29/05";
#else
static const char sccsid[] USED = "@(#)nl.sl	1.18 (gritter) 5/29/05";
#endif

#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<wchar.h>
#include	<locale.h>

#if defined (SUS) || defined (SU3) || defined (S42)
#include	<regex.h>
#else
#include	<regexpr.h>
#endif
#include	<iblok.h>
#include	<mbtowi.h>

struct	type {
	const char	*t_str;
	int	t_chr;
	int	t_type;
	int	t_def;
#if defined (SUS) || defined (SU3) || defined (S42)
	regex_t	t_re;
#else
	char	*t_ep;
#endif
};

static struct type	btype = { "BODY", 'b', 't', 't' };
static struct type	ftype = { "FOOTER", 'f', 'n', 'n' };
static struct type	htype = { "HEADER", 'h', 'n', 'n' };

static wint_t		delim[2] = { '\\', ':' };	/* section delimiter */

static long		incr = 1;		/* line number increment */

static long		whichblank = 1;		/* blank line maximum */
static long		blanks;			/* blank line count */

static enum {
	FMT_LN,
	FMT_RN,
	FMT_RZ
} format = FMT_RN;

static int		pflag;			/* don't restart at pages */

static const char	*sep = "\t";		/* number/text separator */

static long		startnum = 1;		/* initial number */
static long		width = 6;		/* line number width */

static struct iblok	*input;			/* input file */

static unsigned long long	number;		/* line number */

static int		mb_cur_max;		/* MB_CUR_MAX acceleration */

/*
 * Get next character from string s and store it in wc; n is set to
 * the length of the corresponding byte sequence.
 */
#define	next(wc, s, n) (mb_cur_max > 1 && *(s) & 0200 ? \
			((n) = mbtowi(&(wc), (s), mb_cur_max), \
			 (n) = ((n) > 0 ? (n) : (n) < 0 ? (wc=WEOF, 1) : 1)) :\
		((wc) = *(s) & 0377, (n) = 1))

static void *
srealloc(void *vp, size_t nbytes)
{
	void	*p;

	if ((p = realloc(vp, nbytes)) == NULL) {
		write(2, "input line too long\n", 20);
		exit(077);
	}
	return p;
}

static void
invalid(int c, const char *s)
{
	fprintf(stderr, "INVALID OPTION (-%c%s) - PROCESSING TERMINATED\n",
			c, s);
	exit(2);
}

#if defined (SUS) || defined (SU3) || defined (S42)
static int
regemap(int i)
{
	switch (i) {
	case REG_ESUBREG:
		return 25;
	case REG_EBRACK:
		return 49;
	case REG_EPAREN:
		return 42;
	case REG_BADBR:
	case REG_EBRACE:
		return 45;
	case REG_ERANGE:
		return 11;
	case REG_ESPACE:
#ifdef	REG_NOPAT
	case REG_NOPAT:
#endif	/* REG_NOPAT */
		return 50;
#ifdef	REG_EESCAPE
	case REG_EESCAPE:
		return 36;
#endif	/* REG_EESCAPE */
#ifdef	REG_ILLSEQ
	case REG_ILLSEQ:
		return 67;
#endif	/* REG_ILLSEQ */
	default:
		return i + 100;
	}
}
#endif	/* SUS || SU3 || S42 */

static void
settype(struct type *tp, const char *s)
{
	int	i;
#if defined (SUS) || defined (SU3) || defined (S42)
	int	reflags;
#endif

	switch (s[0]) {
	case 'p':
#if defined (SUS) || defined (SU3) || defined (S42)
		reflags = REG_ANGLES;
#if defined (SU3)
		reflags |= REG_AVOIDNULL;
#endif	/* SU3 */
		if ((i=regcomp(&tp->t_re, &s[1], reflags))!=0) {
			i = regemap(i);
#else	/* !SUS, !SU3, !S42 */
		if ((tp->t_ep = compile(&s[1], 0, 0)) == 0) {
			i = regerrno;
#endif	/* !SUS, !SU3, !S42 */
			fprintf(stderr, "%d This is the error code\n", i);
			fprintf(stderr, "Illegal Regular Expression\n");
			exit(2);
		}
		/*FALLTHRU*/
	case 'a':
	case 't':
	case 'n':
		tp->t_type = s[0];
		break;
	case '\0':
		tp->t_type = tp->t_def;
		break;
	default:
		fprintf(stderr, "%s: ", tp->t_str);
		invalid(tp->t_chr, s);
	}
}

static void
setdelim(int c, const char *s)
{
	const char	*os = s;
	wint_t	wc;
	int	n;

	if (next(wc, s, n), wc != '\0') {
		if (wc == WEOF)
			invalid(c, os);
		delim[0] = wc;
		s += n;
		if (next(wc, s, n), wc != '\0') {
			if (wc == WEOF)
				invalid(c, os);
			delim[1] = wc;
			s += n;
			if (next(wc, s, n), wc != '\0')
				invalid(c, os);
		}
	}
}

static void
setnum(int c, const char *s, long *lp, long def)
{
	char	*x;

	if (*s != '\0') {
		if (*s == '+' || *s == '-')
			invalid(c, s);
		*lp = strtol(s, &x, 10);
		if (*lp < 0 || *x != '\0')
			invalid(c, s);
	} else
		*lp = def;
}

static void
setformat(int c, const char *s)
{
	if (s[0] == 'r' && s[1] == 'z' && s[2] == '\0')
		format = FMT_RZ;
	else if (s[0] == 'l' && s[1] == 'n' && s[2] == '\0')
		format = FMT_LN;
	else if (s[0] == '\0' || (s[0] == 'r' && s[1] == 'n' && s[2] == '\0'))
		format = FMT_RN;
	else
		invalid(c, s);
}

static void
setsep(int c, const char *s)
{
	sep = s;
}

#if defined (SUS) || defined (SU3)
#define	arg()	(av[0][2] ? &av[0][2] : (ac-- > 0 ? (av++, (char *)av[0]) : \
					(invalid(av[0][1], ""), (char *)NULL)))
#else	/* !SUS, !SU3 */
#define	arg()	&av[0][2]
#endif

static void
scan(int ac, char **av)
{
	while (ac-- > 0) {
		if (av[0][0] == '-' && av[0][1] != '\0') {
#if defined (SUS) || defined (SU3)
		nxt:
#endif
			switch (av[0][1]) {
			case 'b':
				settype(&btype, arg());
				break;
			case 'f':
				settype(&ftype, arg());
				break;
			case 'h':
				settype(&htype, arg());
				break;
			case 'd':
				setdelim('d', arg());
				break;
			case 'i':
				setnum('i', arg(), &incr, 1);
				break;
			case 'l':
				setnum('l', arg(), &whichblank, 1);
				break;
			case 'n':
				setformat('n', arg());
				break;
			case 'p':
				pflag = 1;
				if (av[0][2]) {
#if defined (SUS) || defined (SU3)
					(av[0])++;
					goto nxt;
#else
					invalid(av[0][1], &av[0][2]);
#endif
				}
				break;
			case 's':
				setsep('s', arg());
				break;
			case 'v':
				setnum('v', arg(), &startnum, 1);
				break;
			case 'w':
				setnum('w', arg(), &width, 6);
				break;
			default:
				invalid(av[0][1], &av[0][2]);
			}
		} else if ((input = ib_open(av[0], 0)) == NULL) {
			fprintf(stderr, "CANNOT OPEN FILE %s\n", av[0]);
			exit(1);
		}
		av++;
	}
}

static size_t
determine(const char *lp, size_t l, struct type **curtp, int *blank)
{
	wint_t	wc;
	int	n, special;

	next(wc, lp, n);
	*blank = special = 0;
	if (wc == delim[0]) {
		lp += n, l -= n;
		next(wc, lp, n);
		if (wc == delim[1]) {
			lp += n, l -= n;
			next(wc, lp, n);
			if (wc == delim[0]) {
				lp += n, l -= n;
				next(wc, lp, n);
				if (wc == delim[1]) {
					lp +=n, l -= n;
					next(wc, lp, n);
					if (wc == delim[0]) {
						lp += n, l -= n;
						next(wc, lp, n);
						if (wc == delim[1]) {
							lp += n, l -= n;
							next(wc, lp, n);
							if (l==0 || wc=='\n') {
								*curtp = &htype;
								if (!pflag)
									number =
								startnum;
								special = 1;
							}
						}
					} else if (l == 0 || wc == '\n') {
						*curtp = &btype;
						special = 1;
					}
				}
			} else if (l == 0 || wc == '\n') {
				*curtp = &ftype;
				special = 1;
			}
		}
#ifdef	notdef	/* the standard seems to be fouled up here */
	} else if (mb_cur_max > 1 ? !iswgraph(wc) : !isgraph(wc)) {
		lp += n, l -= n;;
		while (l != 0) {
			next(wc, lp, n);
			if (mb_cur_max > 1 ? iswgraph(wc) : isgraph(wc))
				break;
			lp += n, l -= n;
		}
		if (l == 0)
			*blank = 1;
	}
#else
	} else if (l == 0 || wc == '\n')
		*blank = 1;
#endif
	if (*blank) {
		if (blanks < whichblank)
			blanks++;
		else
			blanks = 1;
	} else
		blanks = 0;
	return special == 0;
}

static size_t
mknum(char *buf, int size, unsigned long long n)
{
	char	*cp;

	if (size == 0)
		return 0;
	cp = buf + mknum(buf, size - 1, n / 10);
	*cp = n % 10 + '0';
	return cp - buf + 1;
}

#if defined (__GLIBC__) && defined (_IO_putc_unlocked)
#undef	putchar
#define	putchar(c)	_IO_putc_unlocked(c, stdout)
#endif

static void
print(int donumber, const char *lp, size_t l)
{
	char	numbuf[26], *np, *nq;
	const char	*sp;
	long	n;

	if (donumber) {
		mknum(numbuf, sizeof numbuf - 1, number);
		numbuf[sizeof numbuf - 1] = '\0';
		for (np = numbuf; *np == '0'; np++);
		if (&numbuf[sizeof numbuf - 1] - np > width)
			np = &numbuf[sizeof numbuf - 1 - width];
		n = width;
		for (nq = np; *nq; nq++)
			if (n > 0)
				n--;
		switch (format) {
		case FMT_RN:
			while (n--)
				putchar(' ');
			for (nq = np; *nq; nq++)
				putchar(*nq);
			break;
		case FMT_RZ:
			while (n--)
				putchar('0');
			for (nq = np; *nq; nq++)
				putchar(*nq);
			break;
		case FMT_LN:
			for (nq = np; *nq; nq++)
				putchar(*nq);
			while (n--)
				putchar(' ');
			break;
		}
		for (sp = sep; *sp; sp++)
			putchar(*sp);
	} else {
		n = width + 1;
		while (n--)
			putchar(' ');
	}
	while (l--) {
		putchar(*lp);
		lp++;
	}
}

static void
decide(char *lp, size_t l, struct type *curtp, int blank)
{
	int	gotcha = 0, tmp;

	switch (curtp->t_type) {
	case 'a':
		gotcha = !blank || blanks == whichblank;
		break;
	case 't':
		gotcha = !blank;
		break;
	case 'n':
		gotcha = 0;
		break;
	case 'p':
		if ((tmp = lp[l-1]) == '\n')
			lp[l-1] = '\0';
#if defined (SUS) || defined (SU3) || defined (S42)
		gotcha = regexec(&curtp->t_re, lp, 0, NULL, 0) == 0;
#else
		gotcha = step(lp, curtp->t_ep) != 0;
#endif
		lp[l-1] = tmp;
		break;
	}
	print(gotcha, lp, l);
	if (gotcha)
		number += incr;
}

static void
nl(struct iblok *ip)
{
	char	*line = NULL;
	size_t	linesize = 0, len;
	int	blank;
	struct type	*curtp = &btype;

	number = startnum;
	while ((len = ib_getlin(ip, &line, &linesize, srealloc)) != 0) {
		if (determine(line, len, &curtp, &blank))
			decide(line, len, curtp, blank);
		else
			putchar('\n');
	}
}

int
main(int argc, char **argv)
{
	setlocale(LC_COLLATE, "");
	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	scan(argc - 1, &argv[1]);
	if (input == NULL)
		input = ib_alloc(0, 0);
	nl(input);
	return 0;
}
