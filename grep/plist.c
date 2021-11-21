/*
 * grep - search a file for a pattern
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, April 2001.
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

/*	Sccsid @(#)plist.c	1.22 (gritter) 12/8/04>	*/

/*
 * Pattern list routines.
 */

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<ctype.h>

#include	"grep.h"
#include	"alloc.h"

/*
 * Add a pattern starting at the given node of the expression list.
 */
static void
addpat(struct expr **e, char *pat, long len, enum eflags flg)
{
	if (e0) {
		(*e)->e_nxt = (struct expr *)smalloc(sizeof **e);
		(*e) = (*e)->e_nxt;
	} else
		e0 = (*e) = (struct expr *)smalloc(sizeof **e);
	if (wflag)
		wcomp(&pat, &len);
	(*e)->e_nxt = NULL;
	(*e)->e_pat = pat;
	(*e)->e_len = len;
	(*e)->e_flg = flg;
}

/*
 * Read patterns from pattern string. In traditional command versions, -f
 * overrides all -e and all previous -f options. In POSIX.2 command versions,
 * all -e and -f options are cumulated.
 */
void
patstring(char *cp)
{
	struct expr *e = NULL;
	char *ep;
	int nl;

	if (e0) {
		if (sus)
			for (e = e0; e->e_nxt; e = e->e_nxt);
		else if (fflag)
			return;
		else
			e0 = NULL;
	}
	if (cp) {
		do {
			if ((nl = (ep = strchr(cp, '\n')) != NULL) != 0)
				*ep = 0;
			addpat(&e, cp, ep ? ep - cp : strlen(cp), nl);
			cp = ep + 1;
			if (nl)
				*ep = '\n';
		} while (ep);
	} else
		addpat(&e, strdup(""), 0, E_NULL);
}

/*
 * Read patterns from file.
 */
void
patfile(char *fn)
{
	struct stat st;
	struct expr *e = NULL;
	char *cp;
	struct iblok	*ip;
	size_t sz, len;
	int nl;

	if ((ip = ib_open(fn, 0)) == NULL || fstat(ip->ib_fd, &st) < 0) {
		fprintf(stderr, "%s: can't open %s\n", progname, fn);
		exit(2);
	}
	if (e0) {
		if (sus)
			for (e = e0; e->e_nxt; e = e->e_nxt);
		else
			e0 = NULL;
	}
	while (cp = NULL, sz = 0,
			(len = ib_getlin(ip, &cp, &sz, srealloc)) > 0) {
		if ((nl = cp[len - 1] == '\n') != 0)
			cp[len - 1] = '\0';
		addpat(&e, cp, len - nl, nl);
	}
	ib_close(ip);
}

/*
 * getc() substitute operating on the pattern list.
 */
int
nextch(void)
{
	static struct expr *e;
	static char *cp;
	static long len;
	static int oneof;
	wchar_t wc;
	int n;

	if (oneof)
		return EOF;
	if (e == NULL) {
		e = e0;
		if (e->e_flg & E_NULL) {
			oneof++;
			return EOF;
		}
	}
	if (cp == NULL) {
		cp = e->e_pat;
		len = e->e_len;
	}
	if (mbcode && *cp & 0200) {
		if ((n = mbtowc(&wc, cp, MB_LEN_MAX)) < 0) {
			fprintf(stderr, "%s: illegal byte sequence\n",
				progname);
			exit(1);
		}
		cp += n;
		len -= n;
	} else {
		wc = *cp++ & 0377;
		len--;
	}
	if (len >= 0)
		return iflag ? mbcode && wc & ~(wchar_t)0177 ?
			towlower(wc) : tolower(wc) : wc;
	cp = NULL;
	n = e->e_flg & E_NL;
	if ((e = e->e_nxt) == NULL) {
		oneof++;
		if (!n)
			return EOF;
	}
	return '\n';
}

/*
 * Print matching line based on ip->ib_cur and moff. Advance ip->ib_cur to start
 * of next line. Used from special rangematch functions.
 */
void
outline(struct iblok *ip, char *last, size_t moff)
{
	register char *sol, *eol;	/* start and end of line */

	if (qflag == 0) {
		if (status == 1)
			status = 0;
		if (lflag) {
			puts(filename ? filename : stdinmsg);
		} else {
			lmatch++;
			sol = ip->ib_cur + moff;
			if (*sol == '\n' && sol > ip->ib_cur)
				sol--;
			while (sol > ip->ib_cur && *sol != '\n')
				sol--;
			if (sol > ip->ib_cur)
				sol++;
			ip->ib_cur += moff;
			for (eol = ip->ib_cur; eol <= last
					&& *eol != '\n'; eol++);
			if (!cflag)
				report(sol, eol - sol, ib_offs(ip) / BSZ, 1);
			ip->ib_cur = eol + 1;
		}
	} else	/* qflag != 0 */
		exit(0);
}
