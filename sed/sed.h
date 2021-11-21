/*
 * sed -- stream  editor
 *
 * Copyright 1975 Bell Telephone Laboratories, Incorporated
 *
 * Owner: lem
 */

/*	from Unix 7th Edition and Unix 32V sed	*/
/*	Sccsid @(#)sed.h	1.32 (gritter) 2/6/05	*/
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

#include <sys/types.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#if defined (SUS) || defined (SU3) || defined (S42)
#include <regex.h>
#endif	/* SUS || SU3 || S42 */

#ifdef	__GLIBC__
#ifdef	_IO_getc_unlocked
#undef	getc
#define	getc(f)		_IO_getc_unlocked(f)
#endif	/* _IO_getc_unlocked */
#ifdef	_IO_putc_unlocked
#undef	putc
#define	putc(c, f)	_IO_putc_unlocked(c, f)
#endif	/* _IO_putc_unlocked */
#endif	/* __GLIBC__ */

#define	CEND	16
#define	CLNUM	14

#if defined (SUS) || defined (SU3) || defined (S42)
struct	re_emu {
	char	*r_dummy;
	regex_t	r_preg;
};
#endif	/* SUS || SU3 || S42 */

extern int	circf, ceof, nbra, sed;

struct	yitem {
	struct yitem	*y_nxt;
	wint_t	y_oc;
	wint_t	y_yc;
	char	y_mc[MB_LEN_MAX];
};

extern int	ABUFSIZE;
extern int	LBSIZE;
extern struct reptr	**abuf;
extern int	aptr;
extern char	*genbuf;
extern int	gbend;
extern int	lbend;
extern int	hend;
extern char	*linebuf;
extern char	*holdsp;
extern int	nflag;
extern long long	*tlno;

enum cmd {
	ACOM	= 01,
	BCOM	= 020,
	CCOM	= 02,
	CDCOM	= 025,
	CNCOM	= 022,
	COCOM	= 017,
	CPCOM	= 023,
	DCOM	= 03,
	ECOM	= 015,
	EQCOM	= 013,
	FCOM	= 016,
	GCOM	= 027,
	CGCOM	= 030,
	HCOM	= 031,
	CHCOM	= 032,
	ICOM	= 04,
	LCOM	= 05,
	NCOM	= 012,
	PCOM	= 010,
	QCOM	= 011,
	RCOM	= 06,
	SCOM	= 07,
	TCOM	= 021,
	WCOM	= 014,
	CWCOM	= 024,
	YCOM	= 026,
	XCOM	= 033
};

extern char	*cp;

#define	P(n)	((n) > 0 ? &ptrspace[n - 1] : (struct reptr *)0)
#define	L(n)	((n) > 0 ? &ltab[n - 1] : (struct label *)0)
#define	A(n)	((n) > 0 ? &abuf[n - 1] : (struct reptr **)0)

#define	slno(ep, n)	( \
				*(ep)++ = ((n) & 0xff000000) >> 24, \
				*(ep)++ = ((n) & 0x00ff0000) >> 16, \
				*(ep)++ = ((n) & 0x0000ff00) >> 8, \
				*(ep)++ = ((n) & 0x000000ff) \
			)

#define	glno(p)		( \
				((p)[0]&0377) << 24 | \
				((p)[1]&0377) << 16 | \
				((p)[2]&0377) << 8 | \
				((p)[3]&0377) \
			)

struct	reptr {
	char	*ad1;
	char	*ad2;
	union {
		char	*re1;
		int	lb1;
	} bptr;
	char	*rhs;
	FILE	*fcode;
	enum cmd	command;
	short	gfl;
	char	pfl;
	char	inar;
	char	negfl;
	char	nsub;
};

extern struct	reptr *ptrspace;

struct label {
	char	asc[8*MB_LEN_MAX + 1];
	int	chain;
	int	address;
};

extern int	status;
extern int	multibyte;
extern int	invchar;
extern int	needdol;

extern int	eargc;

extern struct reptr	*pending;
extern char	*badp;

extern void	execute(const char *);
extern void	fatal(const char *, ...);
extern void	nonfatal(const char *, ...);
extern void	aptr_inc(void);
extern wint_t	wc_get(char **, int);
#define fetch(s)	(multibyte ? wc_get(s, 1) : (*(*(s))++ & 0377))
#define	peek(s)		(multibyte ? wc_get(s, 0) : (**(s) & 0377))
extern struct yitem	*ylook(wint_t , struct yitem **, int);
extern void	*smalloc(size_t);
extern void	growsp(const char *);
