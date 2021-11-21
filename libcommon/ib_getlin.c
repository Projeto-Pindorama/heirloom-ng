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
