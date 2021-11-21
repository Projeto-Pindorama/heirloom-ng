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

/*	Sccsid @(#)tabspec.c	1.2 (gritter) 5/30/03	*/

#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<unistd.h>

#include	<iblok.h>
#include	<blank.h>
#include	"tabspec.h"

static const struct {
	const char	*c_nam;
	const char	*c_str;
} canned[] = {
	{ "a",	"1,10,16,36,72" },
	{ "a2",	"1,10,16,40,72" },
	{ "c",	"1,8,12,16,20,55" },
	{ "c2",	"1,6,10,14,49" },
	{ "c3",	"1,6,10,14,18,22,26,30,34,38,42,46,50,54,58,62,67" },
	{ "f",	"1,7,11,15,19,23" },
	{ "p",	"1,5,9,13,17,21,25,29,33,37,41,45,49,53,57,61" },
	{ "s",	"1,10,55" },
	{ "u",	"1,12,20,44" },
	{ 0,	0 }
};

void *
scalloc(size_t nmemb, size_t size)
{
	void	*p;

	if ((p = calloc(nmemb, size)) == NULL) {
		write(2, "no memory\n", 10);
		_exit(077);
	}
	return p;
}

static struct tabulator *
repetitive(int repetition, int cols)
{
	struct tabulator	*tp, *tabspec;
	int	col;

	tp = tabspec = scalloc(1, sizeof *tp);
	tp->t_rep = repetition;
	if (repetition > 0) {
		 for (col = 1 + repetition; col <= cols; col += repetition) {
			tp->t_nxt = scalloc(1, sizeof *tp);
			tp = tp->t_nxt;
			tp->t_tab = col;
		}
	}
	return tabspec;
}

static struct tabulator *
tablist(const char *s)
{
	struct tabulator	*tp, *tabspec;
	char	*x;
	int	prev = 0, val;

	tp = tabspec = scalloc(1, sizeof *tp);
	for (;;) {
		while (*s == ',' || isblank(*s))
			s++;
		if (*s == '\0')
			break;
		val = strtol(s, &x, 10);
		if (*s == '+')
			val += prev;
		prev = val;
		if (*s == '-' || (*x != ',' && !isblank(*x) && *x != '\0')) {
			taberrno = *s == '+' ? TABERR_ILLINC : TABERR_ILLTAB;
			return NULL;
		}
		s = x;
		tp->t_nxt = scalloc(1, sizeof *tp);
		tp = tp->t_nxt;
		tp->t_tab = val;
	}
	return tabspec;
}

static struct tabulator	*fspec(const char *fn, int cols);

static struct tabulator *
tabstring(const char *s, int cols, int filind)
{
	int	i;

	if (s[0] == '-') {
		if (s[1] == '-') {
			if (filind) {
				taberrno = TABERR_FILIND;
				return NULL;
			}
			return fspec(&s[2], cols);
		}
		if (isdigit(s[1]) && ((i = atoi(&s[1])) != 0))
			return repetitive(i, cols);
		for (i = 0; canned[i].c_nam; i++)
			if (strcmp(&s[1], canned[i].c_nam) == 0)
				return tablist(canned[i].c_str);
		taberrno = TABERR_UNKTAB;
		return NULL;
	} else
		return tablist(s);
}

static struct tabulator *
specline(char *line, int cols)
{
	struct tabulator	*tp;
	char	*lp, *sp;

	for (sp = line; *sp; sp++)
		if (sp[0] == '<' && sp[1] == ':')
			break;
	if (*sp) {
		while (*sp && *sp != 't')
			sp++;
		if (sp[0] && sp[1]) {
			for (lp = &sp[1]; *lp; lp++)
				if (lp[0] == ' ' || lp[0] == '\t' ||
						lp[0] == ':' && lp[1] == '>') {
					*lp = '\0';
					tp = tabstring(&sp[1], cols, 1);
					if (tp)
						tp->t_str = strdup(&sp[1]);
					return tp;
				}
		}
	}
	return repetitive(8, cols);
}

static struct tabulator	*
fspec(const char *fn, int cols)
{
	struct iblok	*ip;
	struct tabulator	*tabspec;
	char	*line = 0;
	size_t	linesize = 0;

	if ((ip = *fn ? ib_open(fn, 0) : ib_alloc(0, 0)) == NULL) {
		taberrno = TABERR_CANTOP;
		return NULL;
	}
	if (ib_getlin(ip, &line, &linesize, realloc) != 0)
		tabspec = specline(line, cols);
	else
		tabspec = repetitive(8, cols);
	free(line);
	if (ip->ib_fd)
		ib_close(ip);
	else
		ib_free(ip);
	return tabspec;
}

struct tabulator *
tabstops(const char *s, int cols)
{
	struct tabulator	*tp;
	tp = tabstring(s, cols, 0);
	if (tp && tp->t_str == 0 && *s)
		tp->t_str = strdup(s);
	return tp;
}
