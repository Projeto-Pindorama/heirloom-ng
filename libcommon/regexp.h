/*
 * Simple Regular Expression functions. Derived from Unix 7th Edition,
 * /usr/src/cmd/expr.y
 *
 * Modified by Gunnar Ritter, Freiburg i. Br., Germany, February 2002.
 *
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
#define	REGEXP_H_USED	__attribute__ ((used))
#elif defined __GNUC__
#define	REGEXP_H_USED	__attribute__ ((unused))
#else
#define	REGEXP_H_USED
#endif
static const char regexp_h_sccsid[] REGEXP_H_USED =
	"@(#)regexp.sl	1.56 (gritter) 5/29/05";

#if !defined (REGEXP_H_USED_FROM_VI) && !defined (__dietlibc__)
#define	REGEXP_H_WCHARS
#endif

#define	CBRA	2
#define	CCHR	4
#define	CDOT	8
#define	CCL	12
/*	CLNUM	14	used in sed */
/*	CEND	16	used in sed */
#define	CDOL	20
#define	CCEOF	22
#define	CKET	24
#define	CBACK	36
#define	CNCL	40
#define	CBRC	44
#define	CLET	48
#define	CCH1	52
#define	CCH2	56
#define	CCH3	60

#define	STAR	01
#define RNGE	03
#define	REGEXP_H_LEAST	0100

#ifdef	REGEXP_H_WCHARS
#define	CMB	0200
#else	/* !REGEXP_H_WCHARS */
#define	CMB	0
#endif	/* !REGEXP_H_WCHARS */

#define	NBRA	9

#define PLACE(c)	ep[c >> 3] |= bittab[c & 07]
#define ISTHERE(c)	(ep[c >> 3] & bittab[c & 07])

#ifdef	REGEXP_H_WCHARS
#define	REGEXP_H_IS_THERE(ep, c)	((ep)[c >> 3] & bittab[c & 07])
#endif

#include	<ctype.h>
#include	<string.h>
#include	<limits.h>
#ifdef	REGEXP_H_WCHARS
#include	<stdlib.h>
#include	<wchar.h>
#include	<wctype.h>
#endif	/* REGEXP_H_WCHARS */

#define	regexp_h_uletter(c)	(isalpha(c) || (c) == '_')
#ifdef	REGEXP_H_WCHARS
#define	regexp_h_wuletter(c)	(iswalpha(c) || (c) == L'_')

/*
 * Used to allocate memory for the multibyte star algorithm.
 */
#ifndef	regexp_h_malloc
#define	regexp_h_malloc(n)	malloc(n)
#endif
#ifndef	regexp_h_free
#define	regexp_h_free(p)	free(p)
#endif

/*
 * Can be predefined to 'inline' to inline some multibyte functions;
 * may improve performance for files that contain many multibyte
 * sequences.
 */
#ifndef	regexp_h_inline
#define	regexp_h_inline
#endif

/*
 * Mask to determine whether the first byte of a sequence possibly
 * starts a multibyte character. Set to 0377 to force mbtowc() for
 * any byte sequence (except 0).
 */
#ifndef	REGEXP_H_MASK
#define	REGEXP_H_MASK	0200
#endif
#endif	/* REGEXP_H_WCHARS */

/*
 * For regexpr.h.
 */
#ifndef	regexp_h_static
#define	regexp_h_static
#endif
#ifndef	REGEXP_H_STEP_INIT
#define	REGEXP_H_STEP_INIT
#endif
#ifndef	REGEXP_H_ADVANCE_INIT
#define	REGEXP_H_ADVANCE_INIT
#endif

char	*braslist[NBRA];
char	*braelist[NBRA];
int	nbra;
char	*loc1, *loc2, *locs;
int	sed;
int	nodelim;

regexp_h_static int	circf;
regexp_h_static int	low;
regexp_h_static int	size;

regexp_h_static unsigned char	bittab[] = {
	1,
	2,
	4,
	8,
	16,
	32,
	64,
	128
};
static int	regexp_h_advance(register const char *lp,
			register const char *ep);
static void	regexp_h_getrnge(register const char *str, int least);

static const char	*regexp_h_bol;	/* beginning of input line (for \<) */

#ifdef	REGEXP_H_WCHARS
static int	regexp_h_wchars;
static int	regexp_h_mbcurmax;

static const char	*regexp_h_firstwc;	/* location of first
						   multibyte character
						   on input line */

#define	regexp_h_getwc(c)	{ \
	if (regexp_h_wchars) { \
		char mbbuf[MB_LEN_MAX + 1], *mbptr; \
		wchar_t wcbuf; \
		int mb, len; \
		mbptr = mbbuf; \
		do { \
			mb = GETC(); \
			*mbptr++ = mb; \
			*mbptr = '\0'; \
		} while ((len = mbtowc(&wcbuf, mbbuf, regexp_h_mbcurmax)) < 0 \
			&& mb != eof && mbptr < mbbuf + MB_LEN_MAX); \
		if (len == -1) \
			ERROR(67); \
		c = wcbuf; \
	} else { \
		c = GETC(); \
	} \
}

#define	regexp_h_store(wc, mb, me)	{ \
	int len; \
	if (wc == WEOF) \
		ERROR(67); \
	if ((len = me - mb) <= regexp_h_mbcurmax) { \
		char mt[MB_LEN_MAX]; \
		if (wctomb(mt, wc) >= len) \
			ERROR(50); \
	} \
	switch (len = wctomb(mb, wc)) { \
	case -1: \
		 ERROR(67); \
	case 0: \
		mb++; \
		break; \
	default: \
		mb += len; \
	} \
}

static regexp_h_inline wint_t
regexp_h_fetchwc(const char **mb, int islp)
{
	wchar_t wc;
	int len;

	if ((len = mbtowc(&wc, *mb, regexp_h_mbcurmax)) < 0) {
		(*mb)++;
		return WEOF;
	}
	if (islp && regexp_h_firstwc == NULL)
		regexp_h_firstwc = *mb;
	/*if (len == 0) {
		(*mb)++;
		return L'\0';
	} handled in singlebyte code */
	*mb += len;
	return wc;
}

#define	regexp_h_fetch(mb, islp)	((*(mb) & REGEXP_H_MASK) == 0 ? \
						(*(mb)++&0377): \
						regexp_h_fetchwc(&(mb), islp))

static regexp_h_inline wint_t
regexp_h_showwc(const char *mb)
{
	wchar_t wc;

	if (mbtowc(&wc, mb, regexp_h_mbcurmax) < 0)
		return WEOF;
	return wc;
}

#define	regexp_h_show(mb)	((*(mb) & REGEXP_H_MASK) == 0 ? (*(mb)&0377): \
					regexp_h_showwc(mb))

/*
 * Return the character immediately preceding mb. Since no byte is
 * required to be the first byte of a character, the longest multibyte
 * character ending at &[mb-1] is searched.
 */
static regexp_h_inline wint_t
regexp_h_previous(const char *mb)
{
	const char *p = mb;
	wchar_t wc, lastwc = WEOF;
	int len, max = 0;

	if (regexp_h_firstwc == NULL || mb <= regexp_h_firstwc)
		return (mb > regexp_h_bol ? (mb[-1] & 0377) : WEOF);
	while (p-- > regexp_h_bol) {
		mbtowc(NULL, NULL, 0);
		if ((len = mbtowc(&wc, p, mb - p)) >= 0) {
			if (len < max || len < mb - p)
				break;
			max = len;
			lastwc = wc;
		} else if (len < 0 && max > 0)
			break;
	}
	return lastwc;
}

#define	regexp_h_cclass(set, c, af)	\
	((c) == 0 || (c) == WEOF ? 0 : ( \
		((c) > 0177) ? \
			regexp_h_cclass_wc(set, c, af) : ( \
				REGEXP_H_IS_THERE((set)+1, (c)) ? (af) : !(af) \
			) \
		) \
	)

static regexp_h_inline int
regexp_h_cclass_wc(const char *set, register wint_t c, int af)
{
	register wint_t wc, wl = WEOF;
	const char *end;

	end = &set[18] + set[0] - 1;
	set += 17;
	while (set < end) {
		wc = regexp_h_fetch(set, 0);
#ifdef	REGEXP_H_VI_BACKSLASH
		if (wc == '\\' && set < end &&
				(*set == ']' || *set == '-' ||
				 *set == '^' || *set == '\\')) {
			wc = regexp_h_fetch(set, 0);
		} else
#endif	/* REGEXP_H_VI_BACKSLASH */
		if (wc == '-' && wl != WEOF && set < end) {
			wc = regexp_h_fetch(set, 0);
#ifdef	REGEXP_H_VI_BACKSLASH
			if (wc == '\\' && set < end &&
					(*set == ']' || *set == '-' ||
					 *set == '^' || *set == '\\')) {
				wc = regexp_h_fetch(set, 0);
			}
#endif	/* REGEXP_H_VI_BACKSLASH */
			if (c > wl && c < wc)
				return af;
		}
		if (c == wc)
			return af;
		wl = wc;
	}
	return !af;
}
#else	/* !REGEXP_H_WCHARS */
#define	regexp_h_wchars		0
#define	regexp_h_getwc(c)	{ c = GETC(); }
#endif	/* !REGEXP_H_WCHARS */

regexp_h_static char *
compile(char *instring, char *ep, const char *endbuf, int seof)
{
	INIT	/* Dependent declarations and initializations */
	register int c;
	register int eof = seof;
	char *lastep = instring;
	int cclcnt;
	char bracket[NBRA], *bracketp;
	int closed;
	char neg;
	int lc;
	int i, cflg;

#ifdef	REGEXP_H_WCHARS
	char *eq;
	regexp_h_mbcurmax = MB_CUR_MAX;
	regexp_h_wchars = regexp_h_mbcurmax > 1 ? CMB : 0;
#endif
	lastep = 0;
	bracketp = bracket;
	if((c = GETC()) == eof || c == '\n') {
		if (c == '\n') {
			UNGETC(c);
			nodelim = 1;
		}
		if(*ep == 0 && !sed)
			ERROR(41);
		if (bracketp > bracket)
			ERROR(42);
		RETURN(ep);
	}
	circf = closed = nbra = 0;
	if (c == '^')
		circf++;
	else
		UNGETC(c);
	for (;;) {
		if (ep >= endbuf)
			ERROR(50);
		regexp_h_getwc(c);
		if(c != '*' && ((c != '\\') || (PEEKC() != '{')))
			lastep = ep;
		if (c == eof) {
			*ep++ = CCEOF;
			if (bracketp > bracket)
				ERROR(42);
			RETURN(ep);
		}
		switch (c) {

		case '.':
			*ep++ = CDOT|regexp_h_wchars;
			continue;

		case '\n':
			if (sed == 0) {
				UNGETC(c);
				*ep++ = CCEOF;
				nodelim = 1;
				RETURN(ep);
			}
			ERROR(36);
		case '*':
			if (lastep==0 || *lastep==CBRA || *lastep==CKET ||
					*lastep==(CBRC|regexp_h_wchars) ||
					*lastep==(CLET|regexp_h_wchars))
				goto defchar;
			*lastep |= STAR;
			continue;

		case '$':
			if(PEEKC() != eof)
				goto defchar;
			*ep++ = CDOL;
			continue;

		case '[':
#ifdef	REGEXP_H_WCHARS
			if (regexp_h_wchars == 0) {
#endif
				if(&ep[33] >= endbuf)
					ERROR(50);

				*ep++ = CCL;
				lc = 0;
				for(i = 0; i < 32; i++)
					ep[i] = 0;

				neg = 0;
				if((c = GETC()) == '^') {
					neg = 1;
					c = GETC();
				}

				do {
					c &= 0377;
					if(c == '\0' || c == '\n')
						ERROR(49);
#ifdef	REGEXP_H_VI_BACKSLASH
					if(c == '\\' && ((c = PEEKC()) == ']' ||
							c == '-' || c == '^' ||
							c == '\\')) {
						c = GETC();
						c &= 0377;
					} else
#endif	/* REGEXP_H_VI_BACKSLASH */
					if(c == '-' && lc != 0) {
						if ((c = GETC()) == ']') {
							PLACE('-');
							break;
						}
#ifdef	REGEXP_H_VI_BACKSLASH
						if(c == '\\' &&
							((c = PEEKC()) == ']' ||
								c == '-' ||
								c == '^' ||
								c == '\\'))
							c = GETC();
#endif	/* REGEXP_H_VI_BACKSLASH */
						c &= 0377;
						while(lc < c) {
							PLACE(lc);
							lc++;
						}
					}
					lc = c;
					PLACE(c);
				} while((c = GETC()) != ']');
				if(neg) {
					for(cclcnt = 0; cclcnt < 32; cclcnt++)
						ep[cclcnt] ^= 0377;
					ep[0] &= 0376;
				}

				ep += 32;
#ifdef	REGEXP_H_WCHARS
			} else {
				if (&ep[18] >= endbuf)
					ERROR(50);
				*ep++ = CCL|CMB;
				*ep++ = 0;
				lc = 0;
				for (i = 0; i < 16; i++)
					ep[i] = 0;
				eq = &ep[16];
				regexp_h_getwc(c);
				if (c == L'^') {
					regexp_h_getwc(c);
					ep[-2] = CNCL|CMB;
				}
				do {
					if (c == '\0' || c == '\n')
						ERROR(49);
#ifdef	REGEXP_H_VI_BACKSLASH
					if(c == '\\' && ((c = PEEKC()) == ']' ||
							c == '-' || c == '^' ||
							c == '\\')) {
						regexp_h_store(c, eq, endbuf);
						regexp_h_getwc(c);
					} else
#endif	/* REGEXP_H_VI_BACKSLASH */
					if (c == '-' && lc != 0 && lc <= 0177) {
						regexp_h_store(c, eq, endbuf);
						regexp_h_getwc(c);
						if (c == ']') {
							PLACE('-');
							break;
						}
#ifdef	REGEXP_H_VI_BACKSLASH
						if(c == '\\' &&
							((c = PEEKC()) == ']' ||
								c == '-' ||
								c == '^' ||
								c == '\\')) {
							regexp_h_store(c, eq,
								endbuf);
							regexp_h_getwc(c);
						}
#endif	/* REGEXP_H_VI_BACKSLASH */
						while (lc < (c & 0177)) {
							PLACE(lc);
							lc++;
						}
					}
					lc = c;
					if (c <= 0177)
						PLACE(c);
					regexp_h_store(c, eq, endbuf);
					regexp_h_getwc(c);
				} while (c != L']');
				if ((i = eq - &ep[16]) > 255)
					ERROR(50);
				lastep[1] = i;
				ep = eq;
			}
#endif	/* REGEXP_H_WCHARS */

			continue;

		case '\\':
			regexp_h_getwc(c);
			switch(c) {

			case '(':
				if(nbra >= NBRA)
					ERROR(43);
				*bracketp++ = nbra;
				*ep++ = CBRA;
				*ep++ = nbra++;
				continue;

			case ')':
				if(bracketp <= bracket)
					ERROR(42);
				*ep++ = CKET;
				*ep++ = *--bracketp;
				closed++;
				continue;

			case '<':
				*ep++ = CBRC|regexp_h_wchars;
				continue;

			case '>':
				*ep++ = CLET|regexp_h_wchars;
				continue;

			case '{':
				if(lastep == (char *) (0))
					goto defchar;
				*lastep |= RNGE;
				cflg = 0;
			nlim:
				c = GETC();
				i = 0;
				do {
					if ('0' <= c && c <= '9')
						i = 10 * i + c - '0';
					else
						ERROR(16);
				} while(((c = GETC()) != '\\') && (c != ','));
				if (i > 255)
					ERROR(11);
				*ep++ = i;
				if (c == ',') {
					if(cflg++)
						ERROR(44);
					if((c = GETC()) == '\\') {
						*ep++ = (char)255;
						*lastep |= REGEXP_H_LEAST;
					} else {
						UNGETC(c);
						goto nlim; /* get 2'nd number */
					}
				}
				if(GETC() != '}')
					ERROR(45);
				if(!cflg)	/* one number */
					*ep++ = i;
				else if((ep[-1] & 0377) < (ep[-2] & 0377))
					ERROR(46);
				continue;

			case '\n':
				ERROR(36);

			case 'n':
				c = '\n';
				goto defchar;

			default:
				if(c >= '1' && c <= '9') {
					if((c -= '1') >= closed)
						ERROR(25);
					*ep++ = CBACK;
					*ep++ = c;
					continue;
				}
			}
			/* Drop through to default to use \ to turn off special chars */

		defchar:
		default:
			lastep = ep;
#ifdef	REGEXP_H_WCHARS
			if (regexp_h_wchars == 0) {
#endif
				*ep++ = CCHR;
				*ep++ = c;
#ifdef	REGEXP_H_WCHARS
			} else {
				char	mbbuf[MB_LEN_MAX];

				switch (wctomb(mbbuf, c)) {
				case 1: *ep++ = CCH1;
					break;
				case 2:	*ep++ = CCH2;
					break;
				case 3:	*ep++ = CCH3;
					break;
				default:
					*ep++ = CCHR|CMB;
				}
				regexp_h_store(c, ep, endbuf);
			}
#endif	/* REGEXP_H_WCHARS */
		}
	}
}

int
step(const char *p1, const char *p2)
{
	register int c;
#ifdef	REGEXP_H_WCHARS
	register int d;
#endif	/* REGEXP_H_WCHARS */

	REGEXP_H_STEP_INIT	/* get circf */
	regexp_h_bol = p1;
#ifdef	REGEXP_H_WCHARS
	regexp_h_firstwc = NULL;
#endif	/* REGEXP_H_WCHARS */
	if (circf) {
		loc1 = (char *)p1;
		return(regexp_h_advance(p1, p2));
	}
	/* fast check for first character */
	if (*p2==CCHR) {
		c = p2[1] & 0377;
		do {
			if ((*p1 & 0377) != c)
				continue;
			if (regexp_h_advance(p1, p2)) {
				loc1 = (char *)p1;
				return(1);
			}
		} while (*p1++);
		return(0);
	}
#ifdef	REGEXP_H_WCHARS
	else if (*p2==CCH1) {
		do {
			if (p1[0] == p2[1] && regexp_h_advance(p1, p2)) {
				loc1 = (char *)p1;
				return(1);
			}
			c = regexp_h_fetch(p1, 1);
		} while (c);
		return(0);
	} else if (*p2==CCH2) {
		do {
			if (p1[0] == p2[1] && p1[1] == p2[2] &&
					regexp_h_advance(p1, p2)) {
				loc1 = (char *)p1;
				return(1);
			}
			c = regexp_h_fetch(p1, 1);
		} while (c);
		return(0);
	} else if (*p2==CCH3) {
		do {
			if (p1[0] == p2[1] && p1[1] == p2[2] && p1[2] == p2[3]&&
					regexp_h_advance(p1, p2)) {
				loc1 = (char *)p1;
				return(1);
			}
			c = regexp_h_fetch(p1, 1);
		} while (c);
		return(0);
	} else if ((*p2&0377)==(CCHR|CMB)) {
		d = regexp_h_fetch(p2, 0);
		do {
			c = regexp_h_fetch(p1, 1);
			if (c == d && regexp_h_advance(p1, p2)) {
				loc1 = (char *)p1;
				return(1);
			}
		} while(c);
		return(0);
	}
		/* regular algorithm */
	if (regexp_h_wchars)
		do {
			if (regexp_h_advance(p1, p2)) {
				loc1 = (char *)p1;
				return(1);
			}
			c = regexp_h_fetch(p1, 1);
		} while (c);
	else
#endif	/* REGEXP_H_WCHARS */
		do {
			if (regexp_h_advance(p1, p2)) {
				loc1 = (char *)p1;
				return(1);
			}
		} while (*p1++);
	return(0);
}

#ifdef	REGEXP_H_WCHARS
/*
 * It is painfully slow to read character-wise backwards in a
 * multibyte string (see regexp_h_previous() above). For the star
 * algorithm, we therefore keep track of every character as it is
 * read in forward direction.
 *
 * Don't use alloca() for stack blocks since there is no measurable
 * speedup and huge amounts of memory are used up for long input
 * lines.
 */
#ifndef	REGEXP_H_STAKBLOK
#define	REGEXP_H_STAKBLOK	1000
#endif

struct	regexp_h_stack {
	struct regexp_h_stack	*s_nxt;
	struct regexp_h_stack	*s_prv;
	const char	*s_ptr[REGEXP_H_STAKBLOK];
};

#define	regexp_h_push(sb, sp, sc, lp)	(regexp_h_wchars ? \
			regexp_h_pushwc(sb, sp, sc, lp) : (void)0)

static regexp_h_inline void
regexp_h_pushwc(struct regexp_h_stack **sb,
		struct regexp_h_stack **sp,
		const char ***sc, const char *lp)
{
	if (regexp_h_firstwc == NULL || lp < regexp_h_firstwc)
		return;
	if (*sb == NULL) {
		if ((*sb = regexp_h_malloc(sizeof **sb)) == NULL)
			return;
		(*sb)->s_nxt = (*sb)->s_prv = NULL;
		*sp = *sb;
		*sc = &(*sb)->s_ptr[0];
	} else if (*sc >= &(*sp)->s_ptr[REGEXP_H_STAKBLOK]) {
		if ((*sp)->s_nxt == NULL) {
			struct regexp_h_stack	*bq;

			if ((bq = regexp_h_malloc(sizeof *bq)) == NULL)
				return;
			bq->s_nxt = NULL;
			bq->s_prv = *sp;
			(*sp)->s_nxt = bq;
			*sp = bq;
		} else
			*sp = (*sp)->s_nxt;
		*sc = &(*sp)->s_ptr[0];
	}
	*(*sc)++ = lp;
}

static regexp_h_inline const char *
regexp_h_pop(struct regexp_h_stack **sb, struct regexp_h_stack **sp,
		const char ***sc, const char *lp)
{
	if (regexp_h_firstwc == NULL || lp <= regexp_h_firstwc)
		return &lp[-1];
	if (*sp == NULL)
		return regexp_h_firstwc;
	if (*sc == &(*sp)->s_ptr[0]) {
		if ((*sp)->s_prv == NULL) {
			regexp_h_free(*sp);
			*sp = NULL;
			*sb = NULL;
			return regexp_h_firstwc;
		}
		*sp = (*sp)->s_prv;
		regexp_h_free((*sp)->s_nxt);
		(*sp)->s_nxt = NULL ;
		*sc = &(*sp)->s_ptr[REGEXP_H_STAKBLOK];
	}
	return *(--(*sc));
}

static void
regexp_h_zerostak(struct regexp_h_stack **sb, struct regexp_h_stack **sp)
{
	for (*sp = *sb; *sp && (*sp)->s_nxt; *sp = (*sp)->s_nxt)
		if ((*sp)->s_prv)
			regexp_h_free((*sp)->s_prv);
	if (*sp) {
		if ((*sp)->s_prv)
			regexp_h_free((*sp)->s_prv);
		regexp_h_free(*sp);
	}
	*sp = *sb = NULL;
}
#else	/* !REGEXP_H_WCHARS */
#define	regexp_h_push(sb, sp, sc, lp)
#endif	/* !REGEXP_H_WCHARS */

static int
regexp_h_advance(const char *lp, const char *ep)
{
	register const char *curlp;
	int c, least;
#ifdef	REGEXP_H_WCHARS
	int d;
	struct regexp_h_stack	*sb = NULL, *sp = NULL;
	const char	**sc;
#endif	/* REGEXP_H_WCHARS */
	char *bbeg;
	int ct;

	for (;;) switch (least = *ep++ & 0377, least & ~REGEXP_H_LEAST) {

	case CCHR:
#ifdef	REGEXP_H_WCHARS
	case CCH1:
#endif
		if (*ep++ == *lp++)
			continue;
		return(0);

#ifdef	REGEXP_H_WCHARS
	case CCHR|CMB:
		if (regexp_h_fetch(ep, 0) == regexp_h_fetch(lp, 1))
			continue;
		return(0);

	case CCH2:
		if (ep[0] == lp[0] && ep[1] == lp[1]) {
			ep += 2, lp += 2;
			continue;
		}
		return(0);

	case CCH3:
		if (ep[0] == lp[0] && ep[1] == lp[1] && ep[2] == lp[2]) {
			ep += 3, lp += 3;
			continue;
		}
		return(0);
#endif	/* REGEXP_H_WCHARS */

	case CDOT:
		if (*lp++)
			continue;
		return(0);
#ifdef	REGEXP_H_WCHARS
	case CDOT|CMB:
		if ((c = regexp_h_fetch(lp, 1)) != L'\0' && c != WEOF)
			continue;
		return(0);
#endif	/* REGEXP_H_WCHARS */

	case CDOL:
		if (*lp==0)
			continue;
		return(0);

	case CCEOF:
		loc2 = (char *)lp;
		return(1);

	case CCL:
		c = *lp++ & 0377;
		if(ISTHERE(c)) {
			ep += 32;
			continue;
		}
		return(0);

#ifdef	REGEXP_H_WCHARS
	case CCL|CMB:
	case CNCL|CMB:
		c = regexp_h_fetch(lp, 1);
		if (regexp_h_cclass(ep, c, (ep[-1] & 0377) == (CCL|CMB))) {
			ep += (*ep & 0377) + 17;
			continue;
		}
		return 0;
#endif	/* REGEXP_H_WCHARS */

	case CBRA:
		braslist[*ep++ & 0377] = (char *)lp;
		continue;

	case CKET:
		braelist[*ep++ & 0377] = (char *)lp;
		continue;

	case CBRC:
		if (lp == regexp_h_bol && locs == NULL)
			continue;
		if ((isdigit(lp[0] & 0377) || regexp_h_uletter(lp[0] & 0377))
				&& !regexp_h_uletter(lp[-1] & 0377)
				&& !isdigit(lp[-1] & 0377))
			continue;
		return(0);

#ifdef	REGEXP_H_WCHARS
	case CBRC|CMB:
		c = regexp_h_show(lp);
		d = regexp_h_previous(lp);
		if ((iswdigit(c) || regexp_h_wuletter(c))
				&& !regexp_h_wuletter(d)
				&& !iswdigit(d))
			continue;
		return(0);
#endif	/* REGEXP_H_WCHARS */

	case CLET:
		if (!regexp_h_uletter(lp[0] & 0377) && !isdigit(lp[0] & 0377))
			continue;
		return(0);

#ifdef	REGEXP_H_WCHARS
	case CLET|CMB:
		c = regexp_h_show(lp);
		if (!regexp_h_wuletter(c) && !iswdigit(c))
			continue;
		return(0);
#endif	/* REGEXP_H_WCHARS */

	case CCHR|RNGE:
		c = *ep++;
		regexp_h_getrnge(ep, least);
		while(low--)
			if(*lp++ != c)
				return(0);
		curlp = lp;
		while(size--) {
			regexp_h_push(&sb, &sp, &sc, lp);
			if(*lp++ != c)
				break;
		}
		if(size < 0) {
			regexp_h_push(&sb, &sp, &sc, lp);
			lp++;
		}
		ep += 2;
		goto star;

#ifdef	REGEXP_H_WCHARS
	case CCHR|RNGE|CMB:
	case CCH1|RNGE:
	case CCH2|RNGE:
	case CCH3|RNGE:
		c = regexp_h_fetch(ep, 0);
		regexp_h_getrnge(ep, least);
		while (low--)
			if (regexp_h_fetch(lp, 1) != c)
				return 0;
		curlp = lp;
		while (size--) {
			regexp_h_push(&sb, &sp, &sc, lp);
			if (regexp_h_fetch(lp, 1) != c)
				break;
		}
		if(size < 0) {
			regexp_h_push(&sb, &sp, &sc, lp);
			regexp_h_fetch(lp, 1);
		}
		ep += 2;
		goto star;
#endif	/* REGEXP_H_WCHARS */

	case CDOT|RNGE:
		regexp_h_getrnge(ep, least);
		while(low--)
			if(*lp++ == '\0')
				return(0);
		curlp = lp;
		while(size--) {
			regexp_h_push(&sb, &sp, &sc, lp);
			if(*lp++ == '\0')
				break;
		}
		if(size < 0) {
			regexp_h_push(&sb, &sp, &sc, lp);
			lp++;
		}
		ep += 2;
		goto star;

#ifdef	REGEXP_H_WCHARS
	case CDOT|RNGE|CMB:
		regexp_h_getrnge(ep, least);
		while (low--)
			if ((c = regexp_h_fetch(lp, 1)) == L'\0' || c == WEOF)
				return 0;
		curlp = lp;
		while (size--) {
			regexp_h_push(&sb, &sp, &sc, lp);
			if ((c = regexp_h_fetch(lp, 1)) == L'\0' || c == WEOF)
				break;
		}
		if (size < 0) {
			regexp_h_push(&sb, &sp, &sc, lp);
			regexp_h_fetch(lp, 1);
		}
		ep += 2;
		goto star;
#endif	/* REGEXP_H_WCHARS */

	case CCL|RNGE:
		regexp_h_getrnge(ep + 32, least);
		while(low--) {
			c = *lp++ & 0377;
			if(!ISTHERE(c))
				return(0);
		}
		curlp = lp;
		while(size--) {
			regexp_h_push(&sb, &sp, &sc, lp);
			c = *lp++ & 0377;
			if(!ISTHERE(c))
				break;
		}
		if(size < 0) {
			regexp_h_push(&sb, &sp, &sc, lp);
			lp++;
		}
		ep += 34;		/* 32 + 2 */
		goto star;

#ifdef	REGEXP_H_WCHARS
	case CCL|RNGE|CMB:
	case CNCL|RNGE|CMB:
		regexp_h_getrnge(ep + (*ep & 0377) + 17, least);
		while (low--) {
			c = regexp_h_fetch(lp, 1);
			if (!regexp_h_cclass(ep, c,
					(ep[-1] & 0377 & ~REGEXP_H_LEAST)
					== (CCL|RNGE|CMB)))
				return 0;
		}
		curlp = lp;
		while (size--) {
			regexp_h_push(&sb, &sp, &sc, lp);
			c = regexp_h_fetch(lp, 1);
			if (!regexp_h_cclass(ep, c,
					(ep[-1] & 0377 & ~REGEXP_H_LEAST)
					== (CCL|RNGE|CMB)))
				break;
		}
		if (size < 0) {
			regexp_h_push(&sb, &sp, &sc, lp);
			regexp_h_fetch(lp, 1);
		}
		ep += (*ep & 0377) + 19;
		goto star;
#endif	/* REGEXP_H_WCHARS */

	case CBACK:
		bbeg = braslist[*ep & 0377];
		ct = braelist[*ep++ & 0377] - bbeg;

		if(strncmp(bbeg, lp, ct) == 0) {
			lp += ct;
			continue;
		}
		return(0);

	case CBACK|STAR:
		bbeg = braslist[*ep & 0377];
		ct = braelist[*ep++ & 0377] - bbeg;
		curlp = lp;
		while(strncmp(bbeg, lp, ct) == 0)
			lp += ct;

		while(lp >= curlp) {
			if(regexp_h_advance(lp, ep))	return(1);
			lp -= ct;
		}
		return(0);


	case CDOT|STAR:
		curlp = lp;
		do
			regexp_h_push(&sb, &sp, &sc, lp);
		while (*lp++);
		goto star;

#ifdef	REGEXP_H_WCHARS
	case CDOT|STAR|CMB:
		curlp = lp;
		do
			regexp_h_push(&sb, &sp, &sc, lp);
		while ((c = regexp_h_fetch(lp, 1)) != L'\0' && c != WEOF);
		goto star;
#endif	/* REGEXP_H_WCHARS */

	case CCHR|STAR:
		curlp = lp;
		do
			regexp_h_push(&sb, &sp, &sc, lp);
		while (*lp++ == *ep);
		ep++;
		goto star;

#ifdef	REGEXP_H_WCHARS
	case CCHR|STAR|CMB:
	case CCH1|STAR:
	case CCH2|STAR:
	case CCH3|STAR:
		curlp = lp;
		d = regexp_h_fetch(ep, 0);
		do
			regexp_h_push(&sb, &sp, &sc, lp);
		while (regexp_h_fetch(lp, 1) == d);
		goto star;
#endif	/* REGEXP_H_WCHARS */

	case CCL|STAR:
		curlp = lp;
		do {
			regexp_h_push(&sb, &sp, &sc, lp);
			c = *lp++ & 0377;
		} while(ISTHERE(c));
		ep += 32;
		goto star;

#ifdef	REGEXP_H_WCHARS
	case CCL|STAR|CMB:
	case CNCL|STAR|CMB:
		curlp = lp;
		do {
			regexp_h_push(&sb, &sp, &sc, lp);
			c = regexp_h_fetch(lp, 1);
		} while (regexp_h_cclass(ep, c, (ep[-1] & 0377)
					== (CCL|STAR|CMB)));
		ep += (*ep & 0377) + 17;
		goto star;
#endif	/* REGEXP_H_WCHARS */

	star:
#ifdef	REGEXP_H_WCHARS
		if (regexp_h_wchars == 0) {
#endif
			do {
				if(--lp == locs)
					break;
				if (regexp_h_advance(lp, ep))
					return(1);
			} while (lp > curlp);
#ifdef	REGEXP_H_WCHARS
		} else {
			do {
				lp = regexp_h_pop(&sb, &sp, &sc, lp);
				if (lp <= locs)
					break;
				if (regexp_h_advance(lp, ep)) {
					regexp_h_zerostak(&sb, &sp);
					return(1);
				}
			} while (lp > curlp);
			regexp_h_zerostak(&sb, &sp);
		}
#endif	/* REGEXP_H_WCHARS */
		return(0);

	}
}

static void
regexp_h_getrnge(register const char *str, int least)
{
	low = *str++ & 0377;
	size = least & REGEXP_H_LEAST ? /*20000*/INT_MAX : (*str & 0377) - low;
}

int
advance(const char *lp, const char *ep)
{
	REGEXP_H_ADVANCE_INIT	/* skip past circf */
	regexp_h_bol = lp;
#ifdef	REGEXP_H_WCHARS
	regexp_h_firstwc = NULL;
#endif	/* REGEXP_H_WCHARS */
	return regexp_h_advance(lp, ep);
}
