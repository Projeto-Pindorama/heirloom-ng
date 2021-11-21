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


/*	from OpenSolaris "hash.h	1.9	05/06/08 SMI"	 SVr4.0 1.2		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)hash.h	2.2 (gritter) 6/21/05
 */

#include <inttypes.h>

#define	HASHWIDTH 27
#define	HASHSIZE 134217689L	/* prime under 2^HASHWIDTH */
#define	INDEXWIDTH 9
#define	INDEXSIZE (1<<INDEXWIDTH)
#define	NI (INDEXSIZE+1)
#define	ND ((25750/2) * sizeof (*table))
#define	BYTE 8

extern uint32_t *table;
#undef	index
#define	index	sp_index
extern int32_t index[];	/* into dif table based on hi hash bits */

void	hashinit(void);
int	hashlook(const char *);
uint32_t	hash(const char *);
