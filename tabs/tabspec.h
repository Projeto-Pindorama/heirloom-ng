/*
 * tabs - set terminal tabs
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, January 2003.
 */
/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */

/*	Sccsid @(#)tabspec.h	1.1 (gritter) 5/10/03	*/

struct	tabulator {
	struct tabulator	*t_nxt;	/* next list element */
	const char	*t_str;		/* tabulator string */
	int	t_tab;			/* tab stop position */
	int	t_rep;			/* repetitive tab count */
};

enum {
	TABERR_NONE,
	TABERR_CANTOP,	/* can't open */
	TABERR_FILIND,	/* file indirection */
	TABERR_UNKTAB,	/* unknown tab code */
	TABERR_ILLINC,	/* illegal increment */
	TABERR_ILLTAB	/* illegal tabs */
} taberrno;

extern void		*scalloc(size_t nmemb, size_t size);
extern struct tabulator	*tabstops(const char *s, int cols);
