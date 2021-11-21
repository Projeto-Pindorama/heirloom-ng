/*
 * expand - convert tabs to spaces
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, May 2003.
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

/*	Sccsid @(#)tablist.c	1.4 (gritter) 7/16/04	*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "atoll.h"

#include "tablist.h"

struct tab	*tablist;
int		tabstop = 8;

void
settab(const char *s)
{
	char	*x, *y;
	long long	n;

	if (s == NULL || *s == '\0')
		badts();
	tablist = 0;
	n = strtoll(s, &x, 10);
	if (n == 0)
		badts();
	if (*x == '\0' && *s != '+' && *s != '-') {
		tabstop = n;
		return;
	}
	tabstop = 1;
	addtab(n);
	do {
		if (*x == ',' || *x == ' ' || *x == '\t')
			x++;
		n = strtoll(x, &y, 10);
		if (*x == '+' || *x == '-' || x == y)
			badts();
		x = y;
		addtab(n);
	} while (*x);
}

void
addtab(long long n)
{
	struct tab	*tp, *tq;

	tp = scalloc(1, sizeof *tp);
	tp->t_pos = n;
	if (tablist) {
		for (tq = tablist; tq->t_nxt; tq = tq->t_nxt);
		if (tq->t_pos >= tp->t_pos)
			badts();
		tq->t_nxt = tp;
	} else
		tablist = tp;
}

void
badts(void)
{
	fprintf(stderr, "Bad tab stop spec\n");
	exit(2);
}

void *
scalloc(size_t nmemb, size_t size)
{
	void	*vp;

	if ((vp = calloc(nmemb, size)) == NULL) {
		write(2, "no memory\n", 10);
		_exit(077);
	}
	return vp;
}
