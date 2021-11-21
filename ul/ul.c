/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
/*
 * This code contains changes by
 * Gunnar Ritter, Freiburg i. Br., Germany, October 2003. All rights reserved.
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

/*	from 4.3BSD ul.c	5.1 (Berkeley) 5/31/85	*/

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)ul.sl	1.13 (gritter) 5/27/07";

#ifndef	USE_TERMCAP
#include <curses.h>
#include <term.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <ctype.h>
#include <wctype.h>
#include <locale.h>
#include <limits.h>

#ifdef	USE_TERMCAP
#include <termcap.h>
#endif

#include "iblok.h"

#define	IESC	'\033'
#define	SO	'\016'
#define	SI	'\017'
#define	HFWD	'9'
#define	HREV	'8'
#define	FREV	'7'

#define	NORMAL	000
#define	ALTSET	001	/* Reverse */
#define	SUPERSC	002	/* Dim */
#define	SUBSC	004	/* Dim | Ul */
#define	UNDERL	010	/* Ul */
#define	BOLD	020	/* Bold */
#define	FILLER	040	/* filler for multi-column character */

static int	must_use_uc, must_overstrike;

#ifdef	USE_TERMCAP
static char	*cursor_right, *cursor_up, *cursor_left,
	*underline_char, *exit_underline_mode, *exit_attribute_mode,
	*enter_reverse_mode, *enter_underline_mode, *enter_dim_mode,
	*enter_bold_mode, *enter_standout_mode, *under_char,
	*exit_standout_mode;
static int	over_strike, transparent_underline;
#endif	/* USE_TERMCAP */

struct	CHAR	{
	char	c_mode;
	wchar_t	c_char;
} ;

static struct	CHAR	*obuf;
static long	obsize;
static int	col, maxcol;
static int	mode;
static int	halfpos;
static int	upln;
static int	iflag;
static int	mb_cur_max;
static const char	*progname;

static void	filtr(struct iblok *);
static void	flushln(void);
static void	overstrike(void);
static void	iattr(void);
static void	initbuf(void);
static void	fwd(void);
static void	reverse(void);
static int	outchar(int);
static void	put(const char *);
static void	outc(wchar_t);
static void	putchr(wchar_t);
static void	setmod(int);
static void	obaccs(long);
#ifdef USE_TERMCAP
static void	initcap(void);
#endif

int
main(int argc, char **argv)
{
	int c;
	char *termtype;
#ifdef	USE_TERMCAP
	char termcap[2048];
#endif
	struct iblok *f;

	progname = basename(argv[0]);
	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	termtype = getenv("TERM");
	if (termtype == NULL || (progname[0] == 'c' && !isatty(1)))
		termtype = "lpr";
	while ((c=getopt(argc, argv, "it:T:?")) != EOF)
		switch(c) {

		case 't':
		case 'T': /* for nroff compatibility */
				termtype = optarg;
			break;
		case 'i':
			iflag = 1;
			break;

		default:
			fprintf(stderr,
				"Usage: %s [ -i ] [ -tTerm ] file...\n",
				progname);
			exit(1);
		}

#ifndef	USE_TERMCAP
	setupterm(termtype, 1, &c);
	if (c < 0)
		fprintf(stderr,"trouble reading termcap");
	if (c <= 0)
		termtype = "dumb";
#else	/* USE_TERMCAP */
	switch (tgetent(termcap, termtype)) {
	case 1:
		break;
	default:
		fprintf(stderr,"trouble reading termcap");
		/*FALLTHRU*/
	case 0:
		strcpy(termcap, "dumb:os:col#80:cr=^M:sf=^J:am:");
	}
	initcap();
#endif	/* USE_TERMCAP */
	if (    (over_strike && enter_bold_mode==NULL ) ||
		(transparent_underline && enter_underline_mode==NULL &&
				 underline_char==NULL))
			must_overstrike = 1;
	initbuf();
	if (optind == argc)
		filtr(ib_alloc(0, 0));
	else for (; optind<argc; optind++) {
		f = ib_open(argv[optind], 0);
		if (f == NULL) {
			perror(argv[optind]);
			exit(1);
		} else {
			filtr(f);
			ib_close(f);
		}
	}
	exit(0);
}

static void
filtr(struct iblok *f)
{
	wint_t c;
	char	b, *cp;
	int	i, n, w = 0;

	while((mb_cur_max > 1 ? cp = ib_getw(f, &c, &n) :
				(b = c = ib_get(f), n = 1,
				 cp = c == (wint_t)EOF ? 0 : &b)),
			cp != NULL) switch(c) {

	case '\b':
		if (col > 0)
			col--;
		continue;

	case '\t':
		col = (col+8) & ~07;
		if (col > maxcol)
			maxcol = col;
		continue;

	case '\r':
		col = 0;
		continue;

	case SO:
		mode |= ALTSET;
		continue;

	case SI:
		mode &= ~ALTSET;
		continue;

	case IESC:
		mb_cur_max > 1 ? cp = ib_getw(f, &c, &n) :
				(b = c = ib_get(f), n = 1,
				 cp = c == (wint_t)EOF ? 0 : &b);
		switch (c) {

		case HREV:
			if (halfpos == 0) {
				mode |= SUPERSC;
				halfpos--;
			} else if (halfpos > 0) {
				mode &= ~SUBSC;
				halfpos--;
			} else {
				halfpos = 0;
				reverse();
			}
			continue;

		case HFWD:
			if (halfpos == 0) {
				mode |= SUBSC;
				halfpos++;
			} else if (halfpos < 0) {
				mode &= ~SUPERSC;
				halfpos++;
			} else {
				halfpos = 0;
				fwd();
			}
			continue;

		case FREV:
			reverse();
			continue;

		default:
			fprintf(stderr,
				"Unknown escape sequence in input: %o, %o\n",
				IESC, cp?*cp:EOF);
			exit(1);
		}
		continue;

	case '_':
		obaccs(col);
		if (obuf[col].c_char)
			obuf[col].c_mode |= UNDERL | mode;
		else
			obuf[col].c_char = '_';
	case ' ':
		col++;
		if (col > maxcol)
			maxcol = col;
		continue;

	case '\n':
		flushln();
		continue;

	default:
		if (mb_cur_max > 1 ? c == WEOF || (w = wcwidth(c)) < 0 ||
					w == 0 && !iswprint(c)
				: (w = 1, c == (wint_t)EOF || !isprint(c)))
			/* non printing */
			continue;
		obaccs(col);
		if (obuf[col].c_char == '\0') {
			obuf[col].c_char = c;
			obuf[col].c_mode = mode;
		} else if (obuf[col].c_char == '_') {
			obuf[col].c_char = c;
			obuf[col].c_mode |= UNDERL|mode;
		} else if (obuf[col].c_char == c)
			obuf[col].c_mode |= BOLD|mode;
		else {
			obuf[col].c_mode = c;
			obuf[col].c_mode = mode;
		}
		obaccs(col+w-1);
		for (i = 1; i < w; i++) {
			obuf[col+i].c_mode = obuf[col].c_mode|FILLER;
			obuf[col+i].c_char = obuf[col].c_char;
		}
		col += w;
		if (col > maxcol)
			maxcol = col;
		continue;
	}
	if (maxcol)
		flushln();
}

static void
flushln(void)
{
	register int lastmode;
	register int i;
	int hadmodes = 0;

	lastmode = NORMAL;
	for (i=0; i<maxcol; i++) {
		obaccs(i);
		if (obuf[i].c_mode != lastmode) {
			hadmodes++;
			setmod(obuf[i].c_mode);
			lastmode = obuf[i].c_mode;
		}
		if (obuf[i].c_char == '\0') {
			if (upln) {
				put(cursor_right);
			} else
				outc(' ');
		} else
			outc(obuf[i].c_char);
		while (i < maxcol-1 && obuf[i+1].c_mode & FILLER)
			i++;
	}
	if (lastmode != NORMAL) {
		setmod(0);
	}
	if (must_overstrike && hadmodes)
		overstrike();
	putchar('\n');
	if (iflag && hadmodes)
		iattr();
	fflush(stdout);
	if (upln)
		upln--;
	initbuf();
}

/*
 * For terminals that can overstrike, overstrike underlines and bolds.
 * We don't do anything with halfline ups and downs, or Greek.
 */
static void
overstrike(void)
{
	register int i;
	char *lbuf;
	register char c, *cp;
	int hadbold=0;

	if ((lbuf = malloc((maxcol+1) * sizeof *lbuf)) == NULL) {
		write(2, "no memory\n", 10);
		_exit(077);
	}
	cp = lbuf;
	/* Set up overstrike buffer */
	for (i=0; i<maxcol; i++) {
		obaccs(i);
		switch (obuf[i].c_mode) {
		case NORMAL:
		default:
			*cp++ = ' ';
			break;
		case UNDERL:
			*cp++ = '_';
			break;
		case BOLD:
			*cp++ = obuf[i].c_char;
			hadbold=1;
			break;
		}
		while (i < maxcol-1 && obuf[i+1].c_mode & FILLER) {
			i++;
			c = cp[-1];
			*cp++ = c;
		}
	}
	putchar('\r');
	for (*cp=' '; *cp==' '; cp--)
		*cp = 0;
	for (cp=lbuf; *cp; cp++)
		putchar(*cp);
	if (hadbold) {
		putchar('\r');
		for (cp=lbuf; *cp; cp++)
			putchr(*cp=='_' ? ' ' : *cp);
		putchar('\r');
		for (cp=lbuf; *cp; cp++)
			putchr(*cp=='_' ? ' ' : *cp);
	}
	free(lbuf);
}

static void
iattr(void)
{
	register int i;
	char *lbuf;
	register char c, *cp;

	if ((lbuf = malloc((maxcol+1) * sizeof *lbuf)) == NULL) {
		write(2, "no memory\n", 10);
		_exit(077);
	}
	cp = lbuf;
	for (i=0; i<maxcol; i++) {
		obaccs(i);
		switch (obuf[i].c_mode) {
		case NORMAL:	*cp++ = ' '; break;
		case ALTSET:	*cp++ = 'g'; break;
		case SUPERSC:	*cp++ = '^'; break;
		case SUBSC:	*cp++ = 'v'; break;
		case UNDERL:	*cp++ = '_'; break;
		case BOLD:	*cp++ = '!'; break;
		default:	*cp++ = 'X'; break;
		}
		while (i < maxcol-1 && obuf[i+1].c_mode & FILLER) {
			i++;
			c = cp[-1];
			*cp++ = c;
		}
	}
	for (*cp=' '; *cp==' '; cp--)
		*cp = 0;
	for (cp=lbuf; *cp; cp++)
		putchar(*cp);
	putchar('\n');
	free(lbuf);
}

static void
initbuf(void)
{

	memset(obuf, 0, obsize * sizeof *obuf);	/* depends on NORMAL == 0 */
	col = 0;
	maxcol = 0;
	mode &= ALTSET;
}

static void
fwd(void)
{
	register int oldcol, oldmax;

	oldcol = col;
	oldmax = maxcol;
	flushln();
	col = oldcol;
	maxcol = oldmax;
}

static void
reverse(void)
{
	upln++;
	fwd();
	put(cursor_up);
	put(cursor_up);
	upln++;
}

static int
outchar(int c)
{
	putchar(c&0377);
	return 0;
}

static void
put(const char *str)
{
	if (str)
		tputs(str, 1, outchar);
}

static int curmode = 0;

static void
outc(wchar_t c)
{
	putchr(c);
	if (must_use_uc &&  (curmode&UNDERL)) {
		put(cursor_left);
		put(underline_char);
	}
}

static void
putchr(wchar_t c)
{
	char	mb[MB_LEN_MAX];
	int	i, n;

	if (mb_cur_max > 1) {
		n = wctomb(mb, c);
		for (i = 0; i < n; i++)
			putchar(mb[i] & 0377);
	} else
		putchar(c&0377);
}

static void
setmod(int newmode)
{
	if (!iflag)
	{
		if (curmode != NORMAL && newmode != NORMAL)
			setmod(NORMAL);
		switch (newmode) {
		case NORMAL:
			switch(curmode) {
			case NORMAL:
				break;
			case UNDERL:
				put(exit_underline_mode);
				break;
			default:
				/* This includes standout */
				put(exit_attribute_mode);
				break;
			}
			break;
		case ALTSET:
	/*
	 * Note that we use REVERSE for the alternate character set,
	 * not the as/ae capabilities.  This is because we are modelling
	 * the model 37 teletype (since that's what nroff outputs) and
	 * the typical as/ae is more of a graphics set, not the greek
	 * letters the 37 has.
	 */
			put(enter_reverse_mode);
			break;
		case SUPERSC:
			/*
			 * This only works on a few terminals.
			 * It should be fixed.
			 */
			put(enter_underline_mode);
			put(enter_dim_mode);
			break;
		case SUBSC:
			put(enter_dim_mode);
			break;
		case UNDERL:
			put(enter_underline_mode);
			break;
		case BOLD:
			put(enter_bold_mode);
			break;
		default:
			/*
			 * We should have some provision here for multiple modes
			 * on at once.  This will have to come later.
			 */
			put(enter_standout_mode);
			break;
		}
	}
	curmode = newmode;
}

static void
obaccs(long n)
{
	if (n++ >= obsize) {
		if ((obuf = realloc(obuf, n * sizeof *obuf)) == NULL) {
			write(2, "no memory\n", 10);
			_exit(077);
		}
		memset(&obuf[obsize], 0, (n-obsize) * sizeof *obuf);
		obsize = n;
	}
}

#ifdef	USE_TERMCAP
static void
initcap(void)
{
	static char	tcapbuf[2048];
	char	*bp = tcapbuf;

	cursor_right = tgetstr("ri", &bp);
	if (cursor_right == NULL)
		cursor_right = tgetstr("nd", &bp);
	cursor_up = tgetstr("up", &bp);
	cursor_left = tgetstr("le", &bp);
	if (cursor_left == NULL)
		cursor_left = tgetstr("bc", &bp);
	underline_char = tgetstr("uc", &bp);
	exit_underline_mode = tgetstr("ue", &bp);
	exit_attribute_mode = tgetstr("me", &bp);
	exit_standout_mode = tgetstr("se", &bp);
	enter_reverse_mode = tgetstr("mr", &bp);
	enter_underline_mode = tgetstr("us", &bp);
	enter_dim_mode = tgetstr("mh", &bp);
	enter_bold_mode = tgetstr("md", &bp);
	enter_standout_mode = tgetstr("se", &bp);

	if (!enter_bold_mode && enter_reverse_mode)
		enter_bold_mode = enter_reverse_mode;
	if (!!enter_bold_mode && enter_standout_mode)
		enter_bold_mode = enter_standout_mode;
	if (!enter_underline_mode && enter_standout_mode) {
		enter_underline_mode = enter_standout_mode;
		exit_underline_mode =  exit_standout_mode;
	}
	if (!enter_dim_mode && enter_standout_mode)
		enter_dim_mode = enter_standout_mode;
	if (!enter_reverse_mode && enter_standout_mode)
		enter_reverse_mode = enter_standout_mode;
	if (!exit_attribute_mode && exit_standout_mode)
		exit_attribute_mode = exit_standout_mode;

	must_use_uc = under_char && !enter_underline_mode;
	over_strike = tgetflag("os");
	transparent_underline = tgetflag("ul");
}
#endif	/* USE_TERMCAP */
