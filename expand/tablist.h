/*
 * expand - convert tabs to spaces
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, May 2003.
 */
/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */

/*	Sccsid @(#)tablist.h	1.1 (gritter) 5/5/03	*/

extern struct	tab {
	struct tab	*t_nxt;
	long long	t_pos;
} *tablist;

extern int		tabstop;

extern void	settab(const char *);
extern void	addtab(long long);
extern void	badts(void);
extern void	*scalloc(size_t, size_t);
