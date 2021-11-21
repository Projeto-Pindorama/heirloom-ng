/*	from Unix 7th Edition /usr/src/cmd/expr.y	*/
/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, December 2002.
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

%{
#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
#if defined (S42)
static const char sccsid[] USED = "@(#)expr_s42.sl	1.28 (gritter) 5/29/05";
static int	sus = 0;
#elif defined (SU3)
static const char sccsid[] USED = "@(#)expr_su3.sl	1.28 (gritter) 5/29/05";
static int	sus = 3;
#elif defined (SUS)
static const char sccsid[] USED = "@(#)expr_sus.sl	1.28 (gritter) 5/29/05";
static int	sus = 1;
#else
static const char sccsid[] USED = "@(#)expr.sl	1.28 (gritter) 5/29/05";
static int	sus = 0;
#endif

/*	expression command */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <locale.h>
#include <wchar.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <inttypes.h>

#include	"atoll.h"

#define EQL(x,y) !strcmp(x,y)

#define	NUMSZ	25

static char	**Av;
static int	Ac;
static int	Argi;
static int	mb_cur_max;
static char	*progname;
extern int	sysv3;

static char	*Mstring[1];

int		yylex(void);
static char	*_rel(int op, register char *r1, register char *r2);
static char	*_arith(int op, char *r1, char *r2);
static char	*_conj(int op, char *r1, char *r2);
static char	*match(char *s, char *p);
static int	ematch(char *s, register char *p);
static void	errxx(int c);
static int	yyerror(const char *s);
static int	numeric(const char *s);
static int	chars(const char *s, const char *end);
static void	*srealloc(void *, size_t);
static void	*smalloc(size_t);
static char	*numpr(int64_t val);

static char	*substr(char *, const char *, const char *);
static char	*length(const char *);
static char	*eindex(const char *, const char *);

#if defined (SUS) || defined (SU3) || defined (S42)
#include	<regex.h>
static int	nbra;
#else	/* !SUS, !SU3, !S42 */
#include	<regexpr.h>
#endif	/* !SUS, !SU3, !S42 */
%}

/* Yacc productions for "expr" command: */

%union {
	char	*val;
}

%token <val> OR AND ADD SUBT MULT DIV REM EQ GT GEQ LT LEQ NEQ
%token <val> A_STRING SUBSTR LENGTH INDEX NOARG MATCH

%type <val> expr

/* operators listed below in increasing precedence: */
%left OR
%left AND
%left EQ LT GT GEQ LEQ NEQ
%left ADD SUBT
%left MULT DIV REM
%left MCH
%left MATCH
%left SUBSTR
%left LENGTH INDEX
%%

/* a single `expression' is evaluated and printed: */

expression:	expr NOARG {
			if (sus && numeric($1)) {
				int64_t	n;
				n = atoll($1);
				printf("%lld\n", n);
				exit(n == 0);
			} else
				puts($1);
			exit((!strcmp($1,"0")||!strcmp($1,"\0"))? 1: 0);
			}
	;


expr:	'(' expr ')' { $$ = $2; }
	| expr OR expr   { $$ = _conj(OR, $1, $3); }
	| expr AND expr   { $$ = _conj(AND, $1, $3); }
	| expr EQ expr   { $$ = _rel(EQ, $1, $3); }
	| expr GT expr   { $$ = _rel(GT, $1, $3); }
	| expr GEQ expr   { $$ = _rel(GEQ, $1, $3); }
	| expr LT expr   { $$ = _rel(LT, $1, $3); }
	| expr LEQ expr   { $$ = _rel(LEQ, $1, $3); }
	| expr NEQ expr   { $$ = _rel(NEQ, $1, $3); }
	| expr ADD expr   { $$ = _arith(ADD, $1, $3); }
	| expr SUBT expr   { $$ = _arith(SUBT, $1, $3); }
	| expr MULT expr   { $$ = _arith(MULT, $1, $3); }
	| expr DIV expr   { $$ = _arith(DIV, $1, $3); }
	| expr REM expr   { $$ = _arith(REM, $1, $3); }
	| expr MCH expr	 { $$ = match($1, $3); }
	| MATCH expr expr { $$ = match($2, $3); }
	| SUBSTR expr expr expr { $$ = substr($2, $3, $4); }
	| LENGTH expr       { $$ = length($2); }
	| INDEX expr expr { $$ = eindex($2, $3); }
	| A_STRING
	;
%%

int
main(int argc, char **argv)
{
	extern int	 yyparse(void);

	Ac = argc;
	Argi = 1;
	Av = argv;
	progname = basename(argv[0]);
	if (getenv("SYSV3") != NULL)
		sysv3 = 1;
	setlocale(LC_COLLATE, "");
	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	if (Av[1] && Av[1][0] == '-' && Av[1][1] == '-' && Av[1][2] == '\0')
		Argi++;
	yyparse();
	/*NOTREACHED*/
	return 0;
}

static const char *operators[] = {
	"|", "&", "+", "-", "*", "/", "%", ":",
	"=", "==", "<", "<=", ">", ">=", "!=",
	"match", "substr", "length", "index",
	"\0"
};

static int op[] = {
	OR, AND, ADD,  SUBT, MULT, DIV, REM, MCH,
	EQ, EQ, LT, LEQ, GT, GEQ, NEQ,
	MATCH, SUBSTR, LENGTH, INDEX
};

int
yylex(void)
{
	register char *p;
	register int i;

	if(Argi >= Ac) return NOARG;

	p = Av[Argi++];

	if((*p == '(' || *p == ')') && p[1] == '\0')
		return (int)*p;
	for(i = 0; *operators[i]; ++i)
		if(EQL(operators[i], p))
			return op[i];

	yylval.val = p;
	return A_STRING;
}

static char *
_rel(int op, register char *r1, register char *r2)
{
	register int64_t i;

	if (numeric(r1) && numeric(r2))
		i = atoll(r1) - atoll(r2);
	else
		i = strcoll(r1, r2);
	switch(op) {
	case EQ: i = i==0; break;
	case GT: i = i>0; break;
	case GEQ: i = i>=0; break;
	case LT: i = i<0; break;
	case LEQ: i = i<=0; break;
	case NEQ: i = i!=0; break;
	}
	return i? "1": "0";
}

static char *
_arith(int op, char *r1, char *r2)
{
	int64_t i1, i2;
	register char *rv;

	if (!numeric(r1) || !numeric(r2))
		yyerror("non-numeric argument");
	i1 = atoll(r1);
	i2 = atoll(r2);

	switch(op) {
	case ADD: i1 = i1 + i2; break;
	case SUBT: i1 = i1 - i2; break;
	case MULT: i1 = i1 * i2; break;
	case DIV:
		if (i2 == 0) yyerror("division by zero");
		i1 = i1 / i2; break;
	case REM: i1 = i1 % i2; break;
	}
	rv = numpr(i1);
	return rv;
}

static char *
_conj(int op, char *r1, char *r2)
{
	register char *rv = NULL;

	switch(op) {

	case OR:
		if(EQL(r1, "0")
		|| EQL(r1, ""))
			if(EQL(r2, "0")
			|| EQL(r2, ""))
				rv = "0";
			else
				rv = r2;
		else
			rv = r1;
		break;
	case AND:
		if(EQL(r1, "0")
		|| EQL(r1, ""))
			rv = "0";
		else if(EQL(r2, "0")
		|| EQL(r2, ""))
			rv = "0";
		else
			rv = r1;
		break;
	}
	return rv;
}

static char *
match(char *s, char *p)
{
	register char *rv;
	int	gotcha;

	gotcha = ematch(s, p);
	if(nbra) {
		if (gotcha) {
			rv = smalloc(strlen(Mstring[0])+1);
			strcpy(rv, Mstring[0]);
		} else
			rv = "";
	} else
		rv = numpr(gotcha);
	return rv;
}

#if defined (SUS) || defined (SU3) || defined (S42)
static int
ematch(char *s, register char *p)
{
	regex_t	re;
	register int	num;
	regmatch_t	bralist[2];
	int	reflags = 0, val;

#ifdef	REG_ANGLES
	reflags |= REG_ANGLES;
#endif
#if defined (SU3) && defined (REG_AVOIDNULL)
	reflags |= REG_AVOIDNULL;
#endif
	if ((num = regcomp(&re, p, reflags)) != 0)
		errxx(0);
	nbra = re.re_nsub;
	if (regexec(&re, s, 2, bralist, 0) == 0 && bralist[0].rm_so == 0) {
		if (re.re_nsub >= 1) {
			num = bralist[1].rm_eo - bralist[1].rm_so;
			Mstring[0] = srealloc(Mstring[0], num + 1);
			strncpy(Mstring[0], s + bralist[1].rm_so, num);
			Mstring[0][num] = '\0';
		}
		val = chars(s, &s[bralist[0].rm_eo]);
	} else
		val = 0;
	regfree(&re);
	return val;
}
#else	/* !SUS, !SU3, !S42 */
static int
ematch(char *s, register char *p)
{
	char *expbuf;
	register int num, val;

	if ((expbuf = compile(p, NULL, NULL)) == NULL)
		errxx(regerrno);
	if(nbra > 1)
		yyerror("Too many '\\('s");
	if(advance(s, expbuf)) {
		if(nbra == 1) {
			p = braslist[0];
			num = braelist[0] ? braelist[0] - p : 0;
			Mstring[0] = srealloc(Mstring[0], num + 1);
			strncpy(Mstring[0], p, num);
			Mstring[0][num] = '\0';
		}
		val = chars(s, loc2);
	} else
		val = 0;
	free(expbuf);
	return(val);
}
#endif	/* !SUS, !SU3, !S42 */

/*ARGSUSED*/
static void
errxx(int c)
{
	yyerror("RE error");
}

static int
yyerror(const char *s)
{
	fprintf(stderr, "%s: %s\n", progname, s);
	exit(2);
}

static int
numeric(const char *s)
{
	if (*s == '-')
		s++;
	if (!isdigit(*s & 0377))
		return 0;
	do
		s++;
	while (isdigit(*s & 0377));
	return (*s == '\0');
}

static int
chars(const char *s, const char *end)
{
	int	count = 0, n;
	wchar_t	wc;

	if (mb_cur_max > 1) {
		while (s < end) {
			if ((n = mbtowc(&wc, s, MB_LEN_MAX)) >= 0)
				count++;
			s += n > 0 ? n : 1;
		}
	} else
		count = end - s;
	return count;
}

static void *
srealloc(void *vp, size_t nbytes)
{
	void *p;

	if ((p = realloc(vp, nbytes)) == NULL) {
		write(2, "no memory\n", 10);
		exit(077);
	}
	return p;
}

static void *
smalloc(size_t nbytes)
{
	return srealloc(NULL, nbytes);
}

static char *
numpr(int64_t val)
{
	char	*rv;
	int	ret;

	rv = smalloc(NUMSZ);
	ret = snprintf(rv, NUMSZ, "%lld", (long long)val);
	if (ret < 0 || ret >= NUMSZ) {
		rv = srealloc(rv, ret + 1);
		ret = snprintf(rv, ret, "%lld", (long long)val);
		if (ret < 0)
			yyerror("illegal number");
	}
	return rv;
}

#define	next(wc, s, n) (mb_cur_max > 1 && *(s) & 0200 ? \
			((n) = mbtowc(&(wc), (s), mb_cur_max), \
			 (n) = ((n) > 0 ? (n) : (n) < 0 ? illseq() : 1)) :\
		((wc) = *(s) & 0377, (n) = 1))

static int
illseq(void)
{
	yyerror("illegal byte sequence");
	/*NOTREACHED*/
	return 0;
}

static char *
substr(char *v, const char *s, const char *w)
{
	long si, wi;
	char *res;
	wchar_t wc;
	int n;

#ifndef	S42
	if (sysv3 == 0)
		yyerror("syntax error");
#endif
	si = atoll(s);
	wi = atoll(w);
	while (--si)
		if (*v) {
			next(wc, v, n);
			v += n;
		}
	res = v;
	while (wi--)
		if (*v) {
			next(wc, v, n);
			v += n;
		}
	*v = '\0';
	return res;
}

static char *
length(const char *s)
{
	long i = 0;
	char *rv;
	wchar_t wc;
	int n;

#ifndef	S42
	if (sysv3 == 0)
		yyerror("syntax error");
#endif
	while (*s) {
		next(wc, s, n);
		s += n;
		++i;
	}
	rv = numpr(i);
	return rv;
}

static char *
eindex(const char *s, const char *t)
{
	long i, j, x;
	char *rv;
	wchar_t ws, wt;
	int ns, nt;

#ifndef	S42
	if (sysv3 == 0)
		yyerror("syntax error");
#endif
	for (i = 0, x = 0; s[i]; x++, i += ns) {
		next(ws, &s[i], ns);
		for (j = 0; t[j]; j += nt) {
			next(wt, &t[j], nt);
			if (ws == wt) {
				rv = numpr(++x);
				return rv;
			}
		}
	}
	return "0";
}
