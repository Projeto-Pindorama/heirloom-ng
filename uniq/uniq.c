/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, March 2003.
 */
/*	from Unix 32V /usr/src/cmd/uniq.c	*/
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
static const char sccsid[] USED = "@(#)uniq.sl	1.10 (gritter) 5/29/05";

/*
 * Deal with duplicated lines in a file
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <wchar.h>
#include <wctype.h>
#include <iblok.h>
#include <locale.h>
#include <unistd.h>
#include <blank.h>
#include <libgen.h>

static int	fields;
static int	letters;
static int	linec;
static char	mode;
static int	uniq;
static struct iblok	*ip;
static int	mb_cur_max;
static const char	*progname;

static void	pline(const char *, size_t);
static int	equal(const char *, size_t, const char *, size_t);
static const char	*skip(const char *, size_t *);
static void	printe(const char *, const char *);
static void	*srealloc(void *, size_t);

#define	next(wc, s, n)	(*(s) & 0200 ? ((n) = mbtowc(&(wc), (s), mb_cur_max), \
		 (n) = ((n) > 0 ? (n) : (n) < 0 ? (wc=WEOF, 1) : 1)) : \
	((wc) = *(s) & 0377, (n) = 1))

static void
missing(int c)
{
	fprintf(stderr, "%s: missing argument to -%c option\n",
			progname, c);
	exit(2);
}

int
main(int argc, char **argv)
{
	static char *b1, *b2;
	static size_t	s1 = 0, s2 = 0, l1, l2;

	progname = basename(argv[0]);
	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	while(argc > 1) {
		if(argv[1][0] == '-') {
			if (argv[1][1] == '-' && argv[1][2] == '\0') {
				if (argv[2]) {
					argc--;
					argv++;
					goto op;
				}
				break;
			}
		optc:	if (isdigit(argv[1][1]))
				fields = atoi(&argv[1][1]);
			else switch (argv[1][1]) {
			case 'f':
				if (argv[1][2])
					fields = atoi(&argv[1][2]);
				else if (argv[2]) {
					fields = atoi(argv[2]);
					argc--;
					argv++;
				} else
					missing('f');
				break;
			case 's':
				if (argv[1][2])
					letters = atoi(&argv[1][2]);
				else if (argv[2]) {
					letters = atoi(argv[2]);
					argc--;
					argv++;
				} else
					missing('s');
				break;
			default:
				mode = argv[1][1];
				if (argv[1][2]) {
					(argv[1])++;
					goto optc;
				}
			}
			argc--;
			argv++;
			continue;
		}
		if(*argv[1] == '+') {
			letters = atoi(&argv[1][1]);
			argc--;
			argv++;
			continue;
		}
	op:	if ((ip = ib_open(argv[1], 0)) == NULL)
			printe("cannot open %s\n", argv[1]);
		break;
	}
	if (ip == NULL)
		ip = ib_alloc(0, 0);
	if(argc > 2 && freopen(argv[2], "w", stdout) == NULL)
		printe("cannot create %s\n", argv[2]);

	if ((l1 = ib_getlin(ip, &b1, &s1, srealloc)) == 0)
		exit(0);
	if (b1[l1-1] == '\n')
		b1[--l1] = '\0';
	for(;;) {
		linec++;
		if ((l2 = ib_getlin(ip, &b2, &s2, srealloc)) == 0) {
			pline(b1, l1);
			exit(0);
		}
		if (b2[l2-1] == '\n')
			b2[--l2] = '\0';
		if(!equal(b1, l1, b2, l2)) {
			pline(b1, l1);
			linec = 0;
			do {
				linec++;
				if ((l1 = ib_getlin(ip, &b1, &s1,
							srealloc)) == 0) {
					pline(b2, l2);
					exit(0);
				}
				if (b1[l1-1] == '\n')
					b1[--l1] = '\0';
			} while(equal(b1, l1, b2, l2));
			pline(b2, l2);
			linec = 0;
		}
	}
}

static void
pline(register const char *buf, size_t len)
{

	switch(mode) {

	case 'u':
		if(uniq) {
			uniq = 0;
			return;
		}
		break;

	case 'd':
		if(uniq) break;
		return;

	case 'c':
		printf("%4d ", linec);
	}
	uniq = 0;
	fwrite(buf, sizeof *buf, len, stdout);
	putchar('\n');
}

static int
equal(register const char *b1, size_t l1, register const char *b2, size_t l2)
{
	register char c;

	b1 = skip(b1, &l1);
	b2 = skip(b2, &l2);
	while(c = *b1++, l1-- != 0)
		if(l2--, c != *b2++) return(0);
	if(l2 != 0)
		return(0);
	uniq++;
	return(1);
}

static const char *
skip(register const char *s, size_t *l)
{
	register int nf, nl;
	wchar_t	wc;
	int	n;

	nf = nl = 0;
	if (mb_cur_max > 1) {
		while (nf++ < fields) {
			while (next(wc, s, n), iswblank(wc)) {
				s += n;
				*l -= n;
			}
			while (next(wc, s, n), !(iswblank(wc) || *l == 0)) {
				s += n;
				*l -= n;
			}
		}
		while (nl++ < letters && *l != 0)
			next(wc, s, n), s += n, *l -= n;
	} else {
		while (nf++ < fields) {
			while (isblank(*s & 0377))
				s++, (*l)--;
			while (!(isblank(*s & 0377) || *l == 0))
				s++, (*l)--;
		}
		while (nl++ < letters && *l != 0)
			s++, (*l)--;
	}
	return(s);
}

static void
printe(const char *p, const char *s)
{
	fprintf(stderr, p, s);
	exit(1);
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
