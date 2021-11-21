/*
 * random - generate a random number
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, November 2003.
 */
/*	from Unix 7th Edition /usr/src/libc/gen/rand.c	*/
/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   Redistributions of source code and documentation must retain the
 *    above copyright notice, this list of conditions and the following
 *    disclaimer.
 *   Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *   All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed or owned by Caldera
 *      International, Inc.
 *   Neither the name of Caldera International, Inc. nor the names of
 *    other contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE
 * LIABLE FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)random.sl	1.3 (gritter) 5/29/05";

#include <stdio.h>
#include <inttypes.h>
#include <time.h>
#include <libgen.h>
#include <stdlib.h>

static const char	*progname;
static int	sflag;
static int	scale = 1;

static	int32_t	randx = 1;

static void
xsrand(uint32_t x)
{
	randx = x;
}

static int
xrand(void)
{
	return(((randx = randx*1103515245 + 12345)>>16) & 077777);
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-s] [scale(1 - 255)]\n", progname);
	exit(0);
}

int
main(int argc, char **argv)
{
	char	*x;
	time_t	t;
	uint32_t	r;

	progname = basename(argv[0]);
	if (argc > 1 && argv[1][0]=='-' && argv[1][1]=='-' && argv[1][2]=='\0')
		argc--, argv++;
	else if (argc > 1 && argv[1][0] == '-')
		switch (argv[1][1]) {
		case 's':
			sflag = 1;
			argc--, argv++;
			break;
		default:
			usage();
		}
	if (argc > 1) {
		scale = strtol(argv[1], &x, 10);
		if (x == argv[1] || *x || scale < 1 || scale > 255)
			usage();
		argc--, argv++;
	}
	if (argc > 1)
		usage();
	time(&t);
	xsrand(t);
	r = xrand() * (scale+1) >> 15;
	if (!sflag)
		printf("%d\n", r);
	return r;
}
