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
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "spellin.c	1.12	05/06/08 SMI"	 SVr4.0 1.4		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)spellin.c	2.4 (gritter) 4/16/06
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include "hash.h"
#include "huff.h"

int32_t	encode(int32_t, int32_t *);

#define	S (BYTE * sizeof (int32_t))
#define	B (BYTE * sizeof (uint32_t))
uint32_t *table;
int32_t hindex[NI];
uint32_t wp;		/* word pointer */
int32_t bp = B;		/* bit pointer */
static int ignore;
static int extra;

static int32_t
append(register uint32_t w1, register int32_t i)
{
	while (wp < ND - 1) {
		table[wp] |= w1>>(B-bp);
		i -= bp;
		if (i < 0) {
			bp = -i;
			return (1);
		}
		w1 <<= bp;
		bp = B;
		wp++;
	}
	return (0);
}


/*
 *	usage: hashin N
 *	where N is number of words in dictionary
 *	and standard input contains sorted, unique
 *	hashed words in octal
 */

int
main(int argc, char **argv)
{
	int32_t h, k, d;
	unsigned hu;
	int32_t  i;
	int32_t count;
	int32_t w1;
	int32_t x;
	int32_t t, u;
	double z;

	k = 0;
	u = 0;
	if (argc != 2) {
		fprintf(stderr, "%s: arg count\n", argv[0]);
		exit(1);
	}
	table = malloc(ND * sizeof (*table));
	if (table == 0) {
		fprintf(stderr, "%s: no space for table\n", argv[0]);
		exit(1);
	}
	if ((atof(argv[1])) == 0.0) {
		fprintf(stderr, "%s: illegal count", argv[0]);
		exit(1);
	}

	z = huff((1L<<HASHWIDTH)/atof(argv[1]));
	fprintf(stderr, "%s: expected code widths = %f\n",
	    argv[0], z);
	for (count = 0; scanf("%o", &hu) == 1; ++count) {
		h = hu;
		if ((t = h >> (HASHWIDTH - INDEXWIDTH)) != u) {
			if (bp != B)
				wp++;
			bp = B;
			while (u < t)
				hindex[++u] = wp;
			k =  (int32_t)t<<(HASHWIDTH-INDEXWIDTH);
		}
		d = h-k;
		k = h;
		for (;;) {
			for (x = d; ; x /= 2) {
				i = encode(x, &w1);
				if (i > 0)
					break;
			}
			if (i > B) {
				if (!(append((uint32_t)(w1>>(int32_t) (i-B)), B) &&
				    append((uint32_t)(w1<<(int32_t) (B+B-i)),
				    i-B)))
					ignore++;
			} else
				if (!append((uint32_t)(w1<<(int32_t)(B-i)), i))
					ignore++;
			d -= x;
			if (d > 0)
				extra++;
			else
				break;
		}
	}
	if (bp != B)
		wp++;
	while (++u < NI)
		hindex[u] = wp;
	whuff();
	for (i = 0; i < NI; i++)
		le32p(hindex[i], (char *)&hindex[i]);
	fwrite((char *)hindex, sizeof (*hindex), NI, stdout);
	for (i = 0; i < wp; i++)
		le32p(table[i], (char *)&table[i]);
	fwrite((char *)table, sizeof (*table), wp, stdout);
	fprintf(stderr,
	    "%s: %ld items, %d ignored, %d extra, %u words occupied\n",
	    argv[0], (long)count, ignore, extra, (unsigned)wp);
	count -= ignore;
	fprintf(stderr, "%s: %f table bits/item, %f table+index bits\n",
	    argv[0], (((float)BYTE * wp) * sizeof (*table) / count),
	    (BYTE * ((float)wp * sizeof (*table) + sizeof (hindex)) / count));
	return 0;
}
