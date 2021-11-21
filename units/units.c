/*	from Unix 32V /usr/src/cmd/units.c	*/
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
static const char sccsid[] USED = "@(#)units.sl	1.10 (gritter) 5/29/05";

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "sigset.h"
#include <limits.h>

#ifdef	__GLIBC__
#ifdef	_IO_getc_unlocked
#undef	getc
#define	getc(f)	_IO_getc_unlocked(f)
#endif	/* _IO_getc_unlocked */
#ifdef	_IO_putc_unlocked
#undef	putchar
#define	putchar(c)	_IO_putc_unlocked(c, stdout)
#endif	/* _IO_putc_unlocked */
#endif	/* __GLIBC__ */

#define	NDIM	10
#define	NTAB	601
static char	*dfile	= UNITTAB;
static char	*unames[NDIM];
struct unit
{
	double	factor;
	char	dim[NDIM];
};

static struct table
{
	double	factor;
	char	dim[NDIM];
	char	*name;
} table[NTAB];
static char	names[NTAB*10];
static struct prefix
{
	double	factor;
	char	*pname;
} prefix[] = 
{
	{ 1e-18,	"atto" },
	{ 1e-15,	"femto" },
	{ 1e-12,	"pico" },
	{ 1e-9,		"nano" },
	{ 1e-6,		"micro" },
	{ 1e-3,		"milli" },
	{ 1e-2,		"centi" },
	{ 1e-1,		"deci" },
	{ 1e1,		"deka" },
	{ 1e2,		"hecta" },
	{ 1e2,		"hecto" },
	{ 1e3,		"kilo" },
	{ 1e6,		"mega" },
	{ 1e6,		"meg" },
	{ 1e9,		"giga" },
	{ 1e12,		"tera" },
	{ 0.0,		0 }
};
static FILE	*inp;
static int	fperrc;
static int	peekc;
static int	dumpflg;

static void units(struct unit *);
static int pu(int, int, int);
static int convr(struct unit *);
static int lookup(char *, struct unit *, int, int);
static int equal(const char *, const char *);
static void init(void);
static double getflt(void);
static int get(void);
static struct table *hash(const char *);
static void fperr(int);

int
main(int argc, char **argv)
{
	register int i;
	register char *file;
	struct unit u1, u2;
	double f;

	if(argc>1 && *argv[1]=='-') {
		argc--;
		argv++;
		dumpflg++;
	}
	file = dfile;
	if(argc > 1)
		file = argv[1];
	if ((inp = fopen(file, "r")) == NULL) {
		printf("no table\n");
		exit(1);
	}
	sigset(SIGFPE, fperr);
	init();

loop:
	fperrc = 0;
	printf("you have: ");
	if(convr(&u1))
		goto loop;
	if(fperrc)
		goto fp;
loop1:
	printf("you want: ");
	if(convr(&u2))
		goto loop1;
	for(i=0; i<NDIM; i++)
		if(u1.dim[i] != u2.dim[i])
			goto conform;
	f = u1.factor/u2.factor;
	if(fperrc)
		goto fp;
	printf("\t* %e\n", f);
	printf("\t/ %e\n", 1./f);
	goto loop;

conform:
	if(fperrc)
		goto fp;
	printf("conformability\n");
	units(&u1);
	units(&u2);
	goto loop;

fp:
	printf("underflow or overflow\n");
	goto loop;
}

static void
units(struct unit *up)
{
	register struct unit *p;
	register int f, i;

	p = up;
	printf("\t%e ", p->factor);
	f = 0;
	for(i=0; i<NDIM; i++)
		f |= pu(p->dim[i], i, f);
	if(f&1) {
		putchar('/');
		f = 0;
		for(i=0; i<NDIM; i++)
			f |= pu(-p->dim[i], i, f);
	}
	putchar('\n');
}

static int
pu(int u, int i, int f)
{

	if(u > 0) {
		if(f&2)
			putchar('-');
		if(unames[i])
			printf("%s", unames[i]); else
			printf("*%c*", i+'a');
		if(u > 1)
			putchar(u+'0');
			return(2);
	}
	if(u < 0)
		return(1);
	return(0);
}

static int
convr(struct unit *up)
{
	register struct unit *p;
	register int c;
	register char *cp;
	char name[LINE_MAX+1];
	int den, err;

	p = up;
	for(c=0; c<NDIM; c++)
		p->dim[c] = 0;
	p->factor = getflt();
	if(p->factor == 0.)
		p->factor = 1.0;
	err = 0;
	den = 0;
	cp = name;

loop:
	switch(c=get()) {

	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '-':
	case '/':
	case ' ':
	case '\t':
	case '\n':
		if(cp != name) {
			*cp++ = 0;
			cp = name;
			err |= lookup(cp, p, den, c);
		}
		if(c == '/')
			den++;
		if(c == '\n')
			return(err);
		goto loop;
	}
	if (cp >= &name[sizeof name - 2])
		cp--;
	*cp++ = c;
	goto loop;
}

static int
lookup(char *name, struct unit *up, int den, int c)
{
	register struct unit *p;
	register struct table *q;
	register int i;
	char *cp1, *cp2;
	double e;

	p = up;
	e = 1.0;

loop:
	q = hash(name);
	if(q->name) {
		l1:
		if(den) {
			p->factor /= q->factor*e;
			for(i=0; i<NDIM; i++)
				p->dim[i] -= q->dim[i];
		} else {
			p->factor *= q->factor*e;
			for(i=0; i<NDIM; i++)
				p->dim[i] += q->dim[i];
		}
		if(c >= '2' && c <= '9') {
			c--;
			goto l1;
		}
		return(0);
	}
	for(i=0; cp1 = prefix[i].pname; i++) {
		cp2 = name;
		while(*cp1 == *cp2++)
			if(*cp1++ == 0) {
				cp1--;
				break;
			}
		if(*cp1 == 0) {
			e *= prefix[i].factor;
			name = cp2-1;
			goto loop;
		}
	}
	for(cp1 = name; *cp1; cp1++);
	if(cp1 > name+1 && *--cp1 == 's') {
		*cp1 = 0;
		goto loop;
	}
	printf("cannot recognize %s\n", name);
	return(1);
}

static int
equal(const char *s1, const char *s2)
{
	register const char *c1, *c2;

	c1 = s1;
	c2 = s2;
	while(*c1++ == *c2)
		if(*c2++ == 0)
			return(1);
	return(0);
}

static void
init(void)
{
	register char *cp;
	register struct table *tp, *lp;
	int c, i, f, t;
	char *np;

	cp = names;
	for(i=0; i<NDIM; i++) {
		np = cp;
		*cp++ = '*';
		*cp++ = i+'a';
		*cp++ = '*';
		*cp++ = 0;
		lp = hash(np);
		lp->name = np;
		lp->factor = 1.0;
		lp->dim[i] = 1;
	}
	lp = hash("");
	lp->name = cp-1;
	lp->factor = 1.0;

l0:
	c = get();
	if(c == 0) {
		if(dumpflg) {
			printf("%d units; %ld bytes\n\n", i, (long)(cp-names));
			for(tp = &table[0]; tp < &table[NTAB]; tp++) {
				if(tp->name == 0)
					continue;
				printf("%s", tp->name);
				units((struct unit *)tp);
			}
		}
		fclose(inp);
		inp = stdin;
		return;
	}
	if(c == '/') {
		while(c != '\n' && c != 0)
			c = get();
		goto l0;
	}
	if(c == '\n')
		goto l0;
	np = cp;
	while(c != ' ' && c != '\t') {
		*cp++ = c;
		c = get();
		if (c==0)
			goto l0;
		if(c == '\n') {
			*cp++ = 0;
			tp = hash(np);
			if(tp->name)
				goto redef;
			tp->name = np;
			tp->factor = lp->factor;
			for(c=0; c<NDIM; c++)
				tp->dim[c] = lp->dim[c];
			i++;
			goto l0;
		}
	}
	*cp++ = 0;
	lp = hash(np);
	if(lp->name)
		goto redef;
	convr((struct unit *)lp);
	lp->name = np;
	f = 0;
	i++;
	if(lp->factor != 1.0)
		goto l0;
	for(c=0; c<NDIM; c++) {
		t = lp->dim[c];
		if(t>1 || (f>0 && t!=0))
			goto l0;
		if(f==0 && t==1) {
			if(unames[c])
				goto l0;
			f = c+1;
		}
	}
	if(f>0)
		unames[f-1] = np;
	goto l0;

redef:
	printf("redefinition %s\n", np);
	goto l0;
}

static double
getflt(void)
{
	register int c, i, dp;
	double d, e;
	int f;

	d = 0.;
	dp = 0;
	do
		c = get();
	while(c == ' ' || c == '\t');

l1:
	if(c >= '0' && c <= '9') {
		d = d*10. + c-'0';
		if(dp)
			dp++;
		c = get();
		goto l1;
	}
	if(c == '.') {
		dp++;
		c = get();
		goto l1;
	}
	if(dp)
		dp--;
	if(c == '+' || c == '-') {
		f = 0;
		if(c == '-')
			f++;
		i = 0;
		c = get();
		while(c >= '0' && c <= '9') {
			i = i*10 + c-'0';
			c = get();
		}
		if(f)
			i = -i;
		dp -= i;
	}
	e = 1.;
	i = dp;
	if(i < 0)
		i = -i;
	while(i--)
		e *= 10.;
	if(dp < 0)
		d *= e; else
		d /= e;
	if(c == '|')
		return(d/getflt());
	peekc = c;
	return(d);
}

static int
get(void)
{
	register int c;

	if(c=peekc) {
		peekc = 0;
		return(c);
	}
	c = getc(inp);
	if (c == EOF) {
		if (inp == stdin) {
			printf("\n");
			exit(0);
		}
		return(0);
	}
	return(c);
}

static struct table *
hash(const char *name)
{
	register struct table *tp;
	register const char *np;
	register unsigned h;

	h = 0;
	np = name;
	while(*np)
		h = h*57 + *np++ - '0';
	if( ((int)h)<0) h= -(int)h;
	h %= NTAB;
	tp = &table[h];
l0:
	if(tp->name == 0)
		return(tp);
	if(equal(name, tp->name))
		return(tp);
	tp++;
	if(tp >= &table[NTAB])
		tp = table;
	goto l0;
}

/*ARGSUSED*/
static void
fperr(int signo)
{

	fperrc++;
}
