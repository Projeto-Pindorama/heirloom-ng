/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, August 2003.
 */
/*	from Unix 7th Edition /usr/src/cmd/factor.s	*/
/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   Redistributions of source code and documentation must retain the
 *    above copyright notice, this list of conditions and the following
 *    disclaimer.
 *   Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *   All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed or owned by Caldera
 *      International, Inc.
 *   Neither the name of Caldera International, Inc. nor the names of
 *    other contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE
 * LIABLE FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)factor.sl	1.11 (gritter) 5/29/05";

#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <libgen.h>
#include <inttypes.h>
#include "asciitype.h"

#include "config.h"

#ifdef	USE_LONG_DOUBLE
typedef	long double	f_type;
#define	F_MANT_DIG	LDBL_MANT_DIG
#define	f_strtod(a, b)	strtold(a, b)
#define	FFMT		"%.0Lf"
#define	f_pow(a, b)	powl(a, b)
#define	f_fmod(a, b)	fmodl(a, b)
#define	f_sqrt(a)	sqrtl(a)
#else	/* !USE_LONG_DOUBLE */
typedef	double		f_type;
#define	F_MANT_DIG	DBL_MANT_DIG
#define	f_strtod(a, b)	strtod(a, b)
#define	FFMT		"%.0f"
#define	f_pow(a, b)	pow(a, b)
#define	f_fmod(a, b)	fmod(a, b)
#define	f_sqrt(a)	sqrt(a)
#endif	/* !USE_LONG_DOUBLE */

#if defined (__GLIBC__) && defined (_IO_getc_unlocked)
#undef	getc
#define	getc(f)		_IO_getc_unlocked(f)
#endif

#ifdef	__R5900
/*
 * For whatever reason, these definitions are missing on Playstation Linux.
 */
extern long double	powl(long double, long double);
extern long double	fmodl(long double, long double);
extern long double	sqrtl(long double);
#endif	/* __R5900 */

static const char	*progname;

static char	*token(char **, int *, FILE *);
static void	factor(const char *, int);
static void	xt(f_type *, f_type *, f_type);
static void	print(f_type);

int
main(int argc, char **argv)
{
	char	*str = NULL;
	int	siz = 0;

	progname = basename(argv[0]);
	if (argc > 1 && argv[1][0] == '-' && argv[1][1] == '-' &&
			argv[1][2] == '\0')
		argv++, argc--;
	if (argc == 1)
		while (token(&str, &siz, stdin))
			factor(str, 0);
	else if (argc == 2)
		factor(argv[1], 1);
	else {
		fprintf(stderr, "Usage: %s number\n", progname);
		return 2;
	}
	return 0;
}

static char *
token(char **buf, int *sz, FILE *fp)
{
	char	*bp, *ob;
	int	c;

	bp = *buf;
	while ((c = getc(fp)) != EOF && whitechar(c));
	if (c == EOF)
		return NULL;
	do {
		if (bp - *buf >= *sz-1) {
			ob = *buf;
			if ((*buf = realloc(*buf, *sz += 50)) == NULL) {
				write(2, "no memory\n", 10);
				_exit(077);
			}
			bp += *buf - ob;
		}
		*bp++ = c;
	} while ((c = getc(fp)) != EOF && !whitechar(c));
	*bp = '\0';
	return *buf;
}

static void
factor(const char *s, int prnt)
{
	const f_type	tab[] = {
		10,	2,	4,	2,	4,	6,	2,	6,
		4,	2,	4,	6,	6,	2,	6,	4,
		2,	6,	4,	6,	8,	4,	2,	4,
		2,	4,	8,	6,	4,	6,	2,	4,
		6,	2,	6,	6,	4,	2,	4,	6,
		2,	6,	4,	2,	4,	2,	10,	2
	};
	f_type	n, t, v;
	char	*x;
	int	i;

	n = f_strtod(s, &x);
	if (prnt)
		printf(FFMT "\n", n);
	if (n == 0 || *x != '\0')
		exit(0);
	if (n < 0 || n >= f_pow(2, F_MANT_DIG)) {
		puts("Ouch!");
		return;
	}
	if (f_fmod(n, 1) != 0) {
		puts("Not an integer!");
		return;
	}
	v = f_sqrt(n);
	xt(&n, &v, 2);
	xt(&n, &v, 3);
	xt(&n, &v, 5);
	xt(&n, &v, 7);
	xt(&n, &v, 11);
	xt(&n, &v, 13);
	i = 2;
	t = 17;
	while (t <= v) {
		xt(&n, &v, t);
		if (++i == 48)
			i = 0;
		t += tab[i];
	}
	if (n > 1)
		print(n);
	printf("\n");
}

static void
xt(f_type *n, f_type *v, f_type t)
{
	while (f_fmod(*n, t) == 0) {
		print(t);
		*n /= t;
		*v = f_sqrt(*n);
	}
}

static void
print(f_type n)
{
	printf("     " FFMT "\n", n);
}
