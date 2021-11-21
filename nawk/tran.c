/*
   Changes by Gunnar Ritter, Freiburg i. Br., Germany, December 2002.
  
   Sccsid @(#)tran.c	1.16 (gritter) 2/4/05>
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
/*		copyright	"%c%" 	*/

/*	from unixsrc:usr/src/common/cmd/awk/tran.c /main/uw7_nj/1	*/
/*	from RCS Header: tran.c 1.2 91/06/25 	*/


#define	DEBUG
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "awk.h"
#include "y.tab.h"
#include <pfmt.h>

#undef	RS

#define	FULLTAB	2	/* rehash when table gets this x full */
#define	GROWTAB 4	/* grow table by this factor */

Array	*symtab;	/* main symbol table */

unsigned char	**FS;		/* initial field sep */
unsigned char	**RS;		/* initial record sep */
unsigned char	**OFS;		/* output field sep */
unsigned char	**ORS;		/* output record sep */
unsigned char	**OFMT;		/* output format for numbers */
unsigned char	**CONVFMT;	/* generic format for numbers->strings */
Awkfloat *NF;		/* number of fields in current record */
Awkfloat *NR;		/* number of current record */
Awkfloat *FNR;		/* number of current record in current file */
unsigned char	**FILENAME;	/* current filename argument */
Awkfloat *ARGC;		/* number of arguments from command line */
unsigned char	**SUBSEP;	/* subscript separator for a[i,j,k]; default \034 */
Awkfloat *RSTART;	/* start of re matched with ~; origin 1 (!) */
Awkfloat *RLENGTH;	/* length of same */

Cell	*recloc;	/* location of record */
Cell	*nrloc;		/* NR */
Cell	*nfloc;		/* NF */
Cell	*fsloc;		/* FS */
Cell	*fnrloc;	/* FNR */
Array	*ARGVtab;	/* symbol table containing ARGV[...] */
Array	*ENVtab;	/* symbol table containing ENVIRON[...] */
Cell	*rstartloc;	/* RSTART */
Cell	*rlengthloc;	/* RLENGTH */
Cell	*symtabloc;	/* SYMTAB */

Cell	*nullloc;
Node	*nullnode;	/* zero&null, converted into a node for comparisons */

extern Cell **fldtab;
static int hash(register unsigned char *s, int n);
static void rehash(Array *tp);

static	const	char
	assigntovid[] = ":80",
	assigntov[] = "assign to";

const char
	readvofid[] = ":81",
	readvof[] = "read value of",
	outofspace[] = ":82:Out of space in %s",
	nlstring[] = ":83:Newline in string %.10s ...";

void syminit(void)
{
	symtab = makesymtab(NSYMTAB);
	setsymtab("0", "0", 0.0, NUM|STR|CON|DONTFREE, symtab);
	/* this is used for if(x)... tests: */
	nullloc = setsymtab("$zero&null", "", 0.0, NUM|STR|CON|DONTFREE, symtab);
	nullnode = valtonode(nullloc, CCON);
	/* recloc = setsymtab("$0", record, 0.0, REC|STR|DONTFREE, symtab); */
	recloc = fldtab[0];
	fsloc = setsymtab("FS", " ", 0.0, STR|DONTFREE, symtab);
	FS = &fsloc->sval;
	RS = &setsymtab("RS", "\n", 0.0, STR|DONTFREE, symtab)->sval;
	OFS = &setsymtab("OFS", " ", 0.0, STR|DONTFREE, symtab)->sval;
	ORS = &setsymtab("ORS", "\n", 0.0, STR|DONTFREE, symtab)->sval;
	OFMT = &setsymtab("OFMT", "%.6g", 0.0, STR|DONTFREE, symtab)->sval;
	CONVFMT = &setsymtab("CONVFMT", "%.6g", 0.0, STR|DONTFREE, symtab)->sval;
	FILENAME = &setsymtab("FILENAME", "-", 0.0, STR|DONTFREE, symtab)->sval;
	nfloc = setsymtab("NF", "", 0.0, NUM, symtab);
	NF = &nfloc->fval;
	nrloc = setsymtab("NR", "", 0.0, NUM, symtab);
	NR = &nrloc->fval;
	fnrloc = setsymtab("FNR", "", 0.0, NUM, symtab);
	FNR = &fnrloc->fval;
	SUBSEP = &setsymtab("SUBSEP", "\034", 0.0, STR|DONTFREE, symtab)->sval;
	rstartloc = setsymtab("RSTART", "", 0.0, NUM, symtab);
	RSTART = &rstartloc->fval;
	rlengthloc = setsymtab("RLENGTH", "", 0.0, NUM, symtab);
	RLENGTH = &rlengthloc->fval;
	symtabloc = setsymtab("SYMTAB", "", 0.0, ARR, symtab);
	symtabloc->sval = (unsigned char *) symtab;
}

void arginit(int ac, unsigned char **av)
{
	Cell *cp;
	int i;
	unsigned char temp[25];

	for (i = 1; i < ac; i++)	/* first make FILENAME first real argument */
		if (!isclvar(av[i])) {
			setsval(lookup("FILENAME", symtab), av[i]);
			break;
		}
	ARGC = &setsymtab("ARGC", "", (Awkfloat) ac, NUM, symtab)->fval;
	cp = setsymtab("ARGV", "", 0.0, ARR, symtab);
	ARGVtab = makesymtab(NSYMTAB);	/* could be (int) ARGC as well */
	cp->sval = (unsigned char *) ARGVtab;
	for (i = 0; i < ac; i++) {
		snprintf((char *)temp, sizeof temp, "%d", i);
		setsymtab(temp, *av, 0.0, STR|CANBENUM, ARGVtab);
		av++;
	}
}

void envinit(unsigned char **envp)
{
	Cell *cp;
	unsigned char *p;

	cp = setsymtab("ENVIRON", "", 0.0, ARR, symtab);
	ENVtab = makesymtab(NSYMTAB);
	cp->sval = (unsigned char *) ENVtab;
	for ( ; *envp; envp++) {
		if ((p = (unsigned char *) strchr((char *) *envp, '=')) == NULL)	/* index() on bsd */
			continue;
		*p++ = 0;	/* split into two strings at = */
		setsymtab(*envp, p, 0.0, STR|CANBENUM, ENVtab);
		p[-1] = '=';	/* restore in case env is passed down to a shell */
	}
}

Array *makesymtab(int n)
{
	Array *ap;
	Cell **tp;

	ap = (Array *) malloc(sizeof(Array));
	tp = (Cell **) calloc(n, sizeof(Cell *));
	if (ap == NULL || tp == NULL)
		error(MM_ERROR, outofspace, "makesymtab");
	ap->nelem = 0;
	ap->size = n;
	ap->tab = tp;
	return(ap);
}

void freesymtab(Cell *ap)	/* free symbol table */
{
	Cell *cp, *temp;
	Array *tp;
	int i;

	if (!isarr(ap))
		return;
	tp = (Array *) ap->sval;
	if (tp == NULL)
		return;
	for (i = 0; i < tp->size; i++) {
		for (cp = tp->tab[i]; cp != NULL; cp = temp) {
			xfree(cp->nval);
			if (freeable(cp))
				xfree(cp->sval);
			temp = cp->cnext;	/* avoids freeing then using */
			free(cp);
		}
	}
	free(tp->tab);
	free(tp);
}

void freeelem(Cell *ap, unsigned char *s)
	/* free elem s from ap (i.e., ap["s"] */
{
	Array *tp;
	Cell *p, *prev = NULL;
	int h;
	
	tp = (Array *) ap->sval;
	h = hash(s, tp->size);
	for (p = tp->tab[h]; p != NULL; prev = p, p = p->cnext)
		if (strcmp((char *) s, (char *) p->nval) == 0) {
			if (prev == NULL)	/* 1st one */
				tp->tab[h] = p->cnext;
			else			/* middle somewhere */
				prev->cnext = p->cnext;
			if (freeable(p))
				xfree(p->sval);
			free(p->nval);
			free(p);
			tp->nelem--;
			return;
		}
}

Cell *ssetsymtab(unsigned char *n, unsigned char *s, Awkfloat f,
		unsigned t, Array *tp)
{
	register int h;
	register Cell *p;

	if (n != NULL && (p = lookup(n, tp)) != NULL) {
		dprintf( ("setsymtab found %lo: n=%s", (long)p, p->nval) );
		dprintf( (" s=\"%s\" f=%g t=%o\n", p->sval? p->sval : tostring(""), p->fval, p->tval) );
		return(p);
	}
	p = (Cell *) malloc(sizeof(Cell));
	if (p == NULL)
		error(MM_ERROR, ":84:Symbol table overflow at %s", n);
	p->nval = tostring(n);
	p->sval = s ? tostring(s) : tostring("");
	p->fval = f;
	p->tval = t & ~CANBENUM;
	p->csub = 0;
	if (t & CANBENUM)
		(void)is2number(0, p);
	tp->nelem++;
	if (tp->nelem > FULLTAB * tp->size)
		rehash(tp);
	h = hash(n, tp->size);
	p->cnext = tp->tab[h];
	tp->tab[h] = p;
	dprintf( ("setsymtab set %lo: n=%s", (long)p, p->nval) );
	dprintf( (" s=\"%s\" f=%g t=%o\n", p->sval? p->sval : tostring(""), p->fval, p->tval) );
	return(p);
}

static int hash(register unsigned char *s, int n)
	/* form hash value for string s */
{
	register unsigned hashval;

	for (hashval = 0; *s != '\0'; s++)
		hashval = (*s + 31 * hashval);
	return hashval % n;
}

static void rehash(Array *tp)	/* rehash items in small table into big one */
{
	int i, nh, nsz;
	Cell *cp, *op, **np;

	nsz = GROWTAB * tp->size;
	np = (Cell **) calloc(nsz, sizeof(Cell *));
	if (np == NULL)
		error(MM_ERROR, outofspace, "rehash");
	for (i = 0; i < tp->size; i++) {
		for (cp = tp->tab[i]; cp; cp = op) {
			op = cp->cnext;
			nh = hash(cp->nval, nsz);
			cp->cnext = np[nh];
			np[nh] = cp;
		}
	}
	free(tp->tab);
	tp->tab = np;
	tp->size = nsz;
}

Cell *slookup(register unsigned char *s, Array *tp)	/* look for s in tp */
{
	register Cell *p, *prev = NULL;
	int h;

	h = hash(s, tp->size);
	for (p = tp->tab[h]; p != NULL; prev = p, p = p->cnext)
		if (strcmp((char *) s, (char *) p->nval) == 0)
			return(p);	/* found it */
	return(NULL);			/* not found */
}

Awkfloat setfval(register Cell *vp, Awkfloat f)
{
	if ((vp->tval & (NUM | STR)) == 0) 
		funnyvar(vp, (char *)gettxt(assigntovid, assigntov));
	if (vp->tval & FLD) {
		int n;
		donerec = 0;	/* mark $0 invalid */
		for (n = 0; vp != fldtab[n]; n++);
		if (n > *NF)
			newfld(n);
		dprintf( ("setting field %d to %g\n", n, f) );
	} else if (vp->tval & REC) {
		donefld = 0;	/* mark $1... invalid */
		donerec = 1;
	}
	vp->tval &= ~STR;	/* mark string invalid */
	vp->tval |= NUM;	/* mark number ok */
	dprintf( ("setfval %lo: %s = %g, t=%o\n", (long)vp, vp->nval ? vp->nval : tostring(""), f, vp->tval) );
	return vp->fval = f;
}

void funnyvar(Cell *vp, char *rw)
{
	if (vp->tval & ARR)
		error(MM_ERROR, ":85:Cannot %s %s; it's an array name.",
			rw, vp->nval);
	if (vp->tval & FCN)
		error(MM_ERROR, ":86:Cannot %s %s; it's a function.",
			rw, vp->nval);
	error(MM_ERROR, ":87:Funny variable %o: n=%s s=\"%s\" f=%g t=%o",
		vp, vp->nval, vp->sval, vp->fval, vp->tval);
}

unsigned char *setsval(register Cell *vp, unsigned char *s)
{
	if ((vp->tval & (NUM | STR)) == 0)
		funnyvar(vp, (char *)gettxt(assigntovid, assigntov));
	if (vp->tval & FLD) {
		int n;
		donerec = 0;	/* mark $0 invalid */
		for (n = 0; vp != fldtab[n]; n++);
		if (n > *NF)
			newfld(n);
		dprintf( ("setting field %d to %s\n", n, s) );
	} else if (vp->tval & REC) {
		donefld = 0;	/* mark $1... invalid */
		donerec = 1;
	} else if (vp == fsloc && donefld == 0) {
		/*
		* Because POSIX.2 requires that awk act as if it always
		* splits the current input line immediately after reading,
		* we force it to be split into fields just before a change
		* to FS if we haven't needed to do so yet.
		*/
		fldbld();
	}
	vp->tval &= ~NUM;
	vp->tval |= STR;
	s = tostring(s); /* moved to here since "s" can be "vp->sval" */
	if (freeable(vp))
		xfree(vp->sval);
	if (vp->tval & REC) {
		/*
		 * Make sure that recsize is large enough to build
		 * fields afterwards.
		 */
		unsigned char *os = s;

		s = makerec(s, strlen((char *)s) + 1);
		free(os);
	} else
		vp->tval &= ~DONTFREE;
	dprintf( ("setsval %lo: %s = \"%s\", t=%o\n", (long)vp, vp->nval, s, vp->tval) );
	return(vp->sval = s);
}

Awkfloat r_getfval(register Cell *vp)
{
	/* if (vp->tval & ARR)
		ERROR "Illegal reference to array %s", vp->nval FATAL;
		return 0.0; */
	if ((vp->tval & (NUM | STR)) == 0)
		funnyvar(vp, (char *)gettxt(readvofid, readvof));
	if ((vp->tval & FLD) && donefld == 0)
		fldbld();
	else if ((vp->tval & REC) && donerec == 0)
		recbld();
	if (!isnum(vp)) {	/* not marked as a number */
		vp->fval = awk_atof((char *)vp->sval);	/* best guess */
		if (is2number(vp->sval, 0) && !(vp->tval&CON))
			vp->tval |= NUM;	/* make NUM only sparingly */
	}
	dprintf( ("getfval %lo: %s = %g, t=%o\n", (long)vp, vp->nval, vp->fval, vp->tval) );
	return(vp->fval);
}

unsigned char *r_getsval(register Cell *vp)
{
	unsigned char s[100];

	/* if (vp->tval & ARR)
		ERROR "Illegal reference to array %s",
			vp->nval FATAL;
		return ""; */
	if ((vp->tval & (NUM | STR)) == 0)
		funnyvar(vp, (char *)gettxt(readvofid, readvof));
	if ((vp->tval & FLD) && donefld == 0)
		fldbld();
	else if ((vp->tval & REC) && donerec == 0)
		recbld();
	if ((vp->tval & STR) == 0) {
		if (!(vp->tval&DONTFREE))
			xfree(vp->sval);
		if ((long)vp->fval == vp->fval) {
			snprintf((char *)s, sizeof s, "%ld", (long)vp->fval);
			vp->tval |= STR;
		} else {
			snprintf((char *)s, sizeof s,
					(char *)(posix ? *CONVFMT : *OFMT),
					vp->fval);
			/*
			* In case CONVFMT is changed by the program,
			* we leave the string value uncached for non-
			* integer numeric constants.  Ugh.
			*/
			if (!(vp->tval & CON))
				vp->tval |= STR;
		}
		vp->sval = tostring(s);
		vp->tval &= ~DONTFREE;
	}
	dprintf( ("getsval %lo: %s = \"%s\", t=%o\n", (long)vp, vp->nval ? vp->nval : tostring(""), vp->sval ? vp->sval : tostring(""), vp->tval) );
	return(vp->sval);
}

unsigned char *stostring(register const unsigned char *s)
{
	register unsigned char *p;

	p = malloc(strlen((char *) s)+1);
	if (p == NULL)
		error(MM_ERROR, ":88:Out of space in tostring on %s", s);
	strcpy((char *) p, (char *) s);
	return(p);
}

unsigned char *qstring(unsigned char *s, int delim)
	/* collect string up to delim */
{
	unsigned char *q;
	int c, n;

	for (q = cbuf; (c = *s) != delim; s++) {
		if (q >= cbuf + CBUFLEN - 1)
			vyyerror(":89:String %.10s ... too long", cbuf);
		else if (c == '\n')
			vyyerror(nlstring, cbuf);
		else if (c != '\\')
			*q++ = c;
		else	/* \something */	
			switch (c = *++s) {
			case '\\':	*q++ = '\\'; break;
			case 'n':	*q++ = '\n'; break;
			case 't':	*q++ = '\t'; break;
			case 'b':	*q++ = '\b'; break;
			case 'f':	*q++ = '\f'; break;
			case 'r':	*q++ = '\r'; break;
			default:
				if (!isdigit(c)) {
					*q++ = c;
					break;
				}
				n = c - '0';
				if (isdigit(s[1])) {
					n = 8 * n + *++s - '0';
					if (isdigit(s[1]))
						n = 8 * n + *++s - '0';
				}
				*q++ = n;
				break;
			}
	}
	*q = '\0';
	return cbuf;
}
