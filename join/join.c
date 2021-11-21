/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, March 2003.
 */
/*	from Unix 32V /usr/src/cmd/join.c	*/
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
static const char sccsid[] USED = "@(#)join.sl	1.15 (gritter) 5/29/05";

/*	join F1 F2 on stuff */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<libgen.h>
#include	<locale.h>
#include	<wchar.h>
#include	<limits.h>
#include	<unistd.h>

#include	"iblok.h"
#include	"mbtowi.h"

enum {
	F1 = 0,
	F2 = 1,
	JF = -1
};
#define	ppi(f, j)	((j) >= 0 && (j) < ppisize[f] ? ppibuf[f][j] : null)
#define comp() strcoll(ppi(F1, j1),ppi(F2, j2))

#define	next(wc, s, n)	(*(s) & 0200 ? ((n) = mbtowi(&(wc), (s), mb_cur_max), \
		(n) = ((n) > 0 ? (n) : (n) < 0 ? (wc=WEOF, 1) : 1)) : \
	((wc) = *(s) & 0377, (n) = 1))

static struct iblok	 *f[2];
static char *buf[2];		/*input lines */
static size_t bufsize[2];
static const char **ppibuf[2];	/* pointers to fields in lines */
static long ppisize[2];
static const char *s1,*s2;
static long	j1	= 1;	/* join of this field of file 1 */
static long	j2	= 1;	/* join of this field of file 2 */
static long	*olist;		/* output these fields */
static int	*olistf;	/* from these files */
static long	no;		/* number of entries in olist */
static wint_t	sep1	= ' ';	/* default field separator */
static wint_t	sep2	= '\t';
static const char*	null	= "";
static int	aflg;
static int	vflg;
static char	*progname;
static int	mb_cur_max;

static int input(int);
static void output(int, int);
static void error(const char *, const char *);
static void setppi(int, long, const char *);
static void *srealloc(void *, size_t);

static void
usage(void)
{
	fprintf(stderr,
	"%s: usage: %s [-an] [-e s] [-jn m] [-tc] [-o list] file1 file2\n",
		progname, progname);
	exit(2);
}

int
main(int argc, char **argv)
{
	int i;
	int n1, n2;
	off_t top2 = 0, bot2;
	char	*arg = 0, *x;

	progname = basename(argv[0]);
	setlocale(LC_COLLATE, "");
	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	while (argc > 1 && argv[1][0] == '-') {
		if (argv[1][1] == '\0')
			break;
		switch (argv[1][1]) {
		case 'a':
			if (argv[1][2])
				arg = &argv[1][2];
			else if (argv[2]) {
				arg = argv[2];
				argv++, argc--;
			} else
				arg = "3";
			switch(*arg) {
			case '1':
				aflg |= 1;
				break;
			case '2':
				aflg |= 2;
				break;
			default:
				aflg |= 3;
			}
			break;
		case 'e':
			if (argv[1][2])
				null = &argv[1][2];
			else if (argv[2]) {
				null = argv[2];
				argv++;
				argc--;
			} else
				usage();
			break;
		case 't':
			if (argv[1][2]) {
				int n;
				next(sep1, &argv[1][2], n);
				sep2 = sep1;
			} else if (argv[2]) {
				int n;
				next(sep1, argv[2], n);
				sep2 = sep1;
				argv++, argc--;
			} else
				usage();
			break;
		case 'o':
			if (argv[2] == NULL)
				usage();
			arg = argv[1][2] ? &argv[1][2] : argv[2];
			for (no = 0;
				olist = srealloc(olist, (no+1)*sizeof *olist),
				olistf = srealloc(olistf,(no+1)*sizeof *olistf),
				olist[no] = 0, olistf[no] = 0,
					arg; no++) {
				if (arg[0] == '1' && arg[1] == '.') {
					olistf[no] = F1;
					olist[no] = strtol(&arg[2], &x, 10);
				} else if (arg[0] == '2' && arg[1] == '.') {
					olist[no] = strtol(&arg[2], &x, 10);
					olistf[no] = F2;
				} else if (arg[0] == '0') {
					olistf[no] = JF;
					x = &arg[1];
				} else
					break;
				while (*x == ' ' || *x == ',')
					x++;
				if (*x)
					arg = x;
				else {
					argc--;
					argv++;
					arg = argv[2];
				}
			}
			if (no == 0) {
				fprintf(stderr, "%s: invalid file number (%s) "
						"for -o\n", progname, arg);
				exit(2);
			}
			break;
		case 'j':
			if (argv[2] == NULL)
				usage();
			if (argv[1][2] == '1')
				j1 = atoi(argv[2]);
			else if (argv[1][2] == '2')
				j2 = atoi(argv[2]);
			else
				j1 = j2 = atoi(argv[2]);
			argc--;
			argv++;
			break;
		case '1':
			if (argv[1][2])
				arg = &argv[1][2];
			else if (argv[2]) {
				arg = argv[2];
				argv++, argc--;
			} else
				usage();
			j1 = atoi(arg);
			break;
		case '2':
			if (argv[1][2])
				arg = &argv[1][2];
			else if (argv[2]) {
				arg = argv[2];
				argv++, argc--;
			} else
				usage();
			j2 = atoi(arg);
			break;
		case 'v':
			if (argv[1][2])
				arg = &argv[1][2];
			else if (argv[2]) {
				arg = argv[2];
				argv++, argc--;
			} else
				usage();
			if (*arg == '1')
				vflg |= 1;
			else if (*arg == '2')
				vflg |= 2;
			break;
		}
		argc--;
		argv++;
	}
	for (i = 0; i < no; i++)
		olist[i]--;	/* 0 origin */
	if (argc != 3)
		usage();
	j1--;
	j2--;	/* everyone else believes in 0 origin */
	s1 = ppi(F1, j1);
	s2 = ppi(F2, j2);
	if (argv[1][0] == '-' && argv[1][1] == '\0')
		f[F1] = ib_alloc(0, 0);
	else if ((f[F1] = ib_open(argv[1], 0)) == NULL)
		error("can't open ", argv[1]);
	if (argv[2][0] == '-' && argv[2][1] == '\0')
		f[F2] = ib_alloc(0, 0);
	else if ((f[F2] = ib_open(argv[2], 0)) == NULL)
		error("can't open ", argv[2]);

#define get1() n1=input(F1)
#define get2() n2=input(F2)
	get1();
	bot2 = ib_seek(f[F2], 0, SEEK_CUR);
	get2();
	while(n1>=0 && n2>=0 || (aflg|vflg)!=0 && (n1>=0||n2>=0)) {
		if(n1>=0 && n2>=0 && comp()>0 || n1<0) {
			if(aflg&2||vflg&2) output(0, n2);
			bot2 = ib_seek(f[F2], 0, SEEK_CUR);
			get2();
		} else if(n1>=0 && n2>=0 && comp()<0 || n2<0) {
			if(aflg&1||vflg&1) output(n1, 0);
			get1();
		} else /*(n1>=0 && n2>=0 && comp()==0)*/ {
			while(n2>=0 && comp()==0) {
				if(vflg==0) output(n1, n2);
				top2 = ib_seek(f[F2], 0, SEEK_CUR);
				get2();
			}
			ib_seek(f[F2], bot2, SEEK_SET);
			get2();
			get1();
			for(;;) {
				if(n1>=0 && n2>=0 && comp()==0) {
					if(vflg==0) output(n1, n2);
					get2();
				} else if(n1>=0 && n2>=0 && comp()<0 || n2<0) {
					ib_seek(f[F2], bot2, SEEK_SET);
					get2();
					get1();
				} else /*(n1>=0 && n2>=0 && comp()>0 || n1<0)*/{
					ib_seek(f[F2], top2, SEEK_SET);
					bot2 = top2;
					get2();
					break;
				}
			}
		}
	}
	return(0);
}

static int
input(int n)		/* get input line and split into fields */
{
	register int i;
	wint_t	wc;
	int	m;
	char	*bp;
	size_t	length;
	long pc;

	if ((length = ib_getlin(f[n], &buf[n], &bufsize[n], srealloc)) == 0)
		return(-1);
	bp = buf[n];
	pc = 0;
	for (i = 0; ; i++) {
		if (sep1 == ' ')	/* strip multiples */
			while (next(wc, bp, m), wc == sep1 || wc == sep2)
				bp += m;	/* skip blanks */
		else
			next(wc, bp, m);
		if (wc == '\n' || wc == '\0')
			break;
		setppi(n, pc++, bp);	/* record beginning */
		while (next(wc, bp, m), wc != sep1 && wc != '\n' &&
				wc != sep2 && wc != '\0')
			bp += m;
		*bp++ = '\0';	/* mark end by overwriting blank */
			/* fails badly if string doesn't have \n at end */
	}
	if (pc == 0)
		setppi(n, pc++, "");
	setppi(n, pc, 0);
	return(i);
}

static void
output(int on1, int on2)	/* print items from olist */
{
	int i;
	const char *temp;

	if (no <= 0) {	/* default case */
		printf("%s", on1? ppi(F1, j1): on2? ppi(F2, j2) : null);
		for (i = 0; i < on1; i++)
			if (i != j1) {
				if (mb_cur_max > 1)
					printf("%lc%s",(wint_t)sep1,ppi(F1, i));
				else
					printf("%c%s", (int)sep1, ppi(F1, i));
			}
		for (i = 0; i < on2; i++)
			if (i != j2) {
				if (mb_cur_max > 1)
					printf("%lc%s",(wint_t)sep1,ppi(F2, i));
				else
					printf("%c%s", (int)sep1, ppi(F2, i));
			}
		printf("\n");
	} else {
		for (i = 0; i < no; i++) {
			temp = ppi(olistf[i], olist[i]);
			if(olistf[i]==F1 && on1<=olist[i] ||
			   olistf[i]==F2 && on2<=olist[i])
				temp = null;
			else if (olistf[i]==JF) {
				if (on1)
					temp = ppi(F1, j1);
				else if (on2)
					temp = ppi(F2, j2);
				else
					temp = null;
			}
			if (temp == 0 || *temp == 0)
				temp = null;
			printf("%s", temp ? temp : null);
			if (i == no - 1)
				printf("\n");
			else if (mb_cur_max > 1)
				printf("%lc", (wint_t)sep1);
			else
				printf("%c", (int)sep1);
		}
	}
}

static void
error(const char *s1, const char *s2)
{
	fprintf(stderr, "%s: %s%s\n", progname, s1, s2);
	exit(1);
}

static void
setppi(int f, long j, const char *p)
{
	if (j >= ppisize[f]) {
		ppisize[f] = j + 1;
		ppibuf[f] = srealloc(ppibuf[f], ppisize[f] * sizeof *ppibuf[f]);
	}
	ppibuf[f][j] = p;
}

static void *
srealloc(void *vp, size_t nbytes)
{
	if ((vp = realloc(vp, nbytes)) == NULL) {
		write(2, "no memory\n", 10);
		_exit(077);
	}
	return vp;
}
