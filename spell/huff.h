/*
 * SPDX-Licence-Identifier: CDDL-1.0
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
