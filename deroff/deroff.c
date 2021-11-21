/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, May 2004.
 *
 * Derived from Plan 9 v4 /sys/src/cmd/deroff.c:
 *
 * Copyright (C) 2003, Lucent Technologies Inc. and others.
 * All Rights Reserved.
 *
 * Distributed under the terms of the Lucent Public License Version 1.02.
 */

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)deroff.sl	1.9 (gritter) 9/23/06";

#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>
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
 * Deroff command -- strip troff, eqn, and tbl sequences from
 * a file.  Has three flags argument, -w, to cause output one word per line
 * rather than in the original format.
 * -mm (or -ms) causes the corresponding macro's to be interpreted
 * so that just sentences are output
 * -ml  also gets rid of lists.
 * -i causes deroff to ignore .so and .nx commands.
 * Deroff follows .so and .nx commands, removes contents of macro
 * definitions, equations (both .EQ ... .EN and $...$),
 * Tbl command sequences, and Troff backslash vconstructions.
 * 
 * All input is through the C macro; the most recently read character is in c.
 */

/*
#define	C	((c = Bgetrune(infile)) < 0?\
			eof():\
			((c == ldelim) && (filesp == files)?\
				skeqn():\
				(c == '\n'?\
					(linect++,c):\
						c)))

#define	C1	((c = Bgetrune(infile)) == Beof?\
			eof():\
			(c == '\n'?\
				(linect++,c):\
				c))
*/

/* lose those macros! */
#define	C	fC()
#define	C1	fC1()
#define	U(c)	fU(c)

#define	SKIP	while(C != '\n') 
#define SKIP1	while(C1 != '\n')
#define SKIP_TO_COM		SKIP;\
				SKIP;\
				pc=c;\
				while(C != '.' || pc != '\n' || C > 'Z')\
						pc=c

#define YES		1
#define NO		0
#define MS		0
#define MM		1
#define ONE		1
#define TWO		2

#define NOCHAR		-2
#define	EXTENDED	-1		/* All runes above 0x7F */
#define SPECIAL		0
#define APOS		1
#define PUNCT		2
#define DIGIT		3
#define LETTER		4


static int	linect	= 0;
static int	wordflag= NO;
static int	underscoreflag = NO;
static int	msflag	= NO;
static int	iflag	= NO;
static int	mac	= MM;
static int	disp	= 0;
static int	inmacro	= NO;
static int	intable	= NO;
static int	eqnflag	= 0;
static int	_xflag = 1;
static int	xflag;

#define	MAX_ASCII	256

static char	chars[MAX_ASCII];	/* SPECIAL, PUNCT, APOS, DIGIT, or LETTER */

static char	*line;
static int	linec;
static int	li;

#define	LINEC(n)	(li+(n) >= linec ? \
				line = srealloc(line, linec = li+(n)+1) : \
				line)

static long	c;
static long	pc;
static int	ldelim	= NOCHAR;
static int	rdelim	= NOCHAR;


static char	**argv;

static char	*fname;
static int	fnamec;
static FILE	**files;
static int	filec;
static FILE	**filesp;
static FILE	*infile;

static const char	*progname;
static const char	*Progname;

static const char *const skiprq[] = {
	"fp", "fps", "feature", "fallback", "hidechar", "papersize",
	"mediasize", "cropat", "trimat", "bleedat", "letadj", "track",
	"kernpair", "kernafter", "kernbefore", "lhang", "rhang",
	"substring", "index", "flig", "fdeferlig", "fzoom", "rm", "rn",
	"wh", "dwh", "dt", "it", "itc", "als", "rnm", "aln",
	"nr", "nrf", "af", "warn", "ftr", "tr", "trin", "trnt", "rchar",
	"lc_ctype", "hylang", "sentchar", "transchar", "breakchar",
	NULL
};
static const char *const skip1rq[] = {
	"ft", "ds", "as", "lds", "substring", "length", "index", "chop",
	"di", "da", "box", "boxa", "unformat", "asciify", "ch", "dch",
	"blm", "em", "char", "fchar",
	NULL
};

static long	skeqn(void);
static FILE	*opn(char *p);
static int	eof(void);
static int	charclass(int);
static void	getfname(void);
static void	fatal(char *s, char *p);
static void	usage(void);
static void	work(void);
static void	putmac(char *rp, int vconst);
static void	regline(int macline, int vconst);
static void	putwords(void);
static void	comline(void);
static void	macro(void);
static void	eqn(void);
static void	tbl(void);
static void	stbl(void);
static void	sdis(char a1, char a2);
static void	sce(void);
static void	backsl(void);
static char	*copys(char *s);
static void	refer(int c1);
static void	inpic(void);
static void	*srealloc(void *, size_t);

static int
fC(void)
{
	c = getc(infile);
	if(c == EOF)
		return eof();
	if(c == ldelim && filesp == files)
		return skeqn();
	if(c == '\n') {
		linect++;
		xflag = _xflag;
	}
	return c;
}

static int
fC1(void)
{
	c = getc(infile);
	if(c == EOF)
		return eof();
	if(c == '\n') {
		linect++;
		xflag = _xflag;
	}
	return c;
}

static void
fU(int i)
{
	ungetc(i, infile);
	if(i == '\n')
		linect--;
}

int
main(int argc, char *av[])
{
	int i;

	argv = av;
	progname = basename(argv[0]);
	Progname = srealloc(0, strlen(progname) + 1);
	strcpy((char *)Progname, progname);
	((char *)Progname)[0] = toupper(Progname[0]);
	files = srealloc(files, (filec = 1) * sizeof *files);
	while ((i = getopt(argc, argv, "im:wx:")) != EOF) {
	switch (i) {
	case 'w':
		wordflag = YES;
		break;
	case '_':
		wordflag = YES;
		underscoreflag = YES;
		break;
	case 'm':
		msflag = YES;
		switch(*optarg)
		{
			case 'm':	mac = MM; break;
			case 's':	mac = MS; break;
			case 'l':	disp = 1; break;
			default:	usage();
		}
		break;
	case 'i':
		iflag = YES;
		break;
	case 'x':
		_xflag = atoi(optarg);
		break;
	default:
		usage();
	} }
	if(optind<argc)
		infile = opn(argv[optind++]);
	else
		infile = stdin;
	files[0] = infile;
	filesp = &files[0];

	for(i='a'; i<='z' ; ++i)
		chars[i] = LETTER;
	for(i='A'; i<='Z'; ++i)
		chars[i] = LETTER;
	for(i='0'; i<='9'; ++i)
		chars[i] = DIGIT;
	chars['\''] = APOS;
	chars['&'] = APOS;
	chars['\b'] = APOS;
	chars['.'] = PUNCT;
	chars[','] = PUNCT;
	chars[';'] = PUNCT;
	chars['?'] = PUNCT;
	chars[':'] = PUNCT;
	for (i=0200; i<=0377; ++i)
		chars[i] = LETTER;
	xflag = _xflag;
	work();
	return 0;
}

static long
skeqn(void)
{
	while(C1 != rdelim)
		if(c == '\\')
			c = C1;
		else if(c == '"')
			while(C1 != '"')
				if(c == '\\') 
					C1;
	if (msflag)
		eqnflag = 1;
	return(c = ' ');
}

static FILE *
opn(char *p)
{
	FILE *fd;

	if ((fd = fopen(p, "r")) == NULL)
		fatal("Cannot open file %s\n", p);
	linect = 0;
	return(fd);
}

static int
eof(void)
{
	if(infile != stdin)
		fclose(infile);
	if(filesp > files)
		infile = *--filesp;
	else
	if(argv[optind])
		infile = opn(argv[optind++]);
	else
		exit(0);
	return(C);
}

static void
getfname(void)
{
	int i;
	char r;
	struct chain
	{ 
		struct	chain	*nextp; 
		char	*datap; 
	} *q;

	static struct chain *namechain= 0;

	while(C == ' ')
		;
	for(i = 0; (r=c) != '\n' && r != ' ' && r != '\t' && r != '\\'; C) {
		if (fnamec <= i+1)
			fname = srealloc(fname, fnamec = i+1);
		fname[i] = c;
		i++;
	}
	if (fnamec <= i+1)
		fname = srealloc(fname, fnamec = i+1);
	fname[i] = '\0';
	while(c != '\n')
		C;

	/*
	 * see if this name has already been used
	 */

	for(q = namechain; q; q = q->nextp)
		if( !strcmp(fname, q->datap)) {
			fname[0] = '\0';
			return;
		}
	q = srealloc(0, sizeof *q);
	q->nextp = namechain;
	q->datap = copys(fname);
	namechain = q;
}

static void
usage(void)
{
	fprintf(stderr,
		"%s: usage: %s [ -w ] [ -m (m s l) ] [ -i ] [ file ] ...\n",
			Progname, progname);
	exit(2);
}

static void
fatal(char *s, char *p)
{
	fprintf(stderr, "%s: ", Progname);
	fprintf(stderr, s, p);
	exit(1);
}

static void
work(void)
{

	for(;;) {
		eqnflag = 0;
		if(C == '.'  ||  c == '\'')
			comline();
		else
			regline(NO, TWO);
	}
}

static void
regline(int macline, int vconst)
{
	if (linec == 0)
		line = srealloc(line, linec = 1);
	li = 0;
	LINEC(0);
	line[li] = c;
	for(;;) {
		if(c == '\\') {
			LINEC(0);
			line[li] = ' ';
			backsl();
			if(c == '%')	/* no blank for hyphenation char */
				li--;
		}
		if(c == '\n')
			break;
		if(intable && c=='T') {
			LINEC(1);
			line[++li] = C;
			if(c=='{' || c=='}') {
				LINEC(0);
				line[li-1] = ' ';
				line[li] = C;
			}
		} else {
			if(msflag == 1 && eqnflag == 1) {
				eqnflag = 0;
				LINEC(1);
				line[++li] = 'x';
			}
			LINEC(1);
			line[++li] = C;
		}
	}
	LINEC(0);
	line[li] = '\0';
	if(li != 0) {
		if(wordflag)
			putwords();
		else
		if(macline)
			putmac(line,vconst);
		else
			puts(line);
	}
}

static void
putmac(char *rp, int vconst)
{
	char *t;
	int found;
	char last;

	found = 0;
	last = 0;
	while(*rp) {
		while(*rp == ' ' || *rp == '\t') {
			putchar(*rp & 0377);
			rp++;
		}
		for(t = rp; *t != ' ' && *t != '\t' && *t != '\0'; t++)
			;
		if(*rp == '\"')
			rp++;
		if(t > rp+vconst && charclass(*rp&0377) == LETTER
				&& charclass(rp[1]&0377) == LETTER) {
			while(rp < t)
				if(*rp == '\"')
					rp++;
				else {
					putchar(*rp & 0377);
					rp++;
				}
			last = t[-1];
			found++;
		} else
		if(found && charclass(*rp&0377) == PUNCT && rp[1] == '\0') {
			putchar(*rp & 0377);
			rp++;
		} else {
			last = t[-1];
			rp = t;
		}
	}
	putchar('\n');
	if(msflag && charclass(last&0377) == PUNCT)
		printf(" %c\n", last);
}

/*
 * break into words for -w option
 */
static void
putwords(void)
{
	char *p, *p1;
	int i, nlet;


	for(p1 = line;;) {
		/*
		 * skip initial specials ampersands and apostrophes
		 */
		while((i = charclass(*p1&0377)) != EXTENDED && i < DIGIT)
			if(*p1++ == '\0')
				return;
		nlet = 0;
		for(p = p1; (i = charclass(*p&0377)) != SPECIAL || (underscoreflag && *p=='_'); p++)
			if(i == LETTER || (underscoreflag && *p == '_'))
				nlet++;
		/*
		 * MDM definition of word
		 */
		if(nlet > 1) {
			/*
			 * delete trailing ampersands and apostrophes
			 */
			while(*--p == '\'' || *p == '&'
					   || charclass(*p&0377) == PUNCT)
				;
			while(p1 <= p) {
				putchar(*p1 & 0377);
				p1++;
			}
			putchar('\n');
		} else
			p1 = p;
	}
}

static void
comline(void)
{
	long c1, c2;
	int i, j;
	char cc[4096];

	while(C==' ' || c=='\t')
		;
comx:
	if((c1=c) == '\n')
		return;
	c2 = C;
	cc[0] = c1;
	if (c2 != '\n') {
		cc[1] = c2;
		i = 2;
	} else
		i = 0;
	cc[i] = 0;
	if(xflag > 1 && i == 2) {
		for ( ; i < sizeof cc - 1; i++) {
			if (C1 == ' ' || c == '\n') {
				U(c);
				break;
			}
			cc[i] = c;
		}
		cc[i] = 0;
		if (strcmp(cc, "xflag") == 0) {
			while (C1 == ' ');
			U(c);
			for (i = 0; i < sizeof cc - 1; i++) {
				if (C1 == ' ' || c == '\n') {
					U(c);
					break;
				}
				cc[i] = c;
			}
			cc[i] = 0;
			xflag = _xflag = atoi(cc);
			return;
		}
	}
	for (j = 0; skiprq[j]; j++)
		if (strcmp(cc, skiprq[j]) == 0) {
			SKIP;
			return;
		}
	if (i > 2 && xflag > 2)
		goto mac;
	if(c1=='.' && c2!='.')
		inmacro = NO;
	if(msflag && c1 == '['){
		refer(c2);
		return;
	}
	if(c2 == '\n')
		return;
	if(c1 == '\\' && c2 == '\"')
		SKIP;
	else
	if (filesp==files && c1=='E' && c2=='Q')
			eqn();
	else
	if(filesp==files && c1=='T' && (c2=='S' || c2=='C' || c2=='&')) {
		if(msflag)
			stbl(); 
		else
			tbl();
	}
	else
	if(c1=='T' && c2=='E')
		intable = NO;
	else if (!inmacro &&
			((c1 == 'd' && c2 == 'e') ||
		   	 (c1 == 'i' && c2 == 'g') ||
		   	 (c1 == 'a' && c2 == 'm')))
				macro();
	else
	if(c1=='s' && c2=='o') {
		if(iflag)
			SKIP;
		else {
			getfname();
			if(fname[0]) {
				if(infile = opn(fname))
					*++filesp = infile;
				else infile = *filesp;
			}
		}
	}
	else
	if(c1=='n' && c2=='x')
		if(iflag)
			SKIP;
		else {
			getfname();
			if(fname[0] == '\0')
				exit(0);
			if(infile != stdin)
				fclose(infile);
			infile = *filesp = opn(fname);
		}
	else
	if(c1 == 't' && c2 == 'm')
		SKIP;
	else
	if(c1=='h' && c2=='w')
		SKIP; 
	else
	if(xflag && c1=='d' && c2=='o') {
		xflag = 3;
		comline();
	}
	else
	if(msflag && c1 == 'T' && c2 == 'L') {
		SKIP_TO_COM;
		goto comx; 
	}
	else
	if(msflag && c1=='N' && c2 == 'R')
		SKIP;
	else
	if(msflag && c1 == 'A' && (c2 == 'U' || c2 == 'I')){
		if(mac==MM)SKIP;
		else {
			SKIP_TO_COM;
			goto comx; 
		}
	} else
	if(msflag && c1=='F' && c2=='S') {
		SKIP_TO_COM;
		goto comx; 
	}
	else
	if(msflag && (c1=='S' || c1=='N') && c2=='H') {
		SKIP_TO_COM;
		goto comx; 
	} else
	if(c1 == 'U' && c2 == 'X') {
		if(wordflag)
			puts("UNIX");
		else
			printf("UNIX ");
	} else
	if(msflag && c1=='O' && c2=='K') {
		SKIP_TO_COM;
		goto comx; 
	} else
	if(msflag && c1=='N' && c2=='D')
		SKIP;
	else
	if(msflag && mac==MM && c1=='H' && (c2==' '||c2=='U'))
		SKIP;
	else
	if(msflag && mac==MM && c2=='L') {
		if(disp || c1=='R')
			sdis('L', 'E');
		else {
			SKIP;
			printf(" .");
		}
	} else
	if(!msflag && c1=='P' && c2=='S') {
		inpic();
	} else
	if(msflag && (c1=='D' || c1=='N' || c1=='K'|| c1=='P') && c2=='S') { 
		sdis(c1, 'E'); 
	} else
	if(msflag && (c1 == 'K' && c2 == 'F')) { 
		sdis(c1,'E'); 
	} else
	if(msflag && c1=='n' && c2=='f')
		sdis('f','i');
	else
	if(msflag && c1=='c' && c2=='e')
		sce();
	else {
		if(c1=='.' && c2=='.') {
			if(msflag) {
				SKIP;
				return;
			}
			while(C == '.')
				;
		}
	mac:
		inmacro++;
		if(c1 <= 'Z' && msflag)
			regline(YES,ONE);
		else {
			for (j = 0; skip1rq[j]; j++)
				if (strcmp(cc, skip1rq[j]) == 0) {
					while (C1 == ' ');
					U(c);
					while (C1 != ' ' && c != '\n');
					U(c);
				}
			if(skip1rq[j] == 0 && wordflag)
				C;
			regline(YES,TWO);
		}
		inmacro--;
	}
}

static void
macro(void)
{
	if(msflag) {
		do { 
			SKIP1; 
		} while(C1 != '.' || C1 != '.' || C1 == '.');
		if(c != '\n')
			SKIP;
		return;
	}
	SKIP;
	inmacro = YES;
}

static void
sdis(char a1, char a2)
{
	int c1, c2;
	int eqnf;
	int lct;

	if(a1 == 'P'){
		while(C1 == ' ')
			;
		if(c == '<') {
			SKIP1;
			return;
		}
	}
	lct = 0;
	eqnf = 1;
	if(c != '\n')
		SKIP1;
	for(;;) {
		while(C1 != '.')
			if(c == '\n')
				continue;
			else
				SKIP1;
		if((c1=C1) == '\n')
			continue;
		if((c2=C1) == '\n') {
			if(a1 == 'f' && (c1 == 'P' || c1 == 'H'))
				return;
			continue;
		}
		if(c1==a1 && c2 == a2) {
			SKIP1;
			if(lct != 0){
				lct--;
				continue;
			}
			if(eqnf)
				printf(" .");
			putchar('\n');
			return;
		} else
		if(a1 == 'L' && c2 == 'L') {
			lct++;
			SKIP1;
		} else
		if(a1 == 'D' && c1 == 'E' && c2 == 'Q') {
			eqn(); 
			eqnf = 0;
		} else
		if(a1 == 'f') {
			if((mac == MS && c2 == 'P') ||
				(mac == MM && c1 == 'H' && c2 == 'U')){
				SKIP1;
				return;
			}
			SKIP1;
		}
		else
			SKIP1;
	}
}

static void
tbl(void)
{
	while(C != '.')
		;
	SKIP;
	intable = YES;
}

static void
stbl(void)
{
	while(C != '.')
		;
	SKIP_TO_COM;
	if(c != 'T' || C != 'E') {
		SKIP;
		pc = c;
		while(C != '.' || pc != '\n' || C != 'T' || C != 'E')
			pc = c;
	}
}

static void
eqn(void)
{
	long c1, c2;
	int dflg;
	char last;

	last = 0;
	dflg = 1;
	SKIP;

	for(;;) {
		if(C1 == '.'  || c == '\'') {
			while(C1==' ' || c=='\t')
				;
			if(c=='E' && C1=='N') {
				SKIP;
				if(msflag && dflg) {
					putchar('x');
					putchar(' ');
					if(last) {
						putchar(last & 0377);
						putchar('\n');
					}
				}
				return;
			}
		} else
		if(c == 'd') {
			if(C1=='e' && C1=='l')
				if(C1=='i' && C1=='m') {
					while(C1 == ' ')
						;
					if((c1=c)=='\n' || (c2=C1)=='\n' ||
					  (c1=='o' && c2=='f' && C1=='f')) {
						ldelim = NOCHAR;
						rdelim = NOCHAR;
					} else {
						ldelim = c1;
						rdelim = c2;
					}
				}
			dflg = 0;
		}
		if(c != '\n')
			while(C1 != '\n') { 
				if(chars[c] == PUNCT)
					last = c;
				else
				if(c != ' ')
					last = 0;
			}
	}
}

/*
 * skip over a complete backslash vconstruction
 */
static void
backsl(void)
{
	int bdelim;

sw:  
	switch(C1)
	{
	case '"':
		SKIP1;
		return;

	case 's':
		if(C1 == '\\')
			backsl();
		else if (c == '[' && xflag > 1)
			goto bracket;
		else {
			while(C1>='0' && c<='9')
				;
			ungetc(c, infile);
			c = '0';
		}
		li--;
		return;

	case 'f':
	case 'n':
	case '*':
	case 'g':
	case 'k':
	case 'P':
	case 'V':
	case 'Y':
		if(C1 == '[' && xflag > 1)
			goto bracket;
		if(c != '(')
			return;

	case '(':
		if(msflag) {
			if(C == 'e') {
				if(C1 == 'm') {
					LINEC(0);
					line[li] = '-';
					return;
				}
			} else
			if(c != '\n')
				C1;
			return;
		}
		if(C1 != '\n')
			C1;
		return;

	case '$':
		C1;	/* discard argument number */
		return;

	case '[':
	bracket:
		if (xflag)
			while (C1 != ']' && c != '\n');
		return;

	case 'b':
	case 'x':
	case 'v':
	case 'h':
	case 'w':
	case 'o':
	case 'l':
	case 'L':
	case 'X':
	case 'A':
	case 'B':
	case 'D':
	case 'H':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
		if((bdelim=C1) == '\n')
			return;
		while(C1!='\n' && c!=bdelim)
			if(c == '\\')
				backsl();
		return;

	case '\\':
		if(inmacro)
			goto sw;
	default:
		return;
	}
}

static char *
copys(char *s)
{
	char *t, *t0;

	t0 = t = srealloc(0, (strlen(s)+1));
	while(*t++ = *s++)
		;
	return(t0);
}

static void
sce(void)
{
	int n = 1;

	while (C != '\n' && !('0' <= c && c <= '9'))
		;
	if (c != '\n') {
		for (n = c-'0';'0' <= C && c <= '9';)
			n = n*10 + c-'0';
	}
	while(n) {
		if(C == '.') {
			if(C == 'c') {
				if(C == 'e') {
					while(C == ' ')
						;
					if(c == '0') {
						SKIP;
						break;
					} else
						SKIP;
				} else
					SKIP;
			} else
			if(c == 'P' || C == 'P') {
				if(c != '\n')
					SKIP;
				break;
			} else
				if(c != '\n')
					SKIP;
		} else {
			SKIP;
			n--;
		}
	}
}

static void
refer(int c1)
{
	int c2;

	if(c1 != '\n')
		SKIP;
	c2 = 0;
	for(;;) {
		if(C != '.')
			SKIP;
		else {
			if(C != ']')
				SKIP;
			else {
				while(C != '\n')
					c2 = c;
				if(charclass(c2&0377) == PUNCT)
					printf(" %c", c2);
				return;
			}
		}
	}
}

static void
inpic(void)
{
	int c1;
	char *p1;

/*	SKIP1;*/
	while(C1 != '\n')
		if(c == '<'){
			SKIP1;
			return;
		}
	p1 = line;
	c = '\n';
	for(;;) {
		c1 = c;
		if(C1 == '.' && c1 == '\n') {
			if(C1 != 'P' || C1 != 'E') {
				if(c != '\n'){
					SKIP1;
					c = '\n';
				}
				continue;
			}
			SKIP1;
			return;
		} else
		if(c == '\"') {
			while(C1 != '\"') {
				if(c == '\\') {
					if(C1 == '\"')
						continue;
					ungetc(c, infile);
					backsl();
				} else
					*p1++ = c;
			}
			*p1++ = ' ';
		} else
		if(c == '\n' && p1 != line) {
			*p1 = '\0';
			if(wordflag)
				putwords();
			else
				printf("%s\n\n", line);
			p1 = line;
		}
	}
}

static int
charclass(int c)
{
	if(c < MAX_ASCII)
		return chars[c];
	return -1;
}

static void *
srealloc(void *vp, size_t size)
{
	if ((vp = realloc(vp, size)) == NULL) {
		write(2, Progname, strlen(Progname));
		write(2, ": Cannot allocate memory\n", 25);
		_exit(077);
	}
	return vp;
}
