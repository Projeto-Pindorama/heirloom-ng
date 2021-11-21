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

/*	from 4.3BSD more.c	5.4 (Berkeley) 4/3/86	*/
#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)more.sl	1.33 (gritter) 5/29/05";

/*
** more.c - General purpose tty output filter and file perusal program
**
**	by Eric Shienbrood, UC Berkeley
**
**	modified by Geoff Peck, UCB to add underlining, single spacing
**	modified by John Foderaro, UCB to add -c and MORE environment variable
*/

#ifndef	USE_TERMCAP
#include <curses.h>
#include <term.h>
#endif	/* USE_TERMCAP */
#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <signal.h>
#include "sigset.h"
#include <errno.h>
#include <termios.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <locale.h>
#include <libgen.h>
#include <wchar.h>
#include <wctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pathconf.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

#ifdef	USE_TERMCAP
#include <termcap.h>
#endif

#include <regex.h>

#include <mbtowi.h>

#ifdef	__GLIBC__
#ifdef	_IO_getc_unlocked
#undef	getc
#define	getc(f)	_IO_getc_unlocked(f)
#endif	/* _IO_getc_unlocked */
#ifdef	_IO_putc_unlocked
#undef	putchar
#define	putchar(c)	_IO_putc_unlocked(c, stdout)
#endif	/* _IO_putc_unlocked */
#endif	/* __GLIBC__ */

#define	getline(f, l)	xxgetline(f, l)	/* avoid glibc _GNU_SOURCE collision */

#define Fopen(s,m)	(Currline = 0,file_pos=0,fopen(s,m))
#define Ftell(f)	file_pos
#define Fseek(f,off)	(file_pos=off,fseeko(f,off,0))
#define Getc(f)		(++file_pos, getc(f))
#define Ungetc(c,f)	(--file_pos, ungetc(c,f))

#define TBUFSIZ	2048
static int	LINSIZ;
#define ctrl(letter)	(letter & 077)
#define RUBOUT	'\177'
#define ESC	'\033'
#define QUIT	'\034'

static struct termios	otty, savtty;
static off_t	file_pos, file_size;
static int	fnum, no_intty, no_tty;
static int	dum_opt, dlines;
static int	nscroll = 11;	/* Number of lines scrolled by 'd' */
static int	fold_opt = 1;	/* Fold long lines */
static int	stop_opt = 1;	/* Stop after form feeds */
static int	ssp_opt = 0;	/* Suppress white space */
static int	ul_opt = 1;	/* Underline as best we can */
static int	promptlen;
static int	Currline;	/* Line we are currently at */
static int	startup = 1;
static int	firstf = 1;
static int	notell = 1;
static int	docrterase = 0;
static int	docrtkill = 0;
static int	bad_so;	/* True if overwriting does not turn off standout */
static int	inwait, Pause, errors;
static int	within;	/* true if we are within a file,
			false if we are between files */
static int	hard, dumb, noscroll, hardtabs, clreol;
static int	catch_susp;	/* We should catch the SIGTSTP signal */
static char	**fnames;	/* The list of file names */
static int	nfiles;		/* Number of files left to process */
static char	*shell;		/* The name of the shell to use */
static int	shellp;		/* A previous shell command exists */
static char	ch;
static jmp_buf	restore;
static char	*Line;	/* Line buffer */
static int	Lpp = 24;	/* lines per page */
static char	*Clear;		/* clear screen */
static char	*eraseln;	/* erase line */
static char	*Senter, *Sexit;/* enter and exit standout mode */
static char	*ULenter, *ULexit;	/* enter and exit underline mode */
static char	*chUL;		/* underline character */
static char	*chBS;		/* backspace character */
static char	*Home;		/* go to home */
static char	*cursorm;	/* cursor movement */
static char	cursorhome[40];	/* contains cursor movement to home */
static char	*EodClr;	/* clear rest of screen */
static int	Mcol = 80;	/* number of columns */
static int	Wrap = 1;	/* set if automargins */
static int	soglitch;	/* terminal has standout mode glitch */
static int	ulglitch;	/* terminal has underline mode glitch */
static int	pstate = 0;	/* current UL state */
static int	mb_cur_max;
static int	rflag, wflag;
static const char	*progname;
static struct {
    off_t chrctr, line;
} context, screen_start;
#ifndef	__hpux
extern char	PC;		/* pad character */
#endif

static const char helptext[] ="\
Most commands optionally preceded by integer argument k.  Defaults in brackets.\n\
Star (*) indicates argument becomes new default.\n\
-------------------------------------------------------------------------------\n\
<space>			Display next k lines of text [current screen size]\n\
z			Display next k lines of text [current screen size]*\n\
<return>		Display next k lines of text [1]*\n\
d or ctrl-D		Scroll k lines [current scroll size, initially 11]*\n\
q or Q or <interrupt>	Exit from more\n\
s			Skip forward k lines of text [1]\n\
f			Skip forward k screenfuls of text [1]\n\
b or ctrl-B		Skip backwards k screenfuls of text [1]\n\
'			Go to place where previous search started\n\
=			Display current line number\n\
/<regular expression>	Search for kth occurrence of regular expression [1]\n\
n			Search for kth occurrence of last r.e [1]\n\
!<cmd> or :!<cmd>	Execute <cmd> in a subshell\n\
v			Start up /usr/ucb/vi at current line\n\
ctrl-L			Redraw screen\n\
:n			Go to kth next file [1]\n\
:p			Go to kth previous file [1]\n\
:f			Display current file name and line number\n\
.			Repeat previous command\n\
-------------------------------------------------------------------------------\n";

static void	argscan(const char *);
static FILE	*checkf(register const char *, int *);
static int	putch(int);
static void	screen(register FILE *, register int);
static void	onquit(int);
static void	chgwinsz(int);
static void	end_it(int);
static void	copy_file(register FILE *);
static int	printd(int);
static void	scanstr(int, char *);
static void	Sprintf(int);
static void	prompt(const char *);
static int	getline(register FILE *, int *);
static void	eras(register int);
static void	kill_line(void);
static void	cleareol(void);
static void	clreos(void);
static int	pr(const char *);
static void	prbuf(register const char *, register int);
static void	doclear(void);
static void	home(void);
static int	command(const char *, register FILE *);
static int	colon(const char *, int, int);
static int	number(char *);
static void	do_shell(const char *);
static void	search(char *, FILE *, register int);
static void	execute(const char *, const char *, const char *, const char *,
			const char *, const char *);
static void	skiplns(register int, register FILE *);
static void	skipf(register int);
static void	initterm(void);
static char	readch(void);
static void	ttyin(char **, register int *, char);
static int	expand(char **, size_t *, const char *);
static void	show(register char);
static void	error(const char *);
static void	set_tty(void);
static void	reset_tty(void);
static void	rdline(register FILE *);
static void	onsusp(int);
static void	*srealloc(void *, size_t);
static void	*smalloc(size_t);

int
main(int argc, char **argv)
{
    FILE		*f;
    char		*s;
    int			ch;
    int			left, i;
    int			prnames = 0;
    int			initopt = 0;
    int			srchopt = 0;
    int			clearit = 0;
    int			initline = 0;
    char		*initbuf = NULL;
    int			initsize = 0;

#ifdef	__GNUC__
    (void)&f;
    (void)&left;
    (void)&prnames;
    (void)&initopt;
    (void)&srchopt;
    (void)&initline;
    (void)&initbuf;
#endif	/* __GNUC__ */
    setlocale(LC_CTYPE, "");
    mb_cur_max = MB_CUR_MAX;
    progname = basename(argv[0]);
    nfiles = argc;
    fnames = argv;
    initterm ();
    nscroll = Lpp/2 - 1;
    if (nscroll <= 0)
	nscroll = 1;
    Line = smalloc(LINSIZ=256);
    if(s = getenv("MORE")) argscan(s);
    while (--nfiles > 0) {
	if ((ch = (*++fnames)[0]&0377) == '-') {
	    argscan(*fnames+1);
	}
	else if (ch == '+') {
	    s = *fnames;
	    if (*++s == '/') {
		srchopt++;
		for (++s, i = 0; *s != '\0';) {
		    if (i >= initsize)
			initbuf = srealloc(initbuf, initsize = i+1);
		    initbuf[i++] = *s++;
		}
		if (i >= initsize)
		    initbuf = srealloc(initbuf, initsize = i+1);
		initbuf[i] = '\0';
	    }
	    else {
		initopt++;
		for (initline = 0; *s != '\0'; s++)
		    if (isdigit (*s))
			initline = initline*10 + *s -'0';
		--initline;
	    }
	}
	else break;
    }
    /* allow clreol only if Home and eraseln and EodClr strings are
     *  defined, and in that case, make sure we are in noscroll mode
     */
    if(clreol)
    {
        if((Home == NULL) || (*Home == '\0') ||
	   (eraseln == NULL) || (*eraseln == '\0') ||
           (EodClr == NULL) || (*EodClr == '\0') )
	      clreol = 0;
	else noscroll = 1;
    }
    if (dlines == 0)
	dlines = Lpp - (noscroll ? 1 : 2);
    left = dlines;
    if (nfiles > 1)
	prnames++;
    if (!no_intty && nfiles == 0) {
	fprintf(stderr, "Usage: %s [-dflsucrw] [-n] "
			"[+linenum | +/pattern] name1 name2 ...\n", progname);
	reset_tty();
	exit(1);
    }
    else
	f = stdin;
    if (!no_tty) {
	sigset(SIGQUIT, onquit);
	sigset(SIGINT, end_it);
	sigset(SIGWINCH, chgwinsz);
	if (sigset (SIGTSTP, SIG_IGN) == SIG_DFL) {
	    sigset(SIGTSTP, onsusp);
	    catch_susp++;
	}
	tcgetattr(fileno(stderr), &otty);
    }
    if (no_intty) {
	if (no_tty)
	    copy_file (stdin);
	else {
	    if ((ch = Getc (f)) == '\f')
		doclear();
	    else {
		Ungetc (ch, f);
		if (noscroll && (ch != EOF)) {
		    if (clreol)
			home ();
		    else
			doclear ();
		}
	    }
	    if (srchopt)
	    {
		search (initbuf, stdin, 1);
		if (noscroll)
		    left--;
	    }
	    else if (initopt)
		skiplns (initline, stdin);
	    screen (stdin, left);
	}
	no_intty = 0;
	prnames++;
	firstf = 0;
    }

    while (fnum < nfiles) {
	if ((f = checkf (fnames[fnum], &clearit)) != NULL) {
	    context.line = context.chrctr = 0;
	    Currline = 0;
	    if (firstf) setjmp (restore);
	    if (firstf) {
		firstf = 0;
		if (srchopt)
		{
		    search (initbuf, f, 1);
		    if (noscroll)
			left--;
		}
		else if (initopt)
		    skiplns (initline, f);
	    }
	    else if (fnum < nfiles && !no_tty) {
		setjmp (restore);
		left = command (fnames[fnum], f);
	    }
	    if (left != 0) {
		if ((noscroll || clearit) && (file_size!=0x7fffffffffffffffLL))
		    if (clreol)
			home ();
		    else
			doclear ();
		if (prnames) {
		    if (bad_so)
			eras (0);
		    if (clreol)
			cleareol ();
		    pr("::::::::::::::");
		    if (promptlen > 14)
			eras (14);
		    printf ("\n");
		    if(clreol) cleareol();
		    printf("%s\n", fnames[fnum]);
		    if(clreol) cleareol();
		    printf("::::::::::::::\n"/*, fnames[fnum]*/);
		    if (left > Lpp - 4)
			left = Lpp - 4;
		}
		if (no_tty)
		    copy_file (f);
		else {
		    within++;
		    screen(f, left);
		    within = 0;
		}
	    }
	    setjmp (restore);
	    fflush(stdout);
	    fclose(f);
	    screen_start.line = screen_start.chrctr = 0L;
	    context.line = context.chrctr = 0L;
	}
	fnum++;
	firstf = 0;
    }
    if (wflag && !hard) {
	if (Senter && Sexit)
	    tputs(Senter, 1, putch);
	if (clreol)
	    cleareol();
	pr("--No more--");
	if (clreol)
	    clreos();
	if (Senter && Sexit)
	    tputs(Sexit, 1, putch);
	fflush(stdout);
	readch();
	promptlen = 11;
	eras(0);
	fflush(stdout);
    }
    reset_tty ();
    exit(0);
}

static void
argscan(const char *s)
{
	for (dlines = 0; *s != '\0'; s++)
	{
		switch (*s)
		{
		  case '0': case '1': case '2':
		  case '3': case '4': case '5':
		  case '6': case '7': case '8':
		  case '9':
			dlines = dlines*10 + *s - '0';
			break;
		  case 'd':
			dum_opt = 1;
			break;
		  case 'l':
			stop_opt = 0;
			break;
		  case 'f':
			fold_opt = 0;
			break;
		  case 'p':
			noscroll++;
			break;
		  case 'c':
			clreol++;
			break;
		  case 's':
			ssp_opt = 1;
			break;
		  case 'u':
			ul_opt = 0;
			break;
		  case 'r':
			rflag = 1;
			break;
		  case 'w':
			wflag = 1;
			break;
		}
	}
}


/*
** Check whether the file named by fs is an ASCII file which the user may
** access.  If it is, return the opened file. Otherwise return NULL.
*/

static FILE *
checkf (register const char *fs, int *clearfirst)
{
    struct stat stbuf;
    register FILE *f;
    char b[8];
    int	n;

    if (stat (fs, &stbuf) == -1) {
	fflush(stdout);
	if (clreol)
	    cleareol ();
	perror(fs);
	return (NULL);
    }
    if ((stbuf.st_mode & S_IFMT) == S_IFDIR) {
	printf("\n*** %s: directory ***\n\n", fs);
	return (NULL);
    }
    if ((f=Fopen(fs, "r")) == NULL) {
	fflush(stdout);
	perror(fs);
	return (NULL);
    }
    n = fread(b, 1, sizeof b, f);

    /* Try to see whether it is an ASCII file */

    if (n > 1) switch (b[0]&0377) {
    case 0405:
    case 0407:
    case 0410:
    case 0411:
    case 0413:
    not:printf("\n******** %s: Not a text file ********\n\n", fs);
	fclose (f);
	return (NULL);
    default:
	break;
    }
    if (n > 2) switch (((b[1]&0377)<<8) + (b[0]&0377)) {
    case 0570:
    case 0430:
    case 0431:
    case 0575:
    case 0502:
    case 0503:
    case 0510:
    case 0511:
    case 0512:
    case 0522:
    case 0514:
	goto not;
    }
    if (n > 4 && b[0] == '\177' && b[1] == 'E' && b[2] == 'L' && b[3] == 'F')
	goto not;
    if (n > 1 && b[0] == '\f') {
	*clearfirst = 1;
	fseeko(f, 1, SEEK_SET);
    } else {
	*clearfirst = 0;
	fseeko(f, 0, SEEK_SET);
    }
    if ((file_size = stbuf.st_size) == 0)
	file_size = 0x7fffffffffffffffLL;
    return (f);
}

/*
** A real function, for the tputs routine in termlib
*/

static int
putch (int ch)
{
    putchar (ch);
    return 0;
}

/*
** Print out the contents of the file f, one screenful at a time.
*/

#define STOP -10

static void
screen (register FILE *f, register int num_lines)
{
    register int c;
    register int nchars;
    int length;			/* length of current line */
    static int prev_len = 1;	/* length of previous line */

    for (;;) {
	while (num_lines > 0 && !Pause) {
	    if ((nchars = getline (f, &length)) == EOF)
	    {
		if (clreol)
		    clreos();
		return;
	    }
	    if (ssp_opt && length == 0 && prev_len == 0)
		continue;
	    prev_len = length;
	    if (bad_so || (Senter && *Senter == ' ') && promptlen > 0)
		eras (0);
	    /* must clear before drawing line since tabs on some terminals
	     * do not erase what they tab over.
	     */
	    if (clreol)
		cleareol ();
	    prbuf (Line, length);
	    if (nchars < promptlen)
		eras (nchars);	/* eras () sets promptlen to 0 */
	    else promptlen = 0;
	    /* is this needed?
	     * if (clreol)
	     *	cleareol();	/\* must clear again in case we wrapped *
	     */
	    if (nchars < Mcol || !fold_opt)
		prbuf("\n", 1);	/* will turn off UL if necessary */
	    if (nchars == STOP)
		break;
	    num_lines--;
	}
	if (pstate) {
		tputs(ULexit, 1, putch);
		pstate = 0;
	}
	fflush(stdout);
	if ((c = Getc(f)) == EOF)
	{
	    if (clreol)
		clreos ();
	    return;
	}

	if (Pause && clreol)
	    clreos ();
	Ungetc (c, f);
	setjmp (restore);
	Pause = 0; startup = 0;
	if ((num_lines = command (NULL, f)) == 0)
	    return;
	if (hard && promptlen > 0)
		eras (0);
	if (noscroll && num_lines >= dlines)
	{
	    if (clreol)
		home();
	    else
		doclear ();
	}
	screen_start.line = Currline;
	screen_start.chrctr = Ftell (f);
    }
}

/*
** Come here if a quit signal is received
*/

static void
onquit(int signo)
{
    sigset(signo, SIG_IGN);
    if (!inwait) {
	putchar ('\n');
	if (!startup) {
	    sigset(signo, onquit);
	    sigrelse(signo);
	    longjmp (restore, 1);
	}
	else
	    Pause++;
    }
    else if (!dum_opt && notell) {
	write (2, "[Use q or Q to quit]", 20);
	promptlen += 20;
	notell = 0;
    }
    sigset(signo, onquit);
}

/*
** Come here if a signal for a window size change is received
*/

static void
chgwinsz(int signo)
{
    struct winsize win;

    sigset(SIGWINCH, SIG_IGN);
    if (ioctl(fileno(stdout), TIOCGWINSZ, &win) != -1) {
	if (win.ws_row != 0) {
	    Lpp = win.ws_row;
	    nscroll = Lpp/2 - 1;
	    if (nscroll <= 0)
		nscroll = 1;
	    dlines = Lpp - (noscroll ? 1 : 2);
	}
	if (win.ws_col != 0)
	    Mcol = win.ws_col;
    }
    sigset(SIGWINCH, chgwinsz);
}

/*
** Clean up terminal state and exit. Also come here if interrupt signal received
*/

static void
end_it (int signo)
{

    reset_tty ();
    if (clreol) {
	putchar ('\r');
	clreos ();
	fflush (stdout);
    }
    else if (!clreol && (promptlen > 0)) {
	kill_line ();
	fflush (stdout);
    }
    else
	write (2, "\n", 1);
    _exit(0);
}

static void
copy_file(register FILE *f)
{
    register int c;

    while ((c = getc(f)) != EOF)
	putchar(c);
}

/*
** Print an integer as a string of decimal digits,
** returning the length of the print representation.
*/

static int
printd (int n)
{
    int a, nchars;

    if (a = n/10)
	nchars = 1 + printd(a);
    else
	nchars = 1;
    putchar (n % 10 + '0');
    return (nchars);
}

/* Put the print representation of an integer into a string */
static char *sptr;

static void
scanstr (int n, char *str)
{
    sptr = str;
    Sprintf (n);
    *sptr = '\0';
}

static void
Sprintf (int n)
{
    int a;

    if (a = n/10)
	Sprintf (a);
    *sptr++ = n % 10 + '0';
}

static char bel = ctrl('G');

static void
prompt (const char *filename)
{
    if (clreol)
	cleareol ();
    else if (promptlen > 0)
	kill_line ();
    if (!hard) {
	promptlen = 8;
	if (Senter && Sexit) {
	    tputs (Senter, 1, putch);
	    promptlen += (2 * soglitch);
	}
	if (clreol)
	    cleareol ();
	pr("--More--");
	if (filename != NULL) {
	    promptlen += printf ("(Next file: %s)", filename);
	}
	else if (!no_intty) {
	    promptlen += printf ("(%d%%)", (int)((file_pos * 100) / file_size));
	}
	if (dum_opt) {
	    promptlen += pr("[Press space to continue, 'q' to quit.]");
	}
	if (Senter && Sexit)
	    tputs (Sexit, 1, putch);
	if (clreol)
	    clreos ();
	fflush(stdout);
    }
    else
	write (2, &bel, 1);
    inwait++;
}

/*
** Get a logical line
*/

static int
getline(register FILE *f, int *length)
{
    register int	c, i = 0;
    register int	column;
    static int		colflg;
    mbstate_t state;

    memset(&state, 0, sizeof state);
    column = 0;
    c = Getc (f);
    if (colflg && c == '\n') {
	Currline++;
	c = Getc (f);
    }
    for (;;) {
	if (i >= LINSIZ-1)
	    Line = srealloc(Line, LINSIZ+=256);
	if (c == EOF) {
	    if (i > 0) {
		Line[i] = '\0';
		*length = i;
		return (column);
	    }
	    *length = i;
	    return (EOF);
	}
	if (c == '\n') {
	    Currline++;
	    break;
	}
	Line[i++] = c;
	if (c == '\t')
	    if (hardtabs && column < promptlen && !hard) {
		if (eraseln && !dumb) {
		    column = 1 + (column | 7);
		    tputs (eraseln, 1, putch);
		    promptlen = 0;
		}
		else {
		    for (--i; column & 7; column++) {
			if (i >= LINSIZ-1)
			    Line = srealloc(Line, LINSIZ+=256);
			Line[i++] = ' ';
		    }
		    if (column >= promptlen) promptlen = 0;
		}
	    }
	    else
		column = 1 + (column | 7);
	else if (c == '\b' && column > 0)
	    column--;
	else if (c == '\r')
	    column = 0;
	else if (c == '\f' && stop_opt || rflag && !(c&~037)) {
		Line[i-1] = '^';
		if (i >= LINSIZ-1)
		    Line = srealloc(Line, LINSIZ+=256);
		Line[i++] = c|0100;
		column += 2;
		if (c == '\f' && stop_opt)
		    Pause++;
	}
	else if (c == EOF) {
	    *length = i;
	    return (column);
	}
	else if ((mb_cur_max == 1 || !(c&0200)) && isprint(c))
	    column++;
	else if (mb_cur_max > 1 && c&0200) {
	    int w, n;
	    wchar_t wc;
	    int i1;
	    i1 = i - 1;
	    while ((n = mbrtowc(&wc, &Line[i1], i - i1, &state)) == -2) {
		if ((c = Getc(f)) == EOF || c == '\n')
		    break;
		if (i >= LINSIZ-1)
		    Line = srealloc(Line, LINSIZ+=256);
		Line[i++] = c;
		memset(&state, 0, sizeof state);
	    }
	    if (n > 0 && (w = wcwidth(wc)) > 0)
		column += w;
	    else if (n < 0)
		memset(&state, 0, sizeof state);
	}
	if (column >= Mcol && fold_opt) break;
	c = Getc (f);
    }
    if (column >= Mcol && Mcol > 0) {
	if (!Wrap) {
	    if (i >= LINSIZ-1)
		Line = srealloc(Line, LINSIZ+=256);
	    Line[i++] = '\n';
	}
    }
    colflg = column == Mcol && fold_opt;
    *length = i;
    Line[i] = '\0';
    return (column);
}

/*
** Erase the rest of the prompt, assuming we are starting at column col.
*/

static void
eras (register int col)
{

    if (promptlen == 0)
	return;
    if (hard) {
	putchar ('\n');
    }
    else {
	if (col == 0)
	    putchar ('\r');
	if (!dumb && eraseln)
	    tputs (eraseln, 1, putch);
	else
	    for (col = promptlen - col; col > 0; col--)
		putchar (' ');
    }
    promptlen = 0;
}

/*
** Erase the current line entirely
*/

static void
kill_line (void)
{
    eras (0);
    if (!eraseln || dumb) putchar ('\r');
}

/*
 * force clear to end of line
 */
static void
cleareol(void)
{
    tputs(eraseln, 1, putch);
}

static void
clreos(void)
{
    tputs(EodClr, 1, putch);
}

/*
**  Print string and return number of characters
*/

static int
pr(const char *s1)
{
    register const char	*s;
    register char	c;

    for (s = s1; c = *s++; )
	putchar(c);
    return (s - s1 - 1);
}


/* Print a buffer of n characters */

static void
prbuf (register const char *s, register int n)
{
    register char c;			/* next output character */
    register int state;			/* next output char's UL state */
#define wouldul(s,n)	((n) >= 2 && (((s)[0] == '_' && (s)[1] == '\b') || ((s)[1] == '\b' && (s)[2] == '_')))

    while (--n >= 0)
	if (!ul_opt)
	    putchar (*s++);
	else {
	    if (*s == ' ' && pstate == 0 && ulglitch && wouldul(s+1, n-1)) {
		s++;
		continue;
	    }
	    if (state = wouldul(s, n)) {
		c = (*s == '_')? s[2] : *s ;
		n -= 2;
		s += 3;
	    } else
		c = *s++;
	    if (state != pstate) {
		if (c == ' ' && state == 0 && ulglitch && wouldul(s, n-1))
		    state = 1;
		else
		    tputs(state ? ULenter : ULexit, 1, putch);
	    }
	    if (c != ' ' || pstate == 0 || state != 0 || ulglitch == 0)
	        putchar(c);
	    if (mb_cur_max > 1 && c & 0200) {
		    int	x = mblen(&s[-1], mb_cur_max);
		    while (--x > 0) {
			    putchar(*s & 0377);
			    s++;
			    n--;
		    }
	    }
	    if (state && *chUL) {
		pr(chBS);
		tputs(chUL, 1, putch);
	    }
	    pstate = state;
	}
}

/*
**  Clear the screen
*/

static void
doclear(void)
{
    if (Clear && !hard) {
	tputs(Clear, 1, putch);

	/* Put out carriage return so that system doesn't
	** get confused by escape sequences when expanding tabs
	*/
	putchar ('\r');
	promptlen = 0;
    }
}

/*
 * Go to home position
 */
static void
home(void)
{
    tputs(Home,1,putch);
}

static int lastcmd, lastarg, lastp;
static int lastcolon;
static char *shell_line;
static size_t shell_size;

/*
** Read a command and do it. A command consists of an optional integer
** argument followed by the command character.  Return the number of lines
** to display in the next screenful.  If there is nothing more to display
** in the current file, zero is returned.
*/

static int
command (const char *filename, register FILE *f)
{
    register int nlines;
    register int retval = 0;
    register int c;
    char colonch;
    int done;
    char comchar;
    static char *cmdbuf;
    static int cmdsize;

#define ret(val) retval=val;done++;break

    done = 0;
    if (!errors)
	prompt (filename);
    else
	errors = 0;
    for (;;) {
	nlines = number (&comchar);
	lastp = colonch = 0;
	if (comchar == '.') {	/* Repeat last command */
		lastp++;
		comchar = lastcmd;
		nlines = lastarg;
		if (lastcmd == ':')
			colonch = lastcolon;
	}
	lastcmd = comchar;
	lastarg = nlines;
	if (comchar == otty.c_cc[VERASE]) {
	    kill_line ();
	    prompt (filename);
	    continue;
	}
	switch (comchar) {
	case ':':
	    retval = colon (filename, colonch, nlines);
	    if (retval >= 0)
		done++;
	    break;
	case 'b':
	case ctrl('B'):
	    {
		register int initline;

		if (no_intty) {
		    write(2, &bel, 1);
		    return (-1);
		}

		if (nlines == 0) nlines++;

		putchar ('\r');
		eras (0);
		printf ("\n");
		if (clreol)
			cleareol ();
		printf ("...back %d page", nlines);
		if (nlines > 1)
			pr ("s\n");
		else
			pr ("\n");

		if (clreol)
			cleareol ();
		pr ("\n");

		initline = Currline - dlines * (nlines + 1);
		if (! noscroll)
		    --initline;
		if (initline < 0) initline = 0;
		Fseek(f, 0L);
		Currline = 0;	/* skiplns() will make Currline correct */
		skiplns(initline, f);
		if (! noscroll) {
		    ret(dlines + 1);
		}
		else {
		    ret(dlines);
		}
	    }
	case ' ':
	case 'z':
	    if (nlines == 0) nlines = dlines;
	    else if (comchar == 'z') dlines = nlines;
	    ret (nlines);
	case 'd':
	case ctrl('D'):
	    if (nlines != 0) nscroll = nlines;
	    ret (nscroll);
	case 'q':
	case 'Q':
	    end_it (0);
	case 's':
	case 'f':
	    if (nlines == 0) nlines++;
	    if (comchar == 'f')
		nlines *= dlines;
	    putchar ('\r');
	    eras (0);
	    printf ("\n");
	    if (clreol)
		cleareol ();
	    printf ("...skipping %d line", nlines);
	    if (nlines > 1)
		pr ("s\n");
	    else
		pr ("\n");

	    if (clreol)
		cleareol ();
	    pr ("\n");

	    while (nlines > 0) {
		while ((c = Getc (f)) != '\n')
		    if (c == EOF) {
			retval = 0;
			done++;
			goto endsw;
		    }
		    Currline++;
		    nlines--;
	    }
	    ret (dlines);
	case '\n':
	    if (nlines != 0)
		dlines = nlines;
	    else
		nlines = 1;
	    ret (nlines);
	case '\f':
	    if (!no_intty) {
		doclear ();
		Fseek (f, screen_start.chrctr);
		Currline = screen_start.line;
		ret (dlines);
	    }
	    else {
		write (2, &bel, 1);
		break;
	    }
	case '\'':
	    if (!no_intty) {
		kill_line ();
		pr ("\n***Back***\n\n");
		Fseek (f, context.chrctr);
		Currline = context.line;
		ret (dlines);
	    }
	    else {
		write (2, &bel, 1);
		break;
	    }
	case '=':
	    kill_line ();
	    promptlen = printd (Currline);
	    fflush (stdout);
	    break;
	case 'n':
	    lastp++;
	case '/':
	    if (nlines == 0) nlines++;
	    kill_line ();
	    pr ("/");
	    promptlen = 1;
	    fflush (stdout);
	    if (lastp) {
		write (2,"\r", 1);
		search (NULL, f, nlines);	/* Use previous r.e. */
	    }
	    else {
		ttyin (&cmdbuf, &cmdsize, '/');
		write (2, "\r", 1);
		search (cmdbuf, f, nlines);
	    }
	    ret (dlines-1);
	case '!':
	    do_shell (filename);
	    break;
	case '?':
	case 'h':
	    if (noscroll) doclear ();
	    fwrite(helptext, 1, sizeof helptext - 1, stdout);
	    prompt (filename);
	    break;
	case 'v':	/* This case should go right before default */
	    if (!no_intty) {
		kill_line ();
		if (cmdsize < 40)
		    cmdbuf = srealloc(cmdbuf, cmdsize = 40);
		cmdbuf[0] = '+';
		scanstr (Currline - dlines < 0 ? 0
				: Currline - (dlines + 1) / 2, &cmdbuf[1]);
		pr ("vi "); pr (cmdbuf); putchar (' '); pr (fnames[fnum]);
		execute (filename, "vi", "vi", cmdbuf, fnames[fnum], 0);
		break;
	    }
	default:
	    if (dum_opt) {
   		kill_line ();
		if (Senter && Sexit) {
		    tputs (Senter, 1, putch);
		    promptlen = pr ("[Press 'h' for instructions.]") + (2 * soglitch);
		    tputs (Sexit, 1, putch);
		}
		else
		    promptlen = pr ("[Press 'h' for instructions.]");
		fflush (stdout);
	    }
	    else
		write (2, &bel, 1);
	    break;
	}
	if (done) break;
    }
    putchar ('\r');
endsw:
    inwait = 0;
    notell++;
    return (retval);
}

static char ch;

/*
 * Execute a colon-prefixed command.
 * Returns <0 if not a command that should cause
 * more of the file to be printed.
 */

static int
colon (const char *filename, int cmd, int nlines)
{
	if (cmd == 0)
		ch = readch ();
	else
		ch = cmd;
	lastcolon = ch;
	switch (ch) {
	case 'f':
		kill_line ();
		if (!no_intty)
			promptlen = printf ("\"%s\" line %d", fnames[fnum], Currline);
		else
			promptlen = printf ("[Not a file] line %d", Currline);
		fflush (stdout);
		return (-1);
	case 'n':
		if (nlines == 0) {
			if (fnum >= nfiles - 1)
				end_it (0);
			nlines++;
		}
		putchar ('\r');
		eras (0);
		skipf (nlines);
		return (0);
	case 'p':
		if (no_intty) {
			write (2, &bel, 1);
			return (-1);
		}
		putchar ('\r');
		eras (0);
		if (nlines == 0)
			nlines++;
		skipf (-nlines);
		return (0);
	case '!':
		do_shell (filename);
		return (-1);
	case 'q':
	case 'Q':
		end_it (0);
	default:
		write (2, &bel, 1);
		return (-1);
	}
}

/*
** Read a decimal number from the terminal. Set cmd to the non-digit which
** terminates the number.
*/

static int
number(char *cmd)
{
	register int i;

	i = 0; ch = otty.c_cc[VKILL];
	for (;;) {
		ch = readch ();
		if (ch >= '0' && ch <= '9')
			i = i*10 + ch - '0';
		else if (ch == otty.c_cc[VKILL])
			i = 0;
		else {
			*cmd = ch;
			break;
		}
	}
	return (i);
}

static void
do_shell (const char *filename)
{
	static char *cmdbuf;
	static int cmdsize;

	kill_line ();
	pr ("!");
	fflush (stdout);
	promptlen = 1;
	if (lastp)
		pr (shell_line);
	else {
		ttyin (&cmdbuf, &cmdsize, '!');
		if (expand (&shell_line, &shell_size, cmdbuf)) {
			kill_line ();
			promptlen = printf ("!%s", shell_line);
		}
	}
	fflush (stdout);
	write (2, "\n", 1);
	promptlen = 0;
	shellp = 1;
	execute (filename, shell, shell, "-c", shell_line, 0);
}

/*
** Search for nth ocurrence of regular expression contained in buf in the file
*/

static void
search (char *buf, FILE *file, register int n)
{
    off_t startline = Ftell (file);
    register off_t line1 = startline;
    register off_t line2 = startline;
    register off_t line3 = startline;
    register int lncount;
    int saveln;
    static int used;
    static regex_t s;

    context.line = saveln = Currline;
    context.chrctr = startline;
    lncount = 0;
    if (buf == NULL && used == 0)
	    error ("No previous regular expression");
    if (buf && used) {
	regfree(&s);
	used = 0;
    }
    /* emulate regcmp()/regex() */
    if (buf && regcomp(&s, buf, REG_PLUS|REG_BRACES|REG_PARENS|REG_NOBACKREF|
			    REG_BADRANGE|REG_ESCNL|REG_NOI18N|REG_NOSUB) != 0)
	error ("Regular expression botch");
	/*error (s);*/
    used = 1;
    while (!feof (file)) {
	line3 = line2;
	line2 = line1;
	line1 = Ftell (file);
	rdline (file);
	lncount++;
	if (regexec(&s, Line, 0, 0, 0) == 0)
		if (--n == 0) {
		    if (lncount > 3 || (lncount > 1 && no_intty))
		    {
			pr ("\n");
			if (clreol)
			    cleareol ();
			pr("...skipping\n");
		    }
		    if (!no_intty) {
			Currline -= (lncount >= 3 ? 3 : lncount);
			Fseek (file, line3);
			if (noscroll)
			    if (clreol) {
				home ();
				cleareol ();
			    }
			    else
				doclear ();
		    }
		    else {
			kill_line ();
			if (noscroll)
			    if (clreol) {
			        home ();
			        cleareol ();
			    }
			    else
				doclear ();
			pr (Line);
			putchar ('\n');
		    }
		    break;
		}
    }
    if (feof (file)) {
	if (!no_intty) {
	    Currline = saveln;
	    Fseek (file, startline);
	}
	else {
	    pr ("\nPattern not found\n");
	    end_it (0);
	}
	error ("Pattern not found");
    }
}

static void
execute (const char *filename, const char *cmd, const char *arg0,
		const char *arg1, const char *arg2, const char *arg3)
{
	int id;
	int n;

	fflush (stdout);
	reset_tty ();
	for (n = 10; (id = fork ()) < 0 && n > 0; n--)
	    sleep (5);
	if (id == 0) {
	    if (!isatty(0)) {
		close(0);
		open("/dev/tty", 0);
	    }
	    execlp (cmd, arg0, arg1, arg2, arg3, NULL);
	    write (2, "exec failed\n", 12);
	    exit (1);
	}
	if (id > 0) {
	    sigset (SIGINT, SIG_IGN);
	    sigset (SIGQUIT, SIG_IGN);
	    if (catch_susp)
		sigset(SIGTSTP, SIG_DFL);
	    while (wait(0) > 0);
	    sigset (SIGINT, end_it);
	    sigset (SIGQUIT, onquit);
	    if (catch_susp)
		sigset(SIGTSTP, onsusp);
	} else
	    write(2, "can't fork\n", 11);
	set_tty ();
	pr ("------------------------\n");
	prompt (filename);
}
/*
** Skip n lines in the file f
*/

static void
skiplns (register int n, register FILE *f)
{
    register int c;

    while (n > 0) {
	while ((c = Getc (f)) != '\n')
	    if (c == EOF)
		return;
	    n--;
	    Currline++;
    }
}

/*
** Skip nskip files in the file list (from the command line). Nskip may be
** negative.
*/

static void
skipf (register int nskip)
{
    if (nskip == 0) return;
    if (nskip > 0) {
	if (fnum + nskip > nfiles - 1)
	    nskip = nfiles - fnum - 1;
    }
    else if (within)
	++fnum;
    fnum += nskip;
    if (fnum < 0)
	fnum = 0;
    pr ("\n...Skipping ");
    pr ("\n");
    if (clreol)
	cleareol ();
    pr ("...Skipping ");
    pr (nskip > 0 ? "to file " : "back to file ");
    pr (fnames[fnum]);
    pr ("\n");
    if (clreol)
	cleareol ();
    pr ("\n");
    --fnum;
}

/*----------------------------- Terminal I/O -------------------------------*/

static void
initterm (void)
{
    char	buf[TBUFSIZ];
    static char	clearbuf[TBUFSIZ];
    char	*clearptr, *padstr;
    char	*term;
    pid_t	tgrp;
    struct winsize win;
    long	vdis;

    vdis = fpathconf(fileno(stdout), _PC_VDISABLE);
retry:
    if (!(no_tty = tcgetattr(fileno(stdout), &otty))) {
	docrterase = otty.c_cc[VERASE] != vdis;
	docrtkill = otty.c_cc[VKILL] != vdis;
	/*
	 * Wait until we're in the foreground before we save the
	 * the terminal modes.
	 */
	if ((tgrp = tcgetpgrp(fileno(stdout))) < 0) {
	    perror("tcgetpgrp");
	    exit(1);
	}
	if (tgrp != getpgid(0)) {
	    kill(0, SIGTTOU);
	    goto retry;
	}
	if ((term = getenv("TERM")) == 0 || tgetent(buf, term) <= 0) {
	    dumb++; ul_opt = 0;
	}
	else {
	    if (ioctl(fileno(stdout), TIOCGWINSZ, &win) < 0) {
		Lpp = tgetnum("li");
		Mcol = tgetnum("co");
	    } else {
		if ((Lpp = win.ws_row) == 0)
		    Lpp = tgetnum("li");
		if ((Mcol = win.ws_col) == 0)
		    Mcol = tgetnum("co");
	    }
	    if ((Lpp <= 0) || tgetflag("hc")) {
		hard++;	/* Hard copy terminal */
		Lpp = 24;
	    }
	    if (Mcol <= 0)
		Mcol = 80;

	    if (strcmp (progname, "page") == 0 || !hard && tgetflag("ns"))
		noscroll++;
	    Wrap = tgetflag("am");
	    bad_so = tgetflag ("xs");
	    clearptr = clearbuf;
	    eraseln = tgetstr("ce",&clearptr);
	    Clear = tgetstr("cl", &clearptr);
	    Senter = tgetstr("so", &clearptr);
	    Sexit = tgetstr("se", &clearptr);
	    if ((soglitch = tgetnum("sg")) < 0)
		soglitch = 0;

	    /*
	     *  Set up for underlining:  some terminals don't need it;
	     *  others have start/stop sequences, still others have an
	     *  underline char sequence which is assumed to move the
	     *  cursor forward one character.  If underline sequence
	     *  isn't available, settle for standout sequence.
	     */

	    if (tgetflag("ul") || tgetflag("os"))
		ul_opt = 0;
	    if ((chUL = tgetstr("uc", &clearptr)) == NULL )
		chUL = "";
	    if (((ULenter = tgetstr("us", &clearptr)) == NULL ||
	         (ULexit = tgetstr("ue", &clearptr)) == NULL) && !*chUL) {
	        if ((ULenter = Senter) == NULL || (ULexit = Sexit) == NULL) {
			ULenter = "";
			ULexit = "";
		} else
			ulglitch = soglitch;
	    } else {
		if ((ulglitch = tgetnum("ug")) < 0)
		    ulglitch = 0;
	    }

#ifndef	__hpux
	    if (padstr = tgetstr("pc", &clearptr))
		PC = *padstr;
#endif
	    Home = tgetstr("ho",&clearptr);
	    if (Home == 0 || *Home == '\0')
	    {
		if ((cursorm = tgetstr("cm", &clearptr)) != NULL) {
		    strcpy(cursorhome, tgoto(cursorm, 0, 0));
		    Home = cursorhome;
	       }
	    }
	    EodClr = tgetstr("cd", &clearptr);
	    if ((chBS = tgetstr("bc", &clearptr)) == NULL)
		chBS = "\b";

	}
	if ((shell = getenv("SHELL")) == NULL)
	    shell = "/bin/sh";
    }
    no_intty = tcgetattr(fileno(stdin), &otty);
    tcgetattr(fileno(stdout), &otty);
    savtty = otty;
#if defined (TABDLY) && defined (TAB3)
    hardtabs = (otty.c_oflag & TABDLY) != TAB3;
#endif
    if (!no_tty) {
	otty.c_lflag &= ~(tcflag_t)(ICANON|ECHO);
	otty.c_cc[VMIN] = 1;
	otty.c_cc[VTIME] = 0;
        tcsetattr(fileno(stderr), TCSADRAIN, &otty);
    }
}

static char
readch (void)
{
	char ch;

	if (read (2, &ch, 1) <= 0)
		if (errno != EINTR)
			exit(0);
		else
			ch = otty.c_cc[VKILL];
	return (ch);
}

static const char BS = '\b';
static const char *BSB = "\b \b";
static const char CARAT = '^';
#define ERASEONECHAR \
    if (docrterase) \
	write (2, BSB, sizeof(BSB)); \
    else \
	write (2, &BS, sizeof(BS));

static char *
previous(const char *mb, const char *bottom, wchar_t *wp)
{
	const char	*p, *lastp;
	wint_t	wc, lastwc = WEOF;
	int	len, max = 0;

	if (mb <= bottom) {
		*wp = WEOF;
		return (char *)bottom;
	}
	if (mb_cur_max == 1 || !(mb[-1]&0200)) {
		*wp = btowc(mb[-1] & 0377);
		return (char *)&mb[-1];
	}
	p = mb;
	lastp = &mb[-1];
	while (p-- > bottom) {
		mbtowc(NULL, NULL, 0);
		if ((len = mbtowi(&wc, p, mb - p)) >= 0) {
			if (len < max || len < mb - p)
				break;
			max = len;
			lastp = p;
			lastwc = wc;
		} else if (len < 0 && max > 0)
			break;
	}
	*wp = lastwc;
	return (char *)lastp;
}

static void
ttyin (char **buf, int *nmax, char pchar)
{
    register char ch;
    register int slash = 0, i, n, w;
    int	maxlen;
    char cbuf;
    wchar_t wc;

    i = 0;
    maxlen = 0;
    for (;;) {
	if (promptlen > maxlen) maxlen = promptlen;
	ch = readch ();
	if (ch == '\\') {
	    slash++;
	}
	else if ((ch == otty.c_cc[VERASE]) && !slash) {
	    if (i > 0) {
		n = &(*buf)[i] - previous(&(*buf)[i], *buf, &wc);
		i -= n;
		w = wcwidth(wc);
		if (w > 0)
		    promptlen -= w;
		while (w-- > 0) {
		    ERASEONECHAR
		}
		if (iscntrl((*buf)[i]) && (*buf)[i] != '\n') {
		    --promptlen;
		    ERASEONECHAR
		}
		continue;
	    }
	    else {
		if (!eraseln) promptlen = maxlen;
		longjmp (restore, 1);
	    }
	}
	else if ((ch == otty.c_cc[VKILL]) && !slash) {
	    if (hard) {
		show (ch);
		putchar ('\n');
		putchar (pchar);
	    }
	    else {
		putchar ('\r');
		putchar (pchar);
		if (eraseln)
		    eras (1);
		else if (docrtkill)
		    while (promptlen-- > 1)
			write (2, BSB, sizeof(BSB));
		promptlen = 1;
	    }
	    i = 0;
	    fflush (stdout);
	    continue;
	}
	if (slash && (ch == otty.c_cc[VKILL] || ch == otty.c_cc[VERASE])) {
	    ERASEONECHAR
	    --i;
	}
	if (ch != '\\')
	    slash = 0;
	if (i >= *nmax)
	    *buf = srealloc(*buf, *nmax += 20);
	(*buf)[i++] = ch;
	if ((!(ch&~037) && ch != '\n' && ch != ESC) || ch == RUBOUT) {
	    ch += ch == RUBOUT ? -0100 : 0100;
	    write (2, &CARAT, 1);
	    promptlen++;
	}
	cbuf = ch;
	if (ch != '\n' && ch != ESC) {
	    write (2, &cbuf, 1);
	    promptlen++;
	}
	else
	    break;
    }
    if (i >= *nmax)
	*buf = srealloc(*buf, *nmax += 20);
    (*buf)[--i] = '\0';
    if (!eraseln) promptlen = maxlen;
}

static int
expand (char **outbuf, size_t *outsize, const char *inbuf)
{
    register const char *instr;
    register int n, o;
    register char ch;
    char *temp = NULL;
    int size = 0;
    int changed = 0;

    instr = inbuf;
    o = 0;
    while ((ch = *instr++) != '\0')
	switch (ch) {
	case '%':
	    if (!no_intty) {
		n = strlen(fnames[fnum]);
		if (o + n + 1 >= size)
			temp = srealloc(temp, size = o + n + 1);
		strcpy(&temp[o], fnames[fnum]);
		o += n;
		changed++;
	    }
	    else {
		if (o >= size)
		    temp = srealloc(temp, ++size);
		temp[o++] = ch;
	    }
	    break;
	case '!':
	    if (!shellp) {
		free(temp);
		error ("No previous command to substitute for");
	    }
	    n = strlen(shell_line);
	    if (o + n + 1 >= size)
		temp = srealloc(temp, size = o + n + 1);
	    strcpy(&temp[o], shell_line);
	    o += n;
	    changed++;
	    break;
	case '\\':
	    if (*instr == '%' || *instr == '!') {
		if (o >= size)
		    temp = srealloc(temp, ++size);
		temp[o++] = *instr++;
		break;
	    }
	default:
	    if (o >= size)
		temp = srealloc(temp, ++size);
	    temp[o++] = ch;
	}
    if (o >= size)
	temp = srealloc(temp, ++size);
    temp[o++] = '\0';
    if (o > *outsize)
	    *outbuf = srealloc(*outbuf, *outsize = o);
    strcpy (*outbuf, temp);
    return (changed);
}

static void
show (register char ch)
{
    char cbuf;

    if ((ch < ' ' && ch != '\n' && ch != ESC) || ch == RUBOUT) {
	ch += ch == RUBOUT ? -0100 : 0100;
	write (2, &CARAT, 1);
	promptlen++;
    }
    cbuf = ch;
    write (2, &cbuf, 1);
    promptlen++;
}

static void
error (const char *mess)
{
    if (clreol)
	cleareol ();
    else
	kill_line ();
    promptlen += strlen (mess);
    if (Senter && Sexit) {
	tputs (Senter, 1, putch);
	pr(mess);
	tputs (Sexit, 1, putch);
    }
    else
	pr (mess);
    fflush(stdout);
    errors++;
    longjmp (restore, 1);
}


static void
set_tty (void)
{
	if (!no_tty) {
	    otty.c_cc[VMIN] = 1;
	    otty.c_cc[VTIME] = 0;
	    otty.c_lflag &= ~(tcflag_t)(ICANON|ECHO);
	    tcsetattr(fileno(stderr), TCSADRAIN, &otty);
	}
}

static void
reset_tty (void)
{
    if (pstate) {
	tputs(ULexit, 1, putch);
	fflush(stdout);
	pstate = 0;
    }
    if (!no_tty) {
	otty.c_lflag |= ICANON|ECHO;
	tcsetattr(fileno(stderr), TCSADRAIN, &savtty);
    }
}

/* Since this is used for searches only, filter backspace sequences */
static void
rdline (register FILE *f)
{
    register int c;
    wint_t wc, lastwc = '\b';
    wchar_t wb;
    register int i = 0, m, n, lastn = 0;
    mbstate_t state;

    memset(&state, 0, sizeof state);
    while ((c = Getc (f)) != '\n' && c != EOF) {
	if (i >= LINSIZ-1)
	    Line = srealloc(Line, LINSIZ+=256);
	Line[i++] = c;
	n = 1;
	if (mb_cur_max > 1 && c&0200) {
	    while ((m = mbrtowc(&wb, &Line[i-1], 1, &state)) == -2) {
		if ((c = Getc(f)) == EOF || c == '\n')
		    break;
		if (i >= LINSIZ-1)
		    Line = srealloc(Line, LINSIZ+=256);
		Line[i++] = c;
		n++;
	    }
	    wc = wb;
	    if (m < 0) {
		memset(&state, 0, sizeof state);
		wc = WEOF;
		n = 1;
	    } else if (m == 0)
		n = 1;
	} else
	    wc = c;
	if (wc == '\b' && lastwc != '\b')
	    i -= lastn + 1;
	lastn = n;
	lastwc = wc;
    }
    if (c == '\n')
	Currline++;
    Line[i] = '\0';
}

/* Come here when we get a suspend signal from the terminal */

static void
onsusp (int signo)
{
    /* ignore SIGTTOU so we don't get stopped if csh grabs the tty */
    sigset(SIGTTOU, SIG_IGN);
    reset_tty ();
    fflush (stdout);
    sigset(SIGTTOU, SIG_DFL);
    /* Send the TSTP signal to suspend our process group */
    sigset(SIGTSTP, SIG_DFL);
    sigrelse(SIGTSTP);
    kill (0, SIGTSTP);
    /* Pause for station break */

    /* We're back */
    sigset (SIGTSTP, onsusp);
    set_tty ();
    if (inwait)
	    longjmp (restore, 1);
}

static void *
srealloc(void *vp, size_t sz)
{
	if ((vp = realloc(vp, sz)) == NULL) {
		write(2, "no memory\n", 10);
		reset_tty();
		_exit(077);
	}
	return vp;
}

static void *
smalloc(size_t sz)
{
	return srealloc(0, sz);
}
