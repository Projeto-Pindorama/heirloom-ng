/*
 * cpio - copy file archives in and out
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, April 2003.
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

/*	Sccsid @(#)expand.c	1.6 (gritter) 12/15/03	*/

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cpio.h"

#define	DLE	144

static void
zexread(char *data, size_t size, int doswap)
{
	if (bread(data, size) != size)
		unexeoa();
	if (doswap)
		swap(data, size, bflag || sflag, bflag || Sflag);
}

#define	nextbyte()	( \
	ipos >= sizeof ibuf && isize > 0 ? ( \
		zexread(ibuf, isize>sizeof ibuf?sizeof ibuf:isize, doswap), \
		ipos = 0 \
	) : 0, \
	isize--, \
	ibuf[ipos++] & 0377 \
)

#define	nextbit()	( \
	ibit = ibit >= 7 ? (ibyte = nextbyte(), 0) : ibit + 1, \
	isize < 0 ? (ieof = 1, -1) : (ibyte & (1<<ibit)) >> ibit \
)

#define	sixbits(n)	{ \
	int	t; \
	(n) = 0; \
	for (t = 0; t < 6; t++) \
		(n) |= nextbit() << t; \
}

#define	eightbits(n)	{ \
	int	t; \
	(n) = 0; \
	for (t = 0; t < 8; t++) \
		(n) |= nextbit() << t; \
}

static void
zexwrite(int *tfd, char *data, size_t size, uint32_t *crc, int *val,
		const char *tgt, long long *nsize)
{
	if (size) {
		if (size > *nsize)
			size = *nsize;
		if (*tfd >= 0 && write(*tfd, data, size) != size) {
			emsg(3, "Cannot write \"%s\"", tgt);
			*tfd = -1;
			*val = -1;
		}
		*crc = zipcrc(*crc, (unsigned char *)data, size);
		*nsize -= size;
	}
}

#define	wadd(c)		( \
	wpos >= sizeof wbuf ? ( \
		zexwrite(&tfd, wbuf, sizeof wbuf, crc, &val, tgt, &nsize), \
		wpos = 0 \
	) : 0, \
	wsize++, \
	wbuf[wpos++] = (c) \
)

#define	zex_L(x)	( \
	f->f_cmethod == C_REDUCED1 ? (x) & 0177 : \
	f->f_cmethod == C_REDUCED2 ? (x) & 077 : \
	f->f_cmethod == C_REDUCED3 ? (x) & 037 : \
	/* f->f_cmethod == C_REDUCED4 */ (x) & 017 \
)

#define	zex_F(x)	( \
	f->f_cmethod == C_REDUCED1 ? (x) == 0177 ? 2 : 3 : \
	f->f_cmethod == C_REDUCED2 ? (x) == 077 ? 2 : 3 : \
	f->f_cmethod == C_REDUCED3 ? (x) == 037 ? 2 : 3 : \
	/* f->f_cmethod == C_REDUCED4 */ (x) == 017 ? 2 : 3 \
)

#define	zex_D(x, y)	( \
	f->f_cmethod == C_REDUCED1 ? (((x)&0200)>>7) * 0400 + (y) + 1 : \
	f->f_cmethod == C_REDUCED2 ? (((x)&0300)>>6) * 0400 + (y) + 1 : \
	f->f_cmethod == C_REDUCED3 ? (((x)&0340)>>5) * 0400 + (y) + 1 : \
	/* f->f_cmethod == C_REDUCED4 */ (((x)&0360)>>4) * 0400 + (y) + 1 \
)

int
zipexpand(struct file *f, const char *tgt, int tfd, int doswap, uint32_t *crc)
{
	char	fset[256][33];
	char	ibuf[4096], ibyte = 0, wbuf[8192];
	long	ipos = sizeof ibuf, wpos = 0, isize = f->f_csize, wsize = 0;
	int	val = 0, ieof = 0;
	int	c = 0, i, j, k, n, ibit = 7, lastc, state, v = 0, len = 0;
	long long	nsize = f->f_st.st_size;

	*crc = 0;
	memset(fset, 0, sizeof fset);
	for (j = 255; j >= 0; j--) {
		sixbits(n);
		for (i = 0; i < n; i++) {
			eightbits(fset[j][i]);
		}
		fset[j][32] = n<1?0:n<3?1:n<5?2:n<9?3:n<17?4:n<37?5:n<65?6:7;
	}
	lastc = 0;
	state = 0;
	while (ieof == 0) {
		if (fset[lastc][32] == 0) {
			eightbits(c);
		} else {
			if (nextbit() != 0) {
				eightbits(c);
			} else {
				i = 0;
				for (k = 0; k < fset[lastc][32]; k++)
					i |= nextbit() << k;
				c = fset[lastc][i] & 0377;
			}
		}
		lastc = c;
		switch (state) {
		case 0:
			if (c != DLE)
				wadd(c);
			else
				state = 1;
			break;
		case 1:
			if (c != 0) {
				v = c;
				len = zex_L(v);
				state = zex_F(len);
			} else {
				wadd(DLE);
				state = 0;
			}
			break;
		case 2:
			len += c;
			state = 3;
			break;
		case 3:
			n = wsize - zex_D(v, c);
			for (i = 0; i < len + 3; i++) {
				c = n+i >= 0 ? wbuf[n+i&sizeof wbuf-1]&0377 : 0;
				wadd(c);
			}
			state = 0;
		}
	}
	zexwrite(&tfd, wbuf, wpos, crc, &val, tgt, &nsize);
	while (isize >= 0)
		nextbyte();
	return val;
}
