%{
/*	from 4.4BSD /usr/src/usr.bin/bc/bc.y	*/
/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This module is believed to contain source code proprietary to AT&T.
 * Use and redistribution is subject to the Berkeley Software License
 * Agreement and your Software Agreement with AT&T (Western Electric).
 *
 *	from bc.y	8.1 (Berkeley) 6/6/93
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
static const char sccsid[] USED = "@(#)bc.sl	1.24 (gritter) 7/3/05";
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
typedef	intptr_t	YYSTYPE;
#define	YYSTYPE	YYSTYPE
	static int cpeek(int c, int yes, int no);
	static int getch(void);
	static intptr_t bundle(int a, ...);
	static void routput(intptr_t *p);
	static void output(intptr_t *p);
	static void conout(intptr_t p, intptr_t s);
	static void pp(intptr_t);
	static void tp(intptr_t);
	static void yyinit(int argc, char *argv[]);
	static intptr_t *getout(void);
	static intptr_t *getf(intptr_t);
	static intptr_t *geta(intptr_t);
	static void yyerror(const char *);
	static void cantopen(const char *);
	extern int yylex(void);

#if defined (__GLIBC__) && defined (_IO_getc_unlocked)
#undef	getc
#define	getc(f)		_IO_getc_unlocked(f)
#endif
%}
%right '='
%left '+' '-'
%left '*' '/' '%'
%right '^'
%left UMINUS

%term LETTER DIGIT SQRT LENGTH _IF  FFF EQ
%term _WHILE _FOR NE LE GE INCR DECR
%term _RETURN _BREAK _DEFINE BASE OBASE SCALE
%term EQPL EQMI EQMUL EQDIV EQREM EQEXP
%term _AUTO DOT
%term QSTR

%{
#define	THIS_BC_STRING_MAX	1000
static FILE *in;
static char cary[LINE_MAX + 1], *cp = { cary };
static char string[THIS_BC_STRING_MAX + 3], *str = {string};
static int crs = '0';
static int rcrs = '0';  /* reset crs */
static int bindx = 0;
static int lev = 0;
static int ln;
static char *ss;
static int bstack[10] = { 0 };
static char *numb[15] = {
  " 0", " 1", " 2", " 3", " 4", " 5",
  " 6", " 7", " 8", " 9", " 10", " 11",
  " 12", " 13", " 14" };
static intptr_t *pre, *post;
%}
%%
start	: 
	|  start stat tail
		{ output( (intptr_t *)$2 );}
	|  start def dargs ')' '{' dlist slist '}'
		{	bundle( 6,pre, $7, post ,"0",numb[lev],"Q");
			conout( $$, $2 );
			rcrs = crs;
			output( (intptr_t *)"" );
			lev = bindx = 0;
			}
	;

dlist	:  tail
	| dlist _AUTO dlets tail
	;

stat	:  e 
		{ bundle(2, $1, "ps." ); }
	| 
		{ bundle(1, "" ); }
	|  QSTR
		{ bundle(3,"[",$1,"]P");}
	|  LETTER '=' e
		{ bundle(3, $3, "s", $1 ); }
	|  LETTER '[' e ']' '=' e
		{ bundle(4, $6, $3, ":", geta($1)); }
	|  LETTER EQOP e
		{ bundle(6, "l", $1, $3, $2, "s", $1 ); }
	|  LETTER '[' e ']' EQOP e
		{ bundle(8,$3, ";", geta($1), $6, $5, $3, ":", geta($1));}
	|  _BREAK
		{ bundle(2, numb[lev-bstack[bindx-1]], "Q" ); }
	|  _RETURN '(' e ')'
		{ bundle(4, $3, post, numb[lev], "Q" ); }
	|  _RETURN '(' ')'
		{ bundle(4, "0", post, numb[lev], "Q" ); }
	| _RETURN
		{ bundle(4,"0",post,numb[lev],"Q"); }
	| SCALE '=' e
		{ bundle(2, $3, "k"); }
	| SCALE EQOP e
		{ bundle(4,"K",$3,$2,"k"); }
	| BASE '=' e
		{ bundle(2,$3, "i"); }
	| BASE EQOP e
		{ bundle(4,"I",$3,$2,"i"); }
	| OBASE '=' e
		{ bundle(2,$3,"o"); }
	| OBASE EQOP e
		{ bundle(4,"O",$3,$2,"o"); }
	|  '{' slist '}'
		{ $$ = $2; }
	|  FFF
		{ bundle(1,"fY"); }
	|  error
		{ bundle(1,"c"); }
	|  _IF CRS BLEV '(' re ')' stat
		{	conout( $7, $2 );
			bundle(3, $5, $2, " " );
			}
	|  _WHILE CRS '(' re ')' stat BLEV
		{	bundle(3, $6, $4, $2 );
			conout( $$, $2 );
			bundle(3, $4, $2, " " );
			}
	|  fprefix CRS re ';' e ')' stat BLEV
		{	bundle(5, $7, $5, "s.", $3, $2 );
			conout( $$, $2 );
			bundle(5, $1, "s.", $3, $2, " " );
			}
	|  '~' LETTER '=' e
		{	bundle(3,$4,"S",$2); }
	;

EQOP	:  EQPL
		{ $$ = (intptr_t)"+"; }
	|  EQMI
		{ $$ = (intptr_t)"-"; }
	|  EQMUL
		{ $$ = (intptr_t)"*"; }
	|  EQDIV
		{ $$ = (intptr_t)"/"; }
	|  EQREM
		{ $$ = (intptr_t)"%%"; }
	|  EQEXP
		{ $$ = (intptr_t)"^"; }
	;

fprefix	:  _FOR '(' e ';'
		{ $$ = $3; }
	;

BLEV	:
		{ --bindx; }
	;

slist	:  stat
	|  slist tail stat
		{ bundle(2, $1, $3 ); }
	;

tail	:  '\n'
		{ln++;}
	|  ';'
	;

re	:  e EQ e
		{ bundle(3, $1, $3, "=" ); }
	|  e '<' e
		{ bundle(3, $1, $3, ">" ); }
	|  e '>' e
		{ bundle(3, $1, $3, "<" ); }
	|  e NE e
		{ bundle(3, $1, $3, "!=" ); }
	|  e GE e
		{ bundle(3, $1, $3, "!>" ); }
	|  e LE e
		{ bundle(3, $1, $3, "!<" ); }
	|  e
		{ bundle(2, $1, " 0!=" ); }
	;

e	:  e '+' e
		{ bundle(3, $1, $3, "+" ); }
	|  e '-' e
		{ bundle(3, $1, $3, "-" ); }
	| '-' e		%prec UMINUS
		{ bundle(3, " 0", $2, "-" ); }
	|  e '*' e
		{ bundle(3, $1, $3, "*" ); }
	|  e '/' e
		{ bundle(3, $1, $3, "/" ); }
	|  e '%' e
		{ bundle(3, $1, $3, "%%" ); }
	|  e '^' e
		{ bundle(3, $1, $3, "^" ); }
	|  LETTER '[' e ']'
		{ bundle(3,$3, ";", geta($1)); }
	|  LETTER INCR
		{ bundle(4, "l", $1, "d1+s", $1 ); }
	|  INCR LETTER
		{ bundle(4, "l", $2, "1+ds", $2 ); }
	|  DECR LETTER
		{ bundle(4, "l", $2, "1-ds", $2 ); }
	|  LETTER DECR
		{ bundle(4, "l", $1, "d1-s", $1 ); }
	| LETTER '[' e ']' INCR
		{ bundle(7,$3,";",geta($1),"d1+",$3,":",geta($1)); }
	| INCR LETTER '[' e ']'
		{ bundle(7,$4,";",geta($2),"1+d",$4,":",geta($2)); }
	| LETTER '[' e ']' DECR
		{ bundle(7,$3,";",geta($1),"d1-",$3,":",geta($1)); }
	| DECR LETTER '[' e ']'
		{ bundle(7,$4,";",geta($2),"1-d",$4,":",geta($2)); }
	| SCALE INCR
		{ bundle(1,"Kd1+k"); }
	| INCR SCALE
		{ bundle(1,"K1+dk"); }
	| SCALE DECR
		{ bundle(1,"Kd1-k"); }
	| DECR SCALE
		{ bundle(1,"K1-dk"); }
	| BASE INCR
		{ bundle(1,"Id1+i"); }
	| INCR BASE
		{ bundle(1,"I1+di"); }
	| BASE DECR
		{ bundle(1,"Id1-i"); }
	| DECR BASE
		{ bundle(1,"I1-di"); }
	| OBASE INCR
		{ bundle(1,"Od1+o"); }
	| INCR OBASE
		{ bundle(1,"O1+do"); }
	| OBASE DECR
		{ bundle(1,"Od1-o"); }
	| DECR OBASE
		{ bundle(1,"O1-do"); }
	|  LETTER '(' cargs ')'
		{ bundle(4, $3, "l", getf($1), "x" ); }
	|  LETTER '(' ')'
		{ bundle(3, "l", getf($1), "x" ); }
	|  cons
		{ bundle(2, " ", $1 ); }
	|  DOT cons
		{ bundle(2, " .", $2 ); }
	|  cons DOT cons
		{ bundle(4, " ", $1, ".", $3 ); }
	|  cons DOT
		{ bundle(3, " ", $1, "." ); }
	|  DOT
		{ $$ = (intptr_t)"l."; }
	|  LETTER
		{ bundle(2, "l", $1 ); }
	|  LETTER '=' e
		{ bundle(3, $3, "ds", $1 ); }
	|  LETTER EQOP e	%prec '='
		{ bundle(6, "l", $1, $3, $2, "ds", $1 ); }
	| LETTER '[' e ']' '=' e
		{ bundle(5,$6,"d",$3,":",geta($1)); }
	| LETTER '[' e ']' EQOP e
		{ bundle(9,$3,";",geta($1),$6,$5,"d",$3,":",geta($1)); }
	| LENGTH '(' e ')'
		{ bundle(2,$3,"Z"); }
	| SCALE '(' e ')'
		{ bundle(2,$3,"X"); }	/* must be before '(' e ')' */
	|  '(' e ')'
		{ $$ = $2; }
	|  '?'
		{ bundle(1, "?" ); }
	|  SQRT '(' e ')'
		{ bundle(2, $3, "v" ); }
	| '~' LETTER
		{ bundle(2,"L",$2); }
	| SCALE '=' e
		{ bundle(2,$3,"dk"); }
	| SCALE EQOP e		%prec '='
		{ bundle(4,"K",$3,$2,"dk"); }
	| BASE '=' e
		{ bundle(2,$3,"di"); }
	| BASE EQOP e		%prec '='
		{ bundle(4,"I",$3,$2,"di"); }
	| OBASE '=' e
		{ bundle(2,$3,"do"); }
	| OBASE EQOP e		%prec '='
		{ bundle(4,"O",$3,$2,"do"); }
	| SCALE
		{ bundle(1,"K"); }
	| BASE
		{ bundle(1,"I"); }
	| OBASE
		{ bundle(1,"O"); }
	;

cargs	:  eora
	|  cargs ',' eora
		{ bundle(2, $1, $3 ); }
	;
eora:	  e
	| LETTER '[' ']'
		{bundle(2,"l",geta($1)); }
	;

cons	:  constant
		{ *cp++ = '\0'; }

constant:
	  '_'
		{ $$ = (intptr_t)cp; *cp++ = '_'; }
	|  DIGIT
		{ $$ = (intptr_t)cp; *cp++ = $1; }
	|  constant DIGIT
		{ *cp++ = $2; }
	;

CRS	:
		{ $$ = (intptr_t)cp; *cp++ = crs++; *cp++ = '\0';
			if(crs == '[')crs+=3;
			if(crs == 'a')crs='{';
			if(crs >= 0241){yyerror("program too big");
				getout();
			}
			bstack[bindx++] = lev++; }
	;

def	:  _DEFINE LETTER '('
		{	$$ = (intptr_t)getf($2);
			pre = (intptr_t *)"";
			post = (intptr_t *)"";
			lev = 1;
			bstack[bindx=0] = 0;
			}
	;

dargs	:
	|  lora
		{ pp( $1 ); }
	|  dargs ',' lora
		{ pp( $3 ); }
	;

dlets	:  lora
		{ tp($1); }
	|  dlets ',' lora
		{ tp($3); }
	;
lora	:  LETTER
	|  LETTER '[' ']'
		{ $$ = (intptr_t)geta($1); }
	;

%%
# define error 256

static int peekc = -1;
static int sargc;
static int ifile;
static char **sargv;

static char funtab[52] = {
	01,0,02,0,03,0,04,0,05,0,06,0,07,0,010,0,011,0,012,0,013,0,014,0,015,0,016,0,017,0,
	020,0,021,0,022,0,023,0,024,0,025,0,026,0,027,0,030,0,031,0,032,0 };
static char atab[52] = {
	0241,0,0242,0,0243,0,0244,0,0245,0,0246,0,0247,0,0250,0,0251,0,0252,0,0253,0,
	0254,0,0255,0,0256,0,0257,0,0260,0,0261,0,0262,0,0263,0,0264,0,0265,0,0266,0,
	0267,0,0270,0,0271,0,0272,0};
static char *letr[26] = {
  "a","b","c","d","e","f","g","h","i","j",
  "k","l","m","n","o","p","q","r","s","t",
  "u","v","w","x","y","z" } ;
/*static char *dot = { "." };*/

int
yylex(void){
	int c, ch;
restart:
	c = getch();
	peekc = -1;
	while( c == ' ' || c == '\t' ) c = getch();
	if(c == '\\'){
		getch();
		goto restart;
	}
	if( c<= 'z' && c >= 'a' ) {
		/* look ahead to look for reserved words */
		peekc = getch();
		if( peekc >= 'a' && peekc <= 'z' ){ /* must be reserved word */
			if( c=='i' && peekc=='f' ){ c=_IF; goto skip; }
			if( c=='w' && peekc=='h' ){ c=_WHILE; goto skip; }
			if( c=='f' && peekc=='o' ){ c=_FOR; goto skip; }
			if( c=='s' && peekc=='q' ){ c=SQRT; goto skip; }
			if( c=='r' && peekc=='e' ){ c=_RETURN; goto skip; }
			if( c=='b' && peekc=='r' ){ c=_BREAK; goto skip; }
			if( c=='d' && peekc=='e' ){ c=_DEFINE; goto skip; }
			if( c=='s' && peekc=='c' ){ c= SCALE; goto skip; }
			if( c=='b' && peekc=='a' ){ c=BASE; goto skip; }
			if( c=='i' && peekc == 'b'){ c=BASE; goto skip; }
			if( c=='o' && peekc=='b' ){ c=OBASE; goto skip; }
			if( c=='d' && peekc=='i' ){ c=FFF; goto skip; }
			if( c=='a' && peekc=='u' ){ c=_AUTO; goto skip; }
			if( c == 'l' && peekc=='e'){ c=LENGTH; goto skip; }
			if( c == 'q' && peekc == 'u'){getout();}
			/* could not be found */
			return( error );
		skip:	/* skip over rest of word */
			peekc = -1;
			while( (ch = getch()) >= 'a' && ch <= 'z' );
			peekc = ch;
			return( c );
		}

		/* usual case; just one single letter */

		yylval = (intptr_t)letr[c-'a'];
		return( LETTER );
	}
	if( c>= '0' && c <= '9' || c>= 'A' && c<= 'F' ){
		yylval = c;
		return( DIGIT );
	}
	switch( c ){
	case '.':	return( DOT );
	case '=':
		switch( peekc = getch() ){
		case '=': c=EQ; goto gotit;
		case '+': c=EQPL; goto gotit;
		case '-': c=EQMI; goto gotit;
		case '*': c=EQMUL; goto gotit;
		case '/': c=EQDIV; goto gotit;
		case '%': c=EQREM; goto gotit;
		case '^': c=EQEXP; goto gotit;
		default:   return( '=' );
			  gotit:     peekc = -1; return(c);
		  }
	case '+':	return( cpeek( '+', INCR, cpeek( '=', EQPL, '+') ) );
	case '-':	return( cpeek( '-', DECR, cpeek( '=', EQMI, '-') ) ) ;
	case '<':	return( cpeek( '=', LE, '<' ) );
	case '>':	return( cpeek( '=', GE, '>' ) );
	case '!':	return( cpeek( '=', NE, '!' ) );
	case '/':
		if((peekc = getch()) == '*'){
			peekc = -1;
			while((getch() != '*') || ((peekc = getch()) != '/'));
			peekc = -1;
			goto restart;
		}
		else if (peekc == '=') {
			c=EQDIV;
			goto gotit;
		}
		else return(c);
	case '*':
		return( cpeek( '=', EQMUL, '*' ) );
	case '%':
		return( cpeek( '=', EQREM, '%' ) );
	case '^':
		return( cpeek( '=', EQEXP, '^' ) );
	case '"':	
		 yylval = (intptr_t)str;
		 while((c=getch()) != '"'){*str++ = c;
			if(str >= &string[sizeof string - 1]){yyerror("string space exceeded");
			getout();
		}
	}
	 *str++ = '\0';
	return(QSTR);
	default:	 return( c );
	}
}

static int
cpeek(int c, int yes, int no){
	if( (peekc=getch()) != c ) return( no );
	else {
		peekc = -1;
		return( yes );
	}
}

static int
getch(void){
	int ch;
loop:
	ch = (peekc < 0) ? getc(in) : peekc;
	peekc = -1;
	if(ch != EOF)return(ch);
	if(++ifile > sargc){
		if(ifile >= sargc+2)getout();
		in = stdin;
		ln = 0;
		goto loop;
	}
	fclose(in);
	if((in = fopen(sargv[ifile],"r")) != NULL){
		ln = 0;
		ss = sargv[ifile];
		goto loop;
	}
	cantopen(sargv[ifile]);
	return EOF;
}
# define b_sp_max 3000
static intptr_t b_space [ b_sp_max ];
static intptr_t * b_sp_nxt = { b_space };

static int	bdebug = 0;

static intptr_t
bundle(int a, ...){
	intptr_t i, *q;
	va_list ap;

	i = a;
	q = b_sp_nxt;
	if( bdebug ) printf("bundle %ld elements at %lo\n",(long)i,  (long)q );
	va_start(ap, a);
	while(i-- > 0){
		if( b_sp_nxt >= & b_space[b_sp_max] ) yyerror( "bundling space exceeded" );
		* b_sp_nxt++ = va_arg(ap, intptr_t);
	}
	va_end(ap);
	* b_sp_nxt++ = 0;
	yyval = (intptr_t)q;
	return( (intptr_t)q );
}

static void
routput(intptr_t *p) {
	if( bdebug ) printf("routput(%lo)\n", (long)p );
	if( p >= &b_space[0] && p < &b_space[b_sp_max]){
		/* part of a bundle */
		while( *p != 0 ) routput( (intptr_t *)*p++ );
	}
	else printf( (char *)p );	 /* character string */
}

static void
output(intptr_t *p) {
	routput( p );
	b_sp_nxt = & b_space[0];
	printf( "\n" );
	fflush(stdout);
	cp = cary;
	crs = rcrs;
}

static void
conout(intptr_t p, intptr_t s) {
	printf("[");
	routput( (intptr_t *)p );
	printf("]s%s\n", (char *)s );
	fflush(stdout);
	lev--;
}

static void
yyerror(const char *s) {
	if(ifile > sargc)ss="teletype";
	fprintf(stderr, "%s on line %d, %s\n",
		s ,ss?ln+1:0,ss?ss:"command line");
	cp = cary;
	crs = rcrs;
	bindx = 0;
	lev = 0;
	b_sp_nxt = &b_space[0];
}

static void
cantopen(const char *fn)
{
	char	spc[280];
	char	*oss = ss;

	ss = 0;
	snprintf(spc, sizeof spc, "can't open input file %s", fn);
	yyerror(spc);
	ss = oss;
}

static void
pp(intptr_t s) {
	/* puts the relevant stuff on pre and post for the letter s */

	bundle(3, "S", s, pre );
	pre = (intptr_t *)yyval;
	bundle(4, post, "L", s, "s." );
	post = (intptr_t *)yyval;
}

static void
tp(intptr_t s) { /* same as pp, but for temps */
	bundle(3, "0S", s, pre );
	pre = (intptr_t *)yyval;
	bundle(4, post, "L", s, "s." );
	post = (intptr_t *)yyval;
}

static void
yyinit(int argc,char **argv) {
	signal(SIGINT, SIG_IGN);
	sargv=argv;
	sargc= -- argc;
	if(sargc == 0)in=stdin;
	else if((in = fopen(sargv[1],"r")) == NULL) {
		cantopen(sargv[1]);
		exit(0);
	}
	ifile = 1;
	ln = 0;
	ss = sargv[1];
}

static intptr_t *
getout(void){
	printf("q");
	fflush(stdout);
	exit(0);
	/*NOTREACHED*/
	return(NULL);
}

static intptr_t *
getf(intptr_t p) {
	return(intptr_t *)(&funtab[2*(*((char *)p) -0141)]);
}

static intptr_t *
geta(intptr_t p) {
	return(intptr_t *)(&atab[2*(*((char *)p) - 0141)]);
}

int
main(int argc, char **argv)
{
	extern int yyparse(void);
	const char optstring[] = "cdl";
	int p[2];
	int i;
	int cflag = 0, lflag = 0;


#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	while ((i = getopt(argc, argv, optstring)) != EOF) {
		switch (i) {
		case 'd':
		case 'c':
			cflag = 1;
			break;
		case 'l':
			lflag = 1;
			break;
		default:
			exit(2);
		}
	}
	argv += optind - 1, argc -= optind - 1;
	if (cflag) {
		yyinit(argc, argv);
		yyparse();
		exit(0);
	}
	if (lflag) {
		*argv-- = LIBB;
		argc++;
	}
	pipe(p);
	if (fork()==0) {
		close(1);
		dup(p[1]);
		close(p[0]);
		close(p[1]);
		yyinit(argc, argv);
		yyparse();
		exit(0);
	}
	close(0);
	dup(p[0]);
	close(p[0]);
	close(p[1]);
	execl(DC, "dc", "-", NULL);
	execl("/usr/5bin/dc", "dc", "-", NULL);
	execl("/usr/local/bin/dc", "dc", "-", NULL);
	execl("/usr/contrib/bin/dc", "dc", "-", NULL);
	execl("/usr/bin/dc", "dc", "-", NULL);
	return(1);
}
