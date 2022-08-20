/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)ib_getlin.c	1.2 (gritter) 4/17/03	*/

#include	<string.h>
#include	<stdlib.h>
#include	"iblok.h"

size_t
ib_getlin(struct iblok *ip, char **line, size_t *alcd,
		void *(*reallc)(void *, size_t))
{
	char *nl;
	size_t sz, llen = 0, nllen;

	for (;;) {
		if (ip->ib_cur >= ip->ib_end) {
			if (ip->ib_incompl) {
				ip->ib_incompl = 0;
				return 0;
			}
			if (ib_read(ip) == EOF) {
				if (llen) {
					ip->ib_incompl++;
					(*line)[llen] = '\0';
					return llen;
				} else
					return 0;
			}
			/*
			 * ib_read() advances ib_cur since *ib_cur++ gives
			 * better performance than *++ib_cur for ib_get().
			 * Go back again.
			 */
			ip->ib_cur--;
		}
		sz = ip->ib_end - ip->ib_cur;
		if ((nl = memchr(ip->ib_cur, '\n', sz)) != NULL) {
			sz = nl - ip->ib_cur + 1;
			if ((nllen = llen + sz + 1) > *alcd) {
				*line = reallc(*line, nllen);
				*alcd = nllen;
			}
			memcpy(&(*line)[llen], ip->ib_cur, sz);
			(*line)[llen + sz] = '\0';
			ip->ib_cur = nl + 1;
			return llen + sz;
		}
		if ((nllen = llen + sz + 1) > *alcd) {
			*line = reallc(*line, nllen);
			*alcd = nllen;
		}
		memcpy(&(*line)[llen], ip->ib_cur, sz);
		llen += sz;
		ip->ib_cur = ip->ib_end;
	}
	/*NOTREACHED*/
	return 0;
}
