/*
 * Derived from Unix 32V /usr/src/cmd/egrep.y
 *
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, April 2001.
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
static const char sccsid[] USED = "@(#)egrep.sl	2.22 (gritter) 5/29/05";
%}

/*
 * egrep -- print lines containing (or not containing) a regular expression
 *
 *	status returns:
 *		0 - ok, and some matches
 *		1 - ok, but no matches
 *		2 - some error
 */

%token CHAR MCHAR DOT CCL NCCL OR CAT STAR PLUS QUEST
%left OR
%left CHAR MCHAR DOT CCL NCCL '('
%left CAT
%left STAR PLUS QUEST

%{
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <regex.h>
#include <limits.h>
#include <stdlib.h>
#include "grep.h"
#include "alloc.h"

#include <mbtowi.h>

#define NCHARS 256
#define	NSTATES	64
#define FINAL -1
static unsigned	MAXLIN;
static unsigned	MAXPOS;
static unsigned	MAXCHARS;
static unsigned	char gotofn[NSTATES][NCHARS];
static int	lastn;
static int	state[NSTATES];
static char	out[NSTATES];
static int	line = 1;
static int	*name;
static int	*left;
static int	*right;
static int	*parent;
static int	*foll;
static int	*positions;
static wchar_t	*chars;
static int	nxtpos;
static int	initpos;
static int	nxtchar = 0;
static int	*tmpstat;
static int	*initstat;
static int	xstate;
static int	count;
static int	icount;

static void	yyerror(const char *);
static void	*erealloc(void *, size_t);
static void	more_chars(unsigned);
static void	more_lines(unsigned);
static void	more_positions(unsigned);
static int	yylex(void);
static void	synerror(void);
static int	enter(int);
static int	cclenter(int);
static int	menter(int);
static int	node(int, int, int);
static int	unary(int, int);
static void	cfoll(int);
static int	cgotofn(int, int);
static void	igotofn(void);
static int	cstate(int);
static int	member(int, int, int);
static int	notin(int);
static void	add(int *, int);
static void	follow(int);
static void	eg_build(void);
static int	eg_matchw(const char *, size_t);
static int	eg_match(const char *, size_t);
static int	eg_range(struct iblok *, char *);
static int	eg_rangew(struct iblok *, char *);
%}

%%
s:	t
		{ unary(FINAL, $1);
		  line--;
		}
	;
t:	b r
		{ $$ = node(CAT, $1, $2); }
	| OR b r OR
		{ $$ = node(CAT, $2, $3); }
	| OR b r
		{ $$ = node(CAT, $2, $3); }
	| b r OR
		{ $$ = node(CAT, $1, $2); }
	;
b:
		{ $$ = enter(DOT);
		   $$ = unary(STAR, $$); }
	;
r:	CHAR
		{ $$ = enter($1); }
	| MCHAR
		{ $$ = menter($1); }
	| DOT
		{ $$ = enter(DOT); }
	| CCL
		{ $$ = cclenter(CCL); }
	| NCCL
		{ $$ = cclenter(NCCL); }
	;

r:	r OR r
		{ $$ = node(OR, $1, $3); }
	| r r %prec CAT
		{ $$ = node(CAT, $1, $2); }
	| r STAR
		{ $$ = unary(STAR, $1); }
	| r PLUS
		{ $$ = unary(PLUS, $1); }
	| r QUEST
		{ $$ = unary(QUEST, $1); }
	| '(' r ')'
		{ $$ = $2; }
	| error 
	;

%%
static void
yyerror(const char *s) {
	fprintf(stderr, "%s: %s\n", progname, s);
	exit(2);
}

static void *
erealloc(void *vp, size_t size)
{
	if ((vp = realloc(vp, size)) == NULL) {
		write(2, progname, strlen(progname));
		write(2, ": regular expression too long\n", 30);
		_exit(2);
	}
	return (vp);
}

static void
more_chars(unsigned need)
{
	unsigned omaxchars = MAXCHARS;
	unsigned incr;
	incr = need - MAXCHARS + 32;
	MAXCHARS += incr;
	chars = erealloc(chars, MAXCHARS * sizeof *chars);
	memset(&chars[omaxchars], 0, incr * sizeof *chars);
}

static void
more_lines(unsigned need)
{
	unsigned omaxlin = MAXLIN;
	unsigned incr;
	incr = need - MAXLIN + 32;
	MAXLIN += incr;
	name = erealloc(name, MAXLIN * sizeof *name);
	memset(&name[omaxlin], 0, incr * sizeof *name);
	left = erealloc(left, MAXLIN * sizeof *left);
	memset(&left[omaxlin], 0, incr * sizeof *left);
	right = erealloc(right, MAXLIN * sizeof *right);
	memset(&right[omaxlin], 0, incr * sizeof *right);
	parent = erealloc(parent, MAXLIN * sizeof *parent);
	memset(&parent[omaxlin], 0, incr * sizeof *parent);
	foll = erealloc(foll, MAXLIN * sizeof *foll);
	memset(&foll[omaxlin], 0, incr * sizeof *foll);
	tmpstat = erealloc(tmpstat, MAXLIN * sizeof *tmpstat);
	memset(&tmpstat[omaxlin], 0, incr * sizeof *tmpstat);
	initstat = erealloc(initstat, MAXLIN * sizeof *initstat);
	memset(&initstat[omaxlin], 0, incr * sizeof *initstat);
}

static void
more_positions(unsigned need)
{
	unsigned omaxpos = MAXPOS;
	unsigned incr;
	incr = need - MAXPOS + 48;
	MAXPOS += incr;
	positions = erealloc(positions, MAXPOS * sizeof *positions);
	memset(&positions[omaxpos], 0, incr * sizeof *positions);
}

static int
yylex(void) {
	int cclcnt, x;
	register int c;
	switch(c = nextch()) {
		case '$':
		case '^': c = '\n';
			goto defchar;
		case '|': return (OR);
		case '*': return (STAR);
		case '+': return (PLUS);
		case '?': return (QUEST);
		case '(': return (c);
		case ')': return (c);
		case '.': return (DOT);
		case EOF: return (0);
		case '\0': return (0);
		case '\n': return (OR);
		case '[': 
			x = CCL;
			cclcnt = 0;
			count = nxtchar++;
			if ((c = nextch()) == '^') {
				x = NCCL;
				c = nextch();
			}
			do {
				if (c == '\0' || c == EOF) synerror();
				if (nxtchar >= MAXCHARS) more_chars(nxtchar);
				chars[nxtchar++] = c;
				cclcnt++;
			} while ((c = nextch()) != ']');
			chars[count] = cclcnt;
			return (x);
		case '\\':
			if ((c = nextch()) == EOF) synerror();
		defchar:
		default:
			if (mbcode && c & ~(wchar_t)0177) {
				if (nxtchar >= MAXCHARS) more_chars(nxtchar);
				chars[nxtchar] = c;
				yylval = nxtchar++;
				return (MCHAR);
			}
			yylval = c; return (CHAR);
	}
}

static void
synerror(void) {
	yyerror("syntax error");
}

static int
enter(int x) {
	if(line >= MAXLIN) more_lines(line);
	name[line] = x;
	left[line] = 0;
	right[line] = 0;
	return(line++);
}

static int
cclenter(int x) {
	register int linno;
	linno = enter(x);
	right[linno] = count;
	return (linno);
}

static int
menter(int x) {
	register int linno;
	linno = enter(MCHAR);
	right[linno] = x;
	return (linno);
}

static int
node(int x, int l, int r) {
	if(line >= MAXLIN) more_lines(line);
	name[line] = x;
	left[line] = l;
	right[line] = r;
	parent[l] = line;
	parent[r] = line;
	return(line++);
}

static int
unary(int x, int d) {
	if(line >= MAXLIN) more_lines(line);
	name[line] = x;
	left[line] = d;
	right[line] = 0;
	parent[d] = line;
	return(line++);
}

static void
cfoll(int v) {
	register int i;
	if (left[v] == 0) {
		count = 0;
		for (i=1; i<=line; i++) tmpstat[i] = 0;
		follow(v);
		add(foll, v);
	}
	else if (right[v] == 0) cfoll(left[v]);
	else {
		cfoll(left[v]);
		cfoll(right[v]);
	}
}

static int
cgotofn(int s, int c)
{
	register int cc, i, k;
	int n, pos;
	int st;
	int curpos, num;
	int number, newpos;
	n = lastn;
	cc = iflag ? mbcode && c & ~(wchar_t)0177 ? towlower(c):tolower(c) : c;
	num = positions[state[s]];
	count = icount;
	for (i=3; i <= line; i++) tmpstat[i] = initstat[i];
	pos = state[s] + 1;
	for (i=0; i<num; i++) {
		curpos = positions[pos];
		if ((k = name[curpos]) >= 0)
			if (
				((cc & ~(wchar_t)(NCHARS-1)) == 0 && k == cc)
				|| (k == MCHAR && cc == chars[right[curpos]])
				|| (cc != '\n' && cc != WEOF && ((k == DOT)
				|| (k == CCL && member(cc, right[curpos], 1))
				|| (k == NCCL && member(cc, right[curpos], 0))))
			) {
				if (foll[curpos] < MAXPOS)
					number = positions[foll[curpos]];
				else
					number = 0;
				newpos = foll[curpos] + 1;
				for (k=0; k<number; k++) {
					if (tmpstat[positions[newpos]] != 1) {
						tmpstat[positions[newpos]] = 1;
						count++;
					}
					newpos++;
				}
			}
		pos++;
	}
	if (notin(n)) {
		if (++n >= NSTATES) {
			n = gotofn[0]['\n'];
			memset(gotofn, 0, sizeof gotofn);
			gotofn[0]['\n'] = n;
			memset(&out[n], 0, sizeof out - n);
			nxtpos = initpos;
			st = n + 1;
		}
		else {
			st = n + 1;
			if ((c & ~(wchar_t)(NCHARS-1)) == 0)
				gotofn[s][c] = st;
		}
		add(state, n);
		if (tmpstat[line] == 1) out[n] = 1;
	}
	else {
		st = xstate + 1;
		if ((c & ~(wchar_t)(NCHARS-1)) == 0)
			gotofn[s][c] = st;
	}
	lastn = n;
	return (st);
}

static void
igotofn(void)
{
	int n;
	count = 0;
	for (n=3; n<=line; n++) tmpstat[n] = 0;
	if (cstate(line-1)==0) {
		tmpstat[line] = 1;
		count++;
		out[0] = 1;
	}
	for (n=3; n<=line; n++) initstat[n] = tmpstat[n];
	count--;		/*leave out position 1 */
	icount = count;
	tmpstat[1] = 0;
	add(state, 0);
	cgotofn(0, '\n');
	initpos = nxtpos;
}

static int
cstate(int v) {
	register int b;
	if (left[v] == 0) {
		if (tmpstat[v] != 1) {
			tmpstat[v] = 1;
			count++;
		}
		return(1);
	}
	else if (right[v] == 0) {
		if (cstate(left[v]) == 0) return (0);
		else if (name[v] == PLUS) return (1);
		else return (0);
	}
	else if (name[v] == CAT) {
		if (cstate(left[v]) == 0 && cstate(right[v]) == 0) return (0);
		else return (1);
	}
	else { /* name[v] == OR */
		b = cstate(right[v]);
		if (cstate(left[v]) == 0 || b == 0) return (0);
		else return (1);
	}
}

static int
member(int symb, int set, int torf) {
	register int num, pos, lastc = WEOF;
	num = chars[set];
	pos = set + 1;
	num += pos;
	while (pos < num) {
		if (chars[pos] == '-' && lastc != WEOF) {
			if (++pos == num)	/* System V oddity: '[a-]' */
				return (!torf);	/* matches 'a' but not '-' */
			if (symb > lastc && symb < chars[pos])
				return (torf);
		}
		if (symb == chars[pos]) return (torf);
		lastc = chars[pos++];
	}
	return (!torf);
}

static int
notin(int n) {
	register int i, j, pos;
	for (i=0; i<=n; i++) {
		if (positions[state[i]] == count) {
			pos = state[i] + 1;
			for (j=0; j < count; j++)
				if (tmpstat[positions[pos++]] != 1) goto nxt;
			xstate = i;
			return (0);
		}
		nxt: ;
	}
	return (1);
}

static void
add(int *array, int n) {
	register int i;
	unsigned need;
	if ((need = nxtpos + count) >= MAXPOS) more_positions(need);
	array[n] = nxtpos;
	positions[nxtpos++] = count;
	for (i=3; i <= line; i++) {
		if (tmpstat[i] == 1) {
			if (nxtpos >= MAXPOS) more_positions(nxtpos);
			positions[nxtpos++] = i;
		}
	}
}

static void
follow(int v) {
	int p;
	if (v == line) return;
	p = parent[v];
	switch(name[p]) {
		case STAR:
		case PLUS:	cstate(v);
				follow(p);
				return;

		case OR:
		case QUEST:	follow(p);
				return;

		case CAT:	if (v == left[p]) {
					if (cstate(right[p]) == 0) {
						follow(p);
						return;
					}
				}
				else follow(p);
				return;
		case FINAL:	if (tmpstat[line] != 1) {
					tmpstat[line] = 1;
					count++;
				}
				return;
	}
}

static void
eg_build(void)
{
	extern int	yyparse(void);

	more_chars(0);
	more_lines(0);
	more_positions(0);
	yyparse();
	cfoll(line-1);
	igotofn();
	range = mbcode ? eg_rangew : eg_range;
}

static int
eg_matchw(const char *str, size_t sz)
{
	register const char *p;
	register int cstat, nstat;
	wint_t wc;
	int n;

	p = str;
	cstat = gotofn[0]['\n']-1;
	while (out[cstat] == 0 && p < &str[sz]) {
		if (*p & 0200) {
			if ((n = mbtowi(&wc, p, &str[sz] - p)) < 0) {
				p++;
				wc = WEOF;
			}
			else p += n;
		}
		else wc = *p++;
		if ((wc & ~(wchar_t)(NCHARS-1)) != 0 ||
				(nstat = gotofn[cstat][wc]) == 0)
			nstat = cgotofn(cstat, wc);
		cstat = nstat-1;
	}
	if (out[cstat] == 0 && p == &str[sz])
		cstat = gotofn[cstat]['\n']-1;
	return (out[cstat]);
}

static int
eg_match(const char *str, size_t sz)
{
	register const char *p;
	register int cstat, nstat;

	p = str;
	cstat = gotofn[0]['\n']-1;
	while (out[cstat] == 0 && p < &str[sz]) {
		if ((nstat = gotofn[cstat][*p++ & 0377]) == 0)
			nstat = cgotofn(cstat, p[-1] & 0377);
		cstat = nstat-1;
	}
	if (out[cstat] == 0 && p == &str[sz])
		cstat = gotofn[cstat]['\n']-1;
	return (out[cstat]);
}

static int
eg_range(struct iblok *ip, char *last)
{
	register char *p;
	register int cstat, nstat;
	int istat;

	p = ip->ib_cur;
	lineno++;
	istat = cstat = gotofn[0]['\n']-1;
	if (out[cstat]) goto found;
	for (;;) {
		if ((nstat = gotofn[cstat][*p & 0377]) == 0)
			nstat = cgotofn(cstat, *p & 0377);
		cstat = nstat-1;
		if (out[cstat]) {
		found:	for (;;) {
				if (vflag == 0) {
		succeed:		outline(ip, last, p - ip->ib_cur);
					if (qflag || lflag)
						return (1);
				}
				else {
					ip->ib_cur = p;
					while (*ip->ib_cur++ != '\n');
				}
				if ((p = ip->ib_cur) > last)
					return (0);
				lineno++;
				if ((out[(cstat=istat)]) == 0) goto brk2;
			}
		}
		if (*p++ == '\n') {
			if (vflag) {
				p--;
				goto succeed;
			}
			if ((ip->ib_cur = p) > last)
				return (0);
			lineno++;
			if (out[(cstat=istat)]) goto found;
		}
		brk2:	;
	}
}

static int
eg_rangew(struct iblok *ip, char *last)
{
	register char *p;
	register int cstat, nstat;
	int istat;
	wint_t	wc;
	int	n;

	p = ip->ib_cur;
	lineno++;
	istat = cstat = gotofn[0]['\n']-1;
	if (out[cstat]) goto found;
	for (;;) {
		if (*p & 0200) {
			if ((n = mbtowi(&wc, p, last + 1 - p)) < 0) {
				n = 1;
				wc = WEOF;
			}
		}
		else {
			wc = *p;
			n = 1;
		}
		if ((wc & ~(wchar_t)(NCHARS-1)) != 0 ||
			(nstat = gotofn[cstat][wc]) == 0)
			nstat = cgotofn(cstat, wc);
		cstat = nstat-1;
		if (out[cstat]) {
		found:	for (;;) {
				if (vflag == 0) {
		succeed:		outline(ip, last, p - ip->ib_cur);
					if (qflag || lflag)
						return (1);
				}
				else {
					ip->ib_cur = p;
					while (*ip->ib_cur++ != '\n');
				}
				if ((p = ip->ib_cur) > last)
					return (0);
				lineno++;
				if ((out[(cstat=istat)]) == 0) goto brk2;
			}
		}
		p += n;
		if (wc == '\n') {
			if (vflag) {
				p--;
				goto succeed;
			}
			if ((ip->ib_cur = p) > last)
				return (0);
			lineno++;
			if (out[(cstat=istat)]) goto found;
		}
		brk2:	;
	}
}

void
misop(void)
{
	synerror();
}

void
init(void)
{
	Eflag = 1;
	eg_select();
	options = "bce:f:hilnrRvyz";
}

void
eg_select(void)
{
	build = eg_build;
	match = mbcode ? eg_matchw : eg_match;
	matchflags &= ~(MF_NULTERM|MF_LOCONV);
}

/*
 * Some dummy routines for sharing source with other grep flavors.
 */
void
ac_select(void)
{
}

void
st_select(void)
{
}

void
rc_select(void)
{
}

char *usagemsg =
"usage: %s [ -bchilnv ] [ -e exp ] [ -f file ] [ strings ] [ file ] ...\n";
char *stdinmsg;
