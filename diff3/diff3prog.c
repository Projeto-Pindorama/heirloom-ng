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

/*	from 4.3BSD diff3.c	4.4 (Berkeley) 8/27/85	*/

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)/usr/lib/diff3prog.sl	1.9 (gritter) 5/29/05";

#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#if defined (__GLIBC__) && defined (_IO_getc_unlocked)
#undef	getc
#define	getc(f)		_IO_getc_unlocked(f)
#endif

#define	getline(f)	xxgetline(f)	/* glibc _GNU_SOURCE name collision */

/* diff3 - 3-way differential file comparison*/

/* diff3 [-ex3EX] d13 d23 f1 f2 f3 [m1 m3]
 *
 * d13 = diff report on f1 vs f3
 * d23 = diff report on f2 vs f3
 * f1, f2, f3 the 3 files
 * if changes in f1 overlap with changes in f3, m1 and m3 are used
 * to mark the overlaps; otherwise, the file names f1 and f3 are used
 * (only for options E and X).
*/

struct  range {int from,to; };
	/* from is first in range of changed lines
	 * to is last+1
	 * from=to=line after point of insertion
	* for added lines
	*/
struct diff {struct range old, new;};

static long	NC;
static struct diff *d13;
static struct diff *d23;
/* de is used to gather editing scripts,
 * that are later spewed out in reverse order.
 * its first element must be all zero
 * the "new" component of de contains line positions
 * or byte positions depending on when you look(!?)
 * array overlap indicates which sections in de correspond to
 * lines that are different in all three files.
*/
static struct diff *de;
static char *overlap;
static int  overlapcnt =0;

static char *line;
static long linesize;
static FILE *fp[3];
/*	the number of the last-read line in each file
 *	is kept in cline[0-2]
*/
static int cline[3];
/*	the latest known correspondence between line
 *	numbers of the 3 files is stored in last[1-3]
*/
static int last[4];
static int eflag;
static int oflag;      /* indicates whether to mark overlaps (-E or -X)*/
static int debug  = 0;
static char *f1mark, *f3mark; /*markers for -E and -X*/

static int readin(const char *, int);
static int number(const char **);
static int getchange(FILE *);
static int getline(FILE *);
static void merge(int, int);
static void separate(const char *);
static void change(int, struct range *, int);
static void prange(struct range *);
static void keep(int, struct range *);
static int skip(int, int, const char *);
static int duplicate(struct range *, struct range *);
static void repos(int);
static void trouble(void);
static int edit(struct diff *, int, int);
static void edscript(int);
static char *mark(const char *, const char *);
static void *srealloc(void *, size_t);
static void grownc(void);

int
main(int argc,char **argv)
{
	register int i,m,n;
        eflag=0; oflag=0;
	if(argc > 1 && *argv[1]=='-') {
		switch(argv[1][1]) {
		default:
			eflag = 3;
			break;
		case '3':
			eflag = 2;
			break;
		case 'x':
			eflag = 1;
                        break;
                case 'E':
                        eflag = 3;
                        oflag = 1;
                        break;
                case 'X':
                        oflag = eflag = 1;
                        break;
		}
		argv++;
		argc--;
	}
	if(argc<6) {
		fprintf(stderr,"diff3: arg count\n");
		exit(2);
	}
        if (oflag) { 
		f1mark = mark("<<<<<<<",argc>=7?argv[6]:argv[3]);
		f3mark = mark(">>>>>>>",argc>=8?argv[7]:argv[5]);
        }

	m = readin(argv[1],13);
	n = readin(argv[2],23);
	for(i=0;i<=2;i++)
		if((fp[i] = fopen(argv[i+3],"r")) == NULL) {
			printf("diff3: can't open %s\n",argv[i+3]);
			exit(1);
		}
	merge(m,n);
	return 0;
}

/*pick up the line numbers of allcahnges from
 * one change file
 * (this puts the numbers in a vector, which is not
 * strictly necessary, since the vector is processed
 * in one sequential pass. The vector could be optimized
 * out of existence)
*/

static int
readin(const char *name,int n)
{
	register int i;
	int a,b,c,d;
	char kind;
	const char *p;
	fp[0] = fopen(name,"r");
	for(i=0;getchange(fp[0]);i++) {
		if(i>=NC)
			grownc();
		p = line;
		a = b = number(&p);
		if(*p==',') {
			p++;
			b = number(&p);
		}
		kind = *p++;
		c = d = number(&p);
		if(*p==',') {
			p++;
			d = number(&p);
		}
		if(kind=='a')
			a++;
		if(kind=='d')
			c++;
		b++;
		d++;
		(n==13?d13:d23)[i].old.from = a;
		(n==13?d13:d23)[i].old.to = b;
		(n==13?d13:d23)[i].new.from = c;
		(n==13?d13:d23)[i].new.to = d;
	}
	if(i>=NC)
		grownc();
	if (i > 0) {
		(n==13?d13:d23)[i].old.from = (n==13?d13:d23)[i-1].old.to;
		(n==13?d13:d23)[i].new.from = (n==13?d13:d23)[i-1].new.to;
	} else {
		(n==13?d13:d23)[i].old.from = 0;
		(n==13?d13:d23)[i].new.from = 0;
	}
	fclose(fp[0]);
	return(i);
}

static int
number(const char **lc)
{
	register int nn;
	nn = 0;
	while(isdigit(**lc & 0377))
		nn = nn*10 + *(*lc)++ - '0';
	return(nn);
}

static int
getchange(FILE *b)
{
	while(getline(b))
		if(isdigit(line[0] & 0377))
			return(1);
	return(0);
}

static int
getline(FILE *b)
{
	register int i, c;
	for(i=0; ;i++) {
		c = getc(b);
		if (i >= linesize - 1)
			line = srealloc(line, linesize += 128);
		if(c==EOF) {
			line[i] = '\0';
			break;
		}
		line[i] = c;
		if(c=='\n') {
			line[++i] = 0;
			return(i);
		}
	}
	return(i);
}

static void
merge(int m1,int m2)
{
	register struct diff *d1, *d2, *d3;
	int dupl;
	int j;
	int t1,t2;
	d1 = d13;
	d2 = d23;
	j = 0;
	for(;(t1 = d1<d13+m1) | (t2 = d2<d23+m2);) {
		if(debug) {
			printf("%d,%d=%d,%d %d,%d=%d,%d\n",
			d1->old.from,d1->old.to,
			d1->new.from,d1->new.to,
			d2->old.from,d2->old.to,
			d2->new.from,d2->new.to);
		}
/*			first file is different from others*/
		if(!t2||t1&&d1->new.to < d2->new.from) {
/*			stuff peculiar to 1st file */
			if(eflag==0) {
				separate("1");
				change(1,&d1->old,0);
				keep(2,&d1->new);
				change(3,&d1->new,0);
			}
			d1++;
			continue;
		}
/*			second file is different from others*/
		if(!t1||t2&&d2->new.to < d1->new.from) {
			if(eflag==0) {
				separate("2");
				keep(1,&d2->new);
				change(2,&d2->old,0);
				change(3,&d2->new,0);
			}
			d2++;
			continue;
		}
/*			merge overlapping changes in first file
 *			this happens after extension see below*/
		if(d1+1<d13+m1 &&
		   d1->new.to>=d1[1].new.from) {
			d1[1].old.from = d1->old.from;
			d1[1].new.from = d1->new.from;
			d1++;
			continue;
		}
/*			merge overlapping changes in second*/
		if(d2+1<d23+m2 &&
		   d2->new.to>=d2[1].new.from) {
			d2[1].old.from = d2->old.from;
			d2[1].new.from = d2->new.from;
			d2++;
			continue;
		}
/*			stuff peculiar to third file or different in all*/
		if(d1->new.from==d2->new.from&&
		   d1->new.to==d2->new.to) {
			dupl = duplicate(&d1->old,&d2->old);
/*				dupl=0 means all files differ
 *				dupl =1 meands files 1&2 identical*/
			if(eflag==0) {
				separate(dupl?"3":"");
				change(1,&d1->old,dupl);
				change(2,&d2->old,0);
				d3 = d1->old.to>d1->old.from?d1:d2;
				change(3,&d3->new,0);
			} else
				j = edit(d1,dupl,j);
			d1++;
			d2++;
			continue;
		}
/*			overlapping changes from file1 & 2
 *			extend changes appropriately to
 *			make them coincide*/
		 if(d1->new.from<d2->new.from) {
			d2->old.from -= d2->new.from-d1->new.from;
			d2->new.from = d1->new.from;
		}
		else if(d2->new.from<d1->new.from) {
			d1->old.from -= d1->new.from-d2->new.from;
			d1->new.from = d2->new.from;
		}
		if(d1->new.to >d2->new.to) {
			d2->old.to += d1->new.to - d2->new.to;
			d2->new.to = d1->new.to;
		}
		else if(d2->new.to >d1->new.to) {
			d1->old.to += d2->new.to - d1->new.to;
			d1->new.to = d2->new.to;
		}
	}
	if(eflag)
		edscript(j);
}

static void
separate(const char *s)
{
	printf("====%s\n",s);
}

/*	the range of ines rold.from thru rold.to in file i
 *	is to be changed. it is to be printed only if
 *	it does not duplicate something to be printed later
*/
static void
change(int i,struct range *rold,int dupl)
{
	printf("%d:",i);
	last[i] = rold->to;
	prange(rold);
	if(dupl)
		return;
	if(debug)
		return;
	i--;
	skip(i,rold->from,NULL);
	skip(i,rold->to,"  ");
}

/*	print the range of line numbers, rold.from  thru rold.to
 *	as n1,n2 or n1
*/
static void
prange(struct range *rold)
{
	if(rold->to<=rold->from)
		printf("%da\n",rold->from-1);
	else {
		printf("%d",rold->from);
		if(rold->to > rold->from+1)
			printf(",%d",rold->to-1);
		printf("c\n");
	}
}

/*	no difference was reported by diff between file 1(or 2)
 *	and file 3, and an artificial dummy difference (trange)
 *	must be ginned up to correspond to the change reported
 *	in the other file
*/
static void
keep(int i,struct range *rnew)
{
	register int delta;
	struct range trange;
	delta = last[3] - last[i];
	trange.from = rnew->from - delta;
	trange.to = rnew->to - delta;
	change(i,&trange,1);
}

/*	skip to just befor line number from in file i
 *	if "pr" is nonzero, print all skipped stuff
 * w	with string pr as a prefix
*/
static int
skip(int i,int from,const char *pr)
{
	register int j,n;
	for(n=0;cline[i]<from-1;n+=j) {
		if((j=getline(fp[i]))==0)
			trouble();
		if(pr)
			printf("%s%s",pr,line);
		cline[i]++;
	}
	return(n);
}

/*	return 1 or 0 according as the old range
 *	(in file 1) contains exactly the same data
 *	as the new range (in file 2)
*/
static int
duplicate(struct range *r1,struct range *r2)
{
	register int c,d;
	register int nchar;
	int nline;
	if(r1->to-r1->from != r2->to-r2->from)
		return(0);
	skip(0,r1->from,NULL);
	skip(1,r2->from,NULL);
	nchar = 0;
	for(nline=0;nline<r1->to-r1->from;nline++) {
		do {
			c = getc(fp[0]);
			d = getc(fp[1]);
			if(c== -1||d== -1)
				trouble();
			nchar++;
			if(c!=d) {
				repos(nchar);
				return(0);
			}
		} while(c!= '\n');
	}
	repos(nchar);
	return(1);
}

static void
repos(int nchar)
{
	register int i;
	for(i=0;i<2;i++) 
		fseek(fp[i], -nchar, SEEK_CUR);
}

static void
trouble(void)
{
	fprintf(stderr,"diff3: logic error\n");
	abort();
}

/*	collect an editing script for later regurgitation
*/
static int
edit(struct diff *diff,int dupl,int j)
{
	if(((dupl+1)&eflag)==0)
		return(j);
	j++;
        overlap[j] = !dupl;
        if (!dupl) overlapcnt++;
	de[j].old.from = diff->old.from;
	de[j].old.to = diff->old.to;
	de[j].new.from = de[j-1].new.to
	    +skip(2,diff->new.from,NULL);
	de[j].new.to = de[j].new.from
	    +skip(2,diff->new.to,NULL);
	return(j);
}

/*		regurgitate */
static void
edscript(int n)
{
	register int j,k;
	char block[BUFSIZ];
	for(n=n;n>0;n--) {
                if (!oflag || !overlap[n]) 
                        prange(&de[n].old);
                else
                        printf("%da\n=======\n", de[n].old.to -1);
		fseek(fp[2], de[n].new.from, 0);
		for(k=de[n].new.to-de[n].new.from;k>0;k-= j) {
			j = k>BUFSIZ?BUFSIZ:k;
			if(fread(block,1,j,fp[2])!=j)
				trouble();
			fwrite(block, 1, j, stdout);
		}
                if (!oflag || !overlap[n]) 
                        printf(".\nw\nq\n");
                else {
                        printf("%s\n.\n",f3mark);
                        printf("%da\n%s\n.\nw\nq\n",de[n].old.from-1,f1mark);
                }
	}
        exit(overlapcnt);
}

static char *
mark(const char *s1, const char *s2)
{
	const char	*sp;
	char	*buf, *bp;

	if ((bp = buf = malloc(strlen(s1) + strlen(s2) + 2)) == NULL) {
		write(2, "no memory\n", 10);
		exit(077);
	}
	for (sp = s1; *sp; sp++)
		*bp++ = *sp;
	*bp++ = ' ';
	for (sp = s2; *sp; sp++)
		*bp++ = *sp;
	*bp = '\0';
	return buf;
}

static void *
srealloc(void *op, size_t size)
{
	void	*np;

	if ((np = realloc(op, size)) == NULL) {
		write(2, "no memory\n", 10);
		exit(077);
	}
	return np;
}

static void
grownc(void)
{
	NC += 50;
	if ((overlap = realloc(overlap, NC * sizeof *overlap)) == NULL ||
			(d13 = realloc(d13, NC * sizeof *d13)) == NULL ||
			(d23 = realloc(d23, NC * sizeof *d23)) == NULL ||
			(de = realloc(de, NC * sizeof *de)) == NULL) {
		write(2,"diff3: too many changes\n", 24);
		exit(1);
	}
}
