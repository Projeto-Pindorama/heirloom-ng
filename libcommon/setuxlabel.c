/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)setuxlabel.c	1.1 (gritter) 9/21/03	*/

#include	"msgselect.h"

extern char	*pfmt_label__;

int
setuxlabel(const char *s)
{
	static char	lbuf[msgselect(29,26)];
	char	*lp, *mp;

	if (s && s[0]) {
		lp = lbuf;
		mp = msgselect("UX:","");
		while (*mp)
			*lp++ = *mp++;
		lbuf[0] = 'U', lbuf[1] = 'X', lbuf[2] = ':';
		while (*s && lp < &lbuf[sizeof lbuf-1])
			*lp++ = *s++;
		*lp = '\0';
		pfmt_label__ = lbuf;
	} else
		pfmt_label__ = 0;
	return 0;
}
