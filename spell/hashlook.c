/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 1997 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	from OpenSolaris "hashlook.c	1.15	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)hashlook.c	2.4 (gritter) 6/25/05
 */

#include <stdlib.h>
#include <stdio.h>
#include "hash.h"
#include "huff.h"

uint32_t *table;
int32_t hindex[NI];

#define	B (BYTE * sizeof (uint32_t))
#define	L (BYTE * sizeof (int32_t)-1)
#define	MASK (~((uint32_t)1L<<L))

#define	fetch(wp, bp) ((wp[0] << (B - bp)) | (wp[1] >> bp))

int
hashlook(const char *s)
{
	uint32_t h;
	uint32_t t;
	register int32_t bp;
	register uint32_t *wp;
	int32_t sum;
	uint32_t *tp;

	h = hash(s);
	t = h>>(HASHWIDTH-INDEXWIDTH);
	wp = &table[hindex[t]];
	tp = &table[hindex[t+1]];
	bp = B;
	sum = (int32_t)t<<(HASHWIDTH-INDEXWIDTH);
	for (;;) {
		{
			/*
			 * this block is equivalent to:
			 * bp -= decode((fetch(wp, bp) >> 1) & MASK, &t);
			 */
			int32_t y;
			int32_t v;

			/*
			 * shift 32 on those machines leaves destination
			 * unchanged
			 */
			if (bp == 0)
				y = 0;
			else
				y = wp[0] << (B - bp);
			if (bp < 32)
				y |= (wp[1] >> bp);
			y = (y >> 1) & MASK;
			if (y < cs) {
				t = y >> (int32_t) (L+1-w);
				bp -= w-1;
			} else {
				for (bp -= w, v = v0; y >= qcs;
				    y = (y << 1) & MASK, v += n)
					bp -= 1;
				t = v + (y>> (int32_t)(L-w));
			}
		}
		while (bp <= 0) {
			bp += B;
			wp++;
		}
		if (wp >= tp && (wp > tp||bp < B))
			return (0);
		sum += t;
		if (sum < h)
			continue;
		return (sum == h);
	}
}


int
prime(char *file)
{
	register FILE *f;
	int	i;

	if ((f = fopen(file, "r")) == NULL)
		return (0);
	if (rhuff(f) == 0 ||
	    fread((char *)hindex, sizeof (*hindex),  NI, f) != NI)
		return (0);
	for (i = 0; i < NI; i++)
		hindex[i] = ple32((char *)&hindex[i]);
	if ((table = malloc((hindex[NI-1]+1) * sizeof (*table))) == 0 ||
	    fread((char *)table, sizeof (*table), hindex[NI-1], f) !=
	    hindex[NI-1])
		return (0);
	for (i = 0; i < hindex[NI-1]; i++)
		table[i] = ple32((char *)&table[i]);
	table[i] = 0;
	fclose(f);
	hashinit();
	return (1);
}
