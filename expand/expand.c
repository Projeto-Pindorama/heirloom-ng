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

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)expand.sl	1.6 (gritter) 5/29/05";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <locale.h>
#include <wchar.h>
#include <unistd.h>

#if defined (__GLIBC__)
#if defined (_IO_putc_unlocked)
#undef	putchar
#define	putchar(c)	_IO_putc_unlocked(c, stdout)
#endif
#endif

#include <iblok.h>
#include "tablist.h"

static int		status;
static int		mb_cur_max;

static void	expand(const char *);

int
main(int argc, char **argv)
{
	int	i;

	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	for (i = 1; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
		if (argv[i][1] == '-' && argv[i][2] == '\0') {
			i++;
			break;
		} else if (argv[i][1] == 't') {
			if (argv[i][2])
				settab(&argv[i][2]);
			else
				settab(argv[++i]);
		} else
			settab(&argv[i][1]);
	}
	if (i < argc)
		while (i < argc)
			expand(argv[i++]);
	else
		expand(NULL);
	return status;
}

static void
expand(const char *fn)
{
	struct iblok	*ip;
	struct tab	*tp;
	char	*cp;
	char	b;
	wint_t	c;
	int	n, w;
	long long	col;

	if ((ip = fn ? ib_open(fn, 0) : ib_alloc(0, 0)) == NULL) {
		perror(fn ? fn : "stdin");
		status |= 1;
		return;
	}
	col = 0;
	tp = tablist;
	while ((mb_cur_max > 1 ? cp = ib_getw(ip, &c, &n) :
				(b = c = ib_get(ip), n = 1,
				 cp = c == (wint_t)EOF ? 0 : &b)),
			cp != NULL) {
		switch (c) {
		case '\n':
			col = 0;
			tp = tablist;
			break;
		case '\b':
			if (col > 0)
				col--;
			break;
		default:
			if (mb_cur_max > 1) {
				if ((w = wcwidth(c)) >= 0)
					col += w;
				else
					col++;
			} else
				col++;
			break;
		case '\t':
			if (tp)
				while (tp && col > tp->t_pos)
					tp = tp->t_nxt;
			if (tp && col == tp->t_pos) {
				putchar(' ');
				col++;
				tp = tp->t_nxt;
			}
			if (tp) {
				while (col < tp->t_pos) {
					putchar(' ');
					col++;
				}
				tp = tp->t_nxt;
			} else {
				if (col % tabstop == 0) {
					putchar(' ');
					col++;
				}
				while (col % tabstop) {
					putchar(' ');
					col++;
				}
			}
			continue;
		}
		while (n--) {
			putchar(*cp & 0377);
			cp++;
		}
	}
	if (ip->ib_fd != 0)
		ib_close(ip);
	else
		ib_free(ip);
}
