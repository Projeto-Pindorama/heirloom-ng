/*
   Changes by Gunnar Ritter, Freiburg i. Br., Germany, December 2002.
  
   Sccsid @(#)b.c	1.6 (gritter) 5/15/04>
 */
/* UNIX(R) Regular Expression Tools

   Copyright (C) 2001 Caldera International, Inc.
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to:
       Free Software Foundation, Inc.
       59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/*	copyright	"%c%"	*/

/*	from unixsrc:usr/src/common/cmd/awk/b.c /main/uw7_nj/1	*/

#include <stdio.h>
#include "awk.h"
#include <ctype.h>
#include "y.tab.h"
#include <pfmt.h>

unsigned char	*patbeg;
int	patlen;

static void
reprob(fa *f, int e)
{
	char msg[BUFSIZ];

	regerror(e, &f->re, msg, sizeof(msg));
	error(MM_ERROR, ":104:Error in RE `%s': %s", f->restr, msg);
}

static fa *
mkdfa(unsigned char *s)	/* build DFA from s */
{
	fa *pfa;
	int i;
	int flags;

	if ((pfa = (fa *)malloc(sizeof(fa))) == 0)
	{
		error(MM_ERROR,
			"5:Regular expression too big: out of space in %s", s);
	}
	flags = posix ? REG_EXTENDED : REG_OLDERE | REG_OLDESC | REG_NOI18N;
	flags |= REG_ONESUB | REG_BKTEMPTY | REG_BKTESCAPE | REG_ESCSEQ;
	if ((i = regcomp(&pfa->re, (char *)s, flags)) != 0)
	{
		pfa->restr = s;
		reprob(pfa, i);
	}
	pfa->restr = tostring(s);
	pfa->use = 1;
	pfa->notbol = 0;
	return pfa;
}

fa *
makedfa(unsigned char *s, int leftmost)	/* build and cache DFA from s */
{
	static fa *fatab[20];
	static int nfatab;
	int i, n, u;
	fa *pfa;

	if (compile_time)
		return mkdfa(s);
	/*
	* Search for a match to those cached.
	* If not found, save it, tossing least used one when full.
	*/
	for (i = 0; i < nfatab; i++)
	{
		if (strcmp((char *)fatab[i]->restr, (char *)s) == 0)
		{
			fatab[i]->use++;
			return fatab[i];
		}
	}
	pfa = mkdfa(s);
	if ((n = nfatab) < sizeof(fatab) / sizeof(fa *))
		nfatab++;
	else
	{
		n = 0;
		u = fatab[0]->use;
		for (i = 1; i < sizeof(fatab) / sizeof(fa *); i++)
		{
			if (fatab[i]->use < u)
			{
				n = i;
				u = fatab[n]->use;
			}
		}
		free((void *)fatab[n]->restr);
		regfree(&fatab[n]->re);
		free((void *)fatab[n]);
	}
	fatab[n] = pfa;
	return pfa;
}

int
match(void *v, unsigned char *p)	/* does p match f anywhere? */
{
	int err;
	fa *f = v;

	if ((err = regexec(&f->re, (char *)p, (size_t)0, (regmatch_t *)0, 0)) == 0)
		return 1;
	if (err != REG_NOMATCH)
		reprob(f, err);
	return 0;
}

int
pmatch(void *v, unsigned char *p) /* find leftmost longest (maybe empty) match */
{
	regmatch_t m;
	int err;
	fa *f = v;

	if ((err = regexec(&f->re, (char *)p, (size_t)1, &m, f->notbol)) == 0)
	{
		patbeg = &p[m.rm_so];
		patlen = m.rm_eo - m.rm_so;
		return 1;
	}
	if (err != REG_NOMATCH)
		reprob(f, err);
	patlen = -1;
	return 0;
}

int
nematch(void *v, unsigned char *p) /* find leftmost longest nonempty match */
{
	regmatch_t m;
	int err;
	fa *f = v;

	for (;;)
	{
		if ((err = regexec(&f->re, (char *)p, (size_t)1, &m,
			f->notbol | REG_NONEMPTY)) == 0)
		{
			if ((patlen = m.rm_eo - m.rm_so) == 0)
			{
				p += m.rm_eo;
				continue;
			}
			patbeg = &p[m.rm_so];
			return 1;
		}
		if (err != REG_NOMATCH)
			reprob(f, err);
		patlen = -1;
		return 0;
	}
}
