/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, March 2003.
 */
/*	from Unix 32V /usr/src/cmd/comm.c	*/
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
static const char sccsid[] USED = "@(#)comm.sl	1.7 (gritter) 5/29/05";

#include <stdio.h>
#include <iblok.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>

#include "mbtowi.h"

static int	one;
static int	two;
static int	three;

static char	*ldr[3];

struct iblok	*ib1;
struct iblok	*ib2;

static int	ccoll;
static int	mb_cur_max;

static char	*progname;

static int		rd(struct iblok *, char **, size_t *);
static void		wr(const char *, int);
static void		copy(struct iblok *, char **, size_t *, int);
static int		compare(const char *, const char *);
static struct iblok	*openfil(const char *);
static void		*srealloc(void *, size_t);

#define	next(wc, s, n)	(*(s) & 0200 ? ((n) = mbtowi(&(wc), (s), mb_cur_max), \
		(n) = ((n) > 0 ? (n) : (n) < 0 ? (wc=WEOF, 1) : 1)) : \
	((wc) = *(s) & 0377, (n) = 1))

static void
usage(void)
{
	fprintf(stderr, "usage: %s [ - [123] ] file1 file2\n", progname);
	exit(2);
}

int
main(int argc, char **argv)
{
	int	i, l;
	char	*cols;
	char	*lb1 = NULL, *lb2 = NULL;
	size_t	sb1 = 0, sb2 = 0;

	ldr[0] = strdup("");
	ldr[1] = strdup("\t");
	ldr[2] = strdup("\t\t");
	l = 1;
	progname = basename(argv[0]);
	cols = setlocale(LC_COLLATE, "");
	ccoll = cols == 0 || strcmp(cols, "C") == 0 ||
		strcmp(cols, "POSIX") == 0;
	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	while ((i = getopt(argc, argv, ":123")) != EOF) {
		switch (i) {
		case'1':
			if(!one) {
				one = 1;
				ldr[1][0] = ldr[2][l--] = '\0';
			}
			break;
		case '2':
			if(!two) {
				two = 1;
				ldr[2][l--] = '\0';
			}
			break;
		case '3':
			three = 1;
			break;
		default:
			usage();
		}
	}

	argv += optind, argc -= optind;
	if(argc < 2)
		usage();

	ib1 = openfil(argv[0]);
	ib2 = openfil(argv[1]);


	if(rd(ib1,&lb1,&sb1) < 0) {
		if(rd(ib2,&lb2,&sb2) < 0)	exit(0);
		copy(ib2,&lb2,&sb2,2);
	}
	if(rd(ib2,&lb2,&sb2) < 0)
		copy(ib1,&lb1,&sb1,1);

	while(1) {

		switch(compare(lb1,lb2)) {

			case 0:
				wr(lb1,3);
				if(rd(ib1,&lb1,&sb1) < 0) {
					if(rd(ib2,&lb2,&sb2) < 0)	exit(0);
					copy(ib2,&lb2,&sb2,2);
				}
				if(rd(ib2,&lb2,&sb2) < 0)
					copy(ib1,&lb1,&sb1,1);
				continue;

			case 1:
				wr(lb1,1);
				if(rd(ib1,&lb1,&sb1) < 0)
					copy(ib2,&lb2,&sb2,2);
				continue;

			case 2:
				wr(lb2,2);
				if(rd(ib2,&lb2,&sb2) < 0)
					copy(ib1,&lb1,&sb1,1);
				continue;
		}
	}
}

static int
rd(struct iblok *ip, char **lp, size_t *sz)
{
	size_t	len;

	if ((len = ib_getlin(ip, lp, sz, srealloc)) == 0)
		return -1;
	if ((*lp)[len-1] == '\n')
		(*lp)[len-1] = '\0';
	return 0;
}

static void
wr(const char *str, int n)
{

	switch(n) {

		case 1:
			if(one)	return;
			break;

		case 2:
			if(two)	return;
			break;

		case 3:
			if(three)	return;
	}
	printf("%s%s\n",ldr[n-1],str);
}

static void
copy(struct iblok *ip, char **lp, size_t *sz, int n)
{
	do {
		wr(*lp, n);
	} while (rd(ip, lp, sz) >= 0);

	exit(0);
}

static int
compare(const char *a, const char *b)
{
	register const char *ra,*rb;
	int	n;

	if (ccoll) {
		if (mb_cur_max > 1) {
			wint_t	wa, wb;
			int	ma, mb;

			while (next(wa, a, ma), next(wb, b, mb), wa == wb) {
				if (wa == '\0')
					return 0;
				a += ma;
				b += mb;
			}
			if (wa < wb)
				return 1;
			return 2;
		} else {
			ra = --a;
			rb = --b;
			while(*++ra == *++rb)
				if(*ra == '\0')	return(0);
			if(*ra < *rb)	return(1);
			return(2);
		}
	} else {
		n = strcoll(a, b);
		return n ? n > 0 ? 2 : 1 : 0;
	}
}

static struct iblok *
openfil(const char *s)
{
	struct iblok	*ip;

	if (s[0] == '-' && s[1] == '\0')
		ip = ib_alloc(0, 0);
	else if ((ip = ib_open(s, 0)) == NULL) {
		fprintf(stderr,"%s: cannot open %s\n", progname, s);
		exit(1);
	}
	return ip;
}

static void *
srealloc(void *op, size_t sz)
{
	void	*np;

	if ((np = realloc(op, sz)) == NULL) {
		write(2, "no memory\n", 10);
		_exit(077);
	}
	return np;
}
