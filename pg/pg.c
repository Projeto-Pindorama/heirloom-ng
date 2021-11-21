/*
 * pg - file perusal filter for CRTs
 *
 *	Copyright (c) 2000-2003 Gunnar Ritter. All rights reserved.
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
static const char sccsid[] USED = "@(#)pg_su3.sl	2.66 (gritter) 8/14/05";
#elif defined (SUS)
static const char sccsid[] USED = "@(#)pg_sus.sl	2.66 (gritter) 8/14/05";
#else
static const char sccsid[] USED = "@(#)pg.sl	2.66 (gritter) 8/14/05";
#endif

#ifndef	USE_TERMCAP
#ifdef	sun
#include <curses.h>
#include <term.h>
#endif
#endif
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#ifndef	TIOCGWINSZ
#include <sys/ioctl.h>
#endif
#include <termios.h>
#include <fcntl.h>
#if defined (SUS) || defined (SU3)
#include <regex.h>
#else	/* !SUS, !SU3 */
#include <regexpr.h>
#endif	/* !SUS, !SU3 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include "sigset.h"
#include <setjmp.h>
#include <locale.h>
#include <nl_types.h>
#include <libgen.h>
#include <alloca.h>
#ifndef	USE_TERMCAP
#ifndef	sun
#include <curses.h>
#include <term.h>
#endif
#else	/* USE_TERMCAP */
#include <termcap.h>
#endif	/* USE_TERMCAP */

#define	ENABLE_WIDECHAR			/* for util-linux */
#define	catopen(a, b)			0
#define	catgets(a, b, c, d)		d
#define	nl_catd				int

static int	mb_cur_max;		/* MB_CUR_MAX acceleration */

#ifdef	ENABLE_WIDECHAR
#include <wctype.h>
#include <wchar.h>
#include <mbtowi.h>

typedef	wint_t	w_type;

#define	width(wc)	(mb_cur_max > 1 ? iswprint(wc) ? \
				wcwidth(wc) : utf8 ? 1 : -1 : 1)

/*
 * Get next character from string s and store it in wc; n is set to
 * the length of the corresponding byte sequence. Use of both btowc()
 * and mbtowc() provides a decent speedup with glibc 2.3.
 */
#define	next(wc, s, n) (mb_cur_max > 1 ? \
		(((wc) = btowc(*(s) & 0377)) != WEOF ? (n) = 1 : \
			((n) = mbtowi(&(wc), (s), mb_cur_max), \
			 (n) = ((n) > 0 ? (n) : (n) < 0 ? (wc=WEOF, 1) : 1))) :\
		((wc) = *(s) & 0377, (n) = 1))

/*
 * Return the location of the character preceding mb. Since we can know
 * of no byte that it is the first byte of a character in advance, the
 * longest multibyte character that ends at &mb[-1] is searched, not
 * descending below bottom.
 */
static char *
previous(const char *mb, const char *bottom, w_type *wp)
{
	const char	*p, *lastp;
	w_type	wc, lastwc = WEOF;
	int	len, max = 0;

	if (mb <= bottom) {
		*wp = WEOF;
		return (char *)bottom;
	}
	if (mb_cur_max == 1) {
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

static int	utf8;

#else	/* !ENABLE_WIDECHAR */
typedef	int	w_type;
#define	next(wc, s, n)	((wc) = *(s) & 0377, (n) = 1)
#ifdef	MB_CUR_MAX
#undef	MB_CUR_MAX
#endif
#define	MB_CUR_MAX	1
#define	width(c)	1
#define	iswprint(c)	isprint(c)

static char *
previous(const char *mb, const char *bottom, w_type *wp)
{
	return (char *)(mb > bottom ? (*wp = mb[-1], &mb[-1]) :
			(*wp = EOF, bottom));
}

#define	utf8	0

#endif	/* !ENABLE_WIDECHAR */

#define	TABSIZE		8		/* spaces consumed by tab character */

enum okay {
	STOP,
	OKAY
};

enum direction {
	NOSEARCH	= 0,
	FORWARD		= 1,
	BACKWARD	= 2
};	/* search direction */

enum position {
	TOP,
	MIDDLE,
	BOTTOM
};		/* position of matching line */

/*
 * States for syntax-aware command line editor.
 */
enum state {
	COUNT,
	SIGN,
	CMD_FIN,
	SEARCH,
	SEARCH_FIN,
	ADDON_FIN,
	STRING,
	INVALID
};

/*
 * Current command
 */
static struct {
	char	cmdline[256];	/* command line */
	size_t	cmdlen;		/* lenght of command line */
	int	count;		/* count for commands that need it */
	int	key;		/* command key (n, /, ?, etc.) */
	char	addon;		/* t, m, b for search commands */
} cmd;

/*
 * Position of file arguments on av[] to run()
 */
static struct {
	int	first;
	int	current;
	int	last;
} files;

static void	(*oldint)(int);		/* old SIGINT handler */
static void	(*oldquit)(int);	/* old SIGQUIT handler */
static void	(*oldterm)(int);	/* old SIGTERM handler */
static char	*tty;			/* result of ttyname(1) */
static char	*progname;		/* program name */
static unsigned	ontty;			/* whether running on tty device */
static unsigned	exitstatus;		/* exit status */
static int	pagelen = 23;		/* lines on a single screen page */
static int	ttycols = 79;		/* screen columns (starting at 0) */
static struct termios	otio;		/* old termios settings */
static int	tinfostat = -1;		/* terminfo routines initialized */
static enum position searchdisplay = TOP; /* position to print matching line */
static int	cflag;			/* clear screen before each page */
static int	eflag;			/* suppress (EOF) */
static int	fflag;			/* do not split lines */
static int	nflag;			/* no newline for commands required */
static int	rflag;			/* "restricted" pg */
static int	sflag;			/* use standout mode */
static char	*pstring = ":";		/* prompt string */
static char	*searchfor;		/* search pattern from argv[] */
static int	havepagelen;		/* page length is manually defined */
static long	startline;		/* start line from argv[] */
static int	nextfile = 1;		/* files to advance */
static jmp_buf	genenv;			/* general jump from signal handlers */
static int	genjump;		/* genenv is valid */
static jmp_buf	specenv;		/* special jump from signal handlers */
static int	specjump;		/* specenv  is valid */
static nl_catd	catd;			/* message catalogue descriptor */
static off_t	LINE;			/* mark begin of an input line */
static off_t	MASK;			/* mask to strip LINE marker */
static int	must_sgr0;		/* need to print sgr0 before prompt */
static const char	*sgr0;		/* sgr0 sequence */

static const char *helpscreen = "\
-------------------------------------------------------\n\
  h                       this screen\n\
  q or Q                  quit program\n\
  <newline>               next page\n\
  f                       skip a page forward\n\
  d or ^D                 next halfpage\n\
  l                       next line\n\
  $                       last page\n\
  /regex/                 search forward for regex\n\
  ?regex? or \n\
  ^regex^                 search backward for regex\n\
  . or ^L                 redraw screen\n\
  w or z                  set page size and go to next page\n\
  s filename              save current file to filename\n\
  !command                shell escape\n\
  p                       go to previous file\n\
  n                       go to next file\n\
\n\
Many commands accept preceding numbers, for example:\n\
+1<newline> (next page); -1<newline> (previous page); 1<newline> (first page).\n\
\n\
See pg(1) for more information.\n\
-------------------------------------------------------\n";

#ifdef	USE_TERMCAP
static char	*Clear_screen;
#define	clear_screen	Clear_screen
static char	*Bell;
#define	bell	Bell
static char	*Standout;
#define	A_STANDOUT	Standout
static char	*Normal;
#define	A_NORMAL	Normal
static int	Lines;
#define	lines		Lines
static int	Columns;
#define	columns		Columns
static char	termspace[2048];
static char	*termptr = termspace;

static void
vidputs(const char *seq, int (*putfunc)(int))
{
	while (*seq)
		(*putfunc)(*seq++ & 0377);
}
#endif	/* USE_TERMCAP */

/*
 * Quit pg.
 */
static void
quit(int status)
{
	if (must_sgr0)
		write(2, sgr0, strlen(sgr0));
	exit(status < 0100 ? status : 077);
}

static void *
irealloc(void *v, size_t s)
{
	void *p;

	sighold(SIGINT);
	sighold(SIGQUIT);
	p = realloc(v, s);
	if (p == NULL)
		return NULL;
	sigrelse(SIGINT);
	sigrelse(SIGQUIT);
	return p;
}

/*
 * Usage message and similar routines.
 */
static void
usage(void)
{
	fprintf(stderr, catgets(catd, 1, 1,
"Usage: %s [-number] [-p string] [-cefnrs] [+line] [+/pattern/] files\n"),
			progname);
	quit(2);
}

static void
needarg(const char *s)
{
	/*fprintf(stderr, catgets(catd, 1, 2,
		"%s: option requires an argument -- %s\n"), progname, s);*/
	usage();
}

static void
invopt(const char *s)
{
	/*fprintf(stderr, catgets(catd, 1, 3,
		"%s: illegal option -- %s\n"), progname, s);*/
	usage();
}

/*
 * Helper function for tputs().
 */
static int
outcap(int i)
{
	char c = i;
	return write(1, &c, 1);
}

/*
 * Write messages to terminal.
 */
static void
mesg(const char *message)
{
	if (ontty == 0)
		return;
	if (must_sgr0) {
		write(1, sgr0, strlen(sgr0));
		must_sgr0 = 0;
	}
	if (*message != '\n' && sflag)
		vidputs(A_STANDOUT, outcap);
	write(1, message, strlen(message));
	if (*message != '\n' && sflag)
		vidputs(A_NORMAL, outcap);
}

/*
 * Get the window size.
 */
static void
getwinsize(void)
{
	static int initialized, envlines, envcols, deflines, defcols;
#ifdef	TIOCGWINSZ
	struct winsize winsz;
	int badioctl;
#endif
	char *p;

	if (initialized == 0) {
		if ((p = getenv("LINES")) != NULL && *p != '\0')
			if ((envlines = atoi(p)) < 0)
				envlines = 0;
		if ((p = getenv("COLUMNS")) != NULL && *p != '\0')
			if ((envcols = atoi(p)) < 0)
				envcols = 0;
		/* terminfo values. */
		if (tinfostat != 1 || columns == 0)
			defcols = 24;
		else
			defcols = columns;
		if (tinfostat != 1 || lines == 0)
			deflines = 80;
		else
			deflines = lines;
		initialized = 1;
	}
#ifdef	TIOCGWINSZ
	badioctl = ioctl(1, TIOCGWINSZ, &winsz);
#endif
	if (envcols)
		ttycols = envcols - 1;
#ifdef	TIOCGWINSZ
	else if (!badioctl && winsz.ws_col > 0)
		ttycols = winsz.ws_col - 1;
#endif
	else
		ttycols = defcols - 1;
	if (havepagelen == 0) {
		if (envlines)
			pagelen = envlines - 1;
#ifdef	TIOCGWINSZ
		else if (!badioctl && winsz.ws_row > 0)
			pagelen = winsz.ws_row - 1;
#endif
		else
			pagelen = deflines - 1;
	}
}

/*
 * Ring terminal bell.
 */
static void
ring(void)
{
#ifndef	PGNOBELL
	if (bell)
		tputs(bell, 1, outcap);
#endif	/* PGNOBELL */
}

/*
 * Terminfo strings to pass through.
 */
static const char	*ti_sequences[43];
static int		esc1;

#define	seqstart(c)	(esc1 ? (c) == esc1 : iscntrl(c))

static int
one_sequence(char *str, int ix)
{
	char	*esc;

	if (
#ifndef	USE_TERMCAP
			(esc = tigetstr(str))
#else	/* USE_TERMCAP */
			(esc = tgetstr(str, &termptr))
#endif	/* USE_TERMCAP */
			!= NULL && esc != (char *)-1) {
		ti_sequences[ix] = strdup(esc);
		return 1;
	}
	return 0;
}

static void
fill_sequences(void)
{
	int	ix = 0;
	char	*cp;

	/*
	 * Warning: Make sure that ti_sequences[] is large enough
	 * if you add more capabilities here.
	 */
#ifndef	USE_TERMCAP
	if (tinfostat <= 0 || (sgr0 = tigetstr("sgr0")) == NULL ||
			sgr0 == (char *)-1)
		return;
	ti_sequences[ix++] = sgr0;
	ix += one_sequence("bold", ix);
	ix += one_sequence("dim", ix);
	ix += one_sequence("sitm", ix);
	ix += one_sequence("smso", ix);
	ix += one_sequence("smul", ix);
	ix += one_sequence("ritm", ix);
	ix += one_sequence("rmso", ix);
	ix += one_sequence("rmul", ix);
	ix += one_sequence("sc", ix);	/* generated by tbl|nroff on Solaris */
	ix += one_sequence("rc", ix);	/* generated by tbl|nroff on Solaris */
	ix += one_sequence("rmacs", ix);
#else	/* USE_TERMCAP */
	if (tinfostat <= 0 || (sgr0 = tgetstr("me", &termptr)) == NULL)
		return;
	ti_sequences[ix++] = sgr0;
	ix += one_sequence("md", ix);
	ix += one_sequence("mh", ix);
	ix += one_sequence("mr", ix);
	ix += one_sequence("us", ix);
	ix += one_sequence("ue", ix);
	ix += one_sequence("so", ix);
	ix += one_sequence("se", ix);
	ix += one_sequence("sc", ix);	/* generated by tbl|nroff on Solaris */
	ix += one_sequence("rc", ix);	/* generated by tbl|nroff on Solaris */
	ix += one_sequence("ae", ix);
	Clear_screen = tgetstr("cl", &termptr);
#endif	/* USE_TERMCAP */
#if defined (COLOR_BLACK) && !defined (USE_TERMCAP)
	if ((cp = tparm(set_a_foreground, COLOR_BLACK)) != NULL &&
			strcmp(cp, "\33[30m") == 0) {
#else	/* !COLOR_BLACK */
	if ((cp = getenv("TERM")) != NULL && (strncmp(cp, "rxvt", 4) == 0 ||
				strncmp(cp, "xterm", 5) == 0 ||
				strncmp(cp, "dtterm", 6) == 0)) {
		ti_sequences[ix++] = "\33[;1m";
#endif	/* !COLOR_BLACK */
		ti_sequences[ix++] = "\33[30m";
		ti_sequences[ix++] = "\33[31m";
		ti_sequences[ix++] = "\33[32m";
		ti_sequences[ix++] = "\33[33m";
		ti_sequences[ix++] = "\33[34m";
		ti_sequences[ix++] = "\33[35m";
		ti_sequences[ix++] = "\33[36m";
		ti_sequences[ix++] = "\33[37m";

		ti_sequences[ix++] = "\33[m";
		ti_sequences[ix++] = "\33[0m";
		ti_sequences[ix++] = "\33[48m";

		ti_sequences[ix++] = "\33[01;30m";
		ti_sequences[ix++] = "\33[01;31m";
		ti_sequences[ix++] = "\33[01;32m";
		ti_sequences[ix++] = "\33[01;33m";
		ti_sequences[ix++] = "\33[01;34m";
		ti_sequences[ix++] = "\33[01;35m";
		ti_sequences[ix++] = "\33[01;36m";
		ti_sequences[ix++] = "\33[01;37m";

		ti_sequences[ix++] = "\33[0;1m\17";

		ti_sequences[ix++] = "\33[22m";
	}
	esc1 = '\33';
	for (ix = 0; ti_sequences[ix]; ix++)
		if (ti_sequences[ix][0] != '\33') {
			esc1 = '\0';
			break;
		}
}

static int
prefix(const char *s, const char *pfx)
{
	int	i;

	for (i = 0; s[i] && pfx[i] && s[i] == pfx[i]; i++);
	if (pfx[i] == '\0' && s[i-1] == pfx[i-1])
		return i;
	return -1;
}

static int
known_sequence(const char *s)
{
	int	i, n;

	for (i = 0; ti_sequences[i]; i++)
		if ((n = prefix(s, ti_sequences[i])) > 0) {
			must_sgr0 = 1;
			return n;
		}
	return -1;
}

/*
 * Signal handler while reading from input file.
 */
static void
sighandler(int signum)
{
	if (signum == SIGINT || signum == SIGQUIT) {
		if (specjump)
			longjmp(specenv, signum);
		if (genjump)
			longjmp(genenv, signum);
	}
	tcsetattr(1, TCSADRAIN, &otio);
	quit(exitstatus);
}

/*
 * Check whether the requested file was specified on the command line.
 */
static enum okay
checkf(void)
{
	if (files.current + nextfile >= files.last) {
		ring();
		mesg(catgets(catd, 1, 6, "No next file"));
		return STOP;
	}
	if (files.current + nextfile < files.first) {
		ring();
		mesg(catgets(catd, 1, 7, "No previous file"));
		return STOP;
	}
	return OKAY;
}

static int
nextpos(w_type wc, const char *s, int n, int pos, int *add)
{
	int	m;

	if (add)
		*add = 0;
	switch (wc) {
	/*
	 * Cursor left.
	 */
	case '\b':
		if (pos > 0)
			pos--;
		break;
	/*
	 * No cursor movement.
	 */
	/*case '\a':
		break; nonprintable anyway */
	/*
	 * Special.
	 */
	case '\r':
		pos = 0;
		break;
	case '\n':
		pos = -1;
		if (add)
			*add = 1;
		break;
	/*
	 * Cursor right.
	 */
	case '\t':
		pos += TABSIZE - (pos % TABSIZE);
		/*if (pos >= col) {
			while (*s++ == '\t');
			continue;
		} don't use - it's safer to print '\n' at this point */
		break;
	default:
		if (seqstart(*s&0377) && (m = known_sequence(s)) > 0) {
			if (add)
				*add = m - n;
			break;
		} else {
			m = width(wc);
			pos += m >= 0 ? m : n;
		}
	}
	return pos;
}

/*
 * Return the last character that will fit on the line at col columns.
 */
static size_t
endline(unsigned col, const char *s, const char *end)
{
	int pos = 0;
	const char *olds = s;
	int	m, n, add;
	w_type	wc;

	if (fflag)
		return end - s;
	while (s < end) {
		next(wc, s, n);
		pos = nextpos(wc, s, n, pos, &add);
		s += add;
		if (pos < 0)
			goto cend;
		if (pos > col) {
			if (pos == col + 1)
				s += n;
			while (seqstart(*s&0377) && (m = known_sequence(s)) > 0)
				s += m;
			if (*s == '\n')
				s++;
			goto cend;
		}
		s += n;
	}
cend:
	return s - olds;
}

/*
 * Clear the current line on the terminal's screen.
 */
static void
cline(void)
{
	char *buf = alloca(ttycols + 2);
	memset(buf, ' ', ttycols + 2);
	buf[0] = '\r';
	buf[ttycols + 1] = '\r';
	write(1, buf, ttycols + 2);
}

/*
 * Evaluate a command character's semantics.
 */
static enum state
getstate(int c)
{
	switch (c) {
	case '1': case '2': case '3': case '4': case '5':
	case '6': case '7': case '8': case '9': case '0':
	case '\0':
		return COUNT;
	case '-': case '+':
		return SIGN;
	case 'l': case 'd': case '\004': case 'f': case 'z':
	case '.': case '\014': case '$': case 'n': case 'p':
	case 'w': case 'h': case 'q': case 'Q':
		return CMD_FIN;
	case '/': case '?': case '^':
		return SEARCH;
	case 's': case '!':
		return STRING;
	case 'm': case 'b': case 't':
		return ADDON_FIN;
	default:
		ring();
		return INVALID;
	}
}

/*
 * Get the count and ignore last character of string.
 */
static int
getcount(const char *cmdstr)
{
	char *buf;
	char *p;
	int i;

	if (*cmdstr == '\0')
		return 1;
	buf = alloca(strlen(cmdstr) + 1);
	strcpy(buf, cmdstr);
	if (cmd.key != '\0') {
		if (cmd.key == '/' || cmd.key == '?' || cmd.key == '^') {
			if ((p = strchr(buf, cmd.key)) != NULL)
				*p = '\0';
		} else
			*(buf + strlen(buf) - 1) = '\0';
	}
	if (*buf == '\0')
		return 1;
	if (buf[0] == '-' && buf[1] == '\0') {
		i = -1;
	} else {
		if (*buf == '+')
			i = atoi(buf + 1);
		else
			i = atoi(buf);
	}
	return i;
}

/*
 * Format the prompt string and print it. The first occurence of '%d' is
 * to be replaced by the page number, '%%' by '%' and all other types of
 * '%c' by themselves.
 */
static void
printprompt(long long pageno)
{
	char	*p, *q;
	int	n, pd;
	w_type	wc;
	char	promptbuf[LINE_MAX+1];

	p = pstring;
	q = promptbuf;
	pd = 0;
	do {
		if (p[0] == '%') {
			if (p[1] == 'd' && pd++ == 0) {
				q += snprintf(q,
					&promptbuf[sizeof promptbuf] - q,
					"%lld", pageno);
				p += 2;
			} else if (p[1] == '%') {
				p++;
				goto norm;
			} else
				goto norm;
		} else {
		norm:	next(wc, p, n);
			while (n--)
				*q++ = *p++;
		}
	} while (q < &promptbuf[sizeof promptbuf - mb_cur_max] &&
			q[-1] != '\0');
	mesg(promptbuf);
}

/*
 * Read what the user writes at the prompt. This is tricky because
 * we check for valid input.
 */
static void
prompt(long long pageno)
{
	struct termios tio;
	char key;
	enum state state = COUNT, ostate = COUNT;
	int escape = 0, n;
	char *p;
	w_type	wc;

	if (pageno != -1)
		printprompt(pageno);
	cmd.key = cmd.addon = cmd.cmdline[0] = '\0';
	cmd.cmdlen = 0;
	tcgetattr(1, &tio);
	tio.c_lflag &= ~(tcflag_t)(ICANON | ECHO);
	tio.c_iflag |= ICRNL;
	tio.c_cc[VMIN] = 1;
	tio.c_cc[VTIME] = 0;
	tcsetattr(1, TCSADRAIN, &tio);
	tcflush(1, TCIFLUSH);
	for (;;) {
		switch (read(1, &key, 1)) {
		case 0: quit(0);
			/*NOTREACHED*/
		case -1: quit(020);
		}
		if (key == tio.c_cc[VERASE]) {
			if (cmd.cmdlen) {
				p = previous(&cmd.cmdline[cmd.cmdlen],
						cmd.cmdline, &wc);
				*p = '\0';
				cmd.cmdlen = p - cmd.cmdline;
				if ((n = width(wc)) < 0)
					n = 1;
				while (n--)
					write(1, "\b \b", 3);
				if (!escape && cmd.cmdlen &&
				    *(p = previous(&cmd.cmdline[cmd.cmdlen],
						  cmd.cmdline, &wc)) == '\\') {
					*p = '\0';
					cmd.cmdlen = p - cmd.cmdline;
				}
				switch (state) {
				case ADDON_FIN:
					state = SEARCH_FIN;
					cmd.addon = '\0';
					break;
				case CMD_FIN:
					cmd.key = '\0';
					state = COUNT;
					break;
				case SEARCH_FIN:
					state = SEARCH;
					/*FALLTHRU*/
				case SEARCH:
					if (strchr(cmd.cmdline, cmd.key)
							== NULL) {
						cmd.key = '\0';
						state = COUNT;
					}
					break;
				default:	/* make gcc happy */;
				}
			}
			if (cmd.cmdlen == 0) {
				state = COUNT;
				cmd.key = '\0';
			}
			escape = 0;
			continue;
		} else if (key == tio.c_cc[VKILL]) {
			cline();
			cmd.cmdlen = 0;
			cmd.cmdline[0] = '\0';
			state = COUNT;
			cmd.key = '\0';
			escape = 0;
			continue;
		} else if ((nflag && state == COUNT && key == ' ') ||
				key == '\n' || key == '\r')
			break;
		else if (escape)
			write(1, "\b", 1);
		if (cmd.cmdlen >= sizeof cmd.cmdline - 1) {
			ring();
			continue;
		}
		switch (state) {
		case STRING:
			break;
		case SEARCH:
			if (!escape) {
				if (key == cmd.key)
					state = SEARCH_FIN;
			}
			break;
		case SEARCH_FIN:
			if (getstate(key) != ADDON_FIN) {
				ring();
				continue;
			}
			state = ADDON_FIN;
			cmd.addon = key;
			switch (key) {
			case 't':
				searchdisplay = TOP;
				break;
			case 'm':
				searchdisplay = MIDDLE;
				break;
			case 'b':
				searchdisplay = BOTTOM;
				break;
			}
			break;
		case CMD_FIN:
		case ADDON_FIN:
			state = INVALID;
			ring();
			continue;
		default:
			ostate = state;
			state = getstate(key);
			switch (state) {
			case SIGN:
				if (cmd.cmdlen != 0) {
					state = ostate;
					ring();
					continue;
				}
				state = COUNT;
				/*FALLTHRU*/
			case COUNT:
				break;
			case ADDON_FIN:
				ring();
				/*FALLTHRU*/
			case INVALID:
				state = ostate;
				continue;
			default:
				cmd.key = key;
			}
		}
		write(1, &key, 1);
		cmd.cmdline[cmd.cmdlen++] = key;
		cmd.cmdline[cmd.cmdlen] = '\0';
		if (nflag && state == CMD_FIN)
			goto endprompt;
		escape = (escape ? 0 : key == '\\');
	}
endprompt:
	tcsetattr(1, TCSADRAIN, &otio);
	cline();
	cmd.count = getcount(cmd.cmdline);
}

/*
 * Remove backspace formatting, for searches. Also replace NUL characters
 * by '?'.
 */
static char *
colb(char *s, char *end)
{
	char *p, *q;
	w_type	wc, lastwc = '\b';
	int	m, n, lastn = 0, r;

	p = q = s;
	while (p < end) {
		next(wc, p, n);
		if (wc == '\n')
			break;
		if (wc == '\b' && lastwc != '\b')
			q -= lastn + 1;
		else if (wc == 0)
			for (m = 0; m < n; m++)
				q[m] = '?';
		else if (seqstart(*p&0377) && (r = known_sequence(p)) > 0) {
			p += r;
			continue;
		} else
			for (m = 0; m < n; m++)
				q[m] = p[m];
		p += n, q += n;
		lastn = n;
		lastwc = wc;
	}
	*q = '\0';
	return s;
}

/*
 * Output a line, substituting nonprintable characters.
 */
static void
print1(const char *s, const char *end)
{
	char	buf[200], *bp;
	w_type	wc;
	int	i, m, n;
	unsigned	pos = 0;

	bp = buf;
	while (s < end) {
		next(wc, s, n);
		if (pos == 0 && wc == '\b') {
			s += n;
			continue;
		}
		pos = nextpos(wc, s, n, pos, NULL);
		if ((mb_cur_max > 1 ? !iswprint(wc) : !isprint(wc)) &&
				wc != '\n' && wc != '\r' &&
				wc != '\b' && wc != '\t') {
			if (seqstart(*s&0377) && (m = known_sequence(s)) > 0) {
				for (i = 0; i < m; i++) {
					*bp++ = s[i];
					if (bp-buf >= sizeof buf - mb_cur_max) {
						write(1, buf, bp - buf);
						bp = buf;
					}
				}
				s += m - n;
			} else if (utf8) {
				if (wc == WEOF) {
					*bp++ = 0357;
					*bp++ = 0277;
					*bp++ = 0275;
				} else {
					*bp++ = 0342;
					*bp++ = 0220;
					if (wc == 0177)
						*bp++ = 0241;
					else if (wc & ~(w_type)037)
						*bp++ = 0246;
					else
						*bp++ = 0200 + wc;
				}
			} else {
				for (i = 0; i < n; i++)
					*bp++ = '?';
			}
		} else {
			for (i = 0; i < n; i++)
				*bp++ = s[i];
		}
		s += n;
		if (bp - buf >= sizeof buf - mb_cur_max) {
			write(1, buf, bp - buf);
			bp = buf;
		}
	}
	if (bp > buf)
		write(1, buf, bp - buf);
}

/*
 * Extract the search pattern off the command line, i. e. skip the optional
 * count and the first occurence of the command key, then strip backslashes
 * until the command key appears again or the end of the string is reached.
 */
static char *
makepat(void)
{
	char *p, *q, *s;
	int	n;
	w_type	wc;

	s = cmd.cmdline;
	while (*s++ != cmd.key);
	p = q = s;
	do {
		next(wc, p, n);
		if (wc == '\\') {
			p += n;
			next(wc, p, n);
		} else if (wc == cmd.key) {
			*q++ = '\0';
			break;
		}
		while (n--)
			*q++ = *p++;
	} while (q[-1]);
	return s;
}

/*
 * Process errors that occurred in temporary file operations.
 */
static void
tmperr(FILE *f, const char *ftype)
{
	if (ferror(f))
		fprintf(stderr, catgets(catd, 1, 8,
					"%s: Read error from %s file\n"),
				progname, ftype);
	else if (feof(f))
		fprintf(stderr, catgets(catd, 1, 9,
					"%s: Unexpected EOF in %s file\n"),
				progname, ftype);
	else
		fprintf(stderr, catgets(catd, 1, 10,
					"%s: Unknown error in %s file\n"),
				progname, ftype);
	quit(010);
}

/*
 * perror()-like.
 */
static void
pgerror(int eno, const char *string)
{
	fprintf(stderr, "%s: %s\n", string, strerror(eno));
}

/*
 * Just copy stdin to stdout (if output is not a terminal).
 */
static void
plaincopy(FILE *f, const char *name)
{
	char	buf[4096];
	size_t sz;

	while ((sz = fread(buf, 1, sizeof buf, f)) != 0)
		write(1, buf, sz);
	if (ferror(f)) {
		pgerror(errno, name);
		exitstatus |= 1;
	}
}

/*
 * We can be sure that we are not multithreaded, so use the fast glibc
 * macro. Other libc implementations normally provide it as getc() per
 * default.
 */
#if defined (__GLIBC__) && defined (_IO_getc_unlocked)
#undef	getc
#define	getc(c)	_IO_getc_unlocked(c)
#endif

/*
 * Signals that EOF follows the last read line.
 */
static int	eofnext;

/*
 * Read a line from fp, allocating storage as needed.
 */
static char *
fgetline(char **line, size_t *linesize, size_t *llen, FILE *fp, int *colchar,
		int watcheof)
{
	int	c = EOF, n = 0;
	const int	incr = 4096;

	if (*line == NULL || *linesize < (incr << 1) + 1)
		if ((*line = irealloc(*line, *linesize = incr + 1)) == NULL) {
			write(2, "no memory\n", 11);
			quit(077);
		}
	*colchar = 0;
	for (;;) {
		if (n > *linesize - incr) {
			static int	oom;
			char	*nline;
			if (oom)
				break;
			nline = irealloc(*line, *linesize += (incr << 1));
			if (nline == NULL) {
				oom++;
				*linesize -= (incr << 1);
				break;
			} else
				*line = nline;
		}
		c = getc(fp);
		if (c != EOF) {
			if (c == '\b' || c == '\0' || seqstart(c))
				*colchar = 1;
			(*line)[n++] = (char)c;
			if (c == '\n')
				break;
		} else {
			if (n > 0)
				break;
			else
				return NULL;
		}
	}
	(*line)[n] = '\0';
	if (llen)
		*llen = n;
	eofnext = 0;
	if (watcheof) {
		if (c != EOF && (c = getc(fp)) != EOF)
			ungetc(c, fp);
		else
			eofnext = 1;
	}
	return *line;
}

/*
 * The following variables are used by the pgfile() subroutines only.
 *
 * These are the line counters:
 * line		the line desired to display
 * fline	the current line of the input file
 * bline	the current line of the file buffer
 * oldline	the line before a search was started
 * eofline	the last line of the file if it is already reached
 * dline	the line on the display
 */
static off_t	pos, fpos;
static off_t	line = 0, fline = 0, bline = 0, oldline = 0, eofline = 0;
static int	dline = 0;
/*
 * If a search direction is present in 'search', we're searching.
 * searchcount keeps the count of matches required.
 */
static enum direction	search = NOSEARCH;
static unsigned	searchcount = 0;
/*
 * Jump forward and redisplay last page if EOF is reached.
 */
static int	jumpcmd = 0;
/*
 * EOF has been reached by 'line'.
 */
static int	eof = 0;
/*
 * f and fbuf refer to the same file.
 */
static int	nobuf = 0;
/*
 * If 1, we need colb() when searching on the current line. gprof
 * says colb() takes 30% of execution time so it's worth the hack.
 *
 * Further optimization could unify fgetline() and endline(), but
 * we'll miss matches that span across screen line boundaries then
 * -- as Unix pg does --, and I think that's not acceptable.
 */
static int	colchar;
/*
 * Main line buffer, its allocated size and the length of the line
 * kept inside.
 */
static char	*b;
static size_t	bsize;
static size_t	llen;
/*
 * Location of last match, regular expression buffer,
 * and whether a remembered search string is available.
 */
#if !defined (SUS) && !defined (SU3)
static char	*re;
#else	/* SUS, SU3 */
static regex_t	re;
static char	*loc1;
#endif	/* SUS, SU3 */
static int	remembered;
/*
 * fbuf		an exact copy of the input file as it gets read
 * find		index table for input, one entry per line
 * save		for the s command, to save to a file
 */
static FILE	*fbuf, *find, *save;

/*
 * Initialize variables above.
 */
static void
varinit(void)
{
	line = fline = bline = oldline = eofline = 0;
	dline = 0;
	search = NOSEARCH;
	searchcount = 0;
	jumpcmd = 0;
	eof = 0;
	nobuf = 0;
}

/*
 * Initialize temporary files.
 */
static void
tempinit(FILE *f)
{
	if ((fpos = fseeko(f, (off_t)0, SEEK_SET)) == -1)
		fbuf = tmpfile();
	else {
		fbuf = f;
		nobuf = 1;
	}
	find = tmpfile();
	if (fbuf == NULL || find == NULL) {
		fprintf(stderr, catgets(catd, 1, 11,
				"%s: Cannot create tempfile\n"), progname);
		quit(010);
	}
}

/*
 * Message if skipping parts of files.
 */
static void
skipping(int direction)
{
	if (direction > 0) {
		mesg(catgets(catd, 1, 4, "...skipping forward\n"));
		jumpcmd = 1;
	} else
		mesg(catgets(catd, 1, 5, "...skipping backward\n"));
}

/*
 * Compile a pattern.
 */
#if !defined (SUS) && !defined (SU3)
static int
comple(const char *pattern)
{
	const char	*msg;

	if (remembered)
		free(re);
	if ((re = compile(pattern, NULL, NULL)) == NULL) {
		ring();
		switch (regerrno) {
		case 11:
			msg = "Range endpoint too large";
			break;
		case 16:
			msg = "Bad number";
			break;
		case 25:
			msg = "`\\digit' out of range";
			break;
		case 41:
			msg = "No remembered search string";
			break;
		case 42:
			msg = "\\( \\) imbalance";
			break;
		case 43:
			msg = "Too many \\(";
			break;
		case 44:
			msg = "More than two numbers given in \\{ \\}.";
			break;
		case 45:
			msg = "} expected after \\";
			break;
		case 46:
			msg = "First number exceeds second in \\{ \\}";
			break;
		case 49:
			msg = "[] imbalance";
			break;
		case 50:
			msg = "Regular expression overflow";
			break;
		case 67:
			msg = "Illegal byte sequence";
			break;
		default:
			msg = "Bad regular expression";
		}
		mesg(msg);
		remembered = 0;
		search = NOSEARCH;
		searchcount = 0;
		return STOP;
	}
	remembered = 1;
	return OKAY;
}
#else	/* SUS, SU3 */
static int
comple(const char *pattern)
{
	char	errmsg[LINE_MAX];
	int	rerror;
	int	reflags = 0			/* flags for Caldera REs */
#ifdef	REG_ANGLES
				| REG_ANGLES	/* enable \< \> */
#endif	/* REG_ANGLES */
#ifdef	REG_ONESUB
				| REG_ONESUB	/* need one match location */
#endif	/* REG_ONESUB */
#if defined (SU3) && defined (REG_AVOIDNULL)
				| REG_AVOIDNULL	/* avoid null matches */
#endif	/* SU3 && REG_AVOIDNULL */
			;

	if (remembered)
		regfree(&re);
	rerror = regcomp(&re, pattern, reflags);
	if (rerror != 0) {
		ring();
		regerror(rerror, &re, errmsg, sizeof errmsg);
		mesg(errmsg);
		remembered = 0;
		search = NOSEARCH;
		searchcount = 0;
		return STOP;
	}
	remembered = 1;
	return OKAY;
}
#endif	/* SUS, SU3 */

/*
 * Initial search from command line argument.
 */
static enum okay
initsearch(void)
{
	search = FORWARD;
	oldline = 0;
	searchcount = 1;
	if (comple(searchfor) != OKAY)
		return STOP;
	return OKAY;
}

/*
 * Initialize search forward or backward.
 */
static enum okay
patcomp(enum direction direction)
{
	char	*p;

	p = makepat();
	if (p != NULL && *p) {
		if (comple(p) != OKAY)
			return STOP;
	} else if (remembered == 0) {
		mesg(catgets(catd, 1, 15,
			"No remembered search string"));
		return STOP;
	}
	search = direction;
	oldline = line;
	searchcount = cmd.count;
	return OKAY;
}

/*
 * Search forward.
 */
static enum okay
searchfw(void)
{
	size_t	sz;

	if (eof) {
		line = oldline;
		search = NOSEARCH;
		searchcount = 0;
		ring();
		mesg(catgets(catd, 1, 13, "Pattern not found"));
		eof = 0;
		return STOP;
	}
	line++;
	if ((pos & LINE) == 0)
		return OKAY;
	if (colchar)
		colb(b, &b[llen]);
	else if (b[llen-1] == '\n')
		b[llen-1] = '\0';
#if !defined (SUS) && !defined (SU3)
	if (step(b, re))
		searchcount--;
#else	/* SUS, SU3 */
	{
		regmatch_t	loc;
		if (regexec(&re, b, 1, &loc, 0) == 0)
			searchcount--;
		else
			loc1 = &b[loc.rm_so];
	}
#endif	/* SUS, SU3 */
	if (searchcount == 0) {
		sz = 0;
		while (sz += endline(ttycols, &b[sz], &b[llen]), &b[sz] < loc1)
			line++;
		search = NOSEARCH;
		dline = 0;
		switch (searchdisplay) {
		case TOP:
			line -= 1;
			break;
		case MIDDLE:
			line -= pagelen / 2 + 1;
			break;
		case BOTTOM:
			line -= pagelen;
			break;
		}
		skipping(1);
	}
	return OKAY;
}

/*
 * Print match of a backward search.
 */
static void
foundbw(void)
{
	size_t	sz;

	eof = dline = 0;
	search = NOSEARCH;
	skipping(-1);
	sz = 0;
	while (sz += endline(ttycols, &b[sz], &b[llen]), &b[sz] < loc1)
		line++;
	switch (searchdisplay) {
	case TOP:
		/* line -= 1; */
		break;
	case MIDDLE:
		line -= pagelen / 2;
		break;
	case BOTTOM:
		if (line != 0)
			dline = -1;
		line -= pagelen;
		break;
	}
	if (line < 0)
		line = 0;
}

/*
 * Search backward.
 */
static enum okay
searchbw(void)
{
	line -= pagelen;
	while (line > 0) {
		fseeko(find, --line * sizeof pos, SEEK_SET);
		if(fread(&pos, sizeof pos, 1,find)==0)
			tmperr(find, "index");
		if ((pos & LINE) == 0)
			continue;
		fseeko(find, (off_t)0, SEEK_END);
		fseeko(fbuf, pos & MASK, SEEK_SET);
		if (fgetline(&b, &bsize, &llen, fbuf, &colchar, 0) == NULL)
			tmperr(fbuf, "buffer");
		if (colchar)
			colb(b, &b[llen]);
		else if (b[llen-1] == '\n')
			b[llen-1] = '\0';
#if !defined (SUS) && !defined (SU3)
		if (step(b, re))
			searchcount--;
#else	/* SUS, SU3 */
		{
			regmatch_t	loc;
			if (regexec(&re, b, 1, &loc, 0) == 0)
				searchcount--;
			else
				loc1 = &b[loc.rm_so];
		}
#endif	/* SUS, SU3 */
		if (searchcount == 0) {
			foundbw();
			return OKAY;
		}
	}
	line = oldline;
	search = NOSEARCH;
	searchcount = 0;
	ring();
	mesg(catgets(catd, 1, 13, "Pattern not found"));
	return STOP;
}

/*
 * Get the next line from input file or buffer.
 */
static void
nextline(FILE *f, const char *name)
{
	int	sig;
	size_t	sz, l;
	char	*p;

	if (line < bline) {
		fseeko(find, line * sizeof pos, SEEK_SET);
		if (fread(&pos, sizeof pos, 1, find) == 0)
			tmperr(find, "index");
		fseeko(find, (off_t)0, SEEK_END);
		fseeko(fbuf, pos & MASK, SEEK_SET);
		if (fgetline(&b, &bsize, &llen, fbuf, &colchar, 0) == NULL)
			tmperr(fbuf, "buffer");
	} else if (eofline == 0) {
		fseeko(find, (off_t)0, SEEK_END);
		do {
			if (!nobuf)
				fseeko(fbuf, (off_t)0, SEEK_END);
			pos = ftello(fbuf);
			if ((sig = setjmp(specenv)) != 0) {
				specjump = 0;
				sigrelse(sig);
				fseeko(fbuf, pos & MASK, SEEK_SET);
				llen = 0;
				dline = pagelen;
				if (search != NOSEARCH)
					line = oldline;
				search = NOSEARCH;
				searchcount = 0;
				break;
			} else {
				if (nobuf)
					fseeko(f, fpos, SEEK_SET);
				specjump = 1;
				p = fgetline(&b, &bsize, &llen, f, &colchar, 1);
				if (nobuf)
					if ((fpos = ftello(f)) == -1)
						pgerror(errno, name);
				specjump = 0;
			}
			if (p == NULL || llen == 0) {
				if (ferror(f))
					pgerror(errno, name);
				eofline = fline;
				eof = 1;
				break;
			} else {
				off_t	npos;

				if (!nobuf)
					fwrite(b,sizeof *b, llen, fbuf);
				npos = pos;
				pos |= LINE;
				fwrite(&pos, sizeof pos, 1, find);
				p = b;
				l = llen;
				while (sz = endline(ttycols, p, &p[l]),
							l -= sz, l > 0) {
					npos += sz;
					p += sz;
					fwrite(&npos, sizeof npos, 1, find);
					fline++;
					bline++;
				}
				fline++;
			}
		} while (line > bline++);
	} else {
		/*
		 * eofline != 0
		 */
		eof = 1;
	}
}

/*
 * Print current line.
 */
static void
printline(void)
{
	int	sig;
	size_t	sz;

	if (cflag && clear_screen) {
		switch (dline) {
		case 0:
			tputs(clear_screen, 1, outcap);
			dline = 0;
		}
	}
	line++;
	if (eofline && line == eofline)
		eof = 1;
	dline++;
	if ((sig = setjmp(specenv)) != 0) {
		specjump = 0;
		sigrelse(sig);
		dline = pagelen;
	} else {
		sz = endline(ttycols, b, &b[llen]);
		specjump = 1;
		print1(b, &b[sz]);
		/*
		 * Only force a line break if it is really necessary,
		 * i.e. if the line is incomplete or if wraparound on
		 * the terminal is known to be unreliable because of
		 * tabulators. This allows text selection on an xterm
		 * across line wraps.
		 */
		if (b[sz-1] != '\n' && (sz == llen ||
					b[sz-1] == '\t' || b[sz] == '\t'))
			write(1, "\n", 1);
		specjump = 0;
	}
}

/*
 * Save to file.
 */
static void
savefile(FILE *f)
{
	size_t	sz, l;
	char	*p;

	p = cmd.cmdline;
	while (*++p == ' ');
	if (*p == '\0') {
		ring();
		return;
	}
	save = fopen(p, "wb");
	if (save == NULL) {
		cmd.count = errno;
		mesg(catgets(catd, 1, 16, "Cannot open "));
		mesg(p);
		mesg(": ");
		mesg(strerror(cmd.count));
		return;
	}
	/*
	 * Advance to EOF.
	 */
	fseeko(find, (off_t)0, SEEK_END);
	for (;;) {
		off_t npos;

		if (!nobuf)
			fseeko(fbuf,(off_t)0,SEEK_END);
		pos = ftello(fbuf);
		if (fgetline(&b, &bsize, &llen, f, &colchar, 0) == NULL) {
			eofline = fline;
			break;
		}
		if (!nobuf)
			fwrite(b,sizeof *b, llen, fbuf);
		npos = pos;
		pos |= LINE;
		fwrite(&pos, sizeof pos, 1, find);
		p = b;
		l = llen;
		while (sz = endline(ttycols, p, &p[l]), l -= sz, l > 0) {
			npos += sz;
			p += sz;
			fwrite(&npos, sizeof npos, 1, find);
			fline++;
			bline++;
		}
		fline++;
		bline++;
	}
	fseeko(fbuf, (off_t)0, SEEK_SET);
	while ((sz = fread(b, sizeof *b, bsize, fbuf)) != 0) {
		/*
		 * No error check for compat.
		 */
		fwrite(b, sizeof *b, sz, save);
	}
	fclose(save);
	fseeko(fbuf, (off_t)0, SEEK_END);
	mesg(catgets(catd, 1, 17, "saved"));
}

/*
 * Next line.
 */
static void
linefw(void)
{
	size_t	oline = line;

	if (*cmd.cmdline != 'l')
		eof = 0;
	if (cmd.count == 0)
		cmd.count = 1; /* compat */
	if (isdigit(*cmd.cmdline & 0377)) {
		line = cmd.count - 1;
		dline = 0;
	} else {
		if (cmd.count != 1) {
			line += cmd.count - pagelen;
			dline = 0;
		}
		/*
		 * Nothing to do if count==1.
		 */
	}
	if (oline - pagelen > line || oline < line) {
		line--;
		skipping(cmd.count);
		dline = -1;
	}
}

/*
 * Half screen forward.
 */
static void
halffw(void)
{
	if (*cmd.cmdline != cmd.key)
		eof = 0;
	if (cmd.count == 0)
		cmd.count = 1; /* compat */
	line += (cmd.count * pagelen / 2) - pagelen - 1;
	dline = -1;
	skipping(cmd.count);
}

/*
 * Skip forward.
 */
static void
skipfw(void)
{
	if (cmd.count == 0)
		cmd.count = 1; /* compat */
	else if (cmd.count < 0)
		cmd.count--;
	line += cmd.count * (pagelen - 1) - 2;
	if (eof)
		line += 2;
	if (*cmd.cmdline != 'f')
		eof = 0;
	else if (eof)
		return;
	if (eofline && line >= eofline)
		line -= pagelen;
	dline = -1;
	skipping(cmd.count);
}

/*
 * Just a number, or '-', or <newline>.
 */
static void
jump(void)
{
	if (cmd.count == 0)
		cmd.count = 1; /* compat */
	if (isdigit(*cmd.cmdline&0377))
		line = (cmd.count - 1) * (pagelen - 1) - 1;
	else
		line += (cmd.count - 1) * (pagelen - 1) - 1;
	if (*cmd.cmdline != '\0')
		eof = 0;
	if (cmd.count != 1) {
		skipping(cmd.count);
		dline = -1;
	} else {
		dline = 1;
		line += 1;
	}
	if (cmd.count < 0)
		line--;
}

/*
 * Advance to EOF.
 */
static void
toeof(void)
{
	if (!eof)
		skipping(1);
	eof = 0;
	line = LONG_MAX;
	jumpcmd = 1;
	dline = -1;
}

/*
 * EOF has been reached; adjust variables to display last screen.
 */
static void
oneof(void)
{
	eof = jumpcmd = 0;
	if (line >= pagelen)
		line -= pagelen;
	else
		line = 0;
	dline = -1;
}

/*
 * Redraw screen.
 */
static void
redraw(void)
{
	eof = 0;
	if (line >= pagelen)
		line -= pagelen;
	else
		line = 0;
	dline = 0;
}

/*
 * Shell escape.
 */
static void
shell(FILE *f)
{
	pid_t	cpid;
	char	*p;

	if (rflag) {
		mesg(progname);
		mesg(catgets(catd, 1, 18,
				": !command not allowed in rflag mode.\n"));
		return;
	}
	write(1, cmd.cmdline, strlen(cmd.cmdline));
	write(1, "\n", 1);
	sigset(SIGINT, SIG_IGN);
	sigset(SIGQUIT, SIG_IGN);
	switch (cpid = fork()) {
	case 0:
		if ((p = getenv("SHELL")) == NULL)
			p = "/bin/sh";
		if (!nobuf)
			fcntl(fileno(fbuf), F_SETFD, FD_CLOEXEC);
		fcntl(fileno(find), F_SETFD, FD_CLOEXEC);
		if (isatty(0) == 0) {
			close(0);
			open(tty, O_RDONLY);
		} else
			fcntl(fileno(f), F_SETFD, FD_CLOEXEC);
		sigset(SIGINT, oldint);
		sigset(SIGQUIT, oldquit);
		sigset(SIGTERM, oldterm);
		execl(p, p, "-c", cmd.cmdline + 1, NULL);
		pgerror(errno, p);
		_exit(0177);
		/*NOTREACHED*/
	case -1:
		mesg(catgets(catd, 1, 19,
				"fork() failed, try again later\n"));
		break;
	default:
		while (wait(NULL) != cpid);
	}
	if (oldint != SIG_IGN)
		sigset(SIGINT, sighandler);
	if (oldquit != SIG_IGN)
		sigset(SIGQUIT, sighandler);
	mesg("!\n");
}

/*
 * Help!
 */
static void
help(void)
{
	write(1, helpscreen, strlen(helpscreen));
}

/*
 * Next or previous file.
 */
static enum okay
newfile(int count)
{
	nextfile = count;
	if (checkf() != OKAY) {
		nextfile = 1;
		return STOP;
	}
	eof = 1;
	return OKAY;
}

/*
 * Set window size.
 */
static void
windowsize(void)
{
	if (cmd.count < 0) {
		ring();
		cmd.count = 0;
	}
	if (*cmd.cmdline != cmd.key)
		pagelen = ++cmd.count;
	dline = 1;
}

/*
 * Read the file and respond to user input.
 */
static void
pgfile(FILE *f, const char *name)
{
	int	sig;

	if (ontty == 0) {
		plaincopy(f, name);
		return;
	}
	varinit();
	tempinit(f);
	if (searchfor) {
		if (initsearch() != OKAY)
			goto newcmd;
	}
	for (line = startline; eof == 0; ) {
		nextline(f, name);
		if (search == FORWARD) {
			if (searchfw() == OKAY)
				continue;
			goto newcmd;
		} else if (eof)	{	/*
					 * We are not searching.
					 */
			line = bline;
		} else if (llen > 0)
			printline();
		if (dline >= pagelen && !eofnext || eof) {
			if (eof && jumpcmd) {
				oneof();
				continue;
			}
			/*
			 * Time for prompting!
			 */
	newcmd:		if ((sig = setjmp(genenv)) != 0) {
				sigrelse(sig);
				search = NOSEARCH;
				searchcount = 0;
			}
			jumpcmd = 0;
			if (eof) {
				if (fline == 0 || eflag)
					break;
				mesg(catgets(catd, 1, 14, "(EOF)"));
			}
			genjump = 0;
			prompt((line + pagelen - 3) / (pagelen - 1));
			genjump = 1;
			switch (cmd.key) {
			case '/':
				if (patcomp(FORWARD) != OKAY)
					goto newcmd;
				continue;
			case '?':
			case '^':
				if (patcomp(BACKWARD) != OKAY)
					goto newcmd;
				if (searchbw() != OKAY)
					goto newcmd;
				continue;
			case 's':
				savefile(f);
				goto newcmd;
			case 'l':
				linefw();
				break;
			case 'd':
			case '\004':	/* ^D */
				halffw();
				break;
			case 'f':
				skipfw();
				break;
			case '\0':
				jump();
				break;
			case '$':
				toeof();
				break;
			case '.':
			case '\014': /* ^L */
				redraw();
				break;
			case '!':
				shell(f);
				goto newcmd;
			case 'h':
				help();
				goto newcmd;
			case 'n':
				if (newfile(cmd.count ? cmd.count : 1) != OKAY)
					goto newcmd;
				break;
			case 'p':
				if (newfile(cmd.count ? 0 - cmd.count : -1)
						!= OKAY)
					goto newcmd;
				break;
			case 'q':
			case 'Q':
				quit(exitstatus);
				/*NOTREACHED*/
			case 'w':
			case 'z':
				windowsize();
				break;
			}
			if (line <= 0) {
				line = 0;
				dline = 0;
			}
			if (cflag && dline == 1) {
				dline = 0;
				line--;
			}
		}
		if (eof)
			break;
	}
	genjump = 0;
	fclose(find);
	if (!nobuf)
		fclose(fbuf);
}

/*
 * Display the given files.
 */
static void
run(char **av, int ac)
{
	int	arg;
	FILE	*input;

	files.first = 0;
	files.last = ac - 1;
	for (arg = 0; av[arg]; arg += nextfile) {
		nextfile = 1;
		files.current = arg;
		if (ac > 2) {
			static int firsttime;

			if (firsttime++ > 0) {
				mesg(catgets(catd, 1, 20, "(Next file: "));
				mesg(av[arg]);
				mesg(")");
		newfile:	if (ontty) {
					prompt(-1);
					switch(cmd.key) {
					case 'n':
						/*
					 	 * Next file.
					 	 */
						if (cmd.count == 0)
							cmd.count = 1;
						nextfile = cmd.count;
						if (checkf()) {
							nextfile = 1;
							mesg(":");
							goto newfile;
						}
						continue;
					case 'p':
						/*
					 	 * Previous file.
					 	 */
						if (cmd.count == 0)
							cmd.count = 1;
						nextfile = 0 - cmd.count;
						if (checkf()) {
							nextfile = 1;
							mesg(":");
							goto newfile;
						}
						continue;
					case 'q':
					case 'Q':
						quit(exitstatus);
					}
				} else
					mesg("\n");
			}
		}
		if (strcmp(av[arg], "-") == 0)
			input = stdin;
		else {
			input = fopen(av[arg], "r");
			if (input == NULL) {
				pgerror(errno, av[arg]);
				exitstatus |= 01;
				continue;
			}
		}
		if (ontty == 0 && ac > 2) {
			/*
			 * Use the prefix as specified by SUSv2.
			 */
			write(1, "::::::::::::::\n", 15);
			write(1, av[arg], strlen(av[arg]));
			write(1, "\n::::::::::::::\n", 16);
		}
		pgfile(input, av[arg]);
		if (input != stdin)
			fclose(input);
	}
}

int
main(int argc, char **argv)
{
	int arg, i;
	char *cp;

	if (sizeof LINE == 4) {
		LINE = 020000000000UL;
		MASK = 017777777777UL;
	} else if (sizeof LINE == 8) {
		LINE = 01000000000000000000000ULL;
		MASK = 0777777777777777777777ULL;
	} else
		abort();	/* need to fill in values for your off_t here */
	progname = basename(argv[0]);
	/*setlocale(LC_MESSAGES, "");*/
	catd = catopen(CATNAME, NL_CAT_LOCALE);
	if (tcgetattr(1, &otio) == 0) {
		ontty = 1;
		if ((oldint = sigset(SIGINT, SIG_IGN)) != SIG_IGN)
			sigset(SIGINT, sighandler);
		if ((oldquit = sigset(SIGQUIT, SIG_IGN)) != SIG_IGN)
			sigset(SIGQUIT, sighandler);
		if ((oldterm = sigset(SIGTERM, SIG_IGN)) != SIG_IGN)
			sigset(SIGTERM, sighandler);
		setlocale(LC_COLLATE, "");
		setlocale(LC_CTYPE, "");
		tty = ttyname(1);
#ifndef	USE_TERMCAP
		setupterm(NULL, 1, &tinfostat);
#else	/* USE_TERMCAP */
		if ((cp = getenv("TERM")) != NULL) {
			char	buf[2048];
			if ((tinfostat = tgetent(buf, cp)) > 0) {
				lines = tgetnum("li");
				columns = tgetnum("co");
			}
		}
#endif	/* USE_TERMCAP */
		getwinsize();
		helpscreen = catgets(catd, 1, 21, helpscreen);
		fill_sequences();
	}
	if (argc == 0)
		return 0;
#ifdef	ENABLE_WIDECHAR
	if ((mb_cur_max = MB_CUR_MAX) > 1) {
		wchar_t	wc;
		if (mbtowc(&wc, "\303\266", 2) == 2 && wc == 0xF6 &&
				mbtowc(&wc, "\342\202\254", 3) == 3 &&
				wc == 0x20AC)
			utf8 = 1;
	}
#endif	/* ENABLE_WIDECHAR */
	for (arg = 1; argv[arg]; arg++) {
		if (*argv[arg] == '+')
			continue;
		if (*argv[arg] != '-' || argv[arg][1] == '\0')
			break;
		argc--;
		for (i = 1; argv[arg][i]; i++) {
			switch (argv[arg][i]) {
			case '-':
				if (i != 1 || argv[arg][i + 1])
					invopt(&argv[arg][i]);
				goto endargs;
			case '1': case '2': case '3': case '4': case '5':
			case '6': case '7': case '8': case '9': case '0':
				pagelen = atoi(argv[arg] + i);
				havepagelen = 1;
				goto nextarg;
			case 'c':
				cflag = 1;
				break;
			case 'e':
				eflag = 1;
				break;
			case 'f':
				fflag = 1;
				break;
			case 'n':
				nflag = 1;
				break;
			case 'p':
				if (argv[arg][i + 1]) {
					pstring = &argv[arg][i + 1];
				} else if (argv[++arg]) {
					--argc;
					pstring = argv[arg];
				} else
					needarg("-p");
				goto nextarg;
			case 'r':
				rflag = 1;
				break;
			case 's':
				sflag = 1;
				break;
			default:
				invopt(&argv[arg][i]);
			}
		}
nextarg:
		;
	}
endargs:
	for (arg = 1; argv[arg]; arg++) {
		if (*argv[arg] == '-') {
			if (argv[arg][1] == '-') {
				arg++;
				break;
			}
			if (argv[arg][1] == '\0')
				break;
			if (argv[arg][1] == 'p' && argv[arg][2] == '\0')
				arg++;
			continue;
		}
		if (*argv[arg] != '+')
			break;
		argc--;
		switch (*(argv[arg] + 1)) {
		case '\0':
			needarg("+");
			/*NOTREACHED*/
		case '1': case '2': case '3': case '4': case '5':
		case '6': case '7': case '8': case '9': case '0':
			startline = atoi(argv[arg] + 1);
			break;
		case '/':
			searchfor = argv[arg] + 2;
			if (*searchfor == '\0')
				needarg("+/");
			cp = searchfor + strlen(searchfor) - 1;
			if (*cp == '/') *cp = '\0';
			if (*searchfor == '\0')
				needarg("+/");
			break;
		default:
			invopt(argv[arg]);
		}
	}
	if (argc == 1)
		pgfile(stdin, "stdin");
	else
		run(&argv[arg], argc);
	quit(exitstatus);
	/*NOTREACHED*/
	return 0;
}
