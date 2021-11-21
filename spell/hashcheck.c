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


/*	from OpenSolaris "hashcheck.c	1.12	05/06/08 SMI"	 SVr4.0 1.2		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)hashcheck.c	2.4 (gritter) 6/22/05
 */

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)hashcheck.c	2.4 (gritter) 6/22/05";

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <locale.h>
#include "hash.h"
#include "huff.h"

int32_t	decode(int32_t, int32_t *);

int32_t hindex[NI];
uint32_t *table;
uint32_t wp;
int bp;
#define	U (BYTE*sizeof (uint32_t))
#define	L (BYTE*sizeof (int32_t))

static int32_t
fetch(void)
{
	int32_t w1;
	int32_t y = 0;
	int32_t empty = L;
	int32_t i = bp;
	int32_t tp = wp;
	while (empty >= i) {
		empty -= i;
		i = U;
		y |= (int32_t)table[tp++] << empty;
	}
	if (empty > 0)
		y |= table[tp]>>i-empty;
	i = decode((y >> 1) &
	    (((uint32_t)1 << (BYTE * sizeof (y) - 1)) - 1), &w1);
	bp -= i;
	while (bp <= 0) {
		bp += U;
		wp++;
	}
	return (w1);
}


int
main(void)
{
	int i;
	int32_t v;
	int32_t a;

	rhuff(stdin);
	fread((char *)hindex, sizeof (*hindex), NI, stdin);
	for (i = 0; i < NI; i++)
		hindex[i] = ple32((char *)&hindex[i]);
	table = malloc(hindex[NI-1]*sizeof (*table));
	fread((char *)table, sizeof (*table), hindex[NI-1], stdin);
	for (i = 0; i < hindex[NI-1]; i++)
		table[i] = ple32((char *)&table[i]);
	for (i = 0; i < NI-1; i++) {
		bp = U;
		v = (int32_t)i<<(HASHWIDTH-INDEXWIDTH);
		for (wp = hindex[i]; wp < hindex[i+1]; ) {
			if (wp == hindex[i] && bp == U)
				a = fetch();
			else {
				a = fetch();
				if (a == 0)
					break;
			}
			if (wp > hindex[i+1] ||
				wp == hindex[i+1] && bp < U)
				break;
			v += a;
			printf("%.9lo\n", (long)v);
		}
	}
	return 0;
}
