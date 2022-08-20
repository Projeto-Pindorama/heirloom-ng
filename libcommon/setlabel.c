/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)setlabel.c	1.1 (gritter) 9/21/03	*/

extern char	*pfmt_label__;

int
setlabel(const char *s)
{
	static char	lbuf[26];
	char	*lp;

	if (s && s[0]) {
		for (lp = lbuf; *s && lp < &lbuf[sizeof lbuf-1]; s++, lp++)
			*lp = *s;
		*lp = '\0';
		pfmt_label__ = lbuf;
	} else
		pfmt_label__ = 0;
	return 0;
}
