/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, November 2002.
 *
 * Sccsid @(#)_collmult.c	1.4 (gritter) 9/22/03
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
#include "colldata.h"
#include <stddef.h>

#define CCM(p)	((const CollMult *)(p))

LIBUXRE_STATIC const CollElem *
libuxre_collmult(struct lc_collate *col, const CollElem *cep, wchar_t wc)
{
	const char *tbl;
	size_t sz;
	w_type ch;

	if (col == 0 || cep->multbeg == 0
		|| (tbl = (const char *)col->multtbl) == 0)
	{
		return ELEM_BADCHAR;
	}
	sz = col->elemsize + (sizeof(CollMult) - sizeof(CollElem));
	tbl += sz * cep->multbeg;
	while ((ch = CCM(tbl)->ch) != wc)
	{
		if (ch == 0)
			return ELEM_BADCHAR;	/* end of list */
		tbl += sz;
	}
	return &CCM(tbl)->elem;
}
