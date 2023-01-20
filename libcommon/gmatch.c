/*
 * Derived from /usr/src/cmd/sh/expand.c, Unix 7th Edition:
 *
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * SPDX-Licence-Identifier: Caldera
 */

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)gmatch.sl	1.5 (gritter) 5/29/05";

#include	<stdlib.h>
#include	<wchar.h>
#include	<limits.h>

#include	"mbtowi.h"

#define	fetch(wc, s, n)	((mb_cur_max > 1 && *(s) & 0200 ? \
		((n) = mbtowi(&(wc), (s), mb_cur_max), \
		 (n) = ((n) > 0 ? (n) : (n) < 0 ? (wc = WEOF, 1) : 1)) :\
	((wc) = *(s) & 0377, (n) = 1)), (s) += (n), (wc))

int
gmatch(const char *s, const char *p)
{
	const char	*bs = s;
	int	mb_cur_max = MB_CUR_MAX;
	wint_t	c, scc;
	int	n;

	if (fetch(scc, s, n) == WEOF)
		return (0);
	switch (fetch(c, p, n)) {

	case '[':	{
		int	ok = 0, excl;
		unsigned long	lc = ULONG_MAX;
		const char	*bp;

		if (*p == '!') {
			p++;
			excl = 1;
		} else
			excl = 0;
		fetch(c, p, n);
		bp = p;
		while (c != '\0') {
			if (c == ']' && p > bp)
				return (ok ^ excl ? gmatch(s, p) : 0);
			else if (c == '-' && p > bp && *p != ']') {
				if (*p == '\\')
					p++;
				if (fetch(c, p, n) == '\0')
					break;
				if (lc <= scc && scc <= c)
					ok = 1;
			} else {
				if (c == '\\') {
					if (fetch(c, p, n) == '\0')
						break;
				}
				if (scc == (lc = c))
					ok = 1;
			}
			fetch(c, p, n);
		}
		return (0);
	}

	case '\\':
		fetch(c, p, n);
		if (c == '\0')
			return (0);
		/*FALLTHRU*/

	default:
		if (c != scc)
			return (0);
		/*FALLTHRU*/

	case '?':
		return (scc ? gmatch(s, p) : 0);

	case '*':
		if (*p == '\0')
			return (1);
		s = bs;
		while (*s) {
			if (gmatch(s, p))
				return (1);
			fetch(scc, s, n);
		}
		return (0);

	case '\0':
		return (scc == '\0');

	case WEOF:
		return (0);

	}
}
