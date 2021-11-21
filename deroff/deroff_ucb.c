/*	from 4.4BSD deroff.1	*/
/*
 * Modified by Gunnar Ritter, Freiburg i. Br., Germany, August 2002.
 */
/*-
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This module is believed to contain source code proprietary to AT&T.
 * Use and redistribution is subject to the Berkeley Software License
 * Agreement and your Software Agreement with AT&T (Western Electric).
 */
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
static const char sccsid[] USED = "@(#)/usr/ucb/deroff.sl	1.13 (gritter) 12/25/06";

/*	from deroff.c	8.1 (Berkeley) 6/6/93	*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>
#include <ctype.h>

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

/*
 *	Deroff command -- strip troff, eqn, and Tbl sequences from
 *	a file.  Has two flags argument, -w, to cause output one word per line
 *	rather than in the original format.
 *	-mm (or -ms) causes the corresponding macro's to be interpreted
 *	so that just sentences are output
 *	-ml  also gets rid of lists.
 *	Deroff follows .so and .nx commands, removes contents of macro
 *	definitions, equations (both .EQ ... .EN and $...$),
 *	Tbl command sequences, and Troff backslash constructions.
 *
 *	All input is through the Cget macro;
 *	the most recently read character is in c.
 *
 *	Modified by Robert Henry to process -me and -man macros.
 */

#define Cget ( (c=getc(infile)) == EOF ? eof() : ((c==ldelim)&&(filesc==0) ? skeqn() : c) )
#define C1get ( (c=getc(infile)) == EOF ? eof() :  c)

#ifdef DEBUG
#  define C	_C()
#  define C1	_C1()
#else /* not DEBUG */
#  define C	Cget
#  define C1	C1get
#endif /* not DEBUG */

#define SKIP while(C != '\n') 
#define SKIP_TO_COM SKIP; SKIP; pc=c; while(C != '.' || pc != '\n' || C > 'Z')pc=c

#define	YES 1
#define	NO 0
#define	MS 0	/* -ms */
#define	MM 1	/* -mm */
#define	ME 2	/* -me */
#define	MA 3	/* -man */

#ifdef DEBUG
static char *mactab[] = {"-ms", "-mm", "-me", "-ma"};
#endif /* DEBUG */

#define	ONE 1
#define	TWO 2

#define NOCHAR -2
#define SPECIAL 0
#define APOS 1
#define PUNCT 2
#define DIGIT 3
#define LETTER 4

typedef	int pacmac;		/* compressed macro name */
struct	mactab{
	int	condition;
	pacmac	macname;
	int	(*func)(pacmac);
};

static int	wordflag;
static int	msflag;	/* processing a source written using a mac package */
static int	mac;		/* which package */
static int	disp;
static int	parag;
static int	inmacro;
static int	intable;
static int	keepblock; /* keep blocks of text; normally false when msflag */

static char chars[256];  /* SPECIAL, PUNCT, APOS, DIGIT, or LETTER */

static char *line;
static long linesize;
static long lpos;

static char	*progname;
static char	*Progname;

static int c;
static int pc;
static int ldelim;
static int rdelim;


static int argc;
static char **argv;

static char *fname;
static int fnamesize;
static FILE **files;
static int filessize;
static int filesc;
static FILE *infile;
/*
 *	Flags for matching conditions other than
 *	the macro name
 */
#define	NONE		0
#define	FNEST		1		/* no nested files */
#define	NOMAC		2		/* no macro */
#define	MAC		3		/* macro */
#define	PARAG		4		/* in a paragraph */
#define	MSF		5		/* msflag is on */
#define	NBLK		6		/* set if no blocks to be kept */

/*
 *	Return codes from macro minions, determine where to jump,
 *	how to repeat/reprocess text
 */
#define	COMX		1		/* goto comx */
#define	COM		2		/* goto com */

static int skeqn(void);
static FILE *opn(register char *);
static int eof(void);
static void getfname(void);
static void fatal(const char *, const char *);
static void textline(char *, int);
static void work(void);
static void regline(void (*)(char *, int), int);
static void macro(void);
static void tbl(void);
static void stbl(void);
static void eqn(void);
static void backsl(void);
static char *copys(register const char *);
static void sce(void);
static void refer(int);
static void inpic(void);
static void msputmac(register char *, int);
static void msputwords(int);
static void meputmac(register char *, int);
static void meputwords(int);
static void noblock(char, char);
static int EQ(void);
static int domacro(void);
static int PS(void);
static int skip(void);
static int intbl(void);
static int outtbl(void);
static int so(void);
static int nx(void);
static int skiptocom(void);
static int PP(pacmac);
static int AU(void);
static int SH(pacmac);
static int UX(void);
static int MMHU(pacmac);
static int mesnblock(pacmac);
static int mssnblock(pacmac);
static int nf(void);
static int ce(void);
static int meip(pacmac);
static int mepp(pacmac);
static int mesh(pacmac);
static int mefont(pacmac);
static int manfont(pacmac);
static int manpp(pacmac);
static void defcomline(pacmac);
static void comline(void);
static int macsort(const void *, const void *);
static int sizetab(register struct mactab *);
static struct mactab *macfill(register struct mactab *, register struct mactab *);
static void buildtab(struct mactab **, int *);
static void *srealloc(void *, size_t);

int
main(int ac, char **av)
{
	register int i;
	int	errflg = 0;
	int	kflag = NO;
	char	*p;
	const char	optstr[] = "km:pw";

#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	progname = basename(av[0]);
	Progname = srealloc(0, strlen(progname) + 1);
	strcpy(Progname, progname);
	Progname[0] = toupper(Progname[0]);
	wordflag = NO;
	msflag = NO;
	mac = ME;
	disp = NO;
	parag = NO;
	inmacro = NO;
	intable = NO;
	ldelim	= NOCHAR;
	rdelim	= NOCHAR;
	keepblock = YES;

	while ((i = getopt(ac, av, optstr)) != EOF) {
		switch(i) {
		case 'p':
			parag=YES;
			break;
		case 'k':
			kflag = YES;
			break;
		case 'w':
			wordflag = YES;
			kflag = YES;
			break;
		case 'm':
			msflag = YES;
			keepblock = NO;
			switch(optarg[0]){
			case 'm':	mac = MM; p++; break;
			case 's':	mac = MS; p++; break;
			case 'e':	mac = ME; p++; break;
			case 'a':	mac = MA; p++; break;
			case 'l':	disp = YES; p++; break;
			default:	errflg++; break;
			}
			if (optarg[1])
				errflg++;
			break;
		default:
			errflg++;
		}
	}

	argc = ac - optind;
	argv = &av[optind];
	if (kflag)
		keepblock = YES;
	if (errflg) {
		fprintf(stderr,
	"%s: usage: %s [ -w ] [ -k] [ -m (a e m s l) ] [ file ] ... \n",
			Progname, progname);
		exit(2);
	}

#ifdef DEBUG
	printf("msflag = %d, mac = %s, keepblock = %d, disp = %d\n",
		msflag, mactab[mac], keepblock, disp);
#endif /* DEBUG */
	if (argc == 0){
		infile = stdin;
	} else {
		infile = opn(argv[0]);
		--argc;
		++argv;
	}


	files = srealloc(0, (filessize=1) * sizeof *files);
	files[0] = infile;
	filesc = 0;

	for(i='a'; i<='z' ; ++i)
		chars[i] = LETTER;
	for(i='A'; i<='Z'; ++i)
		chars[i] = LETTER;
	for(i='0'; i<='9'; ++i)
		chars[i] = DIGIT;
	chars['\''] = APOS;
	chars['&'] = APOS;
	chars['.'] = PUNCT;
	chars[','] = PUNCT;
	chars[';'] = PUNCT;
	chars['?'] = PUNCT;
	chars[':'] = PUNCT;
	/*
	 *	Unix troff can only handle ASCII, groff can only
	 *	handle ISO-8859-1. Thus assume that ISO-8859-1 is
	 *	used for high-bit characters.
	 */
	for (i=0241; i<=0277; ++i)
		chars[i] = PUNCT;
	for (i=0300; i<=0377; ++i)
		chars[i] = LETTER;
	work();
	return 0;
}

static int
skeqn(void)
{
	while((c = getc(infile)) != rdelim)
		if(c == EOF) {
			c = eof();
		} else if(c == '"') {
			while( (c = getc(infile)) != '"')
				if(c == EOF)
					c = eof();
				else if(c == '\\')
					if((c = getc(infile)) == EOF)
						c = eof();
		}
	if(msflag)return(c='x');
	return(c = ' ');
}

static FILE *opn(register char *p)
{
	FILE *fd;

	if( (fd = fopen(p, "r")) == NULL) {
		fprintf(stderr, "Deroff: ");
		perror(p);
		exit(1);
	}

	return(fd);
}

static int
eof(void)
{
	if(infile != stdin)
		fclose(infile);
	if(filesc > 0)
		infile = files[--filesc];
	else if (argc > 0) {
		infile = opn(argv[0]);
		--argc;
		++argv;
	} else
		exit(0);
	return(C);
}

static void
getfname(void)
{
	register int i;
	struct chain { 
		struct chain *nextp; 
		char *datap; 
	} *chainblock;
	register struct chain *q;
	static struct chain *namechain	= NULL;

	while(C == ' ') ;

	for (i = 0; c!='\n' && c!=' '  && c!='\t' && c!='\\'; i++) {
		if (fnamesize <= i+1)
			fname = srealloc(fname, fnamesize = i+1);
		fname[i] = c;
		C;
	}
	if (fnamesize <= i+1)
		fname = srealloc(fname, fnamesize = i+1);
	fname[i] = '\0';
	while(c != '\n')
		C;

	/* see if this name has already been used */

	for(q = namechain ; q; q = q->nextp)
		if( ! strcmp(fname, q->datap))
		{
			fname[0] = '\0';
			return;
		}

	q = (struct chain *) calloc(1, sizeof(*chainblock));
	q->nextp = namechain;
	q->datap = copys(fname);
	namechain = q;
}

static void
fatal(const char *s,const char *p)
{
	fprintf(stderr, "%s: ", Progname);
	fprintf(stderr, s, p);
	exit(1);
}

/*ARGSUSED1*/
static void
textline(char *str, int constant)
{
	if (wordflag) {
		msputwords(0);
		return;
	}
	puts(str);
}

static void
work(void)
{
	for( ;; )
	{
		C;
#ifdef FULLDEBUG
		printf("Starting work with `%c'\n", c);
#endif /* FULLDEBUG */
		if(c == '.'  ||  c == '\'')
			comline();
		else
			regline(textline, TWO);
	}
}

static void
regline(void (*pfunc)(char *, int), int constant)
{
	if (linesize == 0)
		line = srealloc(line, linesize=1);
	line[0] = c;
	lpos = 0;
	for( ; ; )
	{
		if (linesize <= lpos+3)
			line = srealloc(line, linesize = lpos+3);
		if(c == '\\') {
			line[lpos] = ' ';
			backsl();
		}
		if(c == '\n')
			break;
		if(intable && c=='T') {
			line[++lpos] = C;
			if(c=='{' || c=='}') {
				line[lpos-1] = ' ';
				line[lpos] = C;
			}
		} else {
			line[++lpos] = C;
		}
	}

	line[lpos] = '\0';

	if(line[0] != '\0')
		(*pfunc)(line, constant);
}

static void
macro(void)
{
	if(msflag){
		do { 
			SKIP; 
		}		while(C!='.' || C!='.' || C=='.');	/* look for  .. */
		if(c != '\n')SKIP;
		return;
	}
	SKIP;
	inmacro = YES;
}

static void
tbl(void)
{
	while(C != '.');
	SKIP;
	intable = YES;
}

static void
stbl(void)
{
	while(C != '.');
	SKIP_TO_COM;
	if(c != 'T' || C != 'E'){
		SKIP;
		pc=c;
		while(C != '.' || pc != '\n' || C != 'T' || C != 'E')pc=c;
	}
}

static void
eqn(void)
{
	register int c1, c2;
	register int dflg;
	char last;

	last=0;
	dflg = 1;
	SKIP;

	for( ;;)
	{
		if(C1 == '.'  || c == '\'')
		{
			while(C1==' ' || c=='\t')
				;
			if(c=='E' && C1=='N')
			{
				SKIP;
				if(msflag && dflg){
					putchar('x');
					putchar(' ');
					if(last){
						putchar(last); 
						putchar('\n'); 
					}
				}
				return;
			}
		}
		else if(c == 'd')	/* look for delim */
		{
			if(C1=='e' && C1=='l')
				if( C1=='i' && C1=='m')
				{
					while(C1 == ' ');
					if((c1=c)=='\n' || (c2=C1)=='\n'
					    || (c1=='o' && c2=='f' && C1=='f') )
					{
						ldelim = NOCHAR;
						rdelim = NOCHAR;
					}
					else	{
						ldelim = c1;
						rdelim = c2;
					}
				}
			dflg = 0;
		}

		if(c != '\n') while(C1 != '\n'){ 
			if(chars[c] == PUNCT)last = c;
			else if(c != ' ')last = 0;
		}
	}
}

static void
backsl(void)	/* skip over a complete backslash construction */
{
	int bdelim;

sw:  
	switch(C)
	{
	case '"':
		SKIP;
		return;
	case 's':
		if(C == '\\') backsl();
		else	{
			while(C>='0' && c<='9') ;
			ungetc(c,infile);
			c = '0';
		}
		--lpos;
		return;

	case 'f':
	case 'n':
	case '*':
		if(C != '(')
			return;

	case '(':
		if(msflag){
			if(C == 'e'){
				if(C == 'm'){
					line[lpos] = '-';
					return;
				}
			}
			else if(c != '\n')C;
			return;
		}
		if(C != '\n') C;
		return;

	case '$':
		C;	/* discard argument number */
		return;

	case 'b':
	case 'x':
	case 'v':
	case 'h':
	case 'w':
	case 'o':
	case 'l':
	case 'L':
		if( (bdelim=C) == '\n')
			return;
		while(C!='\n' && c!=bdelim)
			if(c == '\\') backsl();
		return;

	case '\\':
		if(inmacro)
			goto sw;
	default:
		return;
	}
}

static char *
copys(register const char *s)
{
	register char *t, *t0;

	if( (t0 = t = calloc( (unsigned)(strlen(s)+1), sizeof(*t) ) ) == NULL)
		fatal("Cannot allocate memory", (char *) NULL);

	while((*t++ = *s++) != '\0')
		;
	return(t0);
}

static void
sce(void)
{
	register char *ap;
	register int n, i;
	char a[10];
	for(ap=a;C != '\n';ap++){
		*ap = c;
		if(ap == &a[9]){
			SKIP;
			ap=a;
			break;
		}
	}
	if(ap != a)n = atoi(a);
	else n = 1;
	for(i=0;i<n;){
		if(C == '.'){
			if(C == 'c'){
				if(C == 'e'){
					while(C == ' ');
					if(c == '0'){
						SKIP;
						break;
					}
					else SKIP;
				}
				else SKIP;
			}
			else if(c == 'P' || C == 'P'){
				if(c != '\n')SKIP;
				break;
			}
			else if(c != '\n')SKIP;
		}
		else {
			SKIP;
			i++;
		}
	}
}

static void
refer(int c1)
{
	register int c2 = 0;
	if(c1 != '\n')
		SKIP;
	while(1){
		if(C != '.')
			SKIP;
		else {
			if(C != ']')
				SKIP;
			else {
				while(C != '\n')
					c2=c;
				if(chars[c2] == PUNCT)putchar(c2);
				return;
			}
		}
	}
}

static void
inpic(void)
{
	register int c1, i1;
	SKIP;
	i1 = 0;
	c = '\n';
	while(1){
		c1 = c;
		if(C == '.' && c1 == '\n'){
			if(C != 'P'){
				if(c == '\n')continue;
				else { SKIP; c='\n'; continue;}
			}
			if(C != 'E'){
				if(c == '\n')continue;
				else { SKIP; c='\n';continue; }
			}
			SKIP;
			return;
		}
		else if(c == '\"'){
			while(C != '\"'){
				if(c == '\\'){
					if(C == '\"')continue;
					ungetc(c,infile);
					backsl();
				}
				else {
					if (linesize <= i1+1)
						line = srealloc(line,
								linesize=i1+1);
					line[i1++] = c;
				}
			}
			if (linesize <= i1+1)
				line = srealloc(line, linesize=i1+1);
			line[i1++] = ' ';
		}
		else if(c == '\n' && i1 != 0){
			if (linesize <= i1+1)
				line = srealloc(line, linesize=i1+1);
			line[i1] = '\0';
			if(wordflag)msputwords(NO);
			else {
				puts(line);
				putchar('\n');
			}
			i1 = 0;
		}
	}
}

#ifdef DEBUG
static int
_C1(void)
{
	return(C1get);
}
static int
_C(void)
{
	return(Cget);
}
#endif /* DEBUG */

/*
 *	Macro processing
 *
 *	Macro table definitions
 */
#define	reg	register
static int	argconcat = 0;		/* concat arguments together (-me only) */

#define	tomac(c1, c2)		((((c1) & 0xFF) << 8) | ((c2) & 0xFF))
#define	frommac(src, c1, c2)	(((c1)=((src)>>8)&0xFF),((c2) =(src)&0xFF))

extern struct	mactab	troffmactab[];
extern struct	mactab	ppmactab[];
extern struct	mactab	msmactab[];
extern struct	mactab	mmmactab[];
extern struct	mactab	memactab[];
extern struct	mactab	manmactab[];

/*
 *	macro table initialization
 */
#define	M(cond, c1, c2, func) {cond, tomac(c1, c2), (int(*)(pacmac))func}

/*
 *	Put out a macro line, using ms and mm conventions.
 */
static void
msputmac(register char *s, int constant)
{
	register char *t;
	register int found;
	int last = 0;
	found = 0;

	if (wordflag) {
		msputwords(YES);
		return;
	}
	while(*s)
	{
		while(*s==' ' || *s=='\t')
			putchar(*s++);
		for(t = s ; *t!=' ' && *t!='\t' && *t!='\0' ; ++t)
			;
		if(*s == '\"')s++;
		if(t>s+constant && chars[ s[0] & 0377 ]==LETTER &&
				chars[ s[1]  & 0377 ]==LETTER){
			while(s < t)
				if(*s == '\"')s++;
				else
					putchar(*s++);
			last = *(t-1);
			found++;
		}
		else if(found && chars[ s[0] & 0377 ] == PUNCT && s[1] == '\0')
			putchar(*s++);
		else{
			last = *(t-1);
			s = t;
		}
	}
	putchar('\n');
	if(msflag && chars[last] == PUNCT){
		putchar(last);
		putchar('\n');
	}
}
/*
 *	put out words (for the -w option) with ms and mm conventions
 */
static void
msputwords(int macline /* is this is a macro line */)
{
	register char *p, *p1;
	int i, nlet;

	for(p1 = line ; ;) {
		/*
		 *	skip initial specials ampersands and apostrophes
		 */
		while( chars[*p1 & 0377] < DIGIT)
			if(*p1++ == '\0') return;
		nlet = 0;
		for(p = p1 ; (i=chars[*p & 0377]) != SPECIAL ; ++p)
			if(i == LETTER) ++nlet;

		if (nlet > 1 && chars[p1[0] & 0377] == LETTER) {
			/*
			 *	delete trailing ampersands and apostrophes
			 */
			while( (i=chars[p[-1] & 0377]) == PUNCT || i == APOS )
				--p;
			while(p1 < p)
				putchar(*p1++);
			putchar('\n');
		} else {
			p1 = p;
		}
	}
}
/*
 *	put out a macro using the me conventions
 */
#define SKIPBLANK(cp)	while(*cp == ' ' || *cp == '\t') { cp++; }
/*#define SKIPNONBLANK(cp) while(*cp !=' ' && *cp !='\cp' && *cp !='\0') { cp++; }*/

static void
meputmac(reg char *cp, int constant)
{
	reg	char	*np;
		int	found = 0;
		int	argno;
		int	last = 0;
		int	inquote;

	if (wordflag) {
		meputwords(YES);
		return;
	}
	for (argno = 0; *cp; argno++){
		SKIPBLANK(cp);
		inquote = (*cp == '"');
		if (inquote)
			cp++;
		for (np = cp; *np; np++){
			switch(*np){
			case '\n':
			case '\0':	break;
			case '\t':
			case ' ':	if (inquote) {
						continue;
					} else {
						goto endarg;
					}
			case '"':	if(inquote && np[1] == '"'){
						strcpy(np, np + 1);
						np++;
						continue;
					} else {
						*np = ' '; 	/* bye bye " */
						goto endarg;
					}
			default:	continue;
			}
		}
		endarg: ;
		/*
		 *	cp points at the first char in the arg
		 *	np points one beyond the last char in the arg
		 */
		if ((argconcat == 0) || (argconcat != argno)) {
			putchar(' ');
		}
#ifdef FULLDEBUG
		{
			char	*p;
			printf("[%d,%d: ", argno, np - cp);
			for (p = cp; p < np; p++) {
				putchar(*p);
			}
			printf("]");
		}
#endif /* FULLDEBUG */
		/*
		 *	Determine if the argument merits being printed
		 *
		 *	constant is the cut off point below which something
		 *	is not a word.
		 */
		if (   ( (np - cp) > constant)
		    && (    inquote
		         || (chars[cp[0] & 0377] == LETTER)) ){
			for (cp = cp; cp < np; cp++){
				putchar(*cp);
			}
			last = np[-1];
			found++;
		} else
		if(found && (np - cp == 1) && chars[*cp & 0377] == PUNCT){
			putchar(*cp);
		} else {
			last = np[-1];
		}
		cp = np;
	}
	if(msflag && chars[last] == PUNCT)
		putchar(last);
	putchar('\n');
}
/*
 *	put out words (for the -w option) with ms and mm conventions
 */
static void
meputwords(int macline)
{
	msputwords(macline);
}
/*
 *
 *	Skip over a nested set of macros
 *
 *	Possible arguments to noblock are:
 *
 *	fi	end of unfilled text
 *	PE	pic ending
 *	DE	display ending
 *
 *	for ms and mm only:
 *		KE	keep ending
 *
 *		NE	undocumented match to NS (for mm?)
 *		LE	mm only: matches RL or *L (for lists)
 *
 *	for me:
 *		([lqbzcdf]
 */

static void
noblock(char a1, char a2)
{
	register int c1,c2;
	register int eqnf;
	int lct;
	lct = 0;
	eqnf = 1;
	SKIP;
	while(1){
		while(C != '.')
			if(c == '\n')
				continue;
			else
				SKIP;
		if((c1=C) == '\n')
			continue;
		if((c2=C) == '\n')
			continue;
		if(c1==a1 && c2 == a2){
			SKIP;
			if(lct != 0){
				lct--;
				continue;
			}
			if(eqnf)
				putchar('.');
			putchar('\n');
			return;
		} else if(a1 == 'L' && c2 == 'L'){
			lct++;
			SKIP;
		}
		/*
		 *	equations (EQ) nested within a display
		 */
		else if(c1 == 'E' && c2 == 'Q'){
			if (   (mac == ME && a1 == ')')
			    || (mac != ME && a1 == 'D') ) {
				eqn(); 
				eqnf=0;
			}
		}
		/*
		 *	turning on filling is done by the paragraphing
		 *	macros
		 */
		else if(a1 == 'f') {	/* .fi */
			if  (  (mac == ME && (c2 == 'h' || c2 == 'p'))
			     ||(mac != ME && (c1 == 'P' || c2 == 'P')) ) {
				SKIP;
				return;
			}
		} else {
			SKIP;
		}
	}
}

static int
EQ(void)
{
	eqn();
	return(0);
}
static int
domacro(void)
{
	macro();
	return(0);
}
static int
PS(void)
{
	for (C; c == ' ' || c == '\t'; C);
	if (c == '<') {		/* ".PS < file" -- don't expect a .PE */
		SKIP;
		return(0);
	}
	if (!msflag) {
		inpic();
	} else {
		noblock('P', 'E');
	}
	return(0);
}

static int
skip(void)
{
	SKIP;
	return(0);
}

static int
intbl(void)
{
	if(msflag){ 
		stbl(); 
	}
	else tbl(); 
	return(0);
}

static int
outtbl(void){ intable = NO; return 0;}

static int
so(void)
{
	getfname();
	if( fname[0] ) {
		if (filessize <= filesc+2)
			files = srealloc(files,
				(filessize=filesc+2) * sizeof *files);
		infile = files[++filesc] = opn( fname );
	}
	return(0);
}
static int
nx(void)
{
	getfname();
	if(fname[0] == '\0') exit(0);
	if(infile != stdin)
		fclose(infile);
	if (filessize <= filesc+1)
		files = srealloc(files, (filessize=filesc+1) * sizeof *files);
	infile = files[filesc] = opn(fname);
	return(0);
}
static int
skiptocom(void){ SKIP_TO_COM; return(COMX); }

static int
PP(pacmac c12)
{
	int	c1, c2;
	
	frommac(c12, c1, c2);
	printf(".%c%c",c1,c2);
	while(C != '\n')putchar(c);
	putchar('\n');
	return(0);
}
static int
AU(void)
{
	if(mac==MM) {
		return(0);
	} else {
		SKIP_TO_COM;
		return(COMX);
	}
}

static int
SH(pacmac c12)
{
	int	c1, c2;
	
	frommac(c12, c1, c2);

	if(parag){
		printf(".%c%c",c1,c2);
		while(C != '\n')putchar(c);
		putchar(c);
		putchar('!');
		while(1){
			while(C != '\n')putchar(c);
			putchar('\n');
			if(C == '.')
				return(COM);
			putchar('!');
			putchar(c);
		}
		/*NOTREACHED*/
	} else {
		SKIP_TO_COM;
		return(COMX);
	}
}

static int
UX(void)
{
	if(wordflag)
		printf("UNIX\n");
	else
		printf("UNIX ");
	return(0);
}

static int
MMHU(pacmac c12)
{
	int	c1, c2;
	
	frommac(c12, c1, c2);
	if(parag){
		printf(".%c%c",c1,c2);
		while(C != '\n')putchar(c);
		putchar('\n');
	} else {
		SKIP;
	}
	return(0);
}

static int
mesnblock(pacmac c12)
{
	int	c1, c2;
	
	frommac(c12, c1, c2);
	noblock(')',c2);
	return(0);
}
static int
mssnblock(pacmac c12)
{
	int	c1, c2;
	
	frommac(c12, c1, c2);
	noblock(c1,'E');
	return(0);
}	
static int
nf(void)
{
	noblock('f','i');
	return(0);
}

static int
ce(void)
{
	sce();
	return(0);
}

static int
meip(pacmac c12)
{
	if(parag)
		mepp(c12);
	else if (wordflag)	/* save the tag */
		regline(meputmac, ONE);
	else {
		SKIP;
	}
	return(0);
}
/*
 *	only called for -me .pp or .sh, when parag is on
 */
static int
mepp(pacmac c12)
{
	PP(c12);		/* eats the line */
	return(0);
}
/* 
 *	Start of a section heading; output the section name if doing words
 */
static int
mesh(pacmac c12)
{
	if (parag)
		mepp(c12);
	else if (wordflag)
		defcomline(c12);
	else {
		SKIP;
	}
	return(0);
}
/*
 *	process a font setting
 */
static int
mefont(pacmac c12)
{
	argconcat = 1;
	defcomline(c12);
	argconcat = 0;
	return(0);
}
static int
manfont(pacmac c12)
{
	return(mefont(c12));
}
static int
manpp(pacmac c12)
{
	return(mepp(c12));
}

static void
defcomline(pacmac c12)
{
	int	c1, c2;
	
	frommac(c12, c1, c2);
	if(msflag && mac==MM && c2=='L'){
		if(disp || c1 == 'R') {
			noblock('L','E');
		} else {
			SKIP;
			putchar('.');
		}
	}
	else if(c1=='.' && c2=='.'){
		if(msflag){
			SKIP;
			return;
		}
		while(C == '.')
			/*VOID*/;
	}
	++inmacro;
	/*
	 *	Process the arguments to the macro
	 */
	switch(mac){
	default:
	case MM:
	case MS:
		if(c1 <= 'Z' && msflag)
			regline(msputmac, ONE);
		else
			regline(msputmac, TWO);
		break;
	case ME:
		regline(meputmac, ONE);
		break;
	}
	--inmacro;
}

static void
comline(void)
{
	reg	int	c1;
	reg	int	c2;
		pacmac	c12;
	reg	int	mid;
		int	lb, ub;
		int	hit;
	static	int	tabsize = 0;
	static	struct	mactab	*mactab = (struct mactab *)0;
	reg	struct	mactab	*mp;

	if (mactab == 0){
		 buildtab(&mactab, &tabsize);
	}
com:
	while(C==' ' || c=='\t')
		;
comx:
	if( (c1=c) == '\n')
		return;
	c2 = C;
	if(c1=='.' && c2 !='.')
		inmacro = NO;
	if(msflag && c1 == '['){
		refer(c2);
		return;
	}
	if(parag && mac==MM && c1 == 'P' && c2 == '\n'){
		printf(".P\n");
		return;
	}
	if(c2 == '\n')
		return;
	/*
	 *	Single letter macro
	 */
	if (mac == ME && (c2 == ' ' || c2 == '\t') )
		c2 = ' ';
	c12 = tomac(c1, c2);
	/*
	 *	binary search through the table of macros
	 */
	lb = 0;
	ub = tabsize - 1;
	while(lb <= ub){
		mid = (ub + lb) / 2;
		mp = &mactab[mid];
		if (mp->macname < c12)
			lb = mid + 1;
		else if (mp->macname > c12)
			ub = mid - 1;
		else {
			hit = 1;
#ifdef FULLDEBUG
			printf("preliminary hit macro %c%c ", c1, c2);
#endif	/* FULLDEBUG */
			switch(mp->condition){
			case NONE:	hit = YES;			break;
			case FNEST:	hit = (filesc == 0);		break;
			case NOMAC:	hit = !inmacro;			break;
			case MAC:	hit = inmacro;			break;
			case PARAG:	hit = parag;			break;
			case NBLK:	hit = !keepblock;		break;
			default:	hit = 0;
			}
			if (hit) {
#ifdef FULLDEBUG
				printf("MATCH\n");
#endif	/* FULLDEBUG */
				switch( (*(mp->func))(c12) ) {
				default: 	return;
				case COMX:	goto comx;
				case COM:	goto com;
				}
			}
#ifdef FULLDEBUG
			printf("FAIL\n");
#endif	/* FULLDEBUG */
			break;
		}
	}
	defcomline(c12);
}

static int
macsort(const void *v1, const void *v2)
{
	const struct	mactab	*p1 = v1, *p2 = v2;
	return(p1->macname - p2->macname);
}

static int
sizetab(reg struct mactab *mp)
{
	reg	int	i;
	i = 0;
	if (mp){
		for (; mp->macname; mp++, i++)
			/*VOID*/ ;
	}
	return(i);
}

static struct mactab *
macfill(reg struct mactab *dst, reg struct mactab *src)
{
	if (src) {
		while(src->macname){
			*dst++ = *src++;
		}
	}
	return(dst);
}

static void
buildtab(struct mactab **r_back, int *r_size)
{
	int	size;

	struct	mactab	*p, *p1, *p2;
	struct	mactab	*back;

	size = sizetab(troffmactab);
	size += sizetab(ppmactab);
	p1 = p2 = (struct mactab *)0;
	if (msflag){
		switch(mac){
		case ME:	p1 = memactab; break;
		case MM:	p1 = msmactab;
				p2 = mmmactab; break;

		case MS:	p1 = msmactab; break;
		case MA:	p1 = manmactab; break;
		default:	break;
		}
	}
	size += sizetab(p1);
	size += sizetab(p2);
	back = (struct mactab *)calloc(size+2, sizeof(struct mactab));

	p = macfill(back, troffmactab);
	p = macfill(p, ppmactab);
	p = macfill(p, p1);
	p = macfill(p, p2);

	qsort(back, size, sizeof(struct mactab), macsort);
	*r_size = size;
	*r_back = back;
}

/*
 *	troff commands
 */
struct	mactab	troffmactab[] = {
	M(NONE,		'\\','"',	skip),	/* comment */
	M(NOMAC,	'd','e',	domacro),	/* define */
	M(NOMAC,	'i','g',	domacro),	/* ignore till .. */
	M(NOMAC,	'a','m',	domacro),	/* append macro */
	M(NBLK,		'n','f',	nf),	/* filled */
	M(NBLK,		'c','e',	ce),	/* centered */

	M(NONE,		's','o',	so),	/* source a file */
	M(NONE,		'n','x',	nx),	/* go to next file */

	M(NONE,		't','m',	skip),	/* print string on tty */
	M(NONE,		'h','w',	skip),	/* exception hyphen words */
	M(NONE,		0,0,		0)
};
/*
 *	Preprocessor output
 */
struct	mactab	ppmactab[] = {
	M(FNEST,	'E','Q',	EQ),	/* equation starting */
	M(FNEST,	'T','S',	intbl),	/* table starting */
	M(FNEST,	'T','C',	intbl),	/* alternative table? */
	M(FNEST,	'T','&',	intbl),	/* table reformatting */
	M(NONE,		'T','E',	outtbl),/* table ending */
	M(NONE,		'P','S',	PS),	/* picture starting */
	M(NONE,		0,0,		0)
};
/*
 *	Particular to ms and mm
 */
struct	mactab	msmactab[] = {
	M(NONE,		'T','L',	skiptocom),	/* title follows */
	M(NONE,		'F','S',	skiptocom),	/* start footnote */
	M(NONE,		'O','K',	skiptocom),	/* Other kws */

	M(NONE,		'N','R',	skip),	/* undocumented */
	M(NONE,		'N','D',	skip),	/* use supplied date */

	M(PARAG,	'P','P',	PP),	/* begin parag */
	M(PARAG,	'I','P',	PP),	/* begin indent parag, tag x */
	M(PARAG,	'L','P',	PP),	/* left blocked parag */

	M(NONE,		'A','U',	AU),	/* author */
	M(NONE,		'A','I',	AU),	/* authors institution */

	M(NONE,		'S','H',	SH),	/* section heading */
	M(NONE,		'S','N',	SH),	/* undocumented */
	M(NONE,		'U','X',	UX),	/* unix */

	M(NBLK,		'D','S',	mssnblock),	/* start display text */
	M(NBLK,		'K','S',	mssnblock),	/* start keep */
	M(NBLK,		'K','F',	mssnblock),	/* start float keep */
	M(NONE,		0,0,		0)
};

struct	mactab	mmmactab[] = {
	M(NONE,		'H',' ',	MMHU),	/* -mm ? */
	M(NONE,		'H','U',	MMHU),	/* -mm ? */
	M(PARAG,	'P',' ',	PP),	/* paragraph for -mm */
	M(NBLK,		'N','S',	mssnblock),	/* undocumented */
	M(NONE,		0,0,		0)
};

struct	mactab	memactab[] = {
	M(PARAG,	'p','p',	mepp),
	M(PARAG,	'l','p',	mepp),
	M(PARAG,	'n','p',	mepp),
	M(NONE,		'i','p',	meip),

	M(NONE,		's','h',	mesh),
	M(NONE,		'u','h',	mesh),

	M(NBLK,		'(','l',	mesnblock),
	M(NBLK,		'(','q',	mesnblock),
	M(NBLK,		'(','b',	mesnblock),
	M(NBLK,		'(','z',	mesnblock),
	M(NBLK,		'(','c',	mesnblock),

	M(NBLK,		'(','d',	mesnblock),
	M(NBLK,		'(','f',	mesnblock),
	M(NBLK,		'(','x',	mesnblock),

	M(NONE,		'r',' ',	mefont),
	M(NONE,		'i',' ',	mefont),
	M(NONE,		'b',' ',	mefont),
	M(NONE,		'u',' ',	mefont),
	M(NONE,		'q',' ',	mefont),
	M(NONE,		'r','b',	mefont),
	M(NONE,		'b','i',	mefont),
	M(NONE,		'b','x',	mefont),
	M(NONE,		0,0,		0)
};


struct	mactab	manmactab[] = {
	M(PARAG,	'B','I',	manfont),
	M(PARAG,	'B','R',	manfont),
	M(PARAG,	'I','B',	manfont),
	M(PARAG,	'I','R',	manfont),
	M(PARAG,	'R','B',	manfont),
	M(PARAG,	'R','I',	manfont),

	M(PARAG,	'P','P',	manpp),
	M(PARAG,	'L','P',	manpp),
	M(PARAG,	'H','P',	manpp),
	M(NONE,		0,0,		0)
};

static void *
srealloc(void *vp, size_t size)
{
	if ((vp = realloc(vp, size)) == NULL) {
		write(2, "no memory\n", 10);
		_exit(077);
	}
	return vp;
}
