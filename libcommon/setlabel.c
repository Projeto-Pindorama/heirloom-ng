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
