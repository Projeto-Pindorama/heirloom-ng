/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, November 2002.
 *
 * Sccsid @(#)regcomp.c	1.6 (gritter) 9/22/03
 */
/*  UNIX(R) Regular Expresssion Library
 *
 *  Note: Code is released under the GNU LGPL
 *
 *  Copyright (C) 2001 Caldera International, Inc.
 *
 *  SPDX-Licence-Identifier: LGPL-2.1+
 */

/*	#include "synonyms.h"	*/
#include "re.h"

/*	#pragma weak regcomp = _regcomp	*/

int
regcomp(regex_t *ep, const char *pat, int flags)
{
	Tree *tp;
	Lex lex;

	if ((tp=libuxre_regparse(&lex, (const unsigned char *)pat, flags)) == 0)
		goto out;
	ep->re_nsub = lex.nleft;
	ep->re_flags = lex.flags & ~(REG_NOTBOL | REG_NOTEOL | REG_NONEMPTY);
	ep->re_col = lex.col;
	ep->re_mb_cur_max = lex.mb_cur_max;
	/*
	* Build the engine(s).  The factors determining which are built:
	*  1. If the pattern built insists on an NFA, then only build NFA.
	*  2. If flags include REG_NOSUB or REG_ONESUB and not (1),
	*     then only build DFA.
	*  3. Otherwise, build both.
	* Since libuxre_regdfacomp() modifies the tree and libuxre_regnfacomp()
	* doesn't, libuxre_regnfacomp() must be called first, if both are to
	* be called.
	*/
	if (ep->re_nsub != 0 && (flags & (REG_NOSUB | REG_ONESUB)) == 0
		|| lex.flags & REG_NFA)
	{
		ep->re_flags |= REG_NFA;
		if ((lex.err = libuxre_regnfacomp(ep, tp, &lex)) != 0)
			goto out;
	}
	if ((lex.flags & REG_NFA) == 0)
	{
		ep->re_flags |= REG_DFA;
		if ((lex.err = libuxre_regdfacomp(ep, tp, &lex)) != 0)
		{
			if (ep->re_flags & REG_NFA)
				libuxre_regdelnfa(ep->re_nfa);
		}
	}
out:;
	if (lex.err != 0 && lex.col != 0)
		(void)libuxre_lc_collate(lex.col);
	if (tp != 0)
		libuxre_regdeltree(tp, lex.err);
	return lex.err;
}
