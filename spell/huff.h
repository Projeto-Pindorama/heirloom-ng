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
 * Copyright 1994 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	from OpenSolaris "huff.h	1.8	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)huff.h	2.2 (gritter) 6/21/05
 */

extern struct huff {
	int32_t xn;
	int32_t xw;
	int32_t xc;
	int32_t xcq;	/* (c,0) */
	int32_t xcs;	/* c left justified */
	int32_t xqcs;	/* (q-1,c,q) left justified */
	int32_t xv0;
} huffcode;
#define	n huffcode.xn
#define	w huffcode.xw
#define	c huffcode.xc
#define	cq huffcode.xcq
#define	cs huffcode.xcs
#define	qcs huffcode.xqcs
#define	v0 huffcode.xv0

double	huff(float);
int	rhuff(FILE *);
void	whuff(void);

extern uint32_t	ple32(const char *);
extern void	le32p(uint32_t, char *);
