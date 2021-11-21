/*
 * printf - print a text string
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, June 2005.
 */
/*
 * Copyright (c) 2005 Gunnar Ritter
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
static const char sccsid[] USED = "@(#)printf.c	1.7 (gritter) 7/17/05";

#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <locale.h>
#include <wchar.h>
#include <limits.h>
#include <errno.h>
#include "asciitype.h"

#if defined (__GLIBC__) && defined (_IO_getc_unlocked)
#undef	putchar
#define	putchar(c)	_IO_putc_unlocked(c, stdout)
#endif

static char		*fp;		/* format pointer */
static int		a;		/* current argument index */
static int		ab;		/* beginning of arguments */
static int		ac;		/* argc to main() */
static char		**av;		/* argv to main() */
static int		c;		/* last character (byte) read */
static const char	*progname;	/* argv[0] to main() */
static int		status;		/* exit status */
static int		mb_cur_max;	/* MB_CUR_MAX */
static int		dolflag;	/* n$ field encountered */

static void
usage(void)
{
	fprintf(stderr, "Usage: %s format [[[arg1] arg2] ... argn]\n",
			progname);
	exit(2);
}

#define	getnum(T, type, func)	static type \
T(const char *cp) \
{ \
	char	*xp; \
	wchar_t	wc; \
	int	i; \
	type	n; \
\
	if (*cp == '"' || *cp == '\'') { \
		if (mb_cur_max > 1 && cp[1] & 0200) { \
			if ((i = mbtowc(&wc, &cp[1], mb_cur_max)) < 0) \
				return WEOF; \
			return wc; \
		} else \
			return cp[1] & 0377; \
	} \
	errno = 0; \
	n = func(cp, &xp); \
	if (errno) { \
		fprintf(stderr, "%s: \"%s\" arithmetic overflow\n", \
			progname, cp); \
		status |= 1; \
		xp = ""; \
	} \
	if (*xp) { \
		fprintf(stderr, "%s: \"%s\" %s\n", progname, cp, \
			xp > cp ? "not completely converted" : \
				"expected numeric value"); \
		status |= 1; \
	} \
	return n; \
}

#define	getint(a, b)	strtol(a, b, 0)
#define	getuns(a, b)	strtoul(a, b, 0)
#define	getdouble(a, b)	strtod(a, b)

getnum(integer, int, getint)
getnum(unsgned, unsigned, getuns)
getnum(floating, double, getdouble)

static int
backslash(int bflag, int really)
{
	int	c, i, n, z = 1;

	fp++;
	if (mb_cur_max > 1 && *fp & 0200) {
		if ((n = mblen(fp, mb_cur_max)) < 0)
			n = 1;
	} else
		n = 1;
	switch (*fp) {
	case '\0':
		n = 0;
		/*FALLTHRU*/
	case '\\':
		if (really) putchar('\\');
		break;
	case 'a':
		if (really) putchar('\a');
		break;
	case 'b':
		if (really) putchar('\b');
		break;
	case 'c':
		if (bflag) {
			if (really)
				exit(status);
			else {
				while (*fp)
					fp++;
				return 0;
			}
		}
		goto dfl;
	case 'f':
		if (really) putchar('\f');
		break;
	case 'n':
		if (really) putchar('\n');
		break;
	case 'r':
		if (really) putchar('\r');
		break;
	case 't':
		if (really) putchar('\t');
		break;
	case 'v':
		if (really) putchar('\v');
		break;
	case '0':
		if (bflag) {
			if (fp[1]) {
				fp++;
				goto digit;
			}
			if (really) putchar('\0');
			break;
		}
		/*FALLTHRU*/
	case '1': case '2': case '3':
	case '4': case '5': case '6': case '7':
		if (bflag)
			goto dfl;
	digit:	c = 0;
		for (i = 0; i < 3 && octalchar(fp[i] & 0377); i++)
			c = c << 3 | (fp[i] - '0');
		if (really) putchar(c);
		n = i;
		break;
	default:
	dfl:	for (i = 0; i < n; i++)
			if (really) putchar(fp[i] & 0377);
		z = n;
	}
	fp += n - 1;
	return z;
}

static void
bconv(int width, int prec, char *sp)
{
	char	*ofp = fp;
	int	i, n, really = 1;

	fp = sp;
	if (width > 0) {
		really = 0;
		goto try;
	prt:	really = 1;
		fp = sp;
		for (i = 0; i < width - n && i + n < prec; i++)
			putchar(' ');
	}
try:	for (n = 0; *fp && n < prec; fp++) {
		switch (*fp) {
		case '\\':
			n += backslash(1, really);
			break;
		default:
			if (really) putchar(*fp & 0377);
			n++;
		}
	}
	if (width > 0 && really == 0)
		goto prt;
	fp = ofp;
	if (width < 0) {
		while (n < prec && n++ < -width)
			putchar(' ');
	}
}

#define	nextarg()	(a < ac ? av[a++] : "")
static void
percent(void)
{
	char	*fmt = fp, *sp;
	int	width = 0, prec = LONG_MAX;
	int	n;
	double	f;
	int	c;
	int	star = 0;
	int	sign = 1;

	if (*++fp == '\0') {
		fp--;
		return;
	}
	if (digitchar(*fp&0377)) {
		n = 0;
		for (sp = fp; digitchar(*sp&0377); sp++)
			n = n * 10 + *sp - '0';
		if (*sp == '$') {
			a = n + ab - 1;
			dolflag = 1;
			fmt = sp;
			fmt[0] = '%';
			fp = &sp[1];
		}
	}
loop:	switch (*fp) {
	case '-':
		sign = -1;
		/*FALLTHRU*/
	case '+':
	case '#':
	case '0':
	case ' ':
		fp++;
		goto loop;
	}
	if (digitchar(*fp&0377)) {
		do
			width = width * 10 + *fp++ - '0';
		while (digitchar(*fp&0377));
	} else if (*fp == '*') {
		width = a < ac ? integer(av[a++]) : 0;
		fp++;
		star |= 1;
	}
	width *= sign;
	if (*fp == '.') {
		fp++;
		if (digitchar(*fp&0377)) {
			prec = 0;
			do
				prec = prec * 10 + *fp++ - '0';
			while (digitchar(*fp&0377));
		} else if (*fp == '*') {
			prec = a < ac ? integer(av[a++]) : 0;
			fp++;
			star |= 2;
		}
	}
	switch (*fp) {
	case 'b':
		bconv(width, prec, nextarg());
		return;
	case '%':
		putchar('%');
		return;
	case 'd':
	case 'i':
	case 'o':
	case 'u':
	case 'x':
	case 'X':
	case 'c':
		n = *fp == 'c' ? *(nextarg()) & 0377 :
			*fp == 'd' || *fp == 'i' ?
				integer(nextarg()) :
				unsgned(nextarg());
		c = fp[1];
		fp[1] = '\0';
		switch (star) {
		case 3:
			printf(fmt, width, prec, n);
			break;
		case 2:
			printf(fmt, prec, n);
			break;
		case 1:
			printf(fmt, width, n);
			break;
		default:
			printf(fmt, n);
		}
		fp[1] = c;
		break;
	case 'f':
	case 'e':
	case 'E':
	case 'g':
	case 'G':
		f = floating(nextarg());
		c = fp[1];
		fp[1] = '\0';
		switch (star) {
		case 3:
			printf(fmt, width, prec, f);
			break;
		case 2:
			printf(fmt, prec, f);
			break;
		case 1:
			printf(fmt, width, f);
			break;
		default:
			printf(fmt, f);
		}
		fp[1] = c;
		break;
	case 's':
		c = fp[1];
		fp[1] = '\0';
		sp = nextarg();
		switch (star) {
		case 3:
			printf(fmt, width, prec, sp);
			break;
		case 2:
			printf(fmt, prec, sp);
			break;
		case 1:
			printf(fmt, width, sp);
			break;
		default:
			printf(fmt, sp);
		}
		fp[1] = c;
		break;
	default:
		putchar(*fp & 0377);
		return;
	}
}

int
main(int argc, char **argv)
{
	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	progname = basename(argv[0]);
	if (argc > 1 && argv[1][0] == '-' && argv[1][1] == '-' &&
			argv[1][2] == '\0') {
		argv++;
		argc--;
	}
	if (argc <= 1)
		usage();
	ac = argc;
	av = argv;
	a = ab = 2;
	do {
		for (fp = av[1]; *fp; fp++) {
			switch (c = *fp & 0377) {
			case '\\':
				backslash(0, 1);
				break;
			case '%':
				percent();
				break;
			default:
				putchar(c);
			}
		}
	} while (a > ab && a < ac && dolflag == 0);
	if (ferror(stdout))
		status |= 1;
	return status;
}
