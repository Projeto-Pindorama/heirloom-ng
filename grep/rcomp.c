/*
 * grep - search a file for a pattern
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, April 2001.
 */
/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute
 * it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 */

/*	Sccsid @(#)rcomp.c	1.27 (gritter) 2/6/05>	*/

/*
 * Code involving POSIX.2 regcomp()/regexpr() routines.
 */

#include	"grep.h"
#include	"alloc.h"
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<mbtowi.h>

static int	emptypat;

#ifdef	UXRE
#include	<regdfa.h>
static int	rc_range(struct iblok *, char *);
static int	rc_rangew(struct iblok *, char *);
#endif

/*
 * Check whether line matches any pattern of the pattern list.
 */
static int
rc_match(const char *str, size_t sz)
{
#ifndef	UXRE
	struct expr *e;
#endif
	regmatch_t pmatch[1];
	int gotcha = 0;

	if (emptypat) {
		if (xflag) {
			if (*str == '\0')
				return 1;
		} else
			return 1;
	}
#ifdef	UXRE
	if (e0->e_exp)
		gotcha = (regexec(e0->e_exp, str, 1, pmatch, 0) == 0);
#else	/* !UXRE */
	for (e = e0; e; e = e->e_nxt) {
		if (e->e_exp) {
			gotcha = (regexec(e->e_exp, str, 1, pmatch, 0) == 0);
			if (gotcha)
				break;
		}
	}
#endif	/* !UXRE */
	if (gotcha)
		if (!xflag || (pmatch[0].rm_so == 0
				&& pmatch[0].rm_eo == sz))
			return 1;
	return 0;
}

/*
 * Compile a pattern structure using regcomp().
 */
static void
rc_build(void)
{
	int rerror = REG_BADPAT;
	int rflags = 0;
	size_t sz;
#ifdef	UXRE
	char *pat, *cp;
#endif	/* UXRE */
	struct expr *e;

	if ((e0->e_flg & E_NULL) == 0) {
		for (sz = 0, e = e0; e; e = e->e_nxt) {
			if (e->e_len > 0)
				sz += e->e_len + 1;
			else
				emptypat = 1;
		}
	} else
		sz = 1;
	if ((e0->e_flg & E_NULL || emptypat) && sus == 0)
		rc_error(e0, rerror);
	if (sz == 0 || (emptypat && xflag == 0)) {
		e0->e_exp = NULL;
		return;
	}
#ifdef	UXRE
	pat = smalloc(sz);
	for (cp = pat, e = e0; e; e = e->e_nxt) {
		if (e->e_len > 0) {
			memcpy(cp, e->e_pat, e->e_len);
			cp[e->e_len] = '\n';
			cp = &cp[e->e_len + 1];
		}
	}
	pat[sz - 1] = '\0';
	if (iflag)
		rflags |= REG_ICASE;
	if (Eflag)
		rflags |= (sus ? REG_EXTENDED : REG_OLDERE|REG_NOI18N) |
			REG_MTPARENBAD;
	else {
		rflags |= REG_ANGLES;
		if (sus >= 3)
			rflags |= REG_AVOIDNULL;
	}
	if (xflag)
		rflags |= REG_ONESUB;
	else
		rflags |= REG_NOSUB;
	if ((e = e0)->e_nxt)
		rflags |= REG_NLALT;
	e->e_exp = (regex_t *)smalloc(sizeof *e->e_exp);
	if ((rerror = regcomp(e->e_exp, pat, rflags)) != 0)
		rc_error(e, rerror);
	free(pat);
	if (!xflag && e->e_exp->re_flags & REG_DFA)
		range = mbcode ? rc_rangew : rc_range;
#else	/* !UXRE */
	if (iflag)
		rflags |= REG_ICASE;
	if (Eflag)
		rflags |= REG_EXTENDED;
	if (!xflag)
		rflags |= REG_NOSUB;
	for (e = e0; e; e = e->e_nxt) {
		e->e_exp = (regex_t *)smalloc(sizeof *e->e_exp);
		if ((rerror = regcomp(e->e_exp, e->e_pat, rflags)) != 0)
			rc_error(e, rerror);
	}
#endif	/* !UXRE */
}

void
rc_select(void)
{
	build = rc_build;
	match = rc_match;
	matchflags |= MF_NULTERM;
	matchflags &= ~MF_LOCONV;
}

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
#ifdef	UXRE
/*
 * Range search for singlebyte locales using the modified UNIX(R) Regular
 * Expression Library DFA.
 */
static int
rc_range(struct iblok *ip, char *last)
{
	char	*p;
	int	c, cstat, nstat;
	Dfa	*dp = e0->e_exp->re_dfa;

	p = ip->ib_cur;
	lineno++;
	cstat = dp->anybol;
	if (dp->acc[cstat])
		goto found;
	for (;;) {
		if ((nstat = dp->trans[cstat][*p & 0377]) == 0) {
			/*
			 * '\0' is used to indicate end-of-line. If a '\0'
			 * character appears in input, it matches '$' but
			 * the DFA remains in dead state afterwards; there
			 * is thus no need to handle this condition
			 * specially to get the same behavior as in plain
			 * regexec().
			 */
			if ((c = *p & 0377) == '\n')
				c = '\0';
			if ((nstat = regtrans(dp, cstat, c, 1)) == 0)
				goto fail;
			dp->trans[cstat]['\n'] = dp->trans[cstat]['\0'];
		}
		if (dp->acc[cstat = nstat - 1]) {
		found:	for (;;) {
				if (vflag == 0) {
		succeed:		outline(ip, last, p - ip->ib_cur);
					if (qflag || lflag)
						return 1;
				} else {
		fail:			ip->ib_cur = p;
					while (*ip->ib_cur++ != '\n');
				}
				if ((p = ip->ib_cur) > last)
					return 0;
				lineno++;
				if (dp->acc[cstat = dp->anybol] == 0)
					goto brk2;
			}
		}
		if (*p++ == '\n') {
			if (vflag) {
				p--;
				goto succeed;
			}
			if ((ip->ib_cur = p) > last)
				return 0;
			lineno++;
			if (dp->acc[cstat = dp->anybol])
				goto found;
		}
		brk2:;
	}
}

/*
 * Range search for multibyte locales using the modified UNIX(R) Regular
 * Expression Library DFA.
 */
static int
rc_rangew(struct iblok *ip, char *last)
{
	char	*p;
	int	n, cstat, nstat;
	wint_t	wc;
	Dfa	*dp = e0->e_exp->re_dfa;

	p = ip->ib_cur;
	lineno++;
	cstat = dp->anybol;
	if (dp->acc[cstat])
		goto found;
	for (;;) {
		if (*p & 0200) {
			if ((n = mbtowi(&wc, p, last + 1 - p)) < 0) {
				n = 1;
				wc = WEOF;
			}
		} else {
			wc = *p;
			n = 1;
		}
		if ((wc & ~(wchar_t)(NCHAR-1)) != 0 ||
				(nstat = dp->trans[cstat][wc]) == 0) {
			/*
			 * '\0' is used to indicate end-of-line. If a '\0'
			 * character appears in input, it matches '$' but
			 * the DFA remains in dead state afterwards; there
			 * is thus no need to handle this condition
			 * specially to get the same behavior as in plain
			 * regexec().
			 */
			if (wc == '\n')
				wc = '\0';
			if ((nstat = regtrans(dp, cstat, wc, mb_cur_max)) == 0)
				goto fail;
			dp->trans[cstat]['\n'] = dp->trans[cstat]['\0'];
		}
		if (dp->acc[cstat = nstat - 1]) {
		found:	for (;;) {
				if (vflag == 0) {
		succeed:		outline(ip, last, p - ip->ib_cur);
					if (qflag || lflag)
						return 1;
				} else {
		fail:			ip->ib_cur = p;
					while (*ip->ib_cur++ != '\n');
				}
				if ((p = ip->ib_cur) > last)
					return 0;
				lineno++;
				if (dp->acc[cstat = dp->anybol] == 0)
					goto brk2;
			}
		}
		p += n;
		if (p[-n] == '\n') {
			if (vflag) {
				p--;
				goto succeed;
			}
			if ((ip->ib_cur = p) > last)
				return 0;
			lineno++;
			if (dp->acc[cstat = dp->anybol])
				goto found;
		}
		brk2:;
	}
}
#endif	/* UXRE */
