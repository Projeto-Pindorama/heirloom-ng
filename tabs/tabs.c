/*
 * tabs - set terminal tabs
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
static const char sccsid[] USED = "@(#)tabs.sl	1.11 (gritter) 5/29/05";

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<signal.h>
#include	"sigset.h"
#include	<string.h>
#include	<ctype.h>
#include	<libgen.h>
#ifndef	__sun
#include	<termios.h>
#endif	/* __sun */
#ifndef	USE_TERMCAP
#include	<curses.h>
#include	<term.h>
#else	/* USE_TERMCAP */
#include	<termcap.h>
#endif	/* USE_TERMCAP */

#include	"tabspec.h"

static int		status;		/* exit status */
static int		margin = -1;	/* set left margin */
static char		*TERM;		/* TERM environment variable */
static struct tabulator	*tabspec;	/* tab spec */
static struct stat	*devsp;		/* stdout stat */
static struct termios	devts;		/* stdout termios */

#ifdef	USE_TERMCAP
static char	*set_left_margin;
static char	*clear_all_tabs;
static char	*set_tab;
static int	columns;
#endif	/* USE_TERMCAP */

static void
quit(int signo)
{
	if (devsp) {
		tcsetattr(1, TCSADRAIN, &devts);
		fchmod(1, devsp->st_mode & 07777);
	}
	exit(signo ? signo | 0200 : status);
}

static void
prepare(void)
{
	static struct stat	st;
	struct termios	nts;

	if (sigset(SIGINT, SIG_IGN) != SIG_IGN)
		sigset(SIGINT, quit);
	if (tcgetattr(1, &devts) == 0) {
		if (fstat(1, &st) == 0) {
			devsp = &st;
			fchmod(1, 0);
			nts = devts;
			nts.c_oflag &= ~(ONLCR|OCRNL|ONOCR|ONLRET);
			tcsetattr(1, TCSADRAIN, &nts);
		}
	}
}

static int
put(int i)
{
	char	c = i;
	return write(1, &c, 1);
}

static void
set(void)
{
	struct tabulator	*tp;
	int	col;

	if (margin > 0) {
		put('\r');
		for (col = 1; col < margin; col++)
			put(' ');
		tputs(set_left_margin, 1, put);
	}
	put('\r');
	tputs(clear_all_tabs, 1, put);
	for (col = 1, tp = tabspec; tp && tp->t_tab <= columns; tp = tp->t_nxt){
		while (col < tp->t_tab) {
			put(' ');
			col++;
		}
		if (tp->t_tab > 0)
			tputs(set_tab, 1, put);
	}
	put('\r');
}

static void
stspec(const char *s)
{
	if ((tabspec = tabstops(s, columns)) == NULL) {
		switch (taberrno) {
		case TABERR_CANTOP:
			fprintf(stderr, "can't open\n");
			break;
		case TABERR_FILIND:
			fprintf(stderr, "file indirection\n");
			break;
		case TABERR_UNKTAB:
			fprintf(stderr, "unknown tab code\n");
			break;
		case TABERR_ILLINC:
			fprintf(stderr, "illegal increment\n");
			break;
		case TABERR_ILLTAB:
		default:
			fprintf(stderr, "illegal tabs\n");
		}
		status = 2;
		quit(0);
	}
}

static void
scan(int ac, char **av, int really)
{
	int	c;

	for (c = 1; c < ac; c++) {
		switch (av[c][0]) {
		case '+':
			if (really && av[c][1] == 'm') {
				if (av[c][2])
					margin = atoi(&av[c][2]);
				else {
					if (++c < ac)
						margin = atoi(av[c]);
					else
						margin = 10;
				}
				continue;
			}
			/*FALLTHRU*/
		default:
			if (really)
				stspec(av[c]);
			break;
		case '-':
			if (av[c][1] == 'T') {
				if (av[c][2])
					TERM = &av[c][2];
				else if (++c < ac)
						TERM = av[c];
				else {
					fprintf(stderr, "Missing argument "
							"to -T option\n");
					quit(2);
				}
			} /*else if (av[c][1] == '-' && av[c][2] == '\0') {
				while (++c < ac)
					stspec(av[c]);
				return;
			} */else if (really)
				stspec(av[c]);
		}
	}
}

#ifdef	USE_TERMCAP
#ifndef	ERR
#define	ERR	0
#endif
#ifndef	OK
#define	OK	1
#endif
static int
setupterm(char *d1, int d2, int *d3)
{
	static char	buf[2048];
	static char	tspace[2048];
	static char	*tptr = tspace;

	TERM = strdup(getenv("TERM"));
	if (TERM == NULL || tgetent(buf, TERM) == NULL) {
		if (d3)
			*d3 = 0;
		return ERR;
	}
	set_tab = tgetstr("st", &tptr);
	clear_all_tabs = tgetstr("ct", &tptr);
	set_left_margin = tgetstr("ML", &tptr);
	columns = tgetnum("co");
	return OK;
}
#endif	/* USE_TERMCAP */

int
main(int argc, char **argv)
{
	int	err;

	scan(argc, argv, 0);
	if (setupterm(TERM, 1, &err) != OK) {
		fprintf(stderr, "no terminal description for terminal type "
				"%s\n", TERM ? TERM : getenv("TERM"));
		quit(1);
	}
	scan(argc, argv, 1);
	if (tabspec == 0)
		stspec("-8");
	if (set_tab == NULL || clear_all_tabs == NULL) {
		fprintf(stderr, "tabs not supported on terminal type %s\n",
				TERM ? TERM : getenv("TERM"));
		quit(1);
	}
	prepare();
	set();
	quit(0);
	/*NOTREACHED*/
	return 0;
}
