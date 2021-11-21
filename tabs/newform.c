/*
 * newform - change the format of a text file
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
static const char sccsid[] USED = "@(#)newform.sl	1.7 (gritter) 5/29/05";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <locale.h>
#include <ctype.h>
#include <wchar.h>
#include <wctype.h>
#include <unistd.h>

#if defined (__GLIBC__) && defined (_IO_putc_unlocked)
#undef	putchar
#define	putchar(c)	_IO_putc_unlocked(c, stdout)
#endif

#include <iblok.h>
#include <mbtowi.h>
#include "tabspec.h"

#define	next(wc, s, n) (mb_cur_max > 1 && *(s) & 0200 ? \
			((n) = mbtowi(&(wc), (s), mb_cur_max), \
			 (n) = ((n) > 0 ? (n) : (n) < 0 ? (wc=WEOF, 1) : 1)) :\
		((wc) = *(s) & 0377, (n) = 1))

#define	check(lp)	((lp)->l_len < (lp)->l_size ? (lp)->l_line : \
		((lp)->l_line = srealloc((lp)->l_line, \
				((lp)->l_size += 128) * sizeof *(lp)->l_line)))

#define	add(lp, c)	(check(lp)[(lp)->l_len] = (c), (lp)->l_len++)

static struct 	line {
	wchar_t		*l_line;
	struct tabulator	*l_ltab;
	size_t		l_size;
	size_t		l_len;
	size_t		l_max;
} line1, line2;

static struct action {
	struct action	*a_nxt;
	struct line	*(*a_fcn)(struct line *, struct action *);
	struct tabulator	*a_tab;
	wchar_t	a_chr;
	long	a_val;
} *actions;

static const char	*progname;
static int		status;
static int		sflag;
static wint_t		cflag = ' ';
static int		mb_cur_max;

static int		scan(int, char **);
static void		addact(int, const char *);
static void		usage(void);
static void		newform(const char *);
static void		sproc(const char *, size_t);
static struct line	*iproc(struct line *, struct action *);
static struct line	*oproc(struct line *, struct action *);
static void		spaces(struct line *, long *, long *,
				struct tabulator **);
static struct line	*bproc(struct line *, struct action *);
static struct line	*eproc(struct line *, struct action *);
static struct line	*pproc(struct line *, struct action *);
static struct line	*aproc(struct line *, struct action *);
static struct line	*fproc(struct line *, struct action *);
static struct line	*lproc(struct line *, struct action *);
static void		print(const wchar_t *, size_t);
static struct line	*other(struct line *);
static void		toolarge(long);
static void		*srealloc(void *, size_t);

int
main(int argc, char **argv)
{
	int	i;

	progname = basename(argv[0]);
	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	if (scan(argc, argv)) {
		for (i = 1; i < argc; i++)
			if (argv[i][0] != '-')
				newform(argv[i]);
	} else
		newform(NULL);
	return status;
}

static int
scan(int ac, char **av)
{
	int	i, filecnt = 0, n;

	for (i = 1; i < ac; i++)
		if (av[i][0] == '-')
			switch (av[i][1]) {
			case 's':
				sflag = 1;
				break;
			case 'c':
				if (av[i][2]) {
					next(cflag, &av[i][2], n);
					if (cflag == WEOF) {
						fprintf(stderr,
							"%s: illegal -cchar\n",
								progname);
						exit(1);
					}
				} else
					cflag = ' ';
				break;
			default:
				addact(av[i][1], &av[i][2]);
			}
		else
			filecnt++;
	return filecnt != 0;
}

static void
addact(int c, const char *s)
{
	struct action	*ap, *aq;

	ap = scalloc(1, sizeof *ap);
	switch (c) {
	case 'i':
	case 'o':
		ap->a_fcn = c == 'i' ? iproc : oproc;
		if (s[0] == '-' && s[1] == '0' && s[2] == '\0')
			ap->a_tab = NULL;
		else if ((ap->a_tab = tabstops(*s?s:"-8", 80)) == NULL) {
			switch (taberrno) {
			case TABERR_CANTOP:
				fprintf(stderr, "%s: can't open %s\n",
						progname, &s[2]);
				break;
			case TABERR_FILIND:
				fprintf(stderr, "%s: tabspec indirection "
						"illegal\n", progname);
				break;
			default:
				fprintf(stderr, "%s: tabspec in error\n"
			"tabspec is \t-a\t-a2\t-c\t-c2\t-c3\t-f\t-p\t-s\n"
			"\t\t-u\t--\t--file\t-number\tnumber,..,number\n",
					progname);
			}
			exit(1);
		}
		break;
	case 'b':
		ap->a_fcn = bproc;
		ap->a_val = atol(s);
		break;
	case 'e':
		ap->a_fcn = eproc;
		ap->a_val = atol(s);
		break;
	case 'p':
		ap->a_fcn = pproc;
		ap->a_val = atol(s);
		ap->a_chr = cflag;
		break;
	case 'a':
		ap->a_fcn = aproc;
		ap->a_val = atol(s);
		ap->a_chr = cflag;
		break;
	case 'f':
		ap->a_fcn = fproc;
		break;
	case 'l':
		ap->a_fcn = lproc;
		if ((ap->a_val = atol(s)) <= 0)
			ap->a_val = 72;
		break;
	default:
		usage();
	}
	if (actions) {
		for (aq = actions; aq->a_nxt; aq = aq->a_nxt);
		aq->a_nxt = ap;
	} else
		actions = ap;
}

static void
usage(void)
{
	fprintf(stderr, "\
usage: %s  [-s] [-itabspec] [-otabspec] [-pn] [-en] [-an] [-f] [-cchar]\n\
\t\t[-ln] [-bn] [file ...]\n",
		progname);
	exit(1);
}

static void
newform(const char *fn)
{
	struct iblok	*ip;
	static char	*line;
	static size_t	linesize;
	size_t	linelen;

	if ((ip = fn ? ib_open(fn, 0) : ib_alloc(0, 0)) == NULL) {
		fprintf(stderr, "%s: can't open %s\n", progname, fn);
		status |= 1;
		return;
	}
	while ((linelen = ib_getlin(ip, &line, &linesize, srealloc)) != 0) {
		if (line[linelen-1] == '\n')
			line[--linelen] = '\0';
		sproc(line, linelen);
	}
	if (ip->ib_fd)
		ib_close(ip);
	else
		ib_free(ip);
}

static void
sproc(const char *bline, size_t blen)
{
	struct line	*lp;
	struct action	*ap;
	const char	*bp;
	wchar_t	sheared[10];
	wint_t	c = WEOF;
	long	i;
	int	n;

	bp = bline;
	if (sflag) {
		for (i = 1; i <= 9 && bp < &bline[blen]; i++) {
			next(c, bp, n);
			bp += n;
			if (c == '\t') {
				i--;
				break;
			}
			if (c == WEOF)
				i--;
			else
				sheared[i] = c;
		}
		if (i == 10) {
			sheared[i=8] = '*';
			while (bp < &bline[blen]) {
				next(c, bp, n);
				bp += n;
				if (c == '\t')
					break;
			}
		}
		if (c != '\t') {
			fprintf(stderr, "not -s format\n");
			exit(1);
		}
		sheared[0] = i;
	}
	lp = &line1;
	lp->l_len = 0;
	while (bp < &bline[blen]) {
		next(c, bp, n);
		bp += n;
		if (c == WEOF)
			continue;
		add(lp, c);
	}
	lp->l_max = 80;
	lp->l_ltab = 0;
	for (ap = actions; ap; ap = ap->a_nxt)
		lp = ap->a_fcn(lp, ap);
	print(lp->l_line, lp->l_len);
	if (sflag && sheared[0])
		print(&sheared[1], sheared[0]);
	putchar('\n');
}

static struct line *
iproc(struct line *lp, struct action *ap)
{
	struct line	*np = other(lp);
	struct tabulator	*tp = ap->a_tab;
	wchar_t	c;
	long	i, col;
	int	w;

	col = 0;
	for (i = 0; i < lp->l_len; i++) {
		switch (c = lp->l_line[i]) {
		case '\b':
			if (col > 0)
				col--;
			add(np, c);
			break;
		case '\t':
			if (ap->a_tab) {
				if (tp && tp->t_rep) {
					if (col % tp->t_rep == 0) {
						add(np, ' ');
						col++;
					}
					while (col % tp->t_rep) {
						add(np, ' ');
						col++;
					}
					break;
				}
				while (tp && (col>tp->t_tab || tp->t_tab==0))
					tp = tp->t_nxt;
				if (tp && col == tp->t_tab) {
					add(np, ' ');
					col++;
					tp = tp->t_nxt;
				}
				if (tp) {
					while (col < tp->t_tab) {
						add(np, ' ');
						col++;
					}
					tp = tp->t_nxt;
				}
				break;
			} else if (ap->a_tab)
				break;
			c = ' ';
			/*FALLTHRU*/
		default:
			if (mb_cur_max > 1 && (w = wcwidth(c)) >= 0)
				col += w;
			else
				col++;
			add(np, c);
			break;
		}
	}
	return np;
}

static struct line *
oproc(struct line *lp, struct action *ap)
{
	struct line	*np = other(lp);
	struct tabulator	*tp = ap->a_tab;
	wchar_t	c;
	long	i, col, spc;
	int	w;

	np->l_ltab = ap->a_tab;
	col = 0, spc = 0;
	for (i = 0; i < lp->l_len; i++) {
		c = lp->l_line[i];
		if (c == ' ' && tp) {
			spc++;
			continue;
		}
		if (spc)
			spaces(np, &spc, &col, &tp);
		switch (c) {
		case '\b':
			if (col > 0)
				col--;
			break;
		case '\t':
			if (ap->a_tab) {
				if (tp && tp->t_rep) {
					if (col % tp->t_rep == 0)
						col++;
					while (col % tp->t_rep)
						col++;
					break;
				}
				while (tp && (col>tp->t_tab || tp->t_tab==0))
					tp = tp->t_nxt;
				if (tp && col == tp->t_tab) {
					col++;
					tp = tp->t_nxt;
				}
				if (tp) {
					while (col < tp->t_tab)
						col++;
					tp = tp->t_nxt;
				}
			} else if (ap->a_tab == 0) {
				c = ' ';
				col++;
			}
			break;
		default:
			if (mb_cur_max > 1 && (w = wcwidth(c)) >= 0)
				col += w;
			else
				col++;
		}
		add(np, c);
	}
	if (spc)
		spaces(np, &spc, &col, &tp);
	return np;
}

static void
spaces(struct line *np, long *sp, long *cp, struct tabulator **tp)
{
	long	nsp;

	if (*tp && (*tp)->t_rep == 0) {
		while (*tp && (*tp)->t_tab <= *cp)
			*tp = (*tp)->t_nxt;
		while (*sp && *tp && (*cp + *sp) >= (*tp)->t_tab) {
			add(np, '\t');
			*sp -= (*tp)->t_tab - *cp;
			*cp = (*tp)->t_tab;
			*tp = (*tp)->t_nxt;
		}
	} else if (*tp) {
		while (*sp >= (nsp = (*tp)->t_rep - *cp % (*tp)->t_rep)) {
			add(np, '\t');
			*sp -= nsp;
			*cp += nsp;
		}
	}
	while (*sp) {
		add(np, ' ');
		(*cp)++;
		(*sp)--;
	}
}

static struct line *
bproc(struct line *lp, struct action *ap)
{
	struct line	*np;
	long	i, min;

	if (lp->l_len > lp->l_max) {
		min = ap->a_val>0 ? ap->a_val : lp->l_len-lp->l_max;
		if (min >= lp->l_len) {
			toolarge(lp->l_len);
			return lp;
		}
		np = other(lp);
		for (i = min; i < lp->l_len; i++)
			add(np, lp->l_line[i]);
		return np;
	} else
		return lp;
}

static struct line *
eproc(struct line *lp, struct action *ap)
{
	if (lp->l_len > lp->l_max) {
		if (ap->a_val > 0) {
			if (ap->a_val > lp->l_len)
				toolarge(lp->l_len);
			else
				lp->l_len -= ap->a_val;
		} else
			lp->l_len = lp->l_max;
	}
	return lp;
}

static struct line *
pproc(struct line *lp, struct action *ap)
{
	struct line	*np;
	long	i;

	if (lp->l_len < lp->l_max) {
		np = other(lp);
		i = ap->a_val>0 ? ap->a_val : lp->l_max - lp->l_len;
		while (i--)
			add(np, ap->a_chr);
		for (i = 0; i < lp->l_len; i++)
			add(np, lp->l_line[i]);
		return np;
	} else
		return lp;
}

static struct line *
aproc(struct line *lp, struct action *ap)
{
	long	i;

	if (lp->l_len < lp->l_max) {
		i = ap->a_val>0 ? ap->a_val : lp->l_max - lp->l_len;
		while (i--)
			add(lp, ap->a_chr);
	}
	return lp;
}

static struct line *
fproc(struct line *lp, struct action *ap)
{
	if (ap->a_val == 0) {
		printf("<:t%s d:>\n",
			lp->l_ltab&&lp->l_ltab->t_str?lp->l_ltab->t_str:"-8");
		ap->a_val = 1;
	}
	return lp;
}

static struct line *
lproc(struct line *lp, struct action *ap)
{
	lp->l_max = ap->a_val ? ap->a_val : 72;
	return lp;
}

static void
print(const wchar_t *line, size_t length)
{
	const wchar_t	*sp;
	char	mb[MB_LEN_MAX];
	int	i, n;

	for (sp = line; sp < &line[length]; sp++) {
		if (mb_cur_max > 1 && *sp & ~(wchar_t)0177) {
			n = wctomb(mb, *sp);
			for (i = 0; i < n; i++)
				putchar(mb[i] & 0377);
		} else
			putchar(*sp & 0377);
	}
}

static struct line *
other(struct line *lp)
{
	struct line *np;

	np = lp == &line1 ? &line2 : &line1;
	np->l_ltab = lp->l_ltab;
	np->l_len = 0;
	np->l_max = lp->l_max;
	return np;
}

static void
toolarge(long n)
{
	fprintf(stderr, "%s: truncate request larger than line, %ld\n",
			progname, n);
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
