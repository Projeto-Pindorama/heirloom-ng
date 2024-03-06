/*
 * expand - convert tabs to spaces
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, May 2003.
 */
/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
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
