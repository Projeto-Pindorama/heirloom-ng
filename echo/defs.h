/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */

/*
 * Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)defs.h	1.3 (gritter) 7/2/05
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <unistd.h>

#if defined (__GLIBC__) && defined (_IO_getc_unlocked)
#undef	putchar
#define	putchar(c)	_IO_putc_unlocked(c, stdout)
#endif

struct namnod {
	char	*value;
	int	namflg;
};

#define	N_EXPORT	1
#define	N_ENVNAM	2

#ifdef	SUS
#define	ERROR		2
#else	/* !SUS */
#define	ERROR		1
#endif	/* !SUS */

#define	badop		"unknown operator "
#define	badopt		"bad option(s)"
#define	btest		"test"
#define	colon		": "

#ifndef	UCB
#define	ucb_builtins	0
#else	/* UCB */
#define	ucb_builtins	1
#endif	/* UCB */

extern int		chk_access(const char *, mode_t, int);
#define	eq(a, b)	(strcmp(a, b) == 0)
#define	exitsh(n)	exit(n)
extern void		failed(const char *, const char *);
extern struct namnod	*findnam(const char *);
#define	flushb()	fflush(stdout)
#define	newline()	putchar('\n')
#define	prc_buff(c)	putchar(c)
#define	prp()		fputs("test: ", stdout)
#define	prs(s)		fputs(s, stdout)
#define	sigchk()
extern long long	stoifll(const char *);

extern int	mb_cur_max;
#define	nextc(wc, sp)	(mb_cur_max > 1 && *(sp) & 0200 ? \
				mbtowc(wc, sp, mb_cur_max) : \
				(*(wc) = *(sp) & 0377, *(wc) != 0))
