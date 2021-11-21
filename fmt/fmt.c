/*
 * This code contains changes by
 * Gunnar Ritter, Freiburg i. Br., Germany, April 2003. All rights reserved.
 *
 * Conditions 1, 2, and 4 and the no-warranty notice below apply
 * to these changes.
 *
 *
 * Copyright (c) 1991
 * 	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 	This product includes software developed by the University of
 * 	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*	from 4.3BSD fmt.c	5.2 (Berkeley) 6/21/85	*/
#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)fmt.sl	1.9 (gritter) 5/29/05";

#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include <ctype.h>
#include <stdlib.h>
#include <libgen.h>
#include <locale.h>

#ifdef	__GLIBC__
#ifdef	_IO_putc_unlocked
#undef	putchar
#define	putchar(c)	_IO_putc_unlocked(c, stdout)
#endif
#endif

#include <iblok.h>
#include <asciitype.h>

/*
 * fmt -- format the concatenation of input files or standard input
 * onto standard output.  Designed for use with Mail ~|
 *
 * Syntax: fmt [ -width ] [ name ... ]
 * Author: Kurt Shoens (UCB) 12/7/78
 */

static int	pfx;			/* Current leading blank count */
static long long	lineno;			/* Current input line */
static int	mark;			/* we saw a head line */
static long	width = 72;		/* Width that we will not exceed */
static int	cflag;			/* crown margin mode */
static int	sflag;			/* split only */
static const char	*progname;	/* argv0 */
static int	mb_cur_max;


static const char	*headnames[] = {"To", "Subject", "Cc", "Bcc", "bcc", 0};

static void	setwidth(const char *);
static void	usage(void);
static void	fmt(struct iblok *);
static void	prefix(const wchar_t *);
static void	split(const wchar_t *);
static void	setout(void);
static void	pack(const wchar_t *);
static void	oflush(void);
static void	tabulate(wchar_t *);
static void	leadin(void);
static int	chkhead(const char *, const wchar_t *);
static int	fromline(const wchar_t *);
static size_t	colwidth(const wchar_t *);
static size_t	colwidthn(const wchar_t *, const wchar_t *);
static void	growibuf(void);
static void	growobuf(void);

/*
 * Drive the whole formatter by managing input files.  Also,
 * cause initialization of the output stuff and flush it out
 * at the end.
 */

int
main(int argc, char **argv)
{
	register struct iblok *fi;
	register int errs = 0, i;

	progname = basename(argv[0]);
	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	setout();
	lineno = 1;
	for (i = 1; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
		if (argv[i][1] == '-' && argv[i][2] == '\0') {
			i++;
			break;
		}
	nopt:	switch (argv[i][1]) {
		case '\0':
			continue;
		case 'c':
			cflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 'w':
			if (argv[i][2]) {
				setwidth(&argv[i][2]);
				continue;
			} else if (i < argc) {
				setwidth(argv[++i]);
				continue;
			} else
				setwidth(NULL);
			break;
		case '0':
		case '1': case '2': case '3':
		case '4': case '5': case '6':
		case '7': case '8': case '9':
			setwidth(&argv[i][1]);
			continue;
		default:
			usage();
			exit(2);
		}
		argv[i]++;
		goto nopt;
	}
	if (i < argc) {
		while (i < argc) {
			if ((fi = ib_open(argv[i], 0)) == NULL) {
				perror(argv[i]);
				errs |= 1;
			} else
				fmt(fi);
			i++;
		}
	} else {
		if ((fi = ib_alloc(0, 0)) == NULL) {
			perror("stdin");
			errs |= 1;
		} else
			fmt(fi);
	}
	oflush();
	exit(errs);
}

static void
setwidth(const char *s)
{
	char	*x;

	if (s == NULL || (width = strtol(s, &x, 10),
				width <= 0 ||
				*x != '\0' || *s == '+' || *s == '-')) {
		usage();
		fprintf(stderr, "       Non-numeric character found "
				"in width specification\n");
		exit(2);
	}
}

static void
usage(void)
{
	fprintf(stderr,
		"usage: %s [-c] [-s] [-w width | -width] [inputfile...]\n",
		progname);
}

static char *
getvalid(struct iblok *ip, wint_t *wp, int *mp)
{
	char	*cp;

	do
		cp = ib_getw(ip, wp, mp);
	while (cp && *wp == WEOF);
	return cp;
}

#define	get(mp, fi, c, m, b)	(mp = mb_cur_max > 1 ? getvalid(fi, &c, &m) : \
		(b = c = ib_get(fi), m = 1, c != (wint_t)EOF ? &b : 0))

static int	ibufsize;
static wchar_t	*linebuf;
static wchar_t	*canonb;

/*
 * Read up characters from the passed input file, forming lines,
 * doing ^H processing, expanding tabs, stripping trailing blanks,
 * and sending each line down for analysis.
 */
static void
fmt(struct iblok *fi)
{
	register int p, p2;
	wint_t c;
	register long col;
	char	*mp;
	int m;
	char	b;

	get(mp, fi, c, m, b);
	while (c != (wint_t)EOF) {
		
		/*
		 * Collect a line, doing ^H processing.
		 * Leave tabs for now.
		 */

		p = 0;
		while (c != '\n' && c != (wint_t)EOF) {
			if (c == '\b') {
				get(mp, fi, c, m, b);
				continue;
			}
			if (!(mb_cur_max > 1 ? iswprint(c) : isprint(c)) &&
					c != '\t') {
				get(mp, fi, c, m, b);
				continue;
			}
			if (p >= ibufsize)
				growibuf();
			linebuf[p++] = c;
			get(mp, fi, c, m, b);
		}
		if (p >= ibufsize)
			growibuf();
		linebuf[p] = '\0';

		/*
		 * Toss anything remaining on the input line.
		 */

		while (c != '\n' && c != (wint_t)EOF)
			get(mp, fi, c, m, b);
		
		/*
		 * Expand tabs on the way to canonb.
		 */

		col = 0;
		p = p2 = 0;
		while (c = linebuf[p++]) {
			if (c != '\t') {
				if (mb_cur_max > 1)
					col += wcwidth(c);
				else
					col++;
				if (p2 >= ibufsize)
					growibuf();
				canonb[p2++] = c;
				continue;
			}
			do {
				if (p2 >= ibufsize)
					growibuf();
				canonb[p2++] = ' ';
				col++;
			} while ((col & 07) != 0);
		}

		/*
		 * Swipe trailing blanks from the line.
		 */

		for (p2--; p2 >= 0 && canonb[p2] == ' '; p2--)
			;
		if (p2 >= ibufsize-1)
			growibuf();
		canonb[++p2] = '\0';
		prefix(canonb);
		if (c != (wint_t)EOF)
			get(mp, fi, c, m, b);
	}
}

/*
 * Take a line devoid of tabs and other garbage and determine its
 * blank prefix.  If the indent changes, call for a linebreak.
 * If the input line is blank, echo the blank line on the output.
 * Finally, if the line minus the prefix is a mail header, try to keep
 * it on a line by itself.
 */

static void
prefix(const wchar_t *line)
{
	register const wchar_t *cp;
	register const char **hp;
	register long np;
	register int h;
	static int	nlpp;	/* number of lines on current paragraph */

	if (wcslen(line) == 0) {
		nlpp = 0;
		oflush();
		putchar('\n');
		mark = 0;
		return;
	}
	for (cp = line; *cp == ' '; cp++)
		;
	np = cp - line;

	/*
	 * The following horrible expression attempts to avoid linebreaks
	 * when the indent changes due to a paragraph.
	 */

	if (!cflag && np != pfx && (np > pfx || abs(pfx-np) > 8))
		oflush();
	if (h = fromline(cp))
		oflush(), mark = 1;
	else if (mark) {
		for (hp = &headnames[0]; *hp != NULL; hp++)
			if (chkhead(*hp, cp)) {
				h = 1;
				oflush();
				break;
			}
	}
	if (!h && (h = (*cp == '.' || sflag)))
		oflush();
	if (!cflag || nlpp < 2)
		pfx = np;
	split(cp);
	if (h)
		oflush();
	nlpp++;
	lineno++;
}

/*
 * Split up the passed line into output "words" which are
 * maximal strings of non-blanks with the blank separation
 * attached at the end.  Pass these words along to the output
 * line packer.
 */

static wchar_t	*word;

static void
split(const wchar_t *line)
{
	register const wchar_t *cp;
	register wchar_t *cp2;

	cp = line;
	while (*cp) {
		cp2 = word;

		/*
		 * Collect a 'word,' allowing it to contain escaped
		 * white space.
		 */

		while (*cp && *cp != ' ') {
			if (*cp == '\\' && iswspace(cp[1]))
				*cp2++ = *cp++;
			*cp2++ = *cp++;
		}

		/*
		 * Guarantee a space at end of line.
		 * Two spaces after end of sentence punctuation.
		 */

		if (*cp == '\0') {
			*cp2++ = ' ';
			if (strchr(".:!?", cp[-1]))
				*cp2++ = ' ';
		}
		while (*cp == ' ')
			*cp2++ = *cp++;
		*cp2 = '\0';
		pack(word);
	}
}

/*
 * Output section.
 * Build up line images from the words passed in.  Prefix
 * each line with correct number of blanks.  The buffer "outbuf"
 * contains the current partial line image, including prefixed blanks.
 * "outp" points to the next available space therein.  When outp is NOSTR,
 * there ain't nothing in there yet.  At the bottom of this whole mess,
 * leading tabs are reinserted.
 */

static int	obufsize;
static wchar_t	*outbuf;		/* Sandbagged output line image */
static wchar_t	*outp;			/* Pointer in above */

/*
 * Initialize the output section.
 */

static void
setout(void)
{
	outp = NULL;
}

/*
 * Pack a word onto the output line.  If this is the beginning of
 * the line, push on the appropriately-sized string of blanks first.
 * If the word won't fit on the current line, flush and begin a new
 * line.  If the word is too long to fit all by itself on a line,
 * just give it its own and hope for the best.
 */

static void
pack(const wchar_t *word)
{
	register const wchar_t *cp;
	register long s, t;

	if (outp == NULL)
		leadin();
	t = colwidth(word);
	s = colwidthn(outbuf, outp);
	if (t+s <= width) {
		
		/*
		 * In like flint!
		 */

		for (cp = word; *cp; cp++) {
			if (outp >= &outbuf[obufsize])
				growobuf();
			*outp++ = *cp;
		}
		return;
	}
	if (s > pfx) {
		oflush();
		leadin();
	}
	for (cp = word; *cp; cp++) {
		if (outp >= &outbuf[obufsize])
			growobuf();
		*outp++ = *cp;
	}
}

/*
 * If there is anything on the current output line, send it on
 * its way.  Set outp to NULL to indicate the absence of the current
 * line prefix.
 */

static void
oflush(void)
{
	if (outp == NULL)
		return;
	if (outp >= &outbuf[obufsize])
		growobuf();
	*outp = '\0';
	tabulate(outbuf);
	outp = NULL;
}

/*
 * Take the passed line buffer, insert leading tabs where possible, and
 * output on standard output (finally).
 */

static void
tabulate(wchar_t *line)
{
	register wchar_t *cp;
	register int b, t;

	/*
	 * Toss trailing blanks in the output line.
	 */

	cp = line + wcslen(line) - 1;
	while (cp >= line && *cp == ' ')
		cp--;
	*++cp = '\0';
	
	/*
	 * Count the leading blank space and tabulate.
	 */

	for (cp = line; *cp == ' '; cp++)
		;
	b = cp-line;
	t = b >> 3;
	b &= 07;
	if (t > 0)
		do
			putchar('\t');
		while (--t);
	if (b > 0)
		do
			putchar(' ');
		while (--b);
	while (*cp) {
		if (mb_cur_max > 1 && *cp & ~(wchar_t)0177) {
			char	mb[MB_LEN_MAX];
			int	i, n;
			n = wctomb(mb, *cp);
			for (i = 0; i < n; i++)
				putchar(mb[i]);
		} else
			putchar(*cp);
		cp++;
	}
	putchar('\n');
}

/*
 * Initialize the output line with the appropriate number of
 * leading blanks.
 */

static void
leadin(void)
{
	register long b;

	if (outbuf == 0)
		growobuf();
	for (b = 0; b < pfx; b++) {
		if (b >= obufsize)
			growobuf();
		outbuf[b] = ' ';
	}
	outp = &outbuf[b];
}

/*
 * Is s2 the mail header field name s1?
 */

static int
chkhead(register const char *s1, register const wchar_t *s2)
{

	while (*s1 && *s1++ == *s2++);
	if (*s1 != '\0')
		return 0;
	return 1;
}

/*
 * Sloppy recognition of Unix From_ lines (not according to the POSIX.2
 * mailx specification, but oriented on actual Unix tradition). We match
 * the ERE
 * ^From .* [A-Z][a-z][a-z] [A-Z][a-z][a-z] \
 * [0-9 ]?[0-9] [0-9][0-9]:[0-9][0-9]:[0-9][0-9]
 */

static int
fromline(const wchar_t *cp)
{
	if (cp[0] != 'F' || cp[1] != 'r' || cp[2] != 'o' || cp[3] != 'm' ||
			cp[4] != ' ')
		return 0;
	cp += 5;
	while (*cp && *cp != ' ')
		cp++;
	if (*cp++ != ' ')
		return 0;
	if (!upperchar(cp[0]) || !lowerchar(cp[1]) || !lowerchar(cp[2]) ||
			cp[3] != ' ' ||
	    !upperchar(cp[4]) || !lowerchar(cp[5]) || !lowerchar(cp[6]) ||
	    		cp[7] != ' ')
		return 0;
	cp += 8;
	if (digitchar(*cp) || *cp == ' ')
		cp++;
	if (!digitchar(cp[0]) || cp[1] != ' '||
			!digitchar(cp[2]) || !digitchar(cp[3]) ||
				cp[4] != ':' ||
			!digitchar(cp[5]) || !digitchar(cp[6]) ||
				cp[7] != ':' ||
			!digitchar(cp[8]) || !digitchar(cp[9]))
		return 0;
	return 1;
}

static size_t
colwidth(const wchar_t *cp)
{
	size_t	n = 0;

	if (mb_cur_max > 1)
		while (*cp)
			n += wcwidth(*cp++);
	else
		n = wcslen(cp);
	return n;
}

static size_t
colwidthn(const wchar_t *bot, const wchar_t *top)
{
	size_t	n = 0;

	if (mb_cur_max > 1)
		while (bot < top)
			n += wcwidth(*bot++);
	else
		n = top - bot;
	return n;
}

static void
growibuf(void)
{
	ibufsize += 128;
	if ((word = realloc(word, ibufsize * sizeof *word)) == 0 ||
	    (linebuf = realloc(linebuf, ibufsize * sizeof *linebuf)) == 0 ||
	    (canonb = realloc(canonb, ibufsize * sizeof *canonb)) == 0) {
		fprintf(stderr, "%s: input line too long\n", progname);
		exit(1);
	}
}

static void
growobuf(void)
{
	int	diff = 0;

	if (outp != NULL)
		diff = outp - outbuf;
	obufsize += 128;
	if ((outbuf = realloc(outbuf, obufsize * sizeof *outbuf)) == 0) {
		fprintf(stderr, "%s: output line too long\n", progname);
		exit(1);
	}
	if (outp != NULL)
		outp = &outbuf[diff];
}
