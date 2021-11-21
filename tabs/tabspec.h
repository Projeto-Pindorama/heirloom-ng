/*
 * tabs - set terminal tabs
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, January 2003.
 */
/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute
 * it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
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
