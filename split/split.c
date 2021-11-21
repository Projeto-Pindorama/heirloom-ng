/*
 * split - split a file into pieces
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
static const char sccsid[] USED = "@(#)split.sl	1.7 (gritter) 5/29/05";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include "atoll.h"

#if defined (__GLIBC__) 
#if defined (_IO_getc_unlocked)
#undef	getc
#define	getc(f)		_IO_getc_unlocked(f)
#endif
#if defined (_IO_putc_unlocked)
#undef	putc
#define	putc(c, f)	_IO_putc_unlocked(c, f)
#endif
#endif

static const char	*progname;
static long long	bytecount;
static long long	linecount = 1000;
static const char	*prefix = "x";
static int		suffixlength = 2;
static int		status;

static void	setbcount(const char *);
static void	usage(void);
static void	split(const char *);
static FILE	*nextfile(void);
static void	*smalloc(size_t);

int
main(int argc, char **argv)
{
	int	i;

	progname = basename(argv[0]);
	for (i = 1; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
		if (argv[i][1] == '-' && argv[i][2] == '\0') {
			i++;
			break;
		}
	nopt:	switch (argv[i][1]) {
		case '\0':
			continue;
		case 'a':
			if (argv[i][2]) {
				suffixlength = atoi(&argv[i][2]);
				continue;
			} else if (i+1 < argc) {
				suffixlength = atoi(argv[++i]);
				continue;
			} else
				suffixlength = 0;
			break;
		case 'b':
			linecount = 0;
			if (argv[i][2]) {
				setbcount(&argv[i][2]);
				continue;
			} else if (i+1 < argc) {
				setbcount(argv[++i]);
				continue;
			} else
				bytecount = 0;
			break;
		case 'l':
			bytecount = 0;
			if (argv[i][2]) {
				linecount = atoll(&argv[i][2]);
				continue;
			} else if (i+1 < argc) {
				linecount = atoll(argv[++i]);
				continue;
			} else
				linecount = 0;
			break;
		case '0':
		case '1': case '2': case '3':
		case '4': case '5': case '6':
		case '7': case '8': case '9':
			linecount = atoll(&argv[i][1]);
			continue;
		default:
			break;
		}
		argv[i]++;
		goto nopt;
	}
	if (suffixlength <= 0 || bytecount <= 0 && linecount <= 0)
		usage();
	if (i < argc) {
		if (i+1 < argc)
			prefix = argv[i+1];
		split(argv[i]);
	} else
		split("-");
	return status;
}

static void
setbcount(const char *s)
{
	char	*x;

	bytecount = strtoll(s, &x, 10);
	if (*x == 'k')
		bytecount <<= 10;
	else if (*x == 'm')
		bytecount <<= 20;
}

static void
usage(void)
{
	fprintf(stderr,
		"Usage: %s [ -b#[k|m] | -l# | -# ] [ -a# ] [ file [ name ] ]\n",
			progname);
	exit(2);
}

static void
split(const char *fn)
{
	FILE	*fp, *op = NULL;
	long long	bytes = bytecount, lines = linecount, n, m;
	char	b[4096];
	int	c;

	if (fn[0] == '-' && fn[1] == '\0')
		fp = stdin;
	else if ((fp = fopen(fn, "r")) == NULL) {
		fprintf(stderr, "cannot open input");
		exit(1);
	}
	if (linecount > 0) {
		while ((c = getc(fp)) != EOF) {
			if (lines >= linecount) {
				lines = 0;
				if (op)
					fclose(op);
				op = nextfile();
			}
			putc(c, op);
			if (c == '\n')
				lines++;
		}
	} else {
		do {
			if ((n = bytecount - bytes) > sizeof b)
				n = sizeof b;
			else if (n == 0 && (n = bytecount) > sizeof b)
				n = sizeof b;
			m = fread(b, 1, n, fp);
			if (bytes >= bytecount) {
				bytes = 0;
				if (op)
					fclose(op);
				op = nextfile();
			}
			if (m) {
				fwrite(b, 1, m, op);
				bytes += m;
			}
		} while (m);
	}
}

static FILE *
nextfile(void)
{
	static long long	fileno;
	static char		*name, *sufp;
	long long	c;
	const char	*cp;
	char	*sp;
	FILE	*fp;

	if (name == 0) {
		name = smalloc(strlen(prefix)+suffixlength+1);
		sufp = name;
		for (cp = prefix; *cp; cp++)
			*sufp++ = *cp;
	}
	c = fileno++;
	sp = &sufp[suffixlength];
	*sp = '\0';
	while (--sp >= sufp) {
		*sp = (c % 26) + 'a';
		c /= 26;
	}
	if (c) {
		char	*msg, *mp;
		int	i;

		mp = msg = smalloc(suffixlength*2 + strlen(progname) + 60);
		for (cp = "more than "; *cp; cp++)
			*mp++ = *cp;
		for (i = 0; i < suffixlength; i++)
			*mp++ = 'a';
		*mp++ = '-';
		for (i = 0; i < suffixlength; i++)
			*mp++ = 'z';
		for (cp = " output files needed, aborting "; *cp; cp++)
			*mp++ = *cp;
		for (cp = progname; *cp; cp++)
			*mp++ = *cp;
		*mp++ = '\n';
		write(2, msg, mp - msg);
		exit(1);
	}
	if ((fp = fopen(name, "w")) == NULL) {
		fprintf(stderr, "Cannot create output\n");
		exit(1);
	}
	return fp;
}

static void *
smalloc(size_t size)
{
	void	*vp;

	if ((vp = malloc(size)) == NULL) {
		write(2, "no memory\n", 10);
		_exit(077);
	}
	return vp;
}
