/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, May 2003.
 */
/*	from Unix 32V /usr/src/cmd/col.c	*/
/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   Redistributions of source code and documentation must retain the
 *    above copyright notice, this list of conditions and the following
 *    disclaimer.
 *   Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *   All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed or owned by Caldera
 *      International, Inc.
 *   Neither the name of Caldera International, Inc. nor the names of
 *    other contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE
 * LIABLE FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)col.sl	1.9 (gritter) 5/29/05";

# include <stdio.h>
# include <stdlib.h>
# include <libgen.h>
# include <string.h>
# include <locale.h>
# include <ctype.h>
# include <wchar.h>
# include <wctype.h>
# include <unistd.h>
# include <iblok.h>
# if defined (__GLIBC__) && defined (_IO_putc_unlocked)
# undef putchar
# define putchar(c)	_IO_putc_unlocked(c, stdout)
# endif
# define PL 256
# define ESC '\033'
# define RLF '\013'
# define SI '\017'
# define SO '\016'
# define CMASK	0x00000000ffffffffLL
# define EOFC	0x0000010000000000LL
# define TRANS	0x1000000000000000LL
# define GREEK	0x2000000000000000LL
# define ILLSEQ	0x4000000000000000LL
# define ISPRNT	0x8000000000000000LL
# define COLMSK	0x0f00000000000000LL
# define COLSHF	56
# define CHRMSK 0x000000ff00000000LL
# define CHRSHF	32

# define nextc(c, cp, m)	(mb_cur_max > 1 ? \
		((cp) = ib_getw(input, &(c), &(m)), \
		 (cp) == NULL ? EOFC : \
		 (c) == WEOF ? ((long long)(*(cp)&0377)<<CHRSHF)|ILLSEQ : \
		 iswprint(c) ? (c)|((long long)wcwidth(c)<<COLSHF)|ISPRNT : \
		 (c)) : \
		((c) = ib_get(input), \
		 (c) == EOF ? EOFC : \
		 isprint(c) ? (c)|(1LL<<COLSHF)|ISPRNT : \
		 (c)))

# define slbuff(n)	(line+(n)+128 >= LINELN ? growsbuff((n)+128) : lbuff);

static long long *page[PL];
static long long *lbuff;
static long	line, LINELN;
static int bflag, xflag, fflag, pflag;
static int half;
static int cp, lp;
static int ll, llh, mustwr;
static int pcp = 0;
static char *pgmname;
static struct iblok	*input;
static int	mb_cur_max;

static long long	space[] = { ' ' | (1LL<<COLSHF) | ISPRNT, '\0' };

static void	outc(long long);
static void	store(int);
static void	fetch(int);
static void	emit(long long *, int);
static void	incr(void);
static void	decr(void);
static size_t	lllen(const long long *);
static long long	*llcpy(long long *, const long long *);
static long long	*growsbuff(size_t);

int
main (int argc, char **argv)
{
	int i;
	long long greek;
	long long c;
	wint_t	wc;
	char	*mp;

	pgmname = basename(argv[0]);
	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;

	while ((i = getopt(argc, argv, ":bfxp")) != EOF) {
		switch (i) {
		case 'b':
			bflag++;
			break;
		case 'f':
			fflag++;
			break;
		case 'x':
			xflag++;
			break;
		case 'p':
			pflag++;
			break;
		default:
			fprintf (stderr, "%s: bad option letter %c\n",
					pgmname, optopt);
				exit (2);
		}
	}
	if (optind < argc) {
		fprintf (stderr, "%s: bad option %s\n",
				pgmname, argv[optind]);
		exit (2);
	}

	if ((input = ib_alloc(0, 0)) == NULL) {
		perror(pgmname);
		exit(1);
	}
	for (ll=0; ll<PL; ll++)
		page[ll] = 0;

	cp = 0;
	ll = 0;
	greek = 0;
	mustwr = PL;
	line = 0;
	slbuff(0);

	while ((c = nextc(wc, mp, i)) != EOFC) {
		switch (c & CMASK) {
		case '\n':
			incr();
			incr();
			cp = 0;
			continue;

		case '\0':
			continue;

		case ESC:
			c = nextc(wc, mp, i);
			switch (c & CMASK) {
			case '7':	/* reverse full line feed */
				decr();
				decr();
				break;

			case '8':	/* reverse half line feed */
				if (fflag)
					decr();
				else {
					if (--half < -1) {
						decr();
						decr();
						half += 2;
					}
				}
				break;

			case '9':	/* forward half line feed */
				if (fflag)
					incr();
				else {
					if (++half > 0) {
						incr();
						incr();
						half -= 2;
					}
				}
				break;
			default:
				if (pflag) {
					outc(ESC);
					cp++;
					if (c != EOFC) {
						outc(c);
						cp += (c&COLMSK)>>COLSHF;
					}
				}
			}
			continue;

		case SO:
			greek = GREEK;
			continue;

		case SI:
			greek = 0;
			continue;

		case RLF:
			decr();
			decr();
			continue;

		case '\r':
			cp = 0;
			continue;

		case '\t':
			cp = (cp + 8) & -8;
			continue;

		case '\b':
			if (cp > 0)
				cp--;
			continue;

		case ' ':
			cp++;
			continue;

		default:
			if (c & ISPRNT) {/* if printable */
				outc(c | greek);
				cp += (c&COLMSK)>>COLSHF;
			}
			continue;
		}
	}

	for (i=0; i<PL; i++)
		if (page[(mustwr+i)%PL] != 0)
			emit (page[(mustwr+i) % PL], mustwr+i-PL);
	emit (space, (llh + 1) & -2);
	return 0;
}

static void
outc (register long long c)
{
	int i, v, w;
	size_t n;

	if (lp > cp) {
		line = 0;
		lp = 0;
	}

	while (lp < cp) {
		slbuff(0);
		switch (lbuff[line]) {
		case '\0':
			lbuff[line] = ' ' | ISPRNT | (1LL<<COLSHF);
			lp++;
			break;

		case '\b':
			lp--;
			break;

		default:
			lp += (lbuff[line]&COLMSK)>>COLSHF;
			if (lp > cp)
				continue;
		}
		line++;
	}
	slbuff(0);
	while ((lbuff[line] & CMASK) == '\b') {
		line += 2;
		slbuff(0);
	}
	if (bflag || lbuff[line] == '\0' || (lbuff[line]&CMASK) == ' ' &&
			(lbuff[line]&TRANS) == 0) {
		w = (lbuff[line]&COLMSK)>>COLSHF;
		if (lp > cp) {
			w -= lp-cp;
			n = lllen(&lbuff[line]) + 1;
			slbuff(lp-cp+n);
			memmove(&lbuff[line+lp-cp], &lbuff[line],
					n * sizeof *lbuff);
			n = lp - cp;
			while (n--)
				lbuff[line++] = ' '|ISPRNT|(1LL<<COLSHF);
			lp = cp;
		}
		v = (c&COLMSK)>>COLSHF;
		lbuff[line] = c;
		if (w > v) {
			n = lllen(&lbuff[line+w-v]) + 1;
			slbuff(w-v+n);
			memmove(&lbuff[line+w-v], &lbuff[line],
					n * sizeof *lbuff);
			n = w - v;
			while (n--)
				lbuff[++line] = ' '|ISPRNT|(1LL<<COLSHF);
		} else {
			n = 1;
			while (lbuff[line+n] = lbuff[line+v-w+n])
				n++;
		}
	} else {
		if (lp > cp)
			line++;
		slbuff(1);
		w = (lbuff[line++]&COLMSK)>>COLSHF;
		if (lp > cp) {
			w += lp - cp;
			lp = cp;
		}
		v = (c&COLMSK)>>COLSHF;
		n = lllen(&lbuff[line]) + 1;
		slbuff(w+1+n);
		memmove(&lbuff[line+w+1], &lbuff[line],
				n * sizeof *lbuff);
		for (i = 0; i < w; i++) {
			slbuff(0);
			lbuff[line++] = '\b';
		}
		slbuff(0);
		lbuff[line++] = c;
		if (w > v) {
			n = lllen(&lbuff[line+w-v]) + 1;
			slbuff(w-v+n);
			memmove(&lbuff[line+w-v], &lbuff[line],
					n * sizeof *lbuff);
			n = w - v;
			while (n--)
				lbuff[++line] = ' '|ISPRNT|(1LL<<COLSHF)|TRANS;
		} else {
			n = 1;
			while (lbuff[line+n] = lbuff[line+v-w+n])
				n++;
		}
		lp = 0;
		line = 0;
	}
}

static void
store (int lno)
{
	lno %= PL;
	if (page[lno] != 0)
		free (page[lno]);
	page[lno] = malloc((lllen(lbuff) + 2) * sizeof *page[lno]);
	if (page[lno] == 0) {
		fprintf (stderr, "%s: no storage\n", pgmname);
		exit (2);
	}
	llcpy (page[lno],lbuff);
}

static void
fetch(int lno)
{
	register long long *p;
	size_t	n;

	lno %= PL;
	p = lbuff;
	while (*p)
		*p++ = '\0';
	line = 0;
	lp = 0;
	if (page[lno]) {
		n = lllen(page[lno]);
		slbuff(n + 1);
		llcpy (&lbuff[line], page[lno]);
	}
}
static void
emit (long long *s, int lineno)
{
	static int cline = 0;
	register int ncp, i;
	register long long *p;
	static long long gflag = 0;

	if (*s) {
		while (cline < lineno - 1) {
			putchar ('\n');
			pcp = 0;
			cline += 2;
		}
		if (cline != lineno) {
			putchar (ESC);
			putchar ('9');
			cline++;
		}
		if (pcp)
			putchar ('\r');
		pcp = 0;
		p = s;
		while (*p) {
			ncp = pcp;
			while ((*p++&CMASK) == ' ') {
				if ((++ncp & 7) == 0 && xflag) {
					pcp = ncp;
					putchar ('\t');
				}
			}
			if (!*--p)
				break;
			while (pcp < ncp) {
				putchar (' ');
				pcp++;
			}
			if (gflag != (*p & GREEK) && (*p&CMASK) != '\b') {
				if (gflag)
					putchar (SI);
				else
					putchar (SO);
				gflag ^= GREEK;
			}
			if (*p & ILLSEQ)
				putchar((*p&CHRMSK)>>CHRSHF);
			else if (mb_cur_max == 1)
				putchar(*p & 0377);
			else {
				char	mb[MB_LEN_MAX];
				int	n;
				n = wctomb(mb, *p&CMASK);
				for (i = 0; i < n; i++)
					putchar(mb[i] & 0377);
			}
			if ((*p&CMASK) == '\b') {
				pcp--;
			} else
				pcp += (*p&COLMSK)>>COLSHF;
			p++;
		}
	}
}

static void
incr(void)
{
	store (ll++);
	if (ll > llh)
		llh = ll;
	if (ll >= mustwr && page[ll%PL]) {
		emit (page[ll%PL], ll - PL);
		mustwr++;
		free (page[ll%PL]);
		page[ll%PL] = 0;
	}
	fetch (ll);
}

static void
decr(void)
{
	if (ll > mustwr - PL) {
		store (ll--);
		fetch (ll);
	}
}

static size_t
lllen(const long long *lp)
{
	size_t	n = 0;

	while (*lp++)
		n++;
	return n;
}

static long long
*llcpy(long long *dst, const long long *sp)
{
	long long	*dp = dst;

	while (*dp++ = *sp++);
	return dst;
}

static long long *
growsbuff(size_t n)
{
	if ((lbuff = realloc(lbuff, (line+n) * sizeof *lbuff)) == NULL) {
		write(2, "no memory\n", 10);
		_exit(077);
	}
	memset(&lbuff[LINELN], 0, (line+n-LINELN)*sizeof *lbuff);
	LINELN = line + n;
	return lbuff;
}
