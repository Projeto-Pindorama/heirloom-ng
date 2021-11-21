/*
   Changes by Gunnar Ritter, Freiburg i. Br., Germany, December 2002.
  
   Sccsid @(#)lib.c	1.27 (gritter) 12/25/06>
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

/*	from unixsrc:usr/src/common/cmd/awk/lib.c /main/uw7_nj/1	*/
/*	from RCS Header: lib.c 1.2 91/06/25 	*/

#define DEBUG
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include "awk.h"
#include "y.tab.h"
#include <pfmt.h>
#include <stdarg.h>
#include <wctype.h>
#include "asciitype.h"

#undef	RS

static void	eprint(void);

#define	getfval(p)	(((p)->tval & (ARR|FLD|REC|NUM)) == NUM ? (p)->fval : r_getfval(p))
#define	getsval(p)	(((p)->tval & (ARR|FLD|REC|STR)) == STR ? (p)->sval : r_getsval(p))

FILE	*infile	= NULL;
unsigned char	*file	= (unsigned char*) "";
unsigned char	*record;
unsigned char	*recdata;
int		recsize;
unsigned char	*fields;

int	donefld;	/* 1 = implies rec broken into fields */
int	donerec;	/* 1 = record is valid (no flds have changed) */

Cell	**fldtab;	/* room for fields */

static Cell	dollar0 = {
	OCELL, CFLD, (unsigned char*) "$0", (unsigned char *)"", 0.0, REC|STR|DONTFREE
};
static Cell	FINIT = {
	OCELL, CFLD, NULL, (unsigned char*) "", 0.0, FLD|STR|DONTFREE
};


static int	MAXFLD;	/* number of allocated fields */
int	maxfld	= 0;	/* last used field */
int	argno	= 1;	/* current input argument number */
extern	Awkfloat *ARGC;

static void growrec(unsigned char **, int *, int, unsigned char **, int);

char badopen[] = ":11:Cannot open %s: %s";

/* Dynamic field and record allocation inspired by Bell Labs awk. */
static void morefields(void)
{
	int	i;
	const int	n = 32;

	fldtab = realloc(fldtab, (MAXFLD + n + 1) * sizeof *fldtab);
	if (fldtab == NULL)
		error(MM_ERROR, ":13:Record `%.20s...' has too many fields",
			record);
	recloc = fldtab[0];
	for (i = MAXFLD; i < MAXFLD + n; i++) {
		fldtab[i] = malloc(sizeof **fldtab);
		if (fldtab[i] == NULL)
			error(MM_ERROR,
				":13:Record `%.20s...' has too many fields",
				record);
		*fldtab[i] = FINIT;
	}
	MAXFLD += n;
}

void fldinit(void)
{
	record = recdata = malloc(recsize = CHUNK);
	fields = malloc(recsize);
	if (record == NULL || fields == NULL)
		error(MM_ERROR, outofspace, "fldinit");
	*record = '\0';
	morefields();
	*fldtab[0] = dollar0;
}

void initgetrec(void)
{
	extern unsigned char **start_delayed, **after_delayed;
	unsigned char **pp;
	int i;
	unsigned char *p;

	/* first handle delayed name=val arguments */
	for (pp = start_delayed; pp != after_delayed; pp++)
		setclvar(*pp);
	for (i = 1; i < *ARGC; i++) {
		if (!isclvar(p = getargv(i)))	/* find 1st real filename */
			return;
		setclvar(p);	/* a commandline assignment before filename */
		argno++;
	}
	infile = stdin;		/* no filenames, so use stdin */
	/* *FILENAME = file = (unsigned char*) "-"; */
}

int getrec(unsigned char **buf, int *bufsize)
{
	int c, saved;
	static int firsttime = 1;

	if (firsttime) {
		firsttime = 0;
		initgetrec();
	}
	dprintf( ("RS=<%s>, FS=<%s>, ARGC=%d, FILENAME=%s\n",
		*RS ? *RS : tostring(""),
		*FS ? *FS : tostring(""),
		(int) *ARGC,
		*FILENAME ? *FILENAME : tostring("")) );
	donefld = 0;
	donerec = 1;
	if (*bufsize == 0) {
		if ((*buf = malloc(*bufsize = CHUNK)) == NULL)
			error(MM_ERROR, outofspace, "getrec");
		**buf = '\0';
	}
	saved = (*buf)[0];
	(*buf)[0] = 0;
	while (argno < *ARGC || infile == stdin) {
		dprintf( ("argno=%d, file=|%s|\n", argno, file) )
		;
		if (infile == NULL) {	/* have to open a new file */
			file = getargv(argno);
			if (*file == '\0') {	/* it's been zapped */
				argno++;
				continue;
			}
			if (isclvar(file)) {	/* a var=value arg */
				setclvar(file);
				argno++;
				continue;
			}
			*FILENAME = file;
			dprintf( ("opening file %s\n", file) );
			if (*file == '-' && *(file+1) == '\0')
				infile = stdin;
			else if ((infile = fopen((char *)file, "r")) == NULL)
				error(MM_ERROR, badopen, file, strerror(errno));
			setfval(fnrloc, 0.0);
		}
		c = readrec(buf, bufsize, infile);
		if (c != 0 || (*buf)[0] != '\0') {	/* normal record */
			if (*buf == record) {
				if (!(recloc->tval & DONTFREE))
					xfree(recloc->sval);
				recloc->sval = record;
				recloc->tval = REC | STR | DONTFREE;
				(void)is2number(0, recloc);
			}
			setfval(nrloc, nrloc->fval+1);
			setfval(fnrloc, fnrloc->fval+1);
			return 1;
		}
		/* EOF arrived on this file; set up next */
		if (infile != stdin)
			fclose(infile);
		infile = NULL;
		argno++;
	}
	/*
	* POSIX.2 requires that NF stick with its last value
	* at the start of the END code.  The most straightforward
	* way to do this is to restore the contents of record
	* [==buf when called from program()] so that getnf() will
	* recompute the same NF value unless something strange
	* occurs.  This has the side effect of $0...$NF *also*
	* having sticky values into END, but that seems to match
	* the spirit of POSIX.2's rule for NF.
	*/
	if (posix)
		(*buf)[0] = saved;
	return 0;	/* true end of file */
}

int readrec(unsigned char **buf, int *bufsize, FILE *inf)
	/* read one record into buf */
{
	register int sep, c, k, m, n;
	unsigned char *rr;
	register int nrr;
	wchar_t wc;

	next(wc, *RS, n);
	if ((sep = **RS) == 0) {
		sep = '\n';
		while ((c=getc(inf)) == '\n' && c != EOF)	/* skip leading \n's */
			;
		if (c != EOF)
			ungetc(c, inf);
	}
	if (*bufsize == 0)
		growrec(buf, bufsize, CHUNK, NULL, 0);
	for (rr = *buf, nrr = *bufsize; ; ) {
	cont:	for (; (c=getc(inf)) != sep && c != EOF; *rr++ = c)
			if (--nrr < n + 3) {
				growrec(buf, bufsize, *bufsize + CHUNK, &rr, 0);
				nrr += CHUNK;
			}
		if (c != EOF) {
			/*
			 * Note: This code does not restrict occurences of
			 * the multibyte sequence in RS to the start of an
			 * input character.
			 */
			for (m = 1; m < n; m++) {
				if ((c = getc(inf)) == EOF || c != (*RS)[m]) {
					for (k = 0; k < m; k++)
						*rr++ = (*RS)[k];
					nrr -= k;
					if (c == EOF)
						break;
					*rr++ = c;
					nrr--;
					goto cont;
				}
			}
		}
		if (**RS == sep || c == EOF)
			break;
		if ((c = getc(inf)) == '\n' || c == EOF) /* 2 in a row */
			break;
		*rr++ = '\n';
		*rr++ = c;
	}
	/*if (rr > *buf + *bufsize)
		error(MM_ERROR, ":12:Input record `%.20s...' too long", *buf);*/
	*rr = 0;
	dprintf( ("readrec saw <%s>, returns %d\n", *buf, c == EOF
		&& rr == *buf ? 0 : 1) );
	return c == EOF && rr == *buf ? 0 : 1;
}

unsigned char *getargv(int n)	/* get ARGV[n] */
{
	Cell *x;
	unsigned char *s, temp[25];
	extern Array *ARGVtab;

	snprintf((char *)temp, sizeof temp, "%d", n);
	x = setsymtab(temp, "", 0.0, STR, ARGVtab);
	s = getsval(x);
	dprintf( ("getargv(%d) returns |%s|\n", n, s) );
	return s;
}

void setclvar(unsigned char *s)	/* set var=value from s */
{
	unsigned char *p;
	Cell *q;

	for (p=s; *p != '='; p++)
		;
	*p++ = 0;
	p = qstring(p, '\0');
	q = setsymtab(s, p, 0.0, STR, symtab);
	setsval(q, p);
	(void)is2number(0, q);
	dprintf( ("command line set %s to |%s|\n", s, p) );
}

static void	cleanfld(int n1, int n2);
static int	refldbld(unsigned char *rec, unsigned char *fs);

void
fldbld(void)
{
	register unsigned char *r, *fr;
	Cell **p;
	wchar_t wc, sep;
	int i, n;

	if (donefld)
		return;
	if (!(recloc->tval & STR))
		getsval(recloc);
	r = recloc->sval;	/* was record! */
	fr = fields;
	i = 0;	/* number of fields accumulated here */
	if ((sep = **FS) != '\0' && (next(sep, *FS, n), (*FS)[n] != '\0')) {
		/* it's a regular expression */
		i = refldbld(r, *FS);
	} else if (sep == ' ') {
		for (i = 0; ; ) {
			while (*r == ' ' || *r == '\t' || *r == '\n')
				r++;
			if (*r == 0)
				break;
			i++;
			if (i >= MAXFLD)
				morefields();
			if (!(fldtab[i]->tval & DONTFREE))
				xfree(fldtab[i]->sval);
			fldtab[i]->sval = fr;
			fldtab[i]->tval = FLD | STR | DONTFREE;
			next(wc, r, n);
			do {
				do
					*fr++ = *r++;
				while (--n);
				next(wc, r, n);
			} while (wc != ' ' && wc != '\t' && wc != '\n' &&
					wc != '\0');
			*fr++ = 0;
		}
		*fr = 0;
	} else if (*r != 0) {	/* if 0, it's a null field */
		for (;;) {
			i++;
			if (i >= MAXFLD)
				morefields();
			if (!(fldtab[i]->tval & DONTFREE))
				xfree(fldtab[i]->sval);
			fldtab[i]->sval = fr;
			fldtab[i]->tval = FLD | STR | DONTFREE;
			while (next(wc, r, n),
					wc != sep && wc != '\n' && wc != '\0') {
					/* \n always a separator */
				do
					*fr++ = *r++;
				while (--n);
			}
			*fr++ = '\0';
			if (wc == '\0')
				break;
			r += n;
		}
		*fr = 0;
	}
	/*if (i >= MAXFLD)
		error(MM_ERROR, ":13:Record `%.20s...' has too many fields",
			record);*/
	/* clean out junk from previous record */
	cleanfld(i, maxfld);
	maxfld = i;
	donefld = 1;
	for (p = &fldtab[1]; p <= &fldtab[0]+maxfld; p++)
		(void)is2number(0, *p);
	setfval(nfloc, (Awkfloat) maxfld);
	if (dbg)
		for (p = &fldtab[0]; p <= &fldtab[0]+maxfld; p++)
			pfmt(stdout, MM_INFO, ":14:field %d: |%s|\n", p-&fldtab[0], 
				(*p)->sval);
}

static void cleanfld(int n1, int n2)	/* clean out fields n1..n2 inclusive */
{
	static unsigned char *nullstat = (unsigned char *) "";
	register Cell **p, **q;

	for (p = &fldtab[n2], q = &fldtab[n1]; p > q; p--) {
		if (!((*p)->tval & DONTFREE))
			xfree((*p)->sval);
		(*p)->tval = FLD | STR | DONTFREE;
		(*p)->sval = nullstat;
	}
}

void newfld(int n)	/* add field n (after end) */
{
	/*if (n >= MAXFLD)
		error(MM_ERROR, ":15:Creating too many fields", record);*/
	while (n >= MAXFLD)
		morefields();
	cleanfld(maxfld, n);
	maxfld = n;
	setfval(nfloc, (Awkfloat) n);
}

static int refldbld(unsigned char *rec,
		unsigned char *fs)	/* build fields from reg expr in FS */
{
	unsigned char *fr;
	int i;
	fa *pfa;

	fr = fields;
	*fr = '\0';
	if (*rec == '\0')
		return 0;
	pfa = makedfa(fs, 1);
	dprintf( ("into refldbld, rec = <%s>, pat = <%s>\n", rec,
		fs) );
	pfa->notbol = 0;
	for (i = 1; ; i++) {
		if (i >= MAXFLD)
			morefields();
		if (!(fldtab[i]->tval & DONTFREE))
			xfree(fldtab[i]->sval);
		fldtab[i]->tval = FLD | STR | DONTFREE;
		fldtab[i]->sval = fr;
		dprintf( ("refldbld: i=%d\n", i) );
		if (nematch(pfa, rec)) {
			pfa->notbol = REG_NOTBOL;
			dprintf( ("match %s (%d chars\n", 
				patbeg, patlen) );
			strncpy((char*) fr, (char*) rec, patbeg-rec);
			fr += patbeg - rec + 1;
			*(fr-1) = '\0';
			rec = patbeg + patlen;
		} else {
			dprintf( ("no match %s\n", rec) );
			strcpy((char*) fr, (char*) rec);
			pfa->notbol = 0;
			break;
		}
	}
	return i;		
}

void recbld(void)
{
	int i;
	unsigned char *r, *p;

	if (donerec == 1)
		return;
	r = recdata;
	for (i = 1; i <= *NF; i++) {
		p = getsval(fldtab[i]);
		while ((*r = *p++)) {
			if (++r >= &recdata[recsize]) {
				recsize += CHUNK;
				growrec(&recdata, &recsize, recsize, &r, 1);
			}
		}
		if (i < *NF)
			for ((p = *OFS); (*r = *p++); ) {
				if (++r >= &recdata[recsize]) {
					recsize += CHUNK;
					growrec(&recdata, &recsize,
							recsize, &r, 1);
				}
			}
	}
	*r = '\0';
	dprintf( ("in recbld FS=%o, recloc=%lo\n", **FS, 
		(long)recloc) );
	recloc->tval = REC | STR | DONTFREE;
	recloc->sval = record = recdata;
	dprintf( ("in recbld FS=%o, recloc=%lo\n", **FS, 
		(long)recloc) );
	dprintf( ("recbld = |%s|\n", record) );
	donerec = 1;
}

Cell *fieldadr(int n)
{
	if (n < 0)
		error(MM_ERROR, ":17:Trying to access field %d", n);
	while (n >= MAXFLD)
		morefields();
	return(fldtab[n]);
}

int	errorflag	= 0;
char	errbuf[200];

static int been_here = 0;
static char
	atline[] = ":18: at source line %d",
	infunc[] = ":19: in function %s";

void
vyyerror(const char *msg, ...)
{
	extern unsigned char *curfname;
	va_list args;

	if (been_here++ > 2)
		return;
	va_start(args, msg);
	vpfmt(stderr, MM_ERROR, msg, args);
	pfmt(stderr, MM_NOSTD, atline, lineno);
	if (curfname != NULL)
		pfmt(stderr, MM_NOSTD, infunc, curfname);
	fprintf(stderr, "\n");
	errorflag = 2;
	eprint();

	va_end(args);
}
	
void
yyerror(char *s)
{
	extern unsigned char /**cmdname,*/ *curfname;
	static int been_here = 0;

	if (been_here++ > 2)
		return;
	pfmt(stderr, (MM_ERROR | MM_NOGET), "%s", s);
	pfmt(stderr, MM_NOSTD, atline, lineno);
	if (curfname != NULL)
		pfmt(stderr, MM_NOSTD, infunc, curfname);
	fprintf(stderr, "\n");
	errorflag = 2;
	eprint();
}

/*ARGSUSED*/
void fpecatch(int signo)
{
	error(MM_ERROR, ":20:Floating point exception");
}

extern int bracecnt, brackcnt, parencnt;
static void bcheck2(int n, int c1, int c2);

void bracecheck(void)
{
	int c;
	static int beenhere = 0;

	if (beenhere++)
		return;
	while ((c = awk_input()) != EOF && c != '\0')
		bclass(c);
	bcheck2(bracecnt, '{', '}');
	bcheck2(brackcnt, '[', ']');
	bcheck2(parencnt, '(', ')');
}

static void bcheck2(int n, int c1, int c2)
{
	if (n == 1)
		pfmt(stderr, MM_ERROR, ":21:Missing %c\n", c2);
	else if (n > 1)
		pfmt(stderr, MM_ERROR, ":22:%d missing %c's\n", n, c2);
	else if (n == -1)
		pfmt(stderr, MM_ERROR, ":23:Extra %c\n", c2);
	else if (n < -1)
		pfmt(stderr, MM_ERROR, ":24:%d extra %c's\n", -n, c2);
}

void
error(int flag, const char *msg, ...)
{
	int errline;
	extern Node *curnode;
	/*extern unsigned char *cmdname;*/
	va_list args;

	fflush(stdout);
	va_start(args, msg);
	vpfmt(stderr, flag, msg, args);
	putc('\n', stderr);

	if (compile_time != 2 && NR && *NR > 0) {
		pfmt(stderr, MM_INFO,
			":25:Input record number %g", *FNR);
		if (strcmp((char*) *FILENAME, "-") != 0)
			pfmt(stderr, MM_NOSTD,
				":26:, file %s", *FILENAME);
		fprintf(stderr, "\n");
	}
	errline = 0;
	if (compile_time != 2 && curnode)
		errline = curnode->lineno;
	else if (compile_time != 2 && lineno)
		errline = lineno;
	if (errline)
		pfmt(stderr, MM_INFO, ":27:Source line number %d\n", errline);
	eprint();
	if (flag == MM_ERROR) {
		if (dbg)
			abort();
		exit(2);
	}
	va_end(args);
}

static void eprint(void)	/* try to print context around error */
{
	unsigned char *p, *q, *r;
	int c, episnul;
	static int been_here = 0;
	extern unsigned char ebuf[300], *ep;

	if (compile_time == 2 || compile_time == 0 || been_here++ > 0)
		return;
	episnul = ep > ebuf && ep[-1] == '\0';
	p = ep - 1 - episnul;
	if (p > ebuf && *p == '\n')
		p--;
	for ( ; p > ebuf && *p != '\n' && *p != '\0'; p--)
		;
	while (*p == '\n')
		p++;
	if (0 /* posix */)
		pfmt(stderr, MM_INFO, ":28:Context is\n\t");
	else
		pfmt(stderr, MM_INFO|MM_NOSTD, ":2228: context is\n\t");
	for (q=ep-1-episnul; q>=p && *q!=' ' && *q!='\t' && *q!='\n'; q--)
		;
	for (r = q; r < ep; r++) {
		if (*r != ' ' && *r != '\t' && *r != '\n') {
			for ( ; p < q; p++)
				if (*p)
					putc(*p, stderr);
			break;
		}
	}
	fprintf(stderr, " >>> ");
	for ( ; p < ep; p++)
		if (*p)
			putc(*p, stderr);
	fprintf(stderr, " <<< ");
	if (*ep)
		while ((c = awk_input()) != '\n' && c != '\0' && c != EOF) {
			putc(c, stderr);
			bclass(c);
		}
	putc('\n', stderr);
	ep = ebuf;
}

void bclass(int c)
{
	switch (c) {
	case '{': bracecnt++; break;
	case '}': bracecnt--; break;
	case '[': brackcnt++; break;
	case ']': brackcnt--; break;
	case '(': parencnt++; break;
	case ')': parencnt--; break;
	}
}

double errcheck(double x, unsigned char *s)
{
	if (errno == EDOM) {
		errno = 0;
		error(MM_WARNING, ":29:%s argument out of domain", s);
		x = 1;
	} else if (errno == ERANGE) {
		errno = 0;
		error(MM_WARNING, ":30:%s result out of range", s);
		x = 1;
	}
	return x;
}

void PUTS(unsigned char *s) {
	dprintf( ("%s\n", s) );
}

int isclvar(unsigned char *s)	/* is s of form var=something? */
{
	unsigned char *os = s;

	for ( ; *s; s++)
		if (!(alnumchar(*s) || *s == '_'))
			break;
	return *s == '=' && s > os && *(s+1) != '=' && !digitchar(*os);
}

int is2number(register unsigned char *s, Cell *p)
{
	unsigned char *after;
	Awkfloat val;

	/*
	* POSIX.2 says leading <blank>s are skipped and that
	* <blank> is at least ' ' and '\t' and can include other
	* characters, but not in the "POSIX" (aka "C") locale.
	*
	* The historic code skipped those two and newline.  So,
	* unless it's noticed by some test suite, we choose to
	* keep things compatible.  To be safe, reject the string
	* if it starts with other white space characters since
	* strtod() skips any form of white space.
	*
	* Permit similarly spelled trailing white space for
	* compatibility.
	*/
	if (p != 0)
		s = p->sval;
	while (*s == ' ' || *s == '\t' || *s == '\n')
		s++;
	if (isspace(*s))
		return 0;
	/*
	 * Reject hexadecimal numbers, infinity and NaN strings which
	 * are recognized by C99 strtod() implementations.
	 */
	switch (*s) {
	case '0':
		if (s[1] == 'x' || s[1] == 'X')
			return 0;
		break;
	case 'i':
	case 'I':
		if (strncasecmp((char *)s, "inf", 3) == 0)
			return 0;
		break;
	case 'n':
	case 'N':
		if (strncasecmp((char *)s, "NaN", 3) == 0)
			return 0;
		break;
	}
	val = strtod((char *)s, (char **)&after);
	for (s = after; *s == ' ' || *s == '\t' || *s == '\n'; s++)
		;
	if (*s != '\0')
		return 0;
	if (p != 0) {
		p->fval = val;
		p->tval |= NUM;
	}
	return 1;
}

double
awk_atof(const char *s)
{
	wchar_t	wc;
	int	n;

	while (*s) {
		next(wc, s, n);
		if (!(mb_cur_max > 1 ? iswspace(wc) : isspace(wc)))
			break;
		s += n;
	}
	/*
	 * Return 0 for hexadecimal numbers, infinity and NaN strings which
	 * are recognized by C99 atof() implementations.
	 */
	switch (*s) {
	case '0':
		if (s[1] == 'x' || s[1] == 'X')
			return 0;
		break;
	case 'i':
	case 'I':
		if (strncasecmp(s, "inf", 3) == 0)
			return 0;
		break;
	case 'n':
	case 'N':
		if (strncasecmp(s, "NaN", 3) == 0)
			return 0;
		break;
	}
	return atof(s);
}

unsigned char *makerec(const unsigned char *data, int size)
{
	if (!(recloc->tval & DONTFREE))
		xfree(recloc->sval);
	if (recsize < size)
		growrec(&recdata, &recsize, size, NULL, 0);
	record = recdata;
	strcpy((char*)record, (char*)data);
	recloc->sval = record;
	recloc->tval = REC | STR | DONTFREE;
	donerec = 1; donefld = 0;
	return record;
}

static void growrec(unsigned char **buf, int *bufsize, int newsize,
		unsigned char **ptr, int bld)
{
	unsigned char	*np, *op;

	op = *buf;
	if ((np = realloc(op, *bufsize = newsize)) == 0) {
		oflo:	if (bld)
				error(MM_ERROR,
					":16:Built giant record `%.20s...'",
					*buf);
			else
				error(MM_ERROR,
					":12:Input record `%.20s...' too long",
					*buf);
	}
	if (ptr && *ptr)
		*ptr = &np[*ptr - op];
	if (record == op)
		record = np;
	if (recdata == op) {
		recdata = np;
		recsize = *bufsize;
		if ((fields = realloc(fields, recsize)) == NULL)
			goto oflo;
	}
	if (fldtab[0]->sval == op)
		fldtab[0]->sval = np;
	if (recloc->sval == op)
		recloc->sval = np;
	*buf = np;
}

int
vpfmt(FILE *stream, long flags, const char *fmt, va_list ap)
{
	extern char	*pfmt_label__;
	int	n = 0;

	if ((flags & MM_NOGET) == 0) {
		if (*fmt == ':') {
			do
				fmt++;
			while (*fmt != ':');
			fmt++;
		}
	}
	if ((flags & MM_NOSTD) == 0)
		n += fprintf(stream, "%s: ", pfmt_label__);
	if ((flags & MM_ACTION) == 0 && isupper(*fmt&0377))
		n += fprintf(stream, "%c", tolower(*fmt++&0377));
	n += vfprintf(stream, fmt, ap);
	return n;
}
