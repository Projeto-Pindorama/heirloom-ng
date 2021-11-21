/*
 * hd - display files in hexadecimal format
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, September 2003.
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
static const char sccsid[] USED = "@(#)hd.sl	1.12 (gritter) 5/29/05";

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <stdarg.h>
#include <locale.h>
#include <ctype.h>
#include <wctype.h>
#include <wchar.h>
#include <inttypes.h>
#include <limits.h>
#include "atoll.h"
#include "mbtowi.h"

#ifdef	__GLIBC__
#ifdef	_IO_getc_unlocked
#undef	getc
#define	getc(f)		_IO_getc_unlocked(f)
#endif
#ifdef	_IO_putc_unlocked
#undef	putchar
#define	putchar(c)	_IO_putc_unlocked(c, stdout)
#endif
#endif	/* __GLIBC__ */

enum	base {
	BASE_0 = 00,
	BASE_X = 01,
	BASE_D = 02,
	BASE_O = 04
};

union	block {
	int8_t	b_c[16];
	int16_t	b_w[8];
	int32_t	b_l[4];
};

static const struct fm {
	int	f_fmt;
	enum base	f_base;
	char	f_width;
	char	f_align[3];
	const char	*f_prf;
} ofmt[] = {
	{ 'b', BASE_X,  2, {2,5,11}, "%02x" },
	{ 'b', BASE_D,  3, {3,7,15}, "%3u" },
	{ 'b', BASE_O,  3, {3,7,15}, "%03o" },
	{ 'c', BASE_X,  2, {2,5,11}, "%02x" },
	{ 'c', BASE_D,  3, {3,7,15}, "%3u" },
	{ 'c', BASE_O,  3, {3,7,15}, "%03o" },
	{ 'w', BASE_X,  4, {0,4, 9}, "%04x" },
	{ 'w', BASE_D,  5, {0,5,11}, "%5u" },
	{ 'w', BASE_O,  6, {0,6,13}, "%06o" },
	{ 'l', BASE_X,  8, {0,0, 8}, "%08lx" },
	{ 'l', BASE_D, 10, {0,0,10}, "%10lu" },
	{ 'l', BASE_O, 11, {0,0,11}, "%011lo" },
	{   0, BASE_0,  0, {0,0, 0}, NULL }
};

static int		Aflag;	/* print ASCII at right */
static enum base	aflag;	/* address format specifier */
static enum base	bflag;	/* byte format specifier */
static enum base	cflag;	/* print ASCII at center */
static enum base	lflag;	/* long (32 bit) format specifier */
static long long	nflag;	/* number of bytes to process */
static long long	sflag;	/* start offset */
static int		tflag;	/* print text file */
static int		vflag;	/* no '*' for identical lines */
static enum base	wflag;	/* word (16 bit) format specifier */
static char		align[3];
static const char	*progname;
static int		status;
static int		mb_cur_max;

static void		usage(void);
static void		flag(int);
static void		base(enum base, enum base *);
static long long	count(const char *);
static void		usage(void);
static void		diag(const char *, ...);
static void		hd(FILE *);
static void		prna(long long);
static void		prnb(union block *, int);
static void		line(union block *, int, int, enum base, int);
static const struct fm	*getfmt(int, enum base);
static void		getalign(void);
static void		prnt(FILE *, long long);
static void		prnc(int);
static char		*wcget(FILE *fp, wint_t *wc, int *len);

int
main(int argc, char **argv)
{
	FILE	*fp;
	int	i, j;

	progname = basename(argv[0]);
	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	for (i = 1; i < argc && argv[i][0] == '-'; i++) {
		switch (argv[i][1]) {
		case 's':
			if (argv[i][2])
				sflag = count(&argv[i][2]);
			else if (++i < argc)
				sflag = count(argv[i]);
			else
				usage();
			break;
		case 'n':
			if (argv[i][2])
				nflag = count(&argv[i][2]);
			else if (++i < argc)
				nflag = count(argv[i]);
			else
				usage();
			break;
		default:
			for (j = 1; argv[i][j]; j++)
				flag(argv[i][j]&0377);
			flag(0);
		}
	}
	if (tflag && (Aflag|bflag|cflag|lflag|wflag))
		diag("-t flag overrides other flags");
	if ((Aflag|bflag|cflag|lflag|wflag) == 0)
		Aflag = 1;
	if ((bflag|cflag|lflag|wflag) == 0)
		bflag = BASE_X;
	getalign();
	if (i < argc) {
		j = i+1 < argc;
		do {
			if (access(argv[i], R_OK) < 0) {
				diag("cannot access %s", argv[i]);
				continue;
			}
			if ((fp = fopen(argv[i], "r")) == NULL) {
				diag("open of %s failed", argv[i]);
				continue;
			}
			if (j)
				printf("%s:\n", argv[i]);
			hd(fp);
			fclose(fp);
			if (i+1 < argc)
				printf("\n");
		} while (++i < argc);
	} else
		hd(stdin);
	return status;
}

static void
flag(int c)
{
	static enum base	*basep;

	switch (c) {
	case '\0':
		if (basep && basep != &aflag && *basep == BASE_0)
			*basep |= BASE_O|BASE_D|BASE_X;
		basep = NULL;
		break;
	case 'a':
		basep = &aflag;
		break;
	case 'b':
		basep = &bflag;
		break;
	case 'w':
		basep = &wflag;
		break;
	case 'l':
		basep = &lflag;
		break;
	case 'c':
		basep = &cflag;
		break;
	case 'A':
		Aflag = 1;
		break;
	case 'x':
		base(BASE_X, basep);
		break;
	case 'd':
		base(BASE_D, basep);
		break;
	case 'o':
		base(BASE_O, basep);
		break;
	case 't':
		tflag = 1;
		break;
	case 'v':
		vflag = 1;
		break;
	default:
		usage();
	}
}

static void
base(enum base b, enum base *basep)
{
	if (basep) {
		if (basep == &aflag)
			*basep = b;
		else
			*basep |= b;
	} else {
		if (aflag == BASE_0)
			aflag |= b;
		cflag |= b;
		bflag |= b;
		wflag |= b;
		lflag |= b;
	}
}

static long long
count(const char *s)
{
	long long	c;
	int	bs = 10;
	char	*x;

	if (s[0] == '0' && s[1] == 'x') {
		bs = 16;
		s += 2;
	} else if (s[0] == '0') {
		bs = 8;
		s++;
	}
	c = strtoll(s, &x, bs);
	s = x;
	if (*s == '*')
		s++;
	switch (*s) {
	case 'w':
		c *= 2;
		s++;
		break;
	case 'l':
		c *= 4;
		s++;
		break;
	case 'b':
		c *= 512;
		s++;
		break;
	case 'k':
		c *= 1024;
		s++;
		break;
	}
	if (*s) {
		diag("bad count/offset value");
		exit(3);
	}
	return c;
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-acbwlAxdo] [-t] [-s offset[*][wlbk]] "
			"[-n count[*][wlbk]] [file] ...\n",
			progname);
	exit(2);
}

static void
diag(const char *fmt, ...)
{
	va_list	ap;

	fprintf(stderr, "%s: ", progname);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
	status |= 1;
}

static void
hd(FILE *fp)
{
	long long	of = 0, rd = 0;
	union block	b, ob;
	size_t	n, m, on = 0;
	int	star = 0;

	if (sflag)
		while (of < sflag) {
			getc(fp);
			of++;
		}
	if (tflag) {
		prnt(fp, of);
		return;
	}
	do {
		if (nflag == 0 || rd + sizeof b.b_c < nflag)
			m = sizeof b.b_c;
		else
			m = nflag - rd;
		if ((n = fread(b.b_c, 1, m, fp)) > 0) {
			if (!vflag && n==on && memcmp(b.b_c, ob.b_c, n) == 0) {
				if (star == 0)
					printf("*\n");
				star = 1;
			} else {
				star = 0;
				prna(of);
				if (n < sizeof b.b_c)
					memset(&b.b_c[n], 0, sizeof b.b_c - n);
				prnb(&b, n);
			}
		}
		rd += n;
		of += n;
		on = n;
		ob = b;
	} while (n == m && (nflag == 0 || rd < nflag));
	prna(of);
	putchar('\n');
}

static void
prna(long long n)
{
	switch (aflag) {
	case BASE_O:
		printf("%06llo", n);
		break;
	case BASE_D:
		printf("%05llu", n);
		break;
	case BASE_0:
	case BASE_X:
		printf("%04llx", n);
		break;
	}
}

static void
prnb(union block *bp, int n)
{
	int	cnt = 0;

	if (cflag&BASE_X)
		line(bp, n, 'c', BASE_X, cnt++);
	if (cflag&BASE_D)
		line(bp, n, 'c', BASE_D, cnt++);
	if (cflag&BASE_O)
		line(bp, n, 'c', BASE_O, cnt++);
	if (bflag&BASE_X)
		line(bp, n, 'b', BASE_X, cnt++);
	if (bflag&BASE_D)
		line(bp, n, 'b', BASE_D, cnt++);
	if (bflag&BASE_O)
		line(bp, n, 'b', BASE_O, cnt++);
	if (wflag&BASE_X)
		line(bp, n, 'w', BASE_X, cnt++);
	if (wflag&BASE_D)
		line(bp, n, 'w', BASE_D, cnt++);
	if (wflag&BASE_O)
		line(bp, n, 'w', BASE_O, cnt++);
	if (lflag&BASE_X)
		line(bp, n, 'l', BASE_X, cnt++);
	if (lflag&BASE_D)
		line(bp, n, 'l', BASE_D, cnt++);
	if (lflag&BASE_O)
		line(bp, n, 'l', BASE_O, cnt++);
}

static void
line(union block *bp, int n, int fmt, enum base base, int cnt)
{
	int	c, i, j, k, col = 0;
	const char	*cp;
	const struct fm	*fmp;

	putchar('\t');
	i = 0;
	switch (fmt) {
	case 'l':
		fmp = getfmt('l', base);
		for (j = i/4; j < (n>>2); j++, i += 4) {
			if (col > 0) {
				putchar(' ');
				col++;
			}
			if (i == 8) {
				putchar(' ');
				col++;
			}
			for (k = fmp->f_width; k < align[2]; k++) {
				putchar(' ');
				col++;
			}
			col += printf(fmp->f_prf,(long)(bp->b_l[j]&0xffffffff));
		}
		if (i == n)
			break;
		/*FALLTHRU*/
	case 'w':
		fmp = getfmt('w', base);
		for (j = i/2; j < (n>>1); j++, i += 2) {
			if (col > 0) {
				putchar(' ');
				col++;
			}
			if (i == 8) {
				putchar(' ');
				col++;
			}
			for (k = fmp->f_width; k < align[1]; k++) {
				putchar(' ');
				col++;
			}
			col += printf(fmp->f_prf, (int)(bp->b_w[j]&0177777));
		}
		if (i == n)
			break;
		/*FALLTHRU*/
	case 'b':
		fmp = getfmt('b', base);
		for (j = i; j < n; j++, i++) {
			if (col > 0) {
				putchar(' ');
				col++;
			}
			if (i == 8) {
				putchar(' ');
				col++;
			}
			for (k = fmp->f_width; k < align[0]; k++) {
				putchar(' ');
				col++;
			}
			col += printf(fmp->f_prf, bp->b_c[j]&0377);
		}
		break;
	case 'c':
		fmp = getfmt('c', base);
		for (i = 0; i < n; i++) {
			if (col > 0) {
				putchar(' ');
				col++;
			}
			if (i == 8) {
				putchar(' ');
				col++;
			}
			for (k = fmp->f_width; k < align[0]; k++) {
				putchar(' ');
				col++;
			}
			c = bp->b_c[i]&0377;
			cp = NULL;
			if (c == '\b')
				cp = "\\b";
			else if (c == '\t')
				cp = "\\t";
			else if (c == '\n')
				cp = "\\n";
			else if (c == '\f')
				cp = "\\f";
			else if (c == '\r')
				cp = "\\r";
			else if (!isprint(c)) {
				col += printf(fmp->f_prf, c);
			} else {
				if (base != BASE_X) {
					putchar(' ');
					col++;
				}
				col += printf(" %c", c);
			}
			if (cp) {
				if (base != BASE_X) {
					putchar(' ');
					col++;
				}
				printf(cp);
			}
		}
		break;
	}
	if (cnt == 0 && Aflag) {
		while (col++ < 51)
			putchar(' ');
		for (i = 0; i < n; i++) {
			if ((bp->b_c[i]&0340) == 0 || bp->b_c[i] == 0177 ||
					!isprint(bp->b_c[i]&0377))
				putchar('.');
			else
				putchar(bp->b_c[i]&0377);
		}
	}
	putchar('\n');
}

static const struct fm *
getfmt(int fmt, enum base base)
{
	int	i;

	for (i = 0; ofmt[i].f_fmt; i++)
		if (ofmt[i].f_fmt == fmt && ofmt[i].f_base == base)
			return &ofmt[i];
	return NULL;
}

static void
getalign(void)
{
	int	i, j;
	enum base	*basep;

	for (i = 0; ofmt[i].f_fmt; i++) {
		switch (ofmt[i].f_fmt) {
		case 'b':
			basep = &bflag;
			break;
		case 'c':
			basep = &cflag;
			break;
		case 'w':
			basep = &wflag;
			break;
		case 'l':
			basep = &lflag;
			break;
		default:
			basep = NULL;
		}
		if (basep && *basep & ofmt[i].f_base)
			for (j = 0; j < sizeof align; j++)
				if (ofmt[i].f_align[j] > align[j])
					align[j] = ofmt[i].f_align[j];
	}
}

static void
prnt(FILE *fp, long long of)
{
	wint_t	wc;
	char	b, *mb;
	int	c, lastc = '\n', n;
	long long	rd = 0;

	while ((nflag == 0 || rd < nflag)) {
		if (mb_cur_max > 1) {
			if ((mb = wcget(fp, &wc, &n)) == NULL)
				break;
		} else {
			if ((c = getc(fp)) == EOF)
				break;
			b = wc = c;
			mb = &b;
			n = 1;
		}
		if (lastc == '\n') {
			prna(of);
			putchar('\t');
		}
		of += n, rd += n;
		if (n == 1) {
			c = *mb&0377;
			lastc = c;
			if (wc != WEOF && isprint(c) && c != '\\' &&
					c != '^' && c != '~')
				putchar(c);
			else
				prnc(c);
			if (lastc == '\n')
				putchar('\n');
		} else {
			lastc = c = EOF;
			if (wc != WEOF && iswprint(wc))
				while (n--) {
					putchar(*mb&0377);
					mb++;
				}
			else
				while (n--) {
					prnc(*mb&0377);
					mb++;
				}
		}
	}
	if (lastc != '\n')
		putchar('\n');
	prna(of);
	putchar('\n');
}

static void
prnc(int c)
{
	if (c == 0177 || c == 0377) {
		printf("\\%o", c);
		return;
	}
	if (c & 0200) {
		putchar('~');
		c &= 0177;
	}
	if (c < 040) {
		putchar('^');
		c |= 0100;
	}
	if (c == '\\' || c == '~' || c == '^')
		putchar('\\');
	putchar(c);
}

static char *
wcget(FILE *fp, wint_t *wc, int *len)
{
	static char	mbuf[MB_LEN_MAX+1];
	static char	*mcur, *mend;
	static int	incompl;
	size_t	rest;
	int	c, i, n;

	i = 0;
	rest = mend - mcur;
	if (rest && mcur > mbuf) {
		do
			mbuf[i] = mcur[i];
		while (i++, --rest);
	} else if (incompl) {
		incompl = 0;
		*wc = WEOF;
		mend = mcur = NULL;
		return NULL;
	}
	if (i == 0) {
		c = getc(fp);
		if (c == EOF) {
			*wc = WEOF;
			mend = mcur = NULL;
			return NULL;
		}
		mbuf[i++] = c;
	}
	if (mbuf[0] & 0200) {
		while (mbuf[i-1] != '\n' && i < mb_cur_max &&
				incompl == 0) {
			c = getc(fp);
			if (c != EOF)
				mbuf[i++] = c;
			else
				incompl = 1;
		}
		n = mbtowi(wc, mbuf, i);
		if (n < 0) {
			*len = 1;
			*wc = WEOF;
		} else if (n == 0) {
			*len = 1;
			*wc = '\0';
		} else
			*len = n;
	} else {
		*wc = mbuf[0];
		*len = n = 1;
	}
	mcur = &mbuf[*len];
	mend = &mcur[i - *len];
	return mbuf;
}
