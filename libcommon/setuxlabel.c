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
