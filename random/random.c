/*
 * random - generate a random number
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, November 2003.
 */
/*	from Unix 7th Edition /usr/src/libc/gen/rand.c	*/
/*
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
