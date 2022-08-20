/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)ib_getw.c	1.5 (gritter) 7/16/04	*/

#include	<stdlib.h>
#include	<string.h>
#include	"iblok.h"
#include	"mbtowi.h"

char *
ib_getw(struct iblok *ip, wint_t *wc, int *len)
{
	size_t	rest;
	int	c, i, n;

	i = 0;
	rest = ip->ib_mend - ip->ib_mcur;
	if (rest && ip->ib_mcur > ip->ib_mbuf) {
		do
			ip->ib_mbuf[i] = ip->ib_mcur[i];
		while (i++, --rest);
	} else if (ip->ib_incompl) {
		ip->ib_incompl = 0;
		*wc = WEOF;
		ip->ib_mend = ip->ib_mcur = NULL;
		return NULL;
	}
	if (i == 0) {
		c = ib_get(ip);
		if (c == EOF) {
			*wc = WEOF;
			ip->ib_mend = ip->ib_mcur = NULL;
			return NULL;
		}
		ip->ib_mbuf[i++] = (char)c;
	}
	if (ip->ib_mbuf[0] & 0200) {
		while (ip->ib_mbuf[i-1] != '\n' && i < ip->ib_mb_cur_max &&
				ip->ib_incompl == 0) {
			c = ib_get(ip);
			if (c != EOF)
				ip->ib_mbuf[i++] = (char)c;
			else
				ip->ib_incompl = 1;
		}
		n = mbtowi(wc, ip->ib_mbuf, i);
		if (n < 0) {
			*len = 1;
			*wc = WEOF;
		} else if (n == 0) {
			*len = 1;
			*wc = '\0';
		} else
			*len = n;
	} else {
		*wc = ip->ib_mbuf[0];
		*len = n = 1;
	}
	ip->ib_mcur = &ip->ib_mbuf[*len];
	ip->ib_mend = &ip->ib_mcur[i - *len];
	return ip->ib_mbuf;
}
