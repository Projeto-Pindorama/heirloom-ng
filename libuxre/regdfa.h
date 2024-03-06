/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, November 2002.
 *
 * Sccsid @(#)regdfa.h	1.3 (gritter) 9/22/03
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

/*
* Deterministic Finite Automata.
*/

#ifndef	LIBUXRE_REGDFA_H
#define	LIBUXRE_REGDFA_H

#include <re.h>

typedef struct
{
	Bracket	*bkt;	/* extra info for ROP_BKT */
	size_t	nset;	/* number of items in the follow set */
	size_t	seti;	/* index into the follow set strip */
	w_type	op;	/* the leaf match operation */
} Posn;

#define CACHESZ	32	/* max. states to remember (must fit in uchar) */
#define NCHAR	(1 << CHAR_BIT)

struct re_dfa_ /*Dfa*/
{
	unsigned char	*posset;	/* signatures built here */
	size_t		*posfoll;	/* follow strip for posn[] */
	size_t		*sigfoll;	/* follow strip for sigi[] */
	size_t		*cursig;	/* current state's signature */
	Posn		*posn;		/* important positions */
	size_t		nposn;		/* length of posn,cursig,posset */
	size_t		used;		/* used portion of follow strip */
	size_t		avail;		/* unused part of follow strip */
	size_t		nset;		/* # items nonzero in posset[] */
	size_t		nsig[CACHESZ];	/* number of items in signature */
	size_t		sigi[CACHESZ];	/* index into sigfoll[] */
	unsigned char	acc[CACHESZ];	/* nonzero for accepting states */
	unsigned char	leftmost;	/* leftmost() start, not BOL */
	unsigned char	leftbol;	/* leftmost() start, w/BOL */
	unsigned char	anybol;		/* any match start, w/BOL */
	unsigned char	nfix;		/* number of invariant states */
	unsigned char	top;		/* next state index available */
	unsigned char	flags;		/* interesting flags */
	unsigned char	trans[CACHESZ][NCHAR];	/* goto table */
};

extern int	 regtrans(Dfa *, int, w_type, int);

#endif	/* !LIBUXRE_REGDFA_H */
