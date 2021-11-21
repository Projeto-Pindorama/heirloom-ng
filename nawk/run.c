/*
   Changes by Gunnar Ritter, Freiburg i. Br., Germany, December 2002.
  
   Sccsid @(#)run.c	1.33 (gritter) 12/25/06>
 */
/* UNIX(R) Regular Expression Tools

   Copyright (C) 2001 Caldera International, Inc.
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to:
       Free Software Foundation, Inc.
       59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/*	copyright	"%c%"	*/


/*	from unixsrc:usr/src/common/cmd/awk/run.c /main/uw7_nj/1	*/
/*	from RCS Header: run.c 1.3 91/08/12 	*/

#define tempfree(x,s)	if (istemp(x)) tfree(x,s); else

/* #define	execute(p)	(isvalue(p) ? (Cell *)((p)->narg[0]) : r_execute(p)) */
#define	execute(p) r_execute((Node *)p)

#define DEBUG
#include	<math.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<setjmp.h>
#include	<pfmt.h>
#include	<string.h>
#include	<errno.h>
#include	<wctype.h>
#include	<inttypes.h>
#include	<time.h>
#include	"awk.h"
#include	"y.tab.h"

jmp_buf env;

#define	getfval(p)	(((p)->tval & (ARR|FLD|REC|NUM)) == NUM ? (p)->fval : r_getfval(p))
#define	getsval(p)	(((p)->tval & (ARR|FLD|REC|STR)) == STR ? (p)->sval : r_getsval(p))

static void tfree(register Cell *a, char *s);

#define PA2NUM	29
int	pairstack[PA2NUM];
long	paircnt;
Node	*winner = NULL;
Cell	*tmps;

static Cell	truecell	={ OBOOL, BTRUE, 0, 0, 1.0, NUM };
Cell	*true	= &truecell;
static Cell	falsecell	={ OBOOL, BFALSE, 0, 0, 0.0, NUM };
Cell	*false	= &falsecell;
static Cell	breakcell	={ OJUMP, JBREAK, 0, 0, 0.0, NUM };
Cell	*jbreak	= &breakcell;
static Cell	contcell	={ OJUMP, JCONT, 0, 0, 0.0, NUM };
Cell	*jcont	= &contcell;
static Cell	nextcell	={ OJUMP, JNEXT, 0, 0, 0.0, NUM };
Cell	*jnext	= &nextcell;
static Cell	exitcell	={ OJUMP, JEXIT, 0, 0, 0.0, NUM };
Cell	*jexit	= &exitcell;
static Cell	retcell		={ OJUMP, JRET, 0, 0, 0.0, NUM };
Cell	*jret	= &retcell;
static Cell	tempcell	={ OCELL, CTEMP, 0, 0, 0.0, NUM };

Node	*curnode = NULL;	/* the node being executed, for debugging */

static	const	char
	restoobig[] = ":40:%s() result %.20s too big",
	notarray[] = ":41:%s is not an array",
	ioerror[] = ":42:I/O error occurred on %s";
const char
	illstat[] = ":43:Illegal statement";

extern	const	char	readvofid[], readvof[], badopen[];

static int	growsprintf(unsigned char **, unsigned char **,
		int *, const char *, ...);
static void growbuf(unsigned char **buf, int *bufsize, int incr,
		unsigned char **ptr, const char *fn);
static void closeall(void);
static void caseconv(unsigned char *s, wint_t (*conv)(wint_t));

int run(Node *a)
{
	execute(a);
	closeall();
	return 0;
}

Cell *r_execute(Node *u)
{
	register Cell *(*proc)(Node **, int);
	register Cell *x;
	register Node *a;

	if (u == NULL)
		return(true);
	for (a = u; ; a = a->nnext) {
		curnode = a;
		if (isvalue(a)) {
			x = (Cell *) (a->narg[0]);
			if ((x->tval & FLD) && !donefld)
				fldbld();
			else if ((x->tval & REC) && !donerec)
				recbld();
			return(x);
		}
		if (notlegal(a->nobj))	/* probably a Cell* but too risky to print */
			error(MM_ERROR, illstat);
		proc = proctab[a->nobj-FIRSTTOKEN];
		x = (*proc)(a->narg, a->nobj);
		if ((x->tval & FLD) && !donefld)
			fldbld();
		else if ((x->tval & REC) && !donerec)
			recbld();
		if (isexpr(a))
			return(x);
		/* a statement, goto next statement */
		if (isjump(x))
			return(x);
		if (a->nnext == (Node *)NULL)
			return(x);
		tempfree(x, "execute");
	}
}


Cell *program(register Node **a, int n)
{
	register Cell *x = 0;

	if (setjmp(env) != 0)
		goto ex;
	if (a[0]) {		/* BEGIN */
		x = execute(a[0]);
		if (isexit(x))
			return(true);
		if (isjump(x))
			error(MM_ERROR,
				":44:Illegal break, continue or next from BEGIN");
		if(x != 0) { tempfree(x, ""); }
	}
  loop:
	if (a[1] || a[2])
		while (getrec(&record, &recsize) > 0) {
			x = execute(a[1]);
			if (isexit(x))
				break;
			if(x != 0) { tempfree(x, ""); }
		}
  ex:
	if (setjmp(env) != 0)
		goto ex1;
	if (a[2]) {		/* END */
		x = execute(a[2]);
		if (iscont(x))	/* read some more */
			goto loop;
		if (isbreak(x) || isnext(x))
			error(MM_ERROR, ":45:Illegal break or next from END");
		if(x != 0) { tempfree(x, ""); }
	}
  ex1:
	return(true);
}

struct Frame {
	int nargs;	/* number of arguments in this call */
	Cell *fcncell;	/* pointer to Cell for function */
	Cell **args;	/* pointer to array of arguments after execute */
	Cell *retval;	/* return value */
};

#define	NARGS	30

struct Frame *frame = NULL;	/* base of stack frames; dynamically allocated */
int	nframe = 0;		/* number of frames allocated */
struct Frame *fp = NULL;	/* frame pointer. bottom level unused */

Cell *call(Node **a, int n)
{
	static Cell newcopycell = { OCELL, CCOPY, 0, (unsigned char *) "", 0.0, NUM|STR|DONTFREE };
	int i, ncall, ndef;
	Node *x;
	Cell *args[NARGS], *oargs[NARGS], *y, *z, *fcn;
	unsigned char *s;

	fcn = execute(a[0]);	/* the function itself */
	s = fcn->nval;
	if (!isfunc(fcn))
		error(MM_ERROR, ":46:Calling undefined function %s", s);
	if (frame == NULL) {
		fp = frame = (struct Frame *) calloc(nframe += 100, sizeof(struct Frame));
		if (frame == NULL)
			error(MM_ERROR, ":47:Out of space for stack frames calling %s", s);
	}
	for (ncall = 0, x = a[1]; x != NULL; x = x->nnext)	/* args in call */
		ncall++;
	ndef = (int) fcn->fval;			/* args in defn */
	dprintf( ("calling %s, %d args (%d in defn), fp=%ld\n", s,
		ncall, ndef, (long)(fp-frame)) );
	if (ncall > ndef) {
		if (ncall == 1)
			error(MM_WARNING, ":48:Function %s called with 1 arg, uses only %d",
				s, ndef);
		else
			error(MM_WARNING, ":49:Function %s called with %d args, uses only %d",
				s, ncall, ndef);
	}
	if (ncall + ndef > NARGS)
		error(MM_ERROR, ":50:Function %s has %d arguments, limit %d",
			s, ncall+ndef, NARGS);
	for (i = 0, x = a[1]; x != NULL; i++, x = x->nnext) {	/* get call args */
		dprintf( ("evaluate args[%d], fp=%ld:\n", i,
					(long)(fp-frame)) );
		y = execute(x);
		oargs[i] = y;
		dprintf( ("args[%d]: %s %f <%s>, t=%o\n",
			   i, y->nval, y->fval, isarr(y) ?
			   	"(array)" : (char*) y->sval, y->tval) );
		if (isfunc(y))
			error(MM_ERROR, ":51:Cannot use function %s as argument in %s",
				y->nval, s);
		if (isarr(y))
			args[i] = y;	/* arrays by ref */
		else
			args[i] = copycell(y);
		tempfree(y, "callargs");
	}
	for ( ; i < ndef; i++) {	/* add null args for ones not provided */
		args[i] = gettemp("nullargs");
		*args[i] = newcopycell;
	}
	fp++;	/* now ok to up frame */
	if (fp >= frame + nframe) {
		int dfp = fp - frame;	/* old index */
		frame = (struct Frame *)
			realloc(frame, (nframe += 100) * sizeof(struct Frame));
		if (frame == NULL)
			error(MM_ERROR, ":52:Out of space for stack frames in %s", s);
		fp = frame + dfp;
	}
	fp->fcncell = fcn;
	fp->args = args;
	fp->nargs = ndef;	/* number defined with (excess are locals) */
	fp->retval = gettemp("retval");

	dprintf( ("start exec of %s, fp=%ld\n", s, (long)(fp-frame)) );
	y = execute((Node *)(fcn->sval));	/* execute body */
	dprintf( ("finished exec of %s, fp=%ld\n", s, (long)(fp-frame)) );

	for (i = 0; i < ndef; i++) {
		Cell *t = fp->args[i];
		if (isarr(t)) {
			if (t->csub == CCOPY) {
				if (i >= ncall) {
					freesymtab(t);
					t->csub = CTEMP;
				} else {
					oargs[i]->tval = t->tval;
					oargs[i]->tval &= ~(STR|NUM|DONTFREE);
					oargs[i]->sval = t->sval;
					tempfree(t, "oargsarr");
				}
			}
		} else if (t != y) {	/* kludge to prevent freeing twice */
			t->csub = CTEMP;
			tempfree(t, "fp->args");
		}
	}
	tempfree(fcn, "call.fcn");
	if (isexit(y) || isnext(y))
		return y;
	tempfree(y, "fcn ret");		/* this can free twice! */
	z = fp->retval;			/* return value */
	dprintf( ("%s returns %g |%s| %o\n", s, getfval(z), 
		getsval(z), z->tval) );
	fp--;
	return(z);
}

Cell *copycell(Cell *x)	/* make a copy of a cell in a temp */
{
	Cell *y;

	y = gettemp("copycell");
	y->csub = CCOPY;	/* prevents freeing until call is over */
	y->nval = x->nval;
	y->sval = x->sval ? tostring(x->sval) : NULL;
	y->fval = x->fval;
	y->tval = x->tval & ~(CON|FLD|REC|DONTFREE);	/* copy is not constant or field */
							/* is DONTFREE right? */
	return y;
}

/*ARGSUSED2*/
Cell *arg(Node **a, int nnn)
{
	int n;

	n = (intptr_t) a[0];	/* argument number, counting from 0 */
	dprintf( ("arg(%d), fp->nargs=%d\n", n, fp->nargs) );
	if (n+1 > fp->nargs)
		error(MM_ERROR, ":53:Argument #%d of function %s was not supplied",
			n+1, fp->fcncell->nval);
	return fp->args[n];
}

static int in_loop = 0;	/* Flag : are we in a [while|do|for] loop ? */

Cell *jump(Node **a, int n)
{
	register Cell *y;

	switch (n) {
	case EXIT:
		if (a[0] != NULL) {
			y = execute(a[0]);
			errorflag = getfval(y);
			tempfree(y, "");
		}
		longjmp(env, 1);
	case RETURN:
		if (a[0] != NULL) {
			y = execute(a[0]);
			if ((y->tval & (STR|NUM)) == (STR|NUM)) {
				setsval(fp->retval, getsval(y));
				fp->retval->fval = getfval(y);
				fp->retval->tval |= NUM;
			}
			else if (y->tval & STR)
				setsval(fp->retval, getsval(y));
			else if (y->tval & NUM)
				setfval(fp->retval, getfval(y));
			tempfree(y, "");
		}
		return(jret);
	case NEXT:
		return(jnext);
	case BREAK:
		if (posix && !in_loop)
			error(MM_ERROR, ":101:break-statement outside of a loop");
		return(jbreak);
	case CONTINUE:
		if (posix && !in_loop)
			error(MM_ERROR, ":102:continue-statement outside of a loop");
		return(jcont);
	default:	/* can't happen */
		error(MM_ERROR, ":54:Illegal jump type %d", n);
		/*NOTREACHED*/
		return 0;
	}
}

Cell *getline(Node **a, int n)
{
	/* a[0] is variable, a[1] is operator, a[2] is filename */
	register Cell *r, *x;
	unsigned char *buf = NULL;
	int bufsize = 0;
	FILE *fp;

	fflush(stdout);	/* in case someone is waiting for a prompt */
	r = gettemp("");
	if (a[1] != NULL) {		/* getline < file */
		x = execute(a[2]);		/* filename */
		if ((intptr_t) a[1] == '|')	/* input pipe */
			a[1] = (Node *) LE;	/* arbitrary flag */
		fp = openfile((intptr_t) a[1], getsval(x));
		tempfree(x, "");
		if (fp == NULL)
			n = -1;
		else
			n = readrec(&buf, &bufsize, fp);
		if (n <= 0) {
			;
		} else if (a[0] != NULL) {	/* getline var <file */
			setsval(execute(a[0]), buf);
		} else {			/* getline <file */
			makerec(buf, bufsize);
		}
	} else {			/* bare getline; use current input */
		if (a[0] == NULL)	/* getline */
			n = getrec(&record, &recsize);
		else {			/* getline var */
			n = getrec(&buf, &bufsize);
			setsval(execute(a[0]), buf);
		}
	}
	setfval(r, (Awkfloat) n);
	if (bufsize)
		free(buf);
	return r;
}

Cell *getnf(register Node **a, int n)
{
	if (donefld == 0)
		fldbld();
	return (Cell *) a[0];
}

Cell *array(register Node **a, int n)
{
	register Cell *x, *y, *z;
	register unsigned char *s;
	register Node *np;
	unsigned char *buf = NULL;
	int bufsz = 0, subseplen, len = 1, l;

	x = execute(a[0]);	/* Cell* for symbol table */
	subseplen = strlen((char *)*SUBSEP);
	growbuf(&buf, &bufsz, CHUNK, NULL, "array");
	buf[0] = 0;
	for (np = a[1]; np; np = np->nnext) {
		y = execute(np);	/* subscript */
		s = getsval(y);
		len += (l = strlen((char *)s) + subseplen);
		if (len >= bufsz)
			growbuf(&buf, &bufsz, l, NULL, "array");
		strcat((char*)buf, (char*)s);
		if (np->nnext)
			strcat((char*)buf, (char*)*SUBSEP);
		tempfree(y, "");
	}
	if (!isarr(x)) {
		dprintf( ("making %s into an array\n", x->nval) );
		if (freeable(x))
			xfree(x->sval);
		x->tval &= ~(STR|NUM|DONTFREE);
		x->tval |= ARR;
		x->sval = (unsigned char *) makesymtab(NSYMTAB);
	}
	z = setsymtab(buf, "", 0.0, STR|NUM, (Array *) x->sval);
	z->ctype = OCELL;
	z->csub = CVAR;
	tempfree(x, "");
	free(buf);
	return(z);
}

Cell *delete(Node **a, int n)
{
	Cell *x, *y;
	Node *np;
	unsigned char *buf = NULL, *s;
	int bufsz = 0, subseplen, len = 1, l;

	x = execute(a[0]);	/* Cell* for symbol table */
	if (!isarr(x))
		return true;
	subseplen = strlen((char *)*SUBSEP);
	growbuf(&buf, &bufsz, CHUNK, NULL, "delete");
	buf[0] = 0;
	for (np = a[1]; np; np = np->nnext) {
		y = execute(np);	/* subscript */
		s = getsval(y);
		len += (l = strlen((char *)s) + subseplen);
		if (len >= bufsz)
			growbuf(&buf, &bufsz, l, NULL, "delete");
		strcat((char*)buf, (char*)s);
		if (np->nnext)
			strcat((char*)buf, (char*)*SUBSEP);
		tempfree(y, "");
	}
	freeelem(x, buf);
	tempfree(x, "");
	free(buf);
	return true;
}

Cell *intest(Node **a, int n)
{
	register Cell *x, *ap, *k;
	Node *p;
	unsigned char *s;
	unsigned char *buf = NULL;
	int bufsz = 0, subseplen, len = 1, l;

	ap = execute(a[1]);	/* array name */
	if (!isarr(ap))
		error(MM_ERROR, notarray, ap->nval);
	subseplen = strlen((char *)*SUBSEP);
	growbuf(&buf, &bufsz, CHUNK, NULL, "intest");
	buf[0] = 0;
	for (p = a[0]; p; p = p->nnext) {
		x = execute(p);	/* expr */
		s = getsval(x);
		len += (l = strlen((char *)s) + subseplen);
		if (len >= bufsz)
			growbuf(&buf, &bufsz, l, NULL, "array");
		strcat((char *)buf, (char*)s);
		tempfree(x, "");
		if (p->nnext)
			strcat((char *)buf, (char*)*SUBSEP);
	}
	k = lookup(buf, (Array *) ap->sval);
	tempfree(ap, "");
	free(buf);
	if (k == NULL)
		return(false);
	else
		return(true);
}


Cell *matchop(Node **a, int n)
{
	register Cell *x, *y;
	register unsigned char *s, *t;
	register int i;
	fa *pfa;
	int (*mf)(void *, unsigned char *) = match, mode = 0;

	if (n == MATCHFCN) {
		mf = pmatch;
		mode = 1;
	}
	x = execute(a[1]);
	s = getsval(x);
	if (a[0] == 0)
		i = (*mf)(a[2], s);
	else {
		y = execute(a[2]);
		t = getsval(y);
		pfa = makedfa(t, mode);
		i = (*mf)(pfa, s);
		tempfree(y, "");
	}
	tempfree(x, "");
	if (n == MATCHFCN) {
		int start, length;
		if (patlen < 0) {
			start = 0;
			length = patlen;
		} else {
			start = chrdist(s, patbeg);
			length = chrdist(patbeg, &patbeg[patlen - 1]);
		}
		setfval(rstartloc, (Awkfloat) start);
		setfval(rlengthloc, (Awkfloat) length);
		x = gettemp("");
		x->tval = NUM;
		x->fval = start;
		return x;
	} else if ((n == MATCH && i == 1) || (n == NOTMATCH && i == 0))
		return(true);
	else
		return(false);
}


Cell *boolop(Node **a, int n)
{
	register Cell *x, *y;
	register int i;

	x = execute(a[0]);
	i = istrue(x);
	tempfree(x, "");
	switch (n) {
	case BOR:
		if (i) return(true);
		y = execute(a[1]);
		i = istrue(y);
		tempfree(y, "");
		if (i) return(true);
		else return(false);
	case AND:
		if ( !i ) return(false);
		y = execute(a[1]);
		i = istrue(y);
		tempfree(y, "");
		if (i) return(true);
		else return(false);
	case NOT:
		if (i) return(false);
		else return(true);
	default:	/* can't happen */
		error(MM_ERROR, ":55:Unknown boolean operator %d", n);
	}
	/*NOTREACHED*/
	return 0;
}

Cell *relop(Node **a, int n)
{
	register int i;
	register Cell *x, *y;
	Awkfloat j;

	x = execute(a[0]);
	y = execute(a[1]);
	if (x->tval&NUM && y->tval&NUM) {
		j = x->fval - y->fval;
		i = j<0? -1: (j>0? 1: 0);
	} else {
		i = strcoll((char*)getsval(x), (char*)getsval(y));
	}
	tempfree(x, "");
	tempfree(y, "");
	switch (n) {
	case LT:	if (i<0) return(true);
			else return(false);
	case LE:	if (i<=0) return(true);
			else return(false);
	case NE:	if (i!=0) return(true);
			else return(false);
	case EQ:	if (i == 0) return(true);
			else return(false);
	case GE:	if (i>=0) return(true);
			else return(false);
	case GT:	if (i>0) return(true);
			else return(false);
	default:	/* can't happen */
		error(MM_ERROR, ":56:Unknown relational operator %d", n);
	}
	/*NOTREACHED*/
	return 0;
}

static void tfree(register Cell *a, char *s)
{
	if (dbg>1) printf("## tfree %.8s %06lo %s\n",
		s, (long)a, a->sval ? a->sval : (unsigned char *)"");
	if (freeable(a))
		xfree(a->sval);
	if (a == tmps)
		error(MM_ERROR, ":57:Tempcell list is curdled");
	a->cnext = tmps;
	tmps = a;
}

Cell *gettemp(const char *s)
{	int i;
	register Cell *x;

	if (!tmps) {
		tmps = (Cell *) calloc(100, sizeof(Cell));
		if (!tmps)
			error(MM_ERROR, ":58:No space for temporaries");
		for(i = 1; i < 100; i++)
			tmps[i-1].cnext = &tmps[i];
		tmps[i-1].cnext = 0;
	}
	x = tmps;
	tmps = x->cnext;
	*x = tempcell;
	if (dbg>1) printf("## gtemp %.8s %06lo\n", s, (long)x);
	return(x);
}

Cell *indirect(Node **a, int n)
{
	register Cell *x;
	register int m;
	register unsigned char *s;

	x = execute(a[0]);
	m = getfval(x);
	if (m == 0 && !is2number(s = getsval(x), 0))	/* suspicion! */
		error(MM_ERROR, ":59:Illegal field $(%s)", s);
	tempfree(x, "");
	x = fieldadr(m);
	x->ctype = OCELL;
	x->csub = CFLD;
	return(x);
}

Cell *substr(Node **a, int nnn)
{
	register int k, m, n;
	wchar_t wc;
	register unsigned char *s, *sp, *sq;
	int temp;
	register Cell *x, *y, *z = 0;

	x = execute(a[0]);
	y = execute(a[1]);
	if (a[2] != 0)
		z = execute(a[2]);
	s = getsval(x);
	k = strlen((char*)s) + 1;
	if (k <= 1) {
		tempfree(x, "");
		tempfree(y, "");
		if (a[2] != 0) {
			tempfree(z, "");
		}
		x = gettemp("");
		setsval(x, (unsigned char *)"");
		return(x);
	}
	m = getfval(y);
	if (m <= 0)
		m = 1;
	else if (m > k)
		m = k;
	tempfree(y, "");
	if (a[2] != 0) {
		n = getfval(z);
		tempfree(z, "");
	} else
		n = k - 1;
	if (n < 0)
		n = 0;
	else if (n > k - m)
		n = k - m;
	dprintf( ("substr: m=%d, n=%d, s=%s\n", m, n, s) );
	if (mb_cur_max > 1) {
		for (sp = s; m > 1 && *sp; m--) {
			next(wc, sp, k);
			sp += k;
		}
		m = sp - s + 1;
		for (sq = sp ; n > 0 && *sq; n--) {
			next(wc, sq, k);
			sq += k;
		}
		n = sq - sp;
		dprintf( ("substr: multibyte: m=%d, n=%d, s=%s\n", m, n, s) );
	}
	y = gettemp("");
	temp = s[n+m-1];	/* with thanks to John Linderman */
	s[n+m-1] = '\0';
	setsval(y, s + m - 1);
	s[n+m-1] = temp;
	tempfree(x, "");
	return(y);
}

Cell *sindex(Node **a, int nnn)
{
	register Cell *x, *y, *z;
	register unsigned char *s1, *s2, *p1, *p2, *q;
	int n, nq, n2;
	wchar_t wc, wq, w2;
	Awkfloat v = 0.0;

	x = execute(a[0]);
	s1 = getsval(x);
	y = execute(a[1]);
	s2 = getsval(y);

	z = gettemp("");
	for (p1 = s1; next(wc, p1, n), wc != '\0'; p1 += n) {
		for (q = p1, p2 = s2;
				next(wq, q, nq),
				next(w2, p2, n2),
				w2 != '\0' && wq == w2;
				q += nq, p2 += n2)
			;
		if (w2 == '\0') {
			v = (Awkfloat) chrdist(s1, p1);
			break;
		}
	}
	tempfree(x, "");
	tempfree(y, "");
	setfval(z, v);
	return(z);
}


int format(unsigned char **buf, int *bufsize, const unsigned char *s, Node *a)
{
	unsigned char *fmt = NULL;
	int fmtsz = 0;
	unsigned char *p, *t;
	const unsigned char *os;
	register Cell *x;
	int flag = 0;

	os = s;
	fmt = malloc(fmtsz = CHUNK);
	if (*bufsize == 0)
		*buf = malloc(*bufsize = CHUNK);
	if (fmt == NULL || *buf == NULL)
		error(MM_ERROR, outofspace, "format");
	p = *buf;
	while (*s) {
		if (p >= &(*buf)[*bufsize])
			growbuf(buf, bufsize, CHUNK, &p, "format");
		if (*s != '%') {
			*p++ = *s++;
			continue;
		}
		if (*(s+1) == '%') {
			*p++ = '%';
			s += 2;
			continue;
		}
		for (t=fmt; (*t++ = *s) != '\0'; s++) {
			if (t >= &fmt[fmtsz])
				growbuf(&fmt, &fmtsz,  CHUNK, &t, "format");
			if (isalpha(*s) && *s != 'l' && *s != 'h' && *s != 'L')
				break;	/* the ansi panoply */
			if (*s == '*') {
				x = execute(a);
				a = a->nnext;
				t--;
				growsprintf(&fmt, &t, &fmtsz, "%d", (int) getfval(x));
				tempfree(x, "");
			}
		}
		*t = '\0';
		switch (*s) {
		case 'a': case 'A':
		case 'e': case 'E':
		case 'f': case 'F':
		case 'g': case 'G':
			flag = 1;
			break;
		case 'd': case 'i':
			flag = 2;
			if(*(s-1) == 'l') break;
			*(t-1) = 'l';
			*t = 'd';
			*++t = '\0';
			break;
		case 'o': case 'x': case 'X': case 'u':
			flag = *(s-1) == 'l' ? 12 : 13;
			break;
		case 's':
			/*
			 * Note: If MB_CUR_MAX > 1, the precision is in
			 * bytes, not characters. This doesn't make much
			 * sense in awk context, but it seems to match
			 * what POSIX demands.
			 */
			flag = 4;
			break;
		case 'c':
			if (mb_cur_max > 1) {
				*(t-1) = 'l';
				*t = 'c';
				*++t = '\0';
				flag = 6;
			} else
				flag = 5;
			break;
		default:
			flag = 0;
			break;
		}
		if (flag == 0) {
			growsprintf(buf, &p, bufsize, "%s", fmt);
			continue;
		}
		if (a == NULL)
			error(MM_ERROR, ":61:Not enough args in printf(%s)",
				os);
		x = execute(a);
		a = a->nnext;
		switch (flag) {
		case 1:	growsprintf(buf, &p, bufsize, (char *)fmt, getfval(x));
			break;
		case 2:	growsprintf(buf, &p, bufsize, (char *)fmt,
					(long) getfval(x));
			break;
		case 3:	growsprintf(buf, &p, bufsize, (char *)fmt,
					(int) getfval(x));
			break;
		case 12:growsprintf(buf, &p, bufsize, (char *)fmt,
					(unsigned long) getfval(x));
			break;
		case 13:growsprintf(buf, &p, bufsize, (char *)fmt,
					(unsigned int) getfval(x));
			break;
		case 4:	growsprintf(buf, &p, bufsize, (char *)fmt, getsval(x));
			break;
		case 5: isnum(x) ? growsprintf(buf, &p, bufsize, (char *)fmt,
					(int) getfval(x))
				 : growsprintf(buf, &p, bufsize, (char *)fmt,
					 getsval(x)[0]);
			break;
		case 6: isnum(x) ? growsprintf(buf, &p, bufsize, (char *)fmt,
					(wint_t) getfval(x))
				 : growsprintf(buf, &p, bufsize, (char *)fmt,
					 (wint_t) getsval(x)[0]);
			break;
		}
		tempfree(x, "");
		s++;
	}
	*p = '\0';
	for ( ; a; a = a->nnext)		/* evaluate any remaining args */
		execute(a);
	xfree(fmt);
	return 0;
}

Cell *awsprintf(Node **a, int n)
{
	register Cell *x;
	register Node *y;
	unsigned char *buf = NULL;
	int bufsize = 0;

	y = a[0]->nnext;
	x = execute(a[0]);
	if (format(&buf, &bufsize, getsval(x), y) == -1)
		error(MM_ERROR, ":62:sprintf string %.40s ... too long", buf);
	tempfree(x, "");
	x = gettemp("");
	x->sval = /*tostring(buf);*/ buf ? buf : tostring("");
	x->tval = STR;
	return(x);
}

Cell *aprintf(Node **a, int n)
{
	FILE *fp;
	register Cell *x;
	register Node *y;
	unsigned char *buf = NULL;
	int bufsize = 0;

	y = a[0]->nnext;
	x = execute(a[0]);
	if (format(&buf, &bufsize, getsval(x), y) == -1)
		error(MM_ERROR, ":63:printf string %.40s ... too long", buf);
	tempfree(x, "");
	if (buf) {
		if (a[1] == NULL)
			fputs((char *)buf, stdout);
		else {
			fp = redirect((intptr_t)a[1], a[2]);
			fputs((char *)buf, fp);
			fflush(fp);
		}
		free(buf);
	}
	return(true);
}

Cell *arith(Node **a, int n)
{
	Awkfloat i, j = 0;
	double v;
	register Cell *x, *y, *z;

	x = execute(a[0]);
	i = getfval(x);
	tempfree(x, "");
	if (n != UMINUS) {
		y = execute(a[1]);
		j = getfval(y);
		tempfree(y, "");
	}
	z = gettemp("");
	switch (n) {
	case ADD:
		i += j;
		break;
	case MINUS:
		i -= j;
		break;
	case MULT:
		i *= j;
		break;
	case DIVIDE:
		if (j == 0)
			error(MM_ERROR, ":64:Division by zero");
		i /= j;
		break;
	case MOD:
		if (j == 0)
			error(MM_ERROR, ":65:Division by zero in mod");
		modf(i/j, &v);
		i = i - j * v;
		break;
	case UMINUS:
		i = -i;
		break;
	case POWER:
		if (j >= 0 && modf(j, &v) == 0.0)	/* pos integer exponent */
			i = ipow(i, (int) j);
		else
			i = errcheck(pow(i, j), (unsigned char *)"pow");
		break;
	default:	/* can't happen */
		error(MM_ERROR, ":66:Illegal arithmetic operator %d", n);
	}
	setfval(z, i);
	return(z);
}

double ipow(double x, int n)
{
	double v;

	if (n <= 0)
		return 1;
	v = ipow(x, n/2);
	if (n % 2 == 0)
		return v * v;
	else
		return x * v * v;
}

Cell *incrdecr(Node **a, int n)
{
	register Cell *x, *z;
	register int k;
	Awkfloat xf;

	x = execute(a[0]);
	xf = getfval(x);
	k = (n == PREINCR || n == POSTINCR) ? 1 : -1;
	if (n == PREINCR || n == PREDECR) {
		setfval(x, xf + k);
		return(x);
	}
	z = gettemp("");
	setfval(z, xf);
	setfval(x, xf + k);
	tempfree(x, "");
	return(z);
}

Cell *assign(Node **a, int n)
{
	register Cell *x, *y;
	Awkfloat xf, yf;
	double v;

	y = execute(a[1]);
	x = execute(a[0]);	/* order reversed from before... */
	if (n == ASSIGN) {	/* ordinary assignment */
		if ((y->tval & (STR|NUM)) == (STR|NUM)) {
			setsval(x, getsval(y));
			x->fval = getfval(y);
			x->tval |= NUM;
		}
		else if (y->tval & STR)
			setsval(x, getsval(y));
		else if (y->tval & NUM)
			setfval(x, getfval(y));
		else
			funnyvar(y, (char *)gettxt(readvofid, readvof));
		tempfree(y, "");
		return(x);
	}
	xf = getfval(x);
	yf = getfval(y);
	switch (n) {
	case ADDEQ:
		xf += yf;
		break;
	case SUBEQ:
		xf -= yf;
		break;
	case MULTEQ:
		xf *= yf;
		break;
	case DIVEQ:
		if (yf == 0)
			error(MM_ERROR, ":67:Division by zero in /=");
		xf /= yf;
		break;
	case MODEQ:
		if (yf == 0)
			error(MM_ERROR, ":68:Division by zero in %%=");
		modf(xf/yf, &v);
		xf = xf - yf * v;
		break;
	case POWEQ:
		if (yf >= 0 && modf(yf, &v) == 0.0)	/* pos integer exponent */
			xf = ipow(xf, (int) yf);
		else
			xf = errcheck(pow(xf, yf), (unsigned char *)"pow");
		break;
	default:
		error(MM_ERROR, ":69:Illegal assignment operator %d", n);
		break;
	}
	tempfree(y, "");
	setfval(x, xf);
	return(x);
}

Cell *cat(Node **a, int q)
{
	register Cell *x, *y, *z;
	register int n1, n2;
	register unsigned char *s;

	x = execute(a[0]);
	y = execute(a[1]);
	getsval(x);
	getsval(y);
	n1 = (int)strlen((char*)x->sval);
	n2 = (int)strlen((char*)y->sval);
	s = (unsigned char *) malloc(n1 + n2 + 1);
	if (s == NULL)
		error(MM_ERROR, ":70:Out of space concatenating %.15s and %.15s",
			x->sval, y->sval);
	strcpy((char*)s, (char*)x->sval);
	strcpy((char*)s+n1, (char*)y->sval);
	tempfree(y, "");
	z = gettemp("");
	z->sval = s;
	z->tval = STR;
	tempfree(x, "");
	return(z);
}

Cell *pastat(Node **a, int n)
{
	register Cell *x;

	if (a[0] == 0)
		x = execute(a[1]);
	else {
		x = execute(a[0]);
		if (istrue(x)) {
			tempfree(x, "");
			x = execute(a[1]);
		}
	}
	return x;
}

Cell *dopa2(Node **a, int n)
{
	register Cell *x;
	register int pair;

	pair = (intptr_t) a[3];
	if (pairstack[pair] == 0) {
		x = execute(a[0]);
		if (istrue(x))
			pairstack[pair] = 1;
		tempfree(x, "");
	}
	if (pairstack[pair] == 1) {
		x = execute(a[1]);
		if (istrue(x))
			pairstack[pair] = 0;
		tempfree(x, "");
		x = execute(a[2]);
		return(x);
	}
	return(false);
}

Cell *split(Node **a, int nnn)
{
	Cell *x = 0, *y, *ap;
	register unsigned char *s;
	wchar_t sep, wc;
	unsigned char *t, temp, num[25], *fs = 0;
	int m, n, sepl;

	y = execute(a[0]);	/* source string */
	s = getsval(y);
	if (a[2] == 0)		/* fs string */
		fs = *FS;
	else if ((intptr_t) a[3] == STRING) {	/* split(str,arr,"string") */
		x = execute(a[2]);
		fs = getsval(x);
	} else if ((intptr_t) a[3] == REGEXPR)
		fs = (unsigned char*) "(regexpr)";	/* split(str,arr,/regexpr/) */
	else
		error(MM_ERROR, ":71:Illegal type of split()");
	next(sep, fs, sepl);
	ap = execute(a[1]);	/* array name */
	freesymtab(ap);
	dprintf( ("split: s=|%s|, a=%s, sep=|%s|\n", s, ap->nval, fs) );
	ap->tval &= ~STR;
	ap->tval |= ARR;
	ap->sval = (unsigned char *) makesymtab(NSYMTAB);

	n = 0;
	if ((*s != '\0' && sep != '\0' && fs[sepl] != '\0') ||
		((intptr_t) a[3] == REGEXPR)) {	/* reg expr */
		fa *pfa;
		if ((intptr_t) a[3] == REGEXPR) {	/* it's ready already */
			pfa = (fa *) a[2];
		} else {
			pfa = makedfa(fs, 1);
		}
		pfa->notbol = 0;
		if (nematch(pfa,s)) {
			pfa->notbol = REG_NOTBOL;
			do {
				n++;
				snprintf((char *)num, sizeof num, "%d", n);
				temp = *patbeg;
				*patbeg = '\0';
				setsymtab(num, s, 0.0, STR|CANBENUM, (Array *)ap->sval);
				*patbeg = temp;
				s = patbeg + patlen;
				if (*(patbeg+patlen-1) == 0 || *s == 0) {
					n++;
					snprintf((char *)num, sizeof num, "%d", n);
					setsymtab(num, "", 0.0, STR, (Array *) ap->sval);
					pfa->notbol = 0;
					goto spdone;
				}
			} while (nematch(pfa,s));
		}
		n++;
		snprintf((char *)num, sizeof num, "%d", n);
		setsymtab(num, s, 0.0, STR|CANBENUM, (Array *)ap->sval);
  spdone:
		pfa = NULL;
	} else if (sep == ' ') {
		for (n = 0; ; ) {
			while (*s == ' ' || *s == '\t' || *s == '\n')
				s++;
			if (*s == 0)
				break;
			n++;
			t = s;
			next(wc, s, m);
			do {
				s += m;
				next(wc, s, m);
			} while (wc!=' ' && wc!='\t' && wc!='\n' && wc!='\0');
			temp = *s;
			*s = '\0';
			snprintf((char *)num, sizeof num, "%d", n);
			setsymtab(num, t, 0.0, STR|CANBENUM, (Array *)ap->sval);
			*s = temp;
			if (*s != 0)
				s++;
		}
	} else if (*s != 0) {
		for (;;) {
			n++;
			t = s;
			while (next(wc, s, m),
					wc != sep && wc != '\n' && wc != '\0')
				s += m;
			temp = *s;
			*s = '\0';
			snprintf((char *)num, sizeof num, "%d", n);
			setsymtab(num, t, 0.0, STR|CANBENUM, (Array *)ap->sval);
			*s = temp;
			if (wc == '\0')
				break;
			s += m;
		}
	}
	tempfree(ap, "");
	tempfree(y, "");
	if (a[2] != 0 && (intptr_t) a[3] == STRING) {
		tempfree(x, "");
	}
	x = gettemp("");
	x->tval = NUM;
	x->fval = n;
	return(x);
}

Cell *condexpr(Node **a, int n)
{
	register Cell *x;

	x = execute(a[0]);
	if (istrue(x)) {
		tempfree(x, "");
		x = execute(a[1]);
	} else {
		tempfree(x, "");
		x = execute(a[2]);
	}
	return(x);
}

Cell *ifstat(Node **a, int n)
{
	register Cell *x;

	x = execute(a[0]);
	if (istrue(x)) {
		tempfree(x, "");
		x = execute(a[1]);
	} else if (a[2] != 0) {
		tempfree(x, "");
		x = execute(a[2]);
	}
	return(x);
}

Cell *whilestat(Node **a, int n)
{
	register Cell *x;

	in_loop++;
	for (;;) {
		x = execute(a[0]);
		if (!istrue(x)) {
			in_loop--;
			return(x);
		}
		tempfree(x, "");
		x = execute(a[1]);
		if (isbreak(x)) {
			x = true;
			in_loop--;
			return(x);
		}
		if (isnext(x) || isexit(x) || isret(x)) {
			in_loop--;
			return(x);
		}
		tempfree(x, "");
	}
	/*in_loop--;*/
}

Cell *dostat(Node **a, int n)
{
	register Cell *x;

	in_loop++;
	for (;;) {
		x = execute(a[0]);
		if (isbreak(x)) {
			in_loop--;
			return true;
		}
		if (isnext(x) || isexit(x) || isret(x)) {
			in_loop--;
			return(x);
		}
		tempfree(x, "");
		x = execute(a[1]);
		if (!istrue(x)) {
			in_loop--;
			return(x);
		}
		tempfree(x, "");
	}
	/*in_loop--;*/
}

Cell *forstat(Node **xa, int n)
{
	char	**a = (char **)xa;
	register Cell *x;

	in_loop++;
	x = execute(a[0]);
	tempfree(x, "");
	for (;;) {
		if (a[1]!=0) {
			x = execute(a[1]);
			if (!istrue(x)) {
				in_loop--;
				return(x);
			}
			else tempfree(x, "");
		}
		x = execute(a[3]);
		if (isbreak(x)) {		/* turn off break */
			in_loop--;
			return true;
		}
		if (isnext(x) || isexit(x) || isret(x)) {
			in_loop--;
			return(x);
		}
		tempfree(x, "");
		x = execute(a[2]);
		tempfree(x, "");
	}
	/*in_loop--;*/
}

Cell *instat(Node **a, int n)
{
	register Cell *x, *vp, *arrayp, *cp, *ncp;
	Array *tp;
	int i;

	in_loop++;
	vp = execute(a[0]);
	arrayp = execute(a[1]);
	if (!isarr(arrayp))
		error(MM_ERROR, notarray, arrayp->nval);
	tp = (Array *) arrayp->sval;
	tempfree(arrayp, "");
	for (i = 0; i < tp->size; i++) {	/* this routine knows too much */
		for (cp = tp->tab[i]; cp != NULL; cp = ncp) {
			setsval(vp, cp->nval);
			ncp = cp->cnext;
			x = execute(a[2]);
			if (isbreak(x)) {
				tempfree(vp, "");
				in_loop--;
				return true;
			}
			if (isnext(x) || isexit(x) || isret(x)) {
				tempfree(vp, "");
				in_loop--;
				return(x);
			}
			tempfree(x, "");
		}
	}
	in_loop--;
	return true;
}

static int closefile(const char *a);

Cell *bltin(Node **a, int n)
{
	static unsigned saved_srand = 1;
	register Cell *x, *y;
	Awkfloat u;
	register int t;
	unsigned char *p, *buf;
	Node *nextarg;

	t = (intptr_t) a[0];
	x = execute(a[1]);
	nextarg = a[1]->nnext;
	switch (t) {
	case FLENGTH:
		u = (Awkfloat) chrlen(getsval(x)); break;
	case FLOG:
		u = errcheck(log(getfval(x)), (unsigned char *)"log"); break;
	case FINT:
		modf(getfval(x), &u); break;
	case FEXP:
		u = errcheck(exp(getfval(x)), (unsigned char *)"exp"); break;
	case FSQRT:
		u = errcheck(sqrt(getfval(x)), (unsigned char *)"sqrt"); break;
	case FSIN:
		u = sin(getfval(x)); break;
	case FCOS:
		u = cos(getfval(x)); break;
	case FATAN:
		if (nextarg == 0) {
			error(MM_WARNING,
				":72:atan2 requires two arguments; returning 1.0");
			u = 1.0;
		} else {
			y = execute(a[1]->nnext);
			u = atan2(getfval(x), getfval(y));
			tempfree(y, "");
			nextarg = nextarg->nnext;
		}
		break;
	case FSYSTEM:
		fflush(stdout);		/* in case something is buffered already */
		u = (Awkfloat) system((char *)getsval(x)) / 256;   /* 256 is unix-dep */
		break;
	case FRAND:
		u = (Awkfloat) (rand() % 32767) / 32767.0;
		break;
	case FSRAND:
		u = saved_srand; /* return previous seed */
		if (x->tval & REC)	/* no argument provided */
			saved_srand = time(NULL);
		else
			saved_srand = getfval(x);
		srand((int) saved_srand);
		break;
	case FTOUPPER:
	case FTOLOWER:
		p = getsval(x);
		if ((buf = malloc(strlen((char *)p) + 1)) == 0)
			error(MM_ERROR, outofspace, "case-conversion");
		strcpy((char*)buf, (char*)getsval(x));
		if (t == FTOUPPER) {
			if (mb_cur_max == 1) {
				for (p = buf; *p; p++)
					if (islower(*p))
						*p = toupper(*p);
			} else
				caseconv(buf, towupper);
		} else {
			if (mb_cur_max == 1) {
				for (p = buf; *p; p++)
					if (isupper(*p))
						*p = tolower(*p);
			} else
				caseconv(buf, towlower);
		}
		tempfree(x, "");
		x = gettemp("");
		setsval(x, buf);
		free(buf);
		return x;
	case FCLOSE:
		u = (Awkfloat)closefile((char *)getsval(x));
		break;
	default:	/* can't happen */
		error(MM_ERROR, ":73:Illegal function type %d", t);
		break;
	}
	tempfree(x, "");
	x = gettemp("");
	setfval(x, u);
	if (nextarg != 0) {
		error(MM_WARNING, ":74:Function has too many arguments");
		for ( ; nextarg; nextarg = nextarg->nnext)
			execute(nextarg);
	}
	return(x);
}

Cell *print(Node **a, int n)
{
	register Node *x;
	register Cell *y;
	FILE *fp;

	if (a[1] == 0)
		fp = stdout;
	else
		fp = redirect((intptr_t)a[1], a[2]);
	for (x = a[0]; x != NULL; x = x->nnext) {
		y = execute(x);
		/*
		* ALMOST getsval().  POSIX.2 requires that
		* numeric values be converted according to OFMT
		* (not CONVFMT) for print.
		*/
		if (posix && (y->tval & (ARR|FLD|REC|STR)) == STR)
			fputs((char *)y->sval, fp);
		else if (!posix || (y->tval & (ARR|FLD|REC|NUM)) != NUM)
			fputs((char *)r_getsval(y), fp);
		else if ((long)y->fval == y->fval)
			fprintf(fp, "%ld", (long)y->fval);
		else
			fprintf(fp, (char *)*OFMT, y->fval);
		tempfree(y, "");
		if (x->nnext == NULL)
			fputs((char *)*ORS, fp);
		else
			fputs((char *)*OFS, fp);
	}
	if (a[1] != 0)
		fflush(fp);
	return(true);
}

/*ARGSUSED*/
Cell *nullproc(Node **a, int n) { return 0; }


static struct afile
{
	FILE	*fp;
	unsigned char	*fname;
	int	mode;	/* '|', 'a', 'w' */
} *files;
static	int	fopen_max;

FILE *redirect(int a, Node *b)
{
	FILE *fp;
	Cell *x;
	unsigned char *fname;

	x = execute(b);
	fname = getsval(x);
	fp = openfile(a, fname);
	if (fp == NULL)
		error(MM_ERROR, badopen, fname, strerror(errno));
	tempfree(x, "");
	return fp;
}

FILE *openfile(int a, unsigned char *s)
{
	register int i, m;
	register FILE *fp = 0;

	if (*s == '\0')
		error(MM_ERROR, ":75:Null file name in print or getline");
	for (i=0; i < fopen_max; i++)
		if (files[i].fname &&
			strcmp((char*)s, (char*)files[i].fname) == 0)
			if ((a == files[i].mode) || (a==APPEND && files[i].mode==GT))
				return files[i].fp;
	for (i=0; i < fopen_max; i++)
		if (files[i].fp == 0)
			break;
	if (i >= fopen_max) {
		if ((files = realloc(files, sizeof *files *
						(fopen_max = (i + 15))))==0)
			error(MM_ERROR, ":76:%s makes too many open files", s);
		memset(&files[i], 0, (fopen_max - i) * sizeof *files);
	}
	fflush(stdout);	/* force a semblance of order */
	m = a;
	if (a == GT) {
		fp = fopen((char *)s, "w");
	} else if (a == APPEND) {
		fp = fopen((char *)s, "a");
		m = GT;	/* so can mix > and >> */
	} else if (a == '|') {	/* output pipe */
		fp = popen((char *)s, "w");
	} else if (a == LE) {	/* input pipe */
		fp = popen((char *)s, "r");
	} else if (a == LT) {	/* getline <file */
		fp = strcmp((char *)s, "-") == 0 ? stdin : fopen((char *)s, "r");	/* "-" is stdin */
	} else	/* can't happen */
		error(MM_ERROR, ":77:Illegal redirection");
	if (fp != NULL) {
		files[i].fname = tostring(s);
		files[i].fp = fp;
		files[i].mode = m;
	}
	return fp;
}

static int endfile(struct afile *afp)
{
	int ret;

	if (ferror(afp->fp)) {
		clearerr(afp->fp);
		error(MM_WARNING, ioerror, afp->fname);
		errorflag = 1;
	}
	if (afp->mode == '|' || afp->mode == LE)
		ret = pclose(afp->fp);
	else
		ret = fclose(afp->fp);
	if (ret == EOF) {
		error(MM_WARNING, ":79:I/O error occurred while closing %s",
			afp->fname);
		errorflag = 1;
	}
	if (afp->fp != stdout) {
		xfree(afp->fname);
		afp->fp = 0;
	}
	return ret;
}

static int closefile(const char *a)
{
	int i, ret;

	ret = EOF;
	for (i = 0; i < fopen_max; i++)
		if (files[i].fname && strcmp(a, (char*)files[i].fname) == 0)
			ret = endfile(&files[i]);
	return(ret);
}

static void closeall(void)
{
	struct afile std;
	int i;

	for (i = 0; i < fopen_max; i++)
		if (files[i].fp)
			(void)endfile(&files[i]);
	std.fp = stdout;
	std.fname = (unsigned char *)"<stdout>";
	std.mode = GT;
	(void)endfile(&std);
}

Cell *sub(Node **a, int nnn)
{
	unsigned char *sptr, *pb, *q;
	register Cell *x, *y, *result;
	unsigned char *buf = NULL, *t;
	int bufsize = 0;
	fa *pfa;

	x = execute(a[3]);	/* target string */
	t = getsval(x);
	if (a[0] == 0)
		pfa = (fa *) a[1];	/* regular expression */
	else {
		y = execute(a[1]);
		pfa = makedfa(getsval(y), 1);
		tempfree(y, "");
	}
	y = execute(a[2]);	/* replacement string */
	result = false;
	if (pmatch(pfa, t)) {
		growbuf(&buf, &bufsize, CHUNK, NULL, "sub");
		pb = buf;
		sptr = t;
		while (sptr < patbeg) {
			*pb++ = *sptr++;
			if (pb >= &buf[bufsize])
				growbuf(&buf, &bufsize, CHUNK, &pb, "sub");
		}
		sptr = getsval(y);
		while (*sptr != 0) {
			if (*sptr == '\\' && *(sptr+1) == '&') {
				sptr++;		/* skip \, */
				*pb++ = *sptr++; /* add & */
			} else if (*sptr == '&') {
				sptr++;
				for (q = patbeg; q < patbeg+patlen; ) {
					*pb++ = *q++;
					growbuf(&buf, &bufsize, CHUNK,
							&pb, "sub");
				}
			} else
				*pb++ = *sptr++;
			if (pb >= &buf[bufsize])
				growbuf(&buf, &bufsize, CHUNK, &pb, "sub");
		}
		*pb = '\0';
		sptr = patbeg + patlen;
		if ((patlen == 0 && *patbeg) || (patlen && *(sptr-1)))
			while ((*pb++ = *sptr++)) {
				if (pb >= &buf[bufsize])
					growbuf(&buf, &bufsize, CHUNK, &pb,
							"sub");
			}
		setsval(x, buf);
		result = true;;
		free(buf);
	}
	tempfree(x, "");
	tempfree(y, "");
	return result;
}

Cell *gsub(Node **a, int nnn)
{
	register Cell *x, *y;
	unsigned char *rptr, *sptr, *t, *pb;
	unsigned char *buf = NULL;
	int bufsize = 0;
	register fa *pfa;
	int mflag, num;

	mflag = 0;	/* if mflag == 0, can replace empty string */
	num = 0;
	x = execute(a[3]);	/* target string */
	t = getsval(x);
	if (a[0] == 0)
		pfa = (fa *) a[1];	/* regular expression */
	else {
		y = execute(a[1]);
		pfa = makedfa(getsval(y), 1);
		tempfree(y, "");
	}
	y = execute(a[2]);	/* replacement string */
	pfa->notbol = 0;
	if (pmatch(pfa, t)) {
		pfa->notbol = REG_NOTBOL;
		growbuf(&buf, &bufsize, CHUNK, NULL, "gsub");
		pb = buf;
		rptr = getsval(y);
		do {
			/*
			unsigned char *p;
			int i;
			printf("target string: %s, *patbeg = %o, patlen = %d\n",
				t, *patbeg, patlen);
			printf("	match found: ");
			p=patbeg;
			for (i=0; i<patlen; i++)
				printf("%c", *p++);
			printf("\n");
			*/
			if (patlen == 0 && *patbeg != 0) {	/* matched empty string */
				if (mflag == 0) {	/* can replace empty */
					num++;
					sptr = rptr;
					while (*sptr != 0) {
						if (*sptr == '\\' && *(sptr+1) == '&') {
							sptr++;
							*pb++ = *sptr++;
						} else if (*sptr == '&') {
							unsigned char *q;
							sptr++;
							for (q = patbeg; q < patbeg+patlen; ) {
								*pb++ = *q++;
								if (pb >= &buf[bufsize])
									growbuf(&buf,
										&bufsize, CHUNK,
										&pb, "gsub");
							}
						} else
							*pb++ = *sptr++;
						if (pb >= &buf[bufsize])
							growbuf(&buf,
								&bufsize, CHUNK,
								&pb, "gsub");
					}
				}
				if (*t == 0)	/* at end */
					goto done;
				*pb++ = *t++;
				mflag = 0;
			}
			else {	/* matched nonempty string */
				num++;
				sptr = t;
				while (sptr < patbeg) {
					if (pb >= &buf[bufsize])
						growbuf(&buf, &bufsize, CHUNK,
							&pb, "gsub");
					*pb++ = *sptr++;
				}
				sptr = rptr;
				while (*sptr != 0) {
					if (*sptr == '\\' && *(sptr+1) == '&') {
						sptr++;
						*pb++ = *sptr++;
					} else if (*sptr == '&') {
						unsigned char *q;
						sptr++;
						for (q = patbeg; q < patbeg+patlen; ) {
							*pb++ = *q++;
							if (pb >= &buf[bufsize])
								growbuf(&buf,
								&bufsize, CHUNK,
								&pb, "gsub");
						}
					} else
						*pb++ = *sptr++;
					if (pb >= &buf[bufsize])
						growbuf(&buf, &bufsize, CHUNK,
							&pb, "gsub");
				}
				t = patbeg + patlen;
				if ((*(t-1) == 0) || (*t == 0))
					goto done;
				mflag = 1;
			}
		} while (pmatch(pfa,t));
		sptr = t;
		while ((*pb++ = *sptr++)) {
			if (pb >= &buf[bufsize])
				growbuf(&buf, &bufsize, CHUNK, &pb, "gsub");
		}
	done:	*pb = '\0';
		setsval(x, buf);
		pfa->notbol = 0;
		free(buf);
	}
	tempfree(x, "");
	tempfree(y, "");
	x = gettemp("");
	x->tval = NUM;
	x->fval = num;
	return(x);
}
#include <stdarg.h> /* MR ul92-34309a2 */
static int
growsprintf(unsigned char **whole, unsigned char **target, int *size,
		const char *fmt, ...)
{
	va_list ap;
	int ret;
	size_t diff = 0, mx;

	if (*size == 0) {
		if ((*whole = malloc(*size = CHUNK)) == NULL)
			goto oflo;
		*target = *whole;
	}
	diff = *target - *whole;
again:	va_start(ap, fmt);

	mx = *size - diff - 8;
	ret = vsnprintf((char *)*target, mx, fmt, ap);
	va_end(ap);

	if (ret < 0 || ret >= mx) {
		if (ret < 0) {
			char	dummy[2];
			va_start(ap, fmt);
			ret = vsnprintf(dummy, sizeof dummy, fmt, ap);
			va_end(ap);
			if (ret < 0)
				goto oflo;
		}
		if ((*whole = realloc(*whole, *size = ret + 1 + diff + 8)) == 0)
		oflo:	error(MM_ERROR,
			":103:Formatted result would be too long: %.20s ...",
			fmt);
		*target = &(*whole)[diff];
		goto again;
	}
	
	while (**target)	/* NUL characters might have been printed; */
		(*target)++;	/* don't skip past them. */
	return ret;
}

int chrlen(const unsigned char *s)
{
	wchar_t wc;
	int m = 0, n;

	while (next(wc, s, n), wc != '\0') {
		s += n;
		m++;
	}
	return m;
}

int chrdist(const unsigned char *s, const unsigned char *end)
{
	wchar_t wc;
	int m = 0, n;

	while (next(wc, s, n), s <= end) {
		s += n;
		m++;
	}
	return m;
}

static void caseconv(unsigned char *s, wint_t (*conv)(wint_t))
{
	unsigned char *t = s;
	wchar_t wc;
	int len, nlen;

	while (*s) {
		len = mbtowc(&wc, (char *)s, mb_cur_max);
		if (len < 0)
			*t++ = *s++;
		else {
			wc = conv(wc);
			if ((nlen = wctomb((char *)t, wc)) <= len) {
				t += nlen, s += len;
			} else
				*t++ = *s++;
		}
	}
	*t = '\0';
}

static void growbuf(unsigned char **buf, int *bufsize, int incr,
		unsigned char **ptr, const char *fn)
{
	unsigned char *op;

	op = *buf;
	if ((*buf = realloc(*buf, *bufsize += incr)) == NULL)
		error(MM_ERROR, outofspace, fn ? fn : "");
	if (ptr && *ptr)
		*ptr = &(*buf)[*ptr - op];
}
