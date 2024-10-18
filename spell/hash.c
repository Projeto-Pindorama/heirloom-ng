/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*
 * Copyright 1995 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	from OpenSolaris "hash.c	1.10	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)hash.c	2.2 (gritter) 6/21/05
 */


#include <stdlib.h>
#include "hash.h"

#define	LOCHWIDTH 3
#define	HICHWIDTH 3
#define	CHARWIDTH (LOCHWIDTH+HICHWIDTH)
#define	LOCHMASK ((1<<LOCHWIDTH)-1)

/*
 * if HASHWIDTH + CHARWIDTH < bitsizeof(long)
 * one could make LOCHWIDTH=6 and HICHWIDTH=0
 * and simplify accordingly; the hanky-panky
 * is to avoid overflow in long multiplication
 */
#define	NC 30

static int32_t hashsize = HASHSIZE;
static int32_t pow2[NC*2];

static int8_t hashtab[] = {
-1,	-1,	-1,	-1,	-1,	-1,	0,	31,	/*  &' */
-1,	-1,	-1,	-1,	68,	-1,	65,	-1,
2,	25,	20,	35,	54,	61,	40,	39,	/* 0-7 */
42,	33,	64,	67,	-1,	-1,	-1,	66,
-1,	60,	43,	30,	5,	16,	47,	18,	/* A-G */
41,	36,	51,	6,	13,	56,	55,	58,
49,	12,	59,	46,	21,	32,	63,	34,
57,	52,	3,	-1,	-1,	-1,	-1,	-1,
-1,	22,	29,	8,	7,	10,	1,	28,	/* a-g */
11,	62,	37,	48,	15,	50,	9,	4,
19,	38,	45,	24,	23,	26,	17,	44,
27,	14,	53,	-1,	-1,	-1,	-1,	-1
};


uint32_t
hash(const char *s)
{
	int32_t c;
	int32_t *lp;
	uint32_t h = 0;
	for (lp = pow2; (c = *s++) != 0; ) {
		c = hashtab[c-' '];
		h += (c&LOCHMASK) * *lp++;
		h += (c>>LOCHWIDTH) * *lp++;
		h %= hashsize;
	}
	return (h);
}

void
hashinit(void)
{
#if ((1L << (HASHWIDTH+LOCHWIDTH) == 0) || (1L << (HASHWIDTH+HICHWIDTH) == 0))
	abort();	/* overflow is imminent */
#else
	int i;

	pow2[0] = 1L<<(HASHWIDTH-CHARWIDTH-2);
	for (i = 0; i < 2*NC-3; i += 2) {
		pow2[i+1] = (pow2[i]<<LOCHWIDTH) % hashsize;
		pow2[i+2] = (pow2[i+1]<<HICHWIDTH) % hashsize;
	}
#endif
}
