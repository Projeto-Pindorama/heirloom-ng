/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, November 2002.
 *
 * Sccsid @(#)regfree.c	1.3 (gritter) 9/22/03
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

/*	#pragma weak regfree = _regfree	*/

void
regfree(regex_t *ep)
{
	if (ep->re_flags & REG_DFA)
		libuxre_regdeldfa(ep->re_dfa);
	if (ep->re_flags & REG_NFA)
		libuxre_regdelnfa(ep->re_nfa);
	if (ep->re_col != 0)
		(void)libuxre_lc_collate(ep->re_col);
}
