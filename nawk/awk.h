/*
   Changes by Gunnar Ritter, Freiburg i. Br., Germany, December 2002.
  
   Sccsid @(#)awk.h	1.23 (gritter) 12/25/04>
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

/*	from unixsrc:usr/src/common/cmd/awk/awk.h /main/uw7_nj/1	*/
/*	from RCS Header: awk.h 1.2 91/06/25 	*/

typedef double	Awkfloat;

#define	xfree(a)	{ if ((a) != NULL) { free(a); a = NULL; } }
#define MAXLABEL 25

extern const char	version[];

extern	char	errbuf[200];
#define	ERROR	snprintf(errbuf, sizeof errbuf,
#define	FATAL	), error(1, errbuf)
#define	WARNING	), error(0, errbuf)
#define	SYNTAX	), yyerror(errbuf)

extern int	compile_time;	/* 1 if compiling, 0 if running */

extern int	posix;		/* if POSIX behavior is desired */

/*
 * This is done to prevent redefinition of our own definitions for FS with
 * those defined in the system's header files. Same of RS (on HP-UX/PA-RISC).
 */
#undef FS
#undef RS

extern unsigned char	**FS;
extern unsigned char	**RS;
extern unsigned char	**ORS;
extern unsigned char	**OFS;
extern unsigned char	**OFMT;
extern unsigned char	**CONVFMT;
extern Awkfloat *NR;
extern Awkfloat *FNR;
extern Awkfloat *NF;
extern unsigned char	**FILENAME;
extern unsigned char	**SUBSEP;
extern Awkfloat *RSTART;
extern Awkfloat *RLENGTH;

#define	CHUNK	512	/* record and string increment */

extern unsigned char	*record;
extern int	recsize;
extern int	dbg;
extern int	lineno;
extern int	errorflag;
extern int	donefld;	/* 1 if record broken into fields */
extern int	donerec;	/* 1 if record is valid (no fld has changed */

#define	CBUFLEN	5120
extern unsigned char	cbuf[CBUFLEN];	/* miscellaneous character collection */

extern	unsigned char	*patbeg;	/* beginning of pattern matched */
extern	int	patlen;		/* length.  set in b.c */

extern	int	mb_cur_max;	/* MB_CUR_MAX, for acceleration purposes */

extern const char outofspace[];	/* message */

/* Cell:  all information about a variable or constant */

typedef struct Cell {
	unsigned char	ctype;		/* OCELL, OBOOL, OJUMP, etc. */
	unsigned char	csub;		/* CCON, CTEMP, CFLD, etc. */
	unsigned char	*nval;		/* name, for variables only */
	unsigned char	*sval;		/* string value */
	Awkfloat fval;		/* value as number */
	unsigned tval;		/* type info: STR|NUM|ARR|FCN|FLD|CON|DONTFREE */
	struct Cell *cnext;	/* ptr to next if chained */
} Cell;

typedef struct {		/* symbol table array */
	int	nelem;		/* elements in table right now */
	int	size;		/* size of tab */
	Cell	**tab;		/* hash table pointers */
} Array;

#define	NSYMTAB	50	/* initial size of a symbol table */
extern Array	*symtab, *makesymtab(int);
#define	setsymtab(n, s, f, t, tp)	ssetsymtab((unsigned char *)n, \
						(unsigned char *)s, \
						f, t, tp)
extern Cell	*ssetsymtab(unsigned char *, unsigned char *, Awkfloat,
			unsigned, Array *);
#define	lookup(s, tp)	slookup((unsigned char *)s, tp)
extern Cell	*slookup(unsigned char *, Array *);

extern Cell	*recloc;	/* location of input record */
extern Cell	*nrloc;		/* NR */
extern Cell	*fnrloc;	/* FNR */
extern Cell	*fsloc;		/* FS */
extern Cell	*nfloc;		/* NF */
extern Cell	*rstartloc;	/* RSTART */
extern Cell	*rlengthloc;	/* RLENGTH */

/* Cell.tval values: */
#define	NUM	01	/* number value is valid */
#define	STR	02	/* string value is valid */
#define DONTFREE 04	/* string space is not freeable */
#define	CON	010	/* this is a constant */
#define	ARR	020	/* this is an array */
#define	FCN	040	/* this is a function name */
#define FLD	0100	/* this is a field $1, $2, ... */
#define	REC	0200	/* this is $0 */
#define CANBENUM 0400	/* tells setsymtab() to try for NUM, too */

#define freeable(p)	(!((p)->tval & DONTFREE))

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef	__GLIBC__
#ifdef	_IO_getc_unlocked
#undef	getc
#define	getc(c)	_IO_getc_unlocked(c)
#endif	/* _IO_getc_unlocked */
#endif	/* __GLIBC__ */

#define	getline	xxgetline	/* avoid glibc _GNU_SOURCE collision */

#define	DEBUG
#ifdef	DEBUG
			/* uses have to be doubly parenthesized */
#	define	dprintf(x)	if (dbg) printf x
#else
#	define	dprintf(x)
#endif

#ifndef	IN_MAKETAB
#include <wchar.h>

/*
 * Get next character from string s and store it in wc; n is set to
 * the length of the corresponding byte sequence.
 */
#define	next(wc, s, n) (mb_cur_max > 1 && *(s) & 0200 ? \
			((n) = mbtowc(&(wc), (char *)(s), mb_cur_max), \
			 (n) = ((n) > 0 ? (n) : (n) < 0 ? (wc=WEOF, 1) : 1)) :\
		((wc) = *(s), (n) = 1))
#endif	/* !IN_MAKETAB */

/* function types */
#define	FLENGTH	1
#define	FSQRT	2
#define	FEXP	3
#define	FLOG	4
#define	FINT	5
#define	FSYSTEM	6
#define	FRAND	7
#define	FSRAND	8
#define	FSIN	9
#define	FCOS	10
#define	FATAN	11
#define	FTOUPPER 12
#define	FTOLOWER 13
#define FCLOSE	14

/* Node:  parse tree is made of nodes, with Cell's at bottom */

typedef struct Node {
	int	ntype;
	struct	Node *nnext;
	int	lineno;
	int	nobj;
	struct Node *narg[1];	/* variable: actual size set by calling malloc */
} Node;

#define	NIL	((Node *) 0)

extern Node	*winner;
extern Node	*nullstat;
extern Node	*nullnode;

/* ctypes */
#define OCELL	1
#define OBOOL	2
#define OJUMP	3

/* Cell subtypes: csub */
#define	CFREE	7
#define CCOPY	6
#define CCON	5
#define CTEMP	4
#define CNAME	3 
#define CVAR	2
#define CFLD	1

/* bool subtypes */
#define BTRUE	11
#define BFALSE	12

/* jump subtypes */
#define JEXIT	21
#define JNEXT	22
#define	JBREAK	23
#define	JCONT	24
#define	JRET	25

/* node types */
#define NVALUE	1
#define NSTAT	2
#define NEXPR	3
#define	NFIELD	4

extern	Cell	*(*proctab[])(Node **, int);
extern	int	pairstack[];
extern	long	paircnt;

#define notlegal(n)	(n <= FIRSTTOKEN || n >= LASTTOKEN || proctab[n-FIRSTTOKEN] == nullproc)
#define isvalue(n)	((n)->ntype == NVALUE)
#define isexpr(n)	((n)->ntype == NEXPR)
#define isjump(n)	((n)->ctype == OJUMP)
#define isexit(n)	((n)->csub == JEXIT)
#define	isbreak(n)	((n)->csub == JBREAK)
#define	iscont(n)	((n)->csub == JCONT)
#define	isnext(n)	((n)->csub == JNEXT)
#define	isret(n)	((n)->csub == JRET)
#define isstr(n)	((n)->tval & STR)
#define isnum(n)	((n)->tval & NUM)
#define isarr(n)	((n)->tval & ARR)
#define isfunc(n)	((n)->tval & FCN)
#define istrue(n)	((n)->csub == BTRUE)
#define istemp(n)	((n)->csub == CTEMP)

#include <regex.h>

typedef struct fa {
	unsigned char	*restr;
	int	use;
	int	notbol;
	regex_t	re;
} fa;

/* awk.g.c */
extern int	yywrap(void);
extern int	yyparse(void);
/* awk.lx.c */
extern int	yylex(void);
extern void	startreg(void);
extern int	awk_input(void);
/* b.c */
extern fa	*makedfa(unsigned char *, int);
extern int	match(void *, unsigned char *);
extern int	pmatch(void *, unsigned char *);
extern int	nematch(void *, unsigned char *);
/* lib.c */
extern void	fldinit(void);
extern void	initgetrec(void);
extern int	getrec(unsigned char **, int *);
extern int	readrec(unsigned char **, int *, FILE *);
extern unsigned char	*getargv(int);
extern void	setclvar(unsigned char *);
extern void	fldbld(void);
extern void	newfld(int);
extern void	recbld(void);
extern Cell	*fieldadr(int);
extern void	vyyerror(const char *, ...);
extern void	yyerror(char *);
extern void	fpecatch(int);
extern void	bracecheck(void);
extern void	error(int, const char *, ...);
extern void	bclass(int);
extern double	errcheck(double, unsigned char *);
extern void	PUTS(unsigned char *);
extern int	isclvar(unsigned char *);
extern int	is2number(unsigned char *, Cell *);
extern double	awk_atof(const char *);
extern unsigned char	*makerec(const unsigned char *, int);
/* main.c */
extern int	pgetc(void);
/* parse.c */
extern Node	*nodealloc(int);
extern Node	*exptostat(Node *);
extern Node	*node1(int, Node *);
extern Node	*node2(int, Node *, Node *);
extern Node	*node3(int, Node *, Node *, Node *);
extern Node	*node4(int, Node *, Node *, Node *, Node *);
extern Node	*stat3(int, Node *, Node *, Node *);
extern Node	*op2(int, Node *, Node *);
extern Node	*op1(int, Node *);
extern Node	*stat1(int, Node *);
extern Node	*op3(int, Node *, Node *, Node *);
extern Node	*op4(int, Node *, Node *, Node *, Node *);
extern Node	*stat2(int, Node *, Node *);
extern Node	*stat4(int, Node *, Node *, Node *, Node *);
extern Node	*valtonode(Cell *, int);
extern Node	*rectonode(void);
extern Node	*makearr(Node *);
extern Node	*pa2stat(Node *, Node *, Node *);
extern Node	*linkum(Node *, Node *);
extern void	defn(Cell *, Node *, Node *);
extern int	isarg(const char *);
/* proctab.c */
extern unsigned char	*tokname(int);
/* run.c */
extern int	run(Node *);
extern Cell	*r_execute(Node *);
extern Cell	*program(Node **, int);
extern Cell	*call(Node **, int);
extern Cell	*copycell(Cell *);
extern Cell	*arg(Node **, int);
extern Cell	*jump(Node **, int);
extern Cell	*getline(Node **, int);
extern Cell	*getnf(Node **, int);
extern Cell	*array(Node **, int);
extern Cell	*delete(Node **, int);
extern Cell	*intest(Node **, int);
extern Cell	*matchop(Node **, int);
extern Cell	*boolop(Node **, int);
extern Cell	*relop(Node **, int);
extern Cell	*gettemp(const char *);
extern Cell	*indirect(Node **, int);
extern Cell	*substr(Node **, int);
extern Cell	*sindex(Node **, int);
extern int	format(unsigned char **, int *, const unsigned char *, Node *);
extern Cell	*awsprintf(Node **, int);
extern Cell	*aprintf(Node **, int);
extern Cell	*arith(Node **, int);
extern double	ipow(double, int);
extern Cell	*incrdecr(Node **, int);
extern Cell	*assign(Node **, int);
extern Cell	*cat(Node **, int);
extern Cell	*pastat(Node **, int);
extern Cell	*dopa2(Node **, int);
extern Cell	*split(Node **, int);
extern Cell	*condexpr(Node **, int);
extern Cell	*ifstat(Node **, int);
extern Cell	*whilestat(Node **, int);
extern Cell	*dostat(Node **, int);
extern Cell	*forstat(Node **, int);
extern Cell	*instat(Node **, int);
extern Cell	*bltin(Node **, int);
extern Cell	*print(Node **, int);
extern Cell	*nullproc(Node **, int);
extern FILE	*redirect(int, Node *);
extern FILE	*openfile(int, unsigned char *);
extern Cell	*sub(Node **, int);
extern Cell	*gsub(Node **, int);
extern int	chrlen(const unsigned char *);
extern int	chrdist(const unsigned char *, const unsigned char *);
/* tran.c */
extern void	syminit(void);
extern void	arginit(int, unsigned char **);
extern void	envinit(unsigned char **);
extern Array	*makesymtab(int);
extern void	freesymtab(Cell *);
extern void	freeelem(Cell *, unsigned char *);
extern Cell	*ssetsymtab(unsigned char *, unsigned char *,
			Awkfloat, unsigned, Array *);
extern Cell	*slookup(unsigned char *, Array *);
extern Awkfloat	setfval(Cell *, Awkfloat);
extern void	funnyvar(Cell *, char *);
extern unsigned char	*setsval(Cell *, unsigned char *);
extern Awkfloat	r_getfval(Cell *);
extern unsigned char	*r_getsval(Cell *);
#define	tostring(s)	stostring((unsigned char *)s)
extern unsigned char	*stostring(const unsigned char *);
extern unsigned char	*qstring(unsigned char *, int);
