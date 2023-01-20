/*
 * Simple Regular Expression functions. Derived from Unix 7th Edition,
 * /usr/src/cmd/expr.y
 *
 * Modified by Gunnar Ritter, Freiburg i. Br., Germany, January 2003.
 *
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * SPDX-Licence-Identifier: Caldera
 */

/*	Sccsid @(#)regexpr.h	1.2 (gritter) 1/11/03	*/

#define	NBRA	9

extern char	*braslist[NBRA];
extern char	*braelist[NBRA];
extern int	nbra;
extern int	regerrno, reglength;
extern char	*loc1, *loc2, *locs;
extern int	sed;

extern char	*compile(const char *, char *, char *);
extern int	step(const char *, const char *);
extern int	advance(const char *, const char *);
