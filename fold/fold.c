/*
 * fold - fold long lines
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
static const char sccsid[] USED = "@(#)fold.sl	1.8 (gritter) 5/29/05";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <locale.h>
#include <ctype.h>
#include <wchar.h>
#include <wctype.h>
#include <unistd.h>

#if defined (__GLIBC__) && defined (_IO_putc_unlocked)
#undef	putchar
#define	putchar(c)	_IO_putc_unlocked(c, stdout)
#endif

#include <iblok.h>
#include <blank.h>
#include <mbtowi.h>

#define	next(wc, s, n) (mb_cur_max > 1 && *(s) & 0200 ? \
			((n) = mbtowi(&(wc), (s), mb_cur_max), \
			 (n) = ((n) > 0 ? (n) : (n) < 0 ? (wc=WEOF, 1) : 1)) :\
		((wc) = *(s) & 0377, (n) = 1))

static const char	*progname;
static int		bflag;
static int		sflag;
static long		width = 80;
static int		mb_cur_max;

static void	setwidth(const char *);
static void	usage(void);
static int	fold(const char *);
static int	cfold(struct iblok *);
static int	sfold(struct iblok *);
static void	addchr(char *, int, char **, size_t *, size_t *);
static void	cwidth(wint_t, int, long *);

int
main(int argc, char **argv)
{
	int	status = 0;
	int	i;

	progname = basename(argv[0]);
	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	for (i = 1; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
		if (argv[i][1] == '-' && argv[i][2] == '\0') {
			i++;
			break;
		}
	nopt:	switch (argv[i][1]) {
		case '\0':
			continue;
		case 'b':
			bflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 'w':
			if (argv[i][2]) {
				setwidth(&argv[i][2]);
				continue;
			} else if (i < argc) {
				setwidth(argv[++i]);
				continue;
			} else
				width = 0;
			break;
		case '0':
		case '1': case '2': case '3':
		case '4': case '5': case '6':
		case '7': case '8': case '9':
			setwidth(&argv[i][1]);
			continue;
		default:
			usage();
		}
		argv[i]++;
		goto nopt;
	}
	if (i < argc) {
		while (i < argc)
			status |= fold(argv[i++]);
	} else
		status |= fold(NULL);
	return status;
}

static void
setwidth(const char *s)
{
	char	*x;

	width = strtol(s, &x, 10);
	if (*x != '\0' || *s == '+' || *s == '-') {
		fprintf(stderr, "Bad number for %s\n", progname);
		exit(2);
	}
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-w number] file1...\n", progname);
	exit(2);
}

static int
fold(const char *fn)
{
	struct iblok	*ip;
	int	status;

	if ((ip = fn ? ib_open(fn, 0) : ib_alloc(0, 0)) == NULL) {
		perror(fn ? fn : "stdin");
		return 1;
	}
	status = (sflag ? sfold : cfold)(ip);
	if (ip->ib_fd != 0)
		ib_close(ip);
	else
		ib_free(ip);
	return status;
}

static int
cfold(struct iblok *ip)
{
	wint_t	c, nc;
	char	*cp = NULL, *np = NULL, mb[MB_LEN_MAX];
	int	i, m = 1, n = 1;
	long	col;

	col = 0;
	if (mb_cur_max == 1 ? (c = ib_get(ip)) == (wint_t)EOF :
			(cp = ib_getw(ip, &c, &m)) == NULL)
		return 0;
	if (cp)
		memcpy(mb, cp, m);
	do {
		if (mb_cur_max == 1)
			nc = ib_get(ip);
		else
			np = ib_getw(ip, &nc, &n);
		cwidth(c, m, &col);
		if (col > width && nc != '\r' && nc != '\b') {
			putchar('\n');
			col = 0;
			cwidth(c, m, &col);
		}
		if (mb_cur_max == 1)
			putchar(c);
		else if (c != WEOF)
			for (i = 0; i < m; i++)
				putchar(mb[i] & 0377);
		if (np != NULL)
			memcpy(mb, np, n);
	} while (mb_cur_max == 1 ? (c = nc) != (wint_t)EOF :
			(m = n, c = nc, np != NULL));
	return 0;
}

static int
sfold(struct iblok *ip)
{
	char	*line = NULL, *lp;
	size_t	linesize = 0, linelen = 0;
	wint_t	c, nc, xc;
	char	*cp = NULL, *np = NULL, mb[MB_LEN_MAX];
	int	m = 1, n = 1, x;
	long	col, spos = -1;

	col = 0;
	if (mb_cur_max == 1 ? (c = ib_get(ip)) == (wint_t)EOF :
			(cp = ib_getw(ip, &c, &m)) == NULL)
		return 0;
	if (cp)
		memcpy(mb, cp, m);
	else
		mb[0] = c;
	do {
		if (mb_cur_max == 1)
			nc = ib_get(ip);
		else
			np = ib_getw(ip, &nc, &n);
		cwidth(c, m, &col);
		if (c == '\n' || (mb_cur_max == 1 ? nc == (wint_t)EOF :
					np == NULL)) {
			fwrite(line, sizeof *line, linelen, stdout);
			if (c == '\n')
				putchar('\n');
			col = 0;
			linelen = 0;
			spos = -1;
		} else if (col > width && nc != '\r' && nc != '\b') {
		flsh:	if (spos >= 0 && spos < linelen) {
				fwrite(line, sizeof *line, spos, stdout);
				memmove(line, &line[spos], linelen - spos);
				linelen -= spos;
				col = 0;
				spos = -1;
				for (lp = line; lp < &line[linelen]; lp += x) {
					next(xc, lp, x);
					if (mb_cur_max == 1 ? isblank(c) :
							iswblank(c))
						spos = lp - line + x;
					cwidth(xc, m, &col);
				}
				cwidth(c, m, &col);
				if (col > width)
					goto flsh;
			} else {
				fwrite(line, sizeof *line, linelen, stdout);
				col = 0;
				linelen = 0;
				spos = -1;
				cwidth(c, m, &col);
			}
			putchar('\n');
			if (c != WEOF)
				addchr(mb, m, &line, &linesize, &linelen);
		} else if (c != WEOF)
			addchr(mb, m, &line, &linesize, &linelen);
		if (mb_cur_max == 1 ? isblank(c) : iswblank(c))
			spos = linelen;
		if (np)
			memcpy(mb, np, n);
		else
			mb[0] = nc;
	} while (mb_cur_max == 1 ? (c = nc) != (wint_t)EOF :
			(m = n, c = nc, np != NULL));
	free(line);
	return 0;
}

static void
addchr(char *cp, int n, char **lp, size_t *ls, size_t *ll)
{
	char	*xp;

	if (*ll + n > *ls && (*lp = realloc(*lp, *ls += 128)) == NULL) {
		write(2, "no space\n", 9);
		_exit(077);
	}
	xp = &(*lp)[*ll];
	*ll += n;
	while (--n >= 0)
		xp[n] = cp[n];
}

static void
cwidth(wint_t c, int n, long *colp)
{
	if (c == '\n')
		*colp = 0;
	else if (bflag)
		(*colp) += n;
	else {
		switch (c) {
		case '\t':
			*colp |= 07;
			(*colp)++;
			break;
		default:
			if (mb_cur_max > 1)
				*colp += wcwidth(c);
			else if (isprint(c))
				(*colp)++;
			break;
		case '\b':
			if (*colp > 1)
				(*colp)--;
			break;
		case '\r':
			*colp = 0;
			break;
		case WEOF:
			break;
		}
	}
}
