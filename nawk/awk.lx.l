%{
/*
   Changes by Gunnar Ritter, Freiburg i. Br., Germany, December 2002.
  
   Sccsid @(#)awk.lx.l	1.13 (gritter) 11/22/05>
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
/*
 * flex port partially taken from 4.4BSD awk,
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
 */
/*		copyright	"%c%"	*/

/*	from unixsrc:usr/src/common/cmd/awk/awk.lx.l /main/uw7_nj/1	*/
/*	from RCS Header: awk.lx.l 1.2 91/06/25 	*/

/*%Start A str sc reg comment*/
/*%X A str sc reg comment*/
%}
%X A str reg

%{

#include	"awk.h"
#include	"y.tab.h"
#include	<pfmt.h>
#include	<unistd.h>

static void	awk_unputstr(const char *s);

#ifdef	FLEX_SCANNER
static int	awk_yytchar;
int		awk_input(void);
static void	awk_unput(int c);
#undef	YY_INPUT
#define	YY_INPUT(buf, result, max_size)	{ \
	int	c = awk_input(); \
	result = (c == EOF || c == '\0') ? YY_NULL : (buf[0] = c, 1); \
}
#else	/* !FLEX_SCANNER */
#undef	input	/* defeat lex */
#undef	unput
int		input(void);
void		unput(int c);
#define	awk_unput(c)	unput(c)
#define	awk_yytchar	yytchar
#endif	/* !FLEX_SCANNER */


extern YYSTYPE	yylval;
extern int	infunc;

int	lineno	= 1;
int	bracecnt = 0;
int	brackcnt  = 0;
int	parencnt = 0;
#define DEBUG
#ifdef	DEBUG
#	define	RET(x)	{if(dbg)printf("lex %s [%s]\n", tokname(x), yytext); return(x); }
#else
#	define	RET(x)	return(x)
#endif

#define	CADD	cbuf[clen++] = yytext[0]; \
		if (clen >= CBUFLEN-1) { \
			vyyerror(":90:String/reg expr %.10s ... too long", cbuf); \
			BEGIN INITIAL; \
		}

static const char extra[] = ":91:Extra %c";
extern const char nlstring[];

unsigned char	cbuf[CBUFLEN];
unsigned char	*s;
int	clen, cflag;
%}

A	[a-zA-Z_]
B	[a-zA-Z0-9_]
D	[0-9]
O	[0-7]
H	[0-9a-fA-F]
WS	[ \t]

%%
	static int sc_flag = 0;

	if (sc_flag) {
		BEGIN INITIAL;
		sc_flag = 0;
		RET('}');
	}

\n		{ lineno++; RET(NL); }
#.*		{ ; }	/* strip comments */
{WS}+	{ ; }
<INITIAL,reg>"\\"\n	lineno++;
;		{ RET(';'); }

BEGIN	{ RET(XBEGIN); }
END		{ RET(XEND); }
func(tion)?	{ if (infunc) vyyerror(":92:Illegal nested function"); RET(FUNC); }
return	{ if (!infunc) vyyerror(":93:Return not in function"); RET(RETURN); }
"&&"		{ RET(AND); }
"||"		{ RET(BOR); }
"!"		{ RET(NOT); }
"!="		{ yylval.i = NE; RET(NE); }
"~"		{ yylval.i = MATCH; RET(MATCHOP); }
"!~"		{ yylval.i = NOTMATCH; RET(MATCHOP); }
"<"		{ yylval.i = LT; RET(LT); }
"<="		{ yylval.i = LE; RET(LE); }
"=="		{ yylval.i = EQ; RET(EQ); }
">="		{ yylval.i = GE; RET(GE); }
">"		{ yylval.i = GT; RET(GT); }
">>"		{ yylval.i = APPEND; RET(APPEND); }
"++"		{ yylval.i = INCR; RET(INCR); }
"--"		{ yylval.i = DECR; RET(DECR); }
"+="		{ yylval.i = ADDEQ; RET(ASGNOP); }
"-="		{ yylval.i = SUBEQ; RET(ASGNOP); }
"*="		{ yylval.i = MULTEQ; RET(ASGNOP); }
"/="		{ yylval.i = DIVEQ; RET(ASGNOP); }
"%="		{ yylval.i = MODEQ; RET(ASGNOP); }
"^="		{ yylval.i = POWEQ; RET(ASGNOP); }
"**="	{ yylval.i = POWEQ; RET(ASGNOP); }
"="		{ yylval.i = ASSIGN; RET(ASGNOP); }
"**"		{ RET(POWER); }
"^"		{ RET(POWER); }

"$"{D}+	{ yylval.cp = fieldadr(atoi(yytext+1)); RET(FIELD); }
"$NF"	{ awk_unputstr("(NF)"); return(INDIRECT); }
"$"{A}{B}*	{ int c, n;
		  c = awk_yytchar;
		  if (c == '(' || c == '[' || infunc && (n=isarg(yytext+1)) >= 0) {
			awk_unputstr(yytext+1);
			return(INDIRECT);
		  } else {
			yylval.cp = setsymtab((unsigned char *)yytext+1,"",0.0,STR|NUM,symtab);
			RET(IVAR);
		  }
		}
"$"		{ RET(INDIRECT); }
NF		{ yylval.cp = setsymtab((unsigned char *)yytext, "", 0.0, NUM, symtab); RET(VARNF); }

({D}+("."?){D}*|"."{D}+)((e|E)("+"|-)?{D}+)?	{
		  yylval.cp = setsymtab((unsigned char *)yytext, tostring((unsigned char *)yytext), awk_atof(yytext), CON|NUM, symtab);
		  RET(NUMBER); }

while	{ RET(WHILE); }
for		{ RET(FOR); }
do		{ RET(DO); }
if		{ RET(IF); }
else		{ RET(ELSE); }
next		{ RET(NEXT); }
exit		{ RET(EXIT); }
break	{ RET(BREAK); }
continue	{ RET(CONTINUE); }
print	{ yylval.i = PRINT; RET(PRINT); }
printf	{ yylval.i = PRINTF; RET(PRINTF); }
sprintf	{ yylval.i = SPRINTF; RET(SPRINTF); }
split	{ yylval.i = SPLIT; RET(SPLIT); }
substr	{ RET(SUBSTR); }
sub		{ yylval.i = SUB; RET(SUB); }
gsub		{ yylval.i = GSUB; RET(GSUB); }
index	{ RET(INDEX); }
match	{ RET(MATCHFCN); }
in		{ RET(IN); }
getline	{ RET(GETLINE); }
delete	{ RET(DELETE); }
length	{ yylval.i = FLENGTH; RET(BLTIN); }
log		{ yylval.i = FLOG; RET(BLTIN); }
int		{ yylval.i = FINT; RET(BLTIN); }
exp		{ yylval.i = FEXP; RET(BLTIN); }
sqrt		{ yylval.i = FSQRT; RET(BLTIN); }
sin		{ yylval.i = FSIN; RET(BLTIN); }
cos		{ yylval.i = FCOS; RET(BLTIN); }
atan2	{ yylval.i = FATAN; RET(BLTIN); }
system	{ yylval.i = FSYSTEM; RET(BLTIN); }
rand		{ yylval.i = FRAND; RET(BLTIN); }
srand	{ yylval.i = FSRAND; RET(BLTIN); }
toupper	{ yylval.i = FTOUPPER; RET(BLTIN); }
tolower	{ yylval.i = FTOLOWER; RET(BLTIN); }
close	{ yylval.i = FCLOSE; RET(BLTIN); }

{A}{B}*	{ int n, c;
		  c = awk_yytchar;	/* look for '(' */
		  if (c != '(' && infunc && (n=isarg(yytext)) >= 0) {
			yylval.i = n;
			RET(ARG);
		  } else {
			yylval.cp = setsymtab((unsigned char *)yytext,"",0.0,STR|NUM,symtab);
			if (c == '(') {
				RET(CALL);
			} else {
				RET(VAR);
			}
		  }
		}
\"		{ BEGIN str; clen = 0; }

"}"		{ if (--bracecnt < 0) vyyerror(extra, '}'); sc_flag = 1; RET(';'); }
"]"		{ if (--brackcnt < 0) vyyerror(extra, ']'); RET(']'); }
")"		{ if (--parencnt < 0) vyyerror(extra, ')'); RET(')'); }

.		{ if (yytext[0] == '{') bracecnt++;
		  else if (yytext[0] == '[') brackcnt++;
		  else if (yytext[0] == '(') parencnt++;
		  RET(yylval.i = yytext[0]); /* everything else */ }

<reg>\\.	{ cbuf[clen++] = '\\'; cbuf[clen++] = yytext[1]; }
<reg>\n		{ vyyerror(":94:Newline in regular expression %.10s ...", cbuf); lineno++; BEGIN INITIAL; }
<reg>"/"	{ BEGIN INITIAL;
		  cbuf[clen] = 0;
		  yylval.s = tostring(cbuf);
		  awk_unput('/');
		  RET(REGEXPR); }
<reg>.		{ CADD; }

<str>\"		{ BEGIN INITIAL;
		  cbuf[clen] = 0; s = tostring(cbuf);
		  cbuf[clen] = ' '; cbuf[++clen] = 0;
		  yylval.cp = setsymtab(cbuf, s, 0.0, CON|STR, symtab);
		  RET(STRING); }
<str>\n		{ vyyerror(nlstring, cbuf); lineno++; BEGIN INITIAL; }
<str>"\\\""	{ cbuf[clen++] = '"'; }
<str>"\\"n	{ cbuf[clen++] = '\n'; }
<str>"\\"t	{ cbuf[clen++] = '\t'; }
<str>"\\"f	{ cbuf[clen++] = '\f'; }
<str>"\\"r	{ cbuf[clen++] = '\r'; }
<str>"\\"b	{ cbuf[clen++] = '\b'; }
<str>"\\"v	{ cbuf[clen++] = '\v'; }	/* these ANSIisms may not be known by */
<str>"\\"a	{ cbuf[clen++] = '\007'; }	/* your compiler. hence 007 for bell */
<str>"\\\\"	{ cbuf[clen++] = '\\'; }
<str>"\\"({O}{O}{O}|{O}{O}|{O}) { int n;
		  sscanf(yytext+1, "%o", &n); cbuf[clen++] = n; }
<str>"\\"x({H}+) { int n;	/* ANSI permits any number! */
		  sscanf(yytext+2, "%x", &n); cbuf[clen++] = n; }
<str>"\\".	{ cbuf[clen++] = yytext[1]; }
<str>.		{ CADD; }

%%

void
startreg(void)
{
	BEGIN reg;
	clen = 0;
}

/* input() and unput() were transcriptions of the standard lex
   macros for input and output with additions for error message
   printing.  God help us all if someone changes how lex works.
   - Luckily, the BSD people did most of the flex porting already
   for oawk.
*/

unsigned char	ebuf[300];
unsigned char	*ep = ebuf;

#ifdef	FLEX_SCANNER
int
awk_input(void)
{
	register int c;
	extern unsigned char *lexprog;

	if (lexprog != NULL) {	/* awk '...' */
		if (c = *lexprog & 0377)
			lexprog++;
	} else				/* awk -f ... */
		c = pgetc();
	if (c == EOF)
		c = 0;
	if (ep >= ebuf + sizeof ebuf)
		ep = ebuf;
	awk_yytchar = c;
	return *ep++ = c;
}

static void
awk_unput(int c)
{
	awk_yytchar = c;
	unput(c);
}


#else	/* !FLEX_SCANNER */
int
input(void)
{
	register int c;
	extern unsigned char *lexprog;

	if (yysptr > yysbuf)
		c = U(*--yysptr) & 0377;
	else if (lexprog != NULL) {	/* awk '...' */
		if (c = *lexprog & 0377)
			lexprog++;
	} else				/* awk -f ... */
		c = pgetc();
	if (c == '\n')
		yylineno++;
	else if (c == EOF)
		c = 0;
	if (ep >= ebuf + sizeof ebuf)
		ep = ebuf;
	return *ep++ = c;
}

int
awk_input(void)
{
	return input();
}

void
unput(int c)
{
	yytchar = c;
	if (yytchar == '\n')
		yylineno--;
	*yysptr++ = yytchar;
	if (--ep < ebuf)
		ep = ebuf + sizeof(ebuf) - 1;
}
#endif	/* !FLEX_SCANNER */

static void
awk_unputstr(const char *s)
{
	int i;

	for (i = strlen(s)-1; i >= 0; i--)
		awk_unput(s[i]);
}
