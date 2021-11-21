/*
 * Derived from Unix 6th Edition /usr/source/s2/tr.c and
 * Unix 7th Edition /usr/src/cmd/tr.c
 *
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, August 2005.
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
#if defined (SUS)
static const char sccsid[] USED = "@(#)tr_sus.sl	2.3 (gritter) 9/7/05";
#elif defined (UCB)
static const char sccsid[] USED = "@(#)/usr/ucb/tr.sl	2.3 (gritter) 9/7/05";
#else
static const char sccsid[] USED = "@(#)tr.sl	2.3 (gritter) 9/7/05";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <locale.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>
#include <string.h>
#include <inttypes.h>
#include "iblok.h"

#if defined (__GLIBC__) && defined (_IO_putc_unlocked)
#undef	putchar
#define	putchar(c)	_IO_putc_unlocked(c, stdout)
#endif

#undef	iswupper
#undef	iswlower
#undef	towupper
#undef	towlower

#ifdef	SUS
#define	NIL	EOF
#else
#define	NIL	0
#endif

/* tr - transliterate data stream */

/*
 * This implementation of tr keeps the traditional design to hold
 * arrays that contain all characters. This is because experience
 * with tr implementations that use more sophisticated structures
 * has shown that their memory usage is even higher under certain
 * circumstances while their performance is generally worse. This
 * is mostly because these structures usually contain one or more
 * pointers in addition to the actual data for a character; these
 * pointers consume memory and need to be followed. The backdraw
 * with this implementation is that it is expected that the
 * contents of wchar_t values are all within the Unicode range.
 *
 * Tables of different sizes are built as needed. Thus as long as
 * only ASCII characters occur, only a few kilobytes of memory are
 * consumed in any locale. If all characters in a multibyte locale
 * remain within the BMP, around 250 kilobytes are needed. The full
 * Unicode range causes tr to use about 4.5 megabytes of memory.
 * The -c or -s options raise these requirements by up to 125 %
 * to a total maximum of about 10 megabytes.
 */

static int dflag = 0;
static int sflag = 0;
static int cflag = 0;
static int save = NIL;
static int32_t *code;
static char *squeez;
static int32_t *vect;
static struct string {
	int last, max, rep;
	wctype_t type;
	enum { N, U, L } tp;
	char *p;
} string1, string2;
static int status;
static int ccnt;
static int xccnt;
static int mask;
static int mb_cur_max;

static void cmple(char *, char *, int);
static int next(struct string *);
static int nextc(struct string *);
static int nextb(struct string *);
static void bad(void);
static int caseconv(wint_t (*)(wint_t), int (*)(wint_t));

int
main(int argc, char **argv)
{
	int i, j;
	int c;
	char *arg0, *arg1;
	struct iblok *ip;

	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	xccnt = mb_cur_max > 1 ? 0x110000 : 256;
	mask = mb_cur_max > 1 ? 0x1FFFFF : 255;

	ip = ib_alloc(0, 0);

	while((c = getopt(argc, argv, ":cCds")) != EOF) {
		switch(c) {
		case 'c':
		case 'C':
			cflag++;
			break;
		case 'd':
			dflag++;
			break;
		case 's':
			sflag++;
			break;
		}
	}
	argc -= optind;
	argv += optind;
	arg0 = argc>0 ? argv[0] : "";
	arg1 = argc>1 ? argv[1] : "";

	cmple(arg0, arg1, 255);

	if(mb_cur_max > 1) {
		char mb[MB_LEN_MAX];
		wint_t wc;
		char *cp;
		while((cp=ib_getw(ip, &wc, &i)) != NULL) {
			if(wc == NIL) continue;
			if(wc!=WEOF && (wc&mask)>=ccnt)
				cmple(arg0, arg1, wc);
			if(wc!=WEOF && (wc&=mask)<ccnt) {
				if((c = code[wc]) != NIL) {
					if(!sflag || c!=save || !squeez[c]) {
						save = c;
						if(c != wc) {
							if (c & ~0177) {
								j=wctomb(mb, c);
								if(j>0) {
									cp = mb;
									i = j;
								}
							} else {
								i = 1;
								cp = mb;
								*cp = c;
							}
						}
					} else
						continue;
				} else
					continue;
			}
			for(j = 0; j < i; j++)
				putchar(cp[j]&0377);
		}
	} else {
		while((c=ib_get(ip)) != EOF) {
			if(c == NIL) continue;
			if((c = code[c]) != NIL)
				if(!sflag || c!=save || !squeez[c])
					putchar(save = c);
		}
	}
	return status;
}

static void
cmple(char *arg0, char *arg1, int needc)
{
	int i, j;
	int c, d;
	int32_t *compl = NULL;
	int lastd = NIL;
	int nccnt = 0;
	int32_t *ncode, *nvect = NULL;
	char *nsqueez = NULL;
	const int borders[] = {
		256,		/* singlebyte */
		0x3000,		/* Western and technical Unicode characters */
		0x10000,	/* whole BMP */
		0x30000,	/* most of Unicode 4.0 */
		0x110000,	/* whole Unicode range */
		-1
	};

	for(i=0; borders[i]>0; i++)
		if((nccnt=borders[i]) > needc) break;
	if(nccnt<=ccnt) return;

	ncode = realloc(code, nccnt * sizeof *code);
	if(sflag) nsqueez = realloc(squeez, nccnt * sizeof *squeez);
	if(cflag) nvect = realloc(vect, nccnt * sizeof *vect);
	if(ncode==NULL || (sflag && nsqueez==NULL) || (cflag && nvect==NULL))
		return;
	ccnt = nccnt;
	code = ncode;
	squeez = nsqueez;
	vect = nvect;
	if(code==NULL || (sflag && squeez==NULL) || (cflag && vect==NULL)) {
		write(2, "No memory\n", 10);
		_exit(077);
	}
	if(sflag) memset(squeez, 0, nccnt * sizeof *squeez);

	string1.p = arg0;
	string2.p = arg1;
	string1.last = string2.last = NIL;
	string1.max = string2.max = NIL;
	string1.rep = string2.rep = 0;
	string1.tp = string2.tp = N;
	string1.type = string2.type = 0;

	for(i=0; i<ccnt; i++)
		code[i] = NIL;
	if(cflag) {
		for(i=0; i<ccnt; i++)
			vect[i] = NIL;
		while((c = next(&string1)) != NIL)
			vect[c] = 1;
		j = 0;
		for(i=NIL+1; i<ccnt; i++)
			if(vect[i]==NIL) vect[j++] = i;
		vect[j] = NIL;
		compl = vect;
	}
	for(;;){
		if(cflag) c = *compl++;
		else c = next(&string1);
		if(c==NIL) break;
		d = next(&string2);
		if(string1.tp==U && string2.tp==L) {
			lastd = caseconv(towlower, iswlower);
			continue;
		}
		if(string1.tp==L && string2.tp==U) {
			lastd = caseconv(towupper, iswupper);
			continue;
		}
		string1.tp = string2.tp = N;
#ifndef	UCB
		if(d==NIL) d = c;
#else
		if(d==NIL) d = lastd;
#endif
		lastd = d;
		if(c<ccnt) code[c] = d;
		if(d<ccnt && sflag) squeez[d] = 1;
	}
	free(vect);
	while((d = next(&string2)) != NIL) {
		if(sflag) squeez[d] = 1;
		if(string2.max==NIL && (string2.p==NULL || *string2.p==0))
			break;
	}
	for(i=0;i<ccnt;i++) {
		if(code[i]==NIL) code[i] = i;
		else if(dflag) code[i] = NIL;
	}
}

static int
next(struct string *s)
{
	int a, c, n;
	int base;
	char *savep;

	if(--s->rep > 0) return(s->last);
	if(s->max != NIL) {
		if(s->last++ < s->max) {
			if(s->type) {
				do {
					c = mb_cur_max==1 ?
						btowc(s->last) : s->last;
					if(iswctype(c, s->type))
						break;
				} while(s->last++ < s->max);
				if(s->last > s->max) goto skip;
			}
			return(s->last);
		}
	skip:	s->max = s->last = NIL;
		s->tp = N;
		s->type = 0;
	}
#if defined (SUS) || defined (UCB)
	if(s->p && s->last != NIL && *s->p=='-') {
		nextc(s);
		s->max = nextc(s);
		if(s->max==NIL)
			return('-');
		if(s->max < s->last)  {
			s->last = s->max-1;
			return('-');
		}
		return(next(s));
	}
#endif	/* SUS || UCB */
	savep = s->p;
	if(s->p && *s->p=='[') {
		nextc(s);
		s->last = a = nextc(s);
		if(savep[1]==':') {
			char	*sp;
			for(sp=s->p; *sp && *sp != ':'; sp++);
			if(sp[0]==0 || sp[1]!=']') bad();
			*sp=0;
			if((s->type = wctype(s->p)) == 0) bad();
			if(strcmp(s->p,"upper")==0) s->tp = U;
			else if(strcmp(s->p,"lower")==0) s->tp = L;
			*sp=':';
			s->max = ccnt-1;
			s->last = NIL;
			s->p = &sp[2];
			return(next(s));
		}
		if(a=='=') {
			s->last = nextc(s);
			if(s->p[0]!='=' || s->p[1]!=']') bad();
			nextc(s); nextc(s);
			return s->last;
		}
		s->max = NIL;
		switch(s->p ? *s->p : 0) {
#if defined (SUS) || defined (UCB)
		default:
			s->p = savep;
			break;
#else	/* !SUS, !UCB */
		case '-': {
			int b;
			nextc(s);
			b = nextc(s);
			if(b<a || *s->p++!=']')
				bad();
			s->max = b;
			return(a);
		    }
		default:
			bad();
#endif	/* !SUS, !UCB */
		case '*':
			nextc(s);
			base = (*s->p=='0')?8:10;
			n = 0;
			while((c = *s->p)>='0' && c<'0'+base) {
				n = base*n + c - '0';
				s->p++;
			}
			if(*s->p++!=']') bad();
			if(n==0) n = INT_MAX;
			s->rep = n;
			return(a);
		}
	}
	return(s->last = nextc(s));
}

static int
nextc(struct string *s)
{
	char mb[MB_LEN_MAX+1];
	wchar_t wc;
	int c, i, n;
	mbstate_t state;

	if(s->p == NULL)
		return NIL;
	c = nextb(s);
	if(c!=NIL && mb_cur_max>1 && c&0200) {
		i = 0;
		for(;;) {
			mb[i++] = c;
			mb[i] = '\0';
			memset(&state, 0, sizeof state);
			if((n = mbrtowc(&wc, mb, i, &state))==(size_t)-1)
				bad();
			if(n==(size_t)-2) {
				if((c = nextb(s))==NIL) bad();
				continue;
			}
			return(wc);
		}
	}
	return(c);
}

static int
nextb(struct string *s)
{
	int c, i, n = -1;

	if (s->p == NULL)
		return NIL;
	c = *s->p++;
	if(c=='\\') {
		i = n = 0;
		while(i<3 && (c = *s->p)>='0' && c<='7') {
			n = n*8 + c - '0';
			i++;
			s->p++;
		}
		if(i>0) c = n;
		else {
			switch (c = *s->p++) {
			case 'a': c = '\a'; break;
			case 'b': c = '\b'; break;
			case 'f': c = '\f'; break;
			case 'n': c = '\n'; break;
			case 'r': c = '\r'; break;
			case 't': c = '\t'; break;
			case 'v': c = '\v'; break;
			}
		}
	}
	if (s->p[-1] == '\0')
		s->p = NULL;
	return(c || n>=0 ? c&0377 : NIL);
}

static void
bad(void)
{
	write(2,"Bad string\n",11);
	_exit(2);
}

static int
caseconv(wint_t (*to)(wint_t), int (*is)(wint_t))
{
	int c, d = NIL, i;

	string1.tp = string2.tp = N;
	string1.type = string2.type = 0;
	string1.last = string2.last = NIL;
	string1.max = string2.max = NIL;
	for(i=0;i<ccnt;i++) {
		c = mb_cur_max==1 ? btowc(i) : i;
		if(c!=WEOF) {
			d = (*to)(c);
			code[i] = mb_cur_max==1 ? wctob(d) : d;
			if(sflag && d<ccnt && (*is)(d)) squeez[d] = 1;
		}
	}
	return(d);
}
