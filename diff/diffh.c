/*
 * This code contains changes by
 * Gunnar Ritter, Freiburg i. Br., Germany, March 2003. All rights reserved.
 *
 * Conditions 1, 2, and 4 and the no-warranty notice below apply
 * to these changes.
 *
 *
 * Copyright (c) 1991
 * 	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 	This product includes software developed by the University of
 * 	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
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
static const char sccsid[] USED = "@(#)diffh.sl	1.11 (gritter) 5/29/05";

/*	from 4.3BSD diffh.c 4.4 11/27/85>	*/

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <locale.h>
#include <wchar.h>
#include <wctype.h>

#include <iblok.h>
#include <mbtowi.h>

#define C 3
#define RANGE 30
#define INF 16384

#define	next(wc, s, n)	(*(s) & 0200 ? ((n) = mbtowi(&(wc), (s), mb_cur_max), \
		(n) = ((n) > 0 ? (n) : (n) < 0 ? (wc=WEOF, 1) : 1)) :\
		((wc) = *(s) & 0377, (n) = 1))

static char *text[2][RANGE];
static size_t size[2][RANGE];
static long long lineno[2] = {1, 1};	/*no. of 1st stored line in each file*/
static int ntext[2];		/*number of stored lines in each*/
static long long n0,n1;		/*scan pointer in each*/
static int bflag;
static int mb_cur_max;
static int debug = 0;
static struct iblok *file[2];
static int eof[2];

static char *getl(int, long long);
static void clrl(int, long long);
static void movstr(int, int, int);
static int easysynch(void);
static int output(int, int);
static void change(long long, int, long long, int, const char *);
static void range(long long, int);
static int cmp(const char *, const char *);
static struct iblok *dopen(const char *, const char *);
static void progerr(const char *);
static void error(const char *, const char *);
static int hardsynch(void);
static void *lrealloc(void *, size_t);

	/* return pointer to line n of file f*/
static char *
getl(int f, long long n)
{
	register int delta, nt;
	size_t	len;

	delta = n - lineno[f];
	nt = ntext[f];
	if(delta<0)
		progerr("1");
	if(delta<nt)
		return(text[f][delta]);
	if(delta>nt)
		progerr("2");
	if(nt>=RANGE)
		progerr("3");
	if(eof[f])
		return(NULL);
	len = ib_getlin(file[f], &text[f][nt], &size[f][nt], lrealloc);
	if (len != 0) {
		ntext[f]++;
		return(text[f][nt]);
	} else {
		eof[f]++;
		return NULL;
	}
}

	/*remove thru line n of file f from storage*/
static void
clrl(int f,long long n)
{
	register long long i,j;
	j = n-lineno[f]+1;
	for(i=0;i+j<ntext[f];i++)
		movstr(f, i+j, i);
	lineno[f] = n+1;
	ntext[f] -= j;
}

static void
movstr(register int f, register int i, register int j)
{
	free(text[f][j]);
	text[f][j] = text[f][i];
	size[f][j] = size[f][i];
	text[f][i] = 0;
	size[f][i] = 0;
}

int
main(int argc,char **argv)
{
	char *s0,*s1;
	register int c, status = 0;

	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	while((c=getopt(argc,argv,":b")) != EOF) {
		switch (c) {
		case 'b':
			bflag++;
			break;
		}
	}
	if(argc-optind!=2)
		error("must have 2 file arguments","");
	file[0] = dopen(argv[optind],argv[optind+1]);
	file[1] = dopen(argv[optind+1],argv[optind]);
	for(;;) {
		s0 = getl(0,++n0);
		s1 = getl(1,++n1);
		if(s0==NULL||s1==NULL)
			break;
		if(cmp(s0,s1)!=0) {
			if(!easysynch()&&!hardsynch())
				progerr("5");
			status = 1;
		} else {
			clrl(0,n0);
			clrl(1,n1);
		}
	}
	if(s0==NULL&&s1==NULL)
		exit(status);
	if(s0==NULL)
		output(-1,INF);
	if(s1==NULL)
		output(INF,-1);
	return (1);
}

	/* synch on C successive matches*/
static int
easysynch(void)
{
	int i,j;
	register int k,m;
	char *s0,*s1;
	for(i=j=1;i<RANGE&&j<RANGE;i++,j++) {
		s0 = getl(0,n0+i);
		if(s0==NULL)
			return(output(INF,INF));
		for(k=C-1;k<j;k++) {
			for(m=0;m<C;m++)
				if(cmp(getl(0,n0+i-m),
					getl(1,n1+k-m))!=0)
					goto cont1;
			return(output(i-C,k-C));
cont1:			;
		}
		s1 = getl(1,n1+j);
		if(s1==NULL)
			return(output(INF,INF));
		for(k=C-1;k<=i;k++) {
			for(m=0;m<C;m++)
				if(cmp(getl(0,n0+k-m),
					getl(1,n1+j-m))!=0)
					goto cont2;
			return(output(k-C,j-C));
cont2:			;
		}
	}
	return(0);
}

static int
output(int a,int b)
{
	register int i;
	char *s;
	if(a<0)
		change(n0-1,0,n1,b,"a");
	else if(b<0)
		change(n0,a,n1-1,0,"d");
	else
		change(n0,a,n1,b,"c");
	for(i=0;i<=a;i++) {
		s = getl(0,n0+i);
		if(s==NULL)
			break;
		printf("< %s",s);
		clrl(0,n0+i);
	}
	n0 += i-1;
	if(a>=0&&b>=0)
		printf("---\n");
	for(i=0;i<=b;i++) {
		s = getl(1,n1+i);
		if(s==NULL)
			break;
		printf("> %s",s);
		clrl(1,n1+i);
	}
	n1 += i-1;
	return(1);
}

static void
change(long long a,int b,long long c,int d,const char *s)
{
	range(a,b);
	printf("%s",s);
	range(c,d);
	printf("\n");
}

static void
range(long long a,int b)
{
	if(b==INF)
		printf("%lld,$",a);
	else if(b==0)
		printf("%lld",a);
	else
		printf("%lld,%lld",a,a+b);
}

static int
cmp(const char *s,const char *t)
{
	if(debug)
		printf("%s:%s\n",s,t);
	for(;;){
		if(bflag) {
			if (mb_cur_max > 1) {
				wint_t	wc, wd;
				int	n, m;

				if (next(wc, s, n), next(wd, t, m),
						iswspace(wc) && iswspace(wd)) {
					while (s += n, next(wc, s, n),
							iswspace(wc));
					while (t += m, next(wd, t, m),
							iswspace(wd));
				}
			} else {
				if (isspace(*s)&&isspace(*t)) {
					while(isspace(*++s)) ;
					while(isspace(*++t)) ;
				}
			}
		}
		if(*s!=*t||*s==0)
			break;
		s++;
		t++;
	}
	return((*s&0377)-(*t&0377));
}

static struct iblok *
dopen(const char *f1,const char *f2)
{
	struct iblok *ip;
	char *b=0,*bptr;
	const char *eptr;
	struct stat statbuf;
	if(cmp(f1,"-")==0)
		if(cmp(f2,"-")==0)
			error("can't do - -","");
		else
			return(ib_alloc(0, 0));
	if(stat(f1,&statbuf)==-1)
		error("can't access ",f1);
	if((statbuf.st_mode&S_IFMT)==S_IFDIR) {
		b = lrealloc(0, strlen(f1) + strlen(f2) + 2);
		for(bptr=b;*bptr= *f1++;bptr++) ;
		*bptr++ = '/';
		for(eptr=f2;*eptr;eptr++)
			if(*eptr=='/'&&eptr[1]!=0&&eptr[1]!='/')
				f2 = eptr+1;
		while(*bptr++= *f2++) ;
		f1 = b;
	}
	ip = ib_open(f1,0);
	if(ip==NULL)
		error("can't open",f1);
	if (b)
		free(b);
	return(ip);
}

static void
progerr(const char *s)
{
	error("program error ",s);
}

static void
error(const char *s,const char *t)
{
	fprintf(stderr,"diffh: %s%s\n",s,t);
	exit(2);
}

	/*stub for resychronization beyond limits of text buf*/
static int
hardsynch(void)
{
	change(n0,INF,n1,INF,"c");
	printf("---change record omitted\n");
	error("can't resynchronize","");
	return(0);
}

static void *
lrealloc(void *op, size_t size)
{
	void	*np;

	if ((np = realloc(op, size)) == NULL) {
		write(2, "diffh: line too long\n", 21);
		_exit(1);
	}
	return np;
}
