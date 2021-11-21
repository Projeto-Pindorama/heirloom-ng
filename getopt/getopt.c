/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright (c) 2000 Sun Microsystems, Inc.
 * All rights reserved.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "getopt.c	1.10	05/06/08 SMI"	 SVr4.0 1.7		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 */
#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)getopt.c	1.4 (gritter) 7/1/05";

#include <stdio.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

#define	BLOCKLEN	5120

/* incr doesn't include a null termination */
#define	ALLOC_BUFMEM(buf, size, incr) \
	{ \
		size_t	len = strlen(buf); \
		if ((len + incr) >= size) { \
			size = len + incr + 1; \
			if ((buf = realloc(buf, size)) == NULL) { \
				fprintf(stderr, "%s: Out of memory\n", \
						progname); \
				exit(2); \
			} \
		} \
	}

static const char	*progname;

int
main(int argc, char **argv)
{
	int	c;
	int	errflg = 0;
	char	tmpstr[4];
	char	*outstr;
	char	*goarg;
	size_t	bufsize;

	progname = basename(argv[0]);
	if (argc < 2) {
		fprintf(stderr, "usage: %s legal-args $*\n", progname);
		exit(2);
	}

	goarg = argv[1];
	argv[1] = argv[0];
	argv++;
	argc--;

	bufsize = BLOCKLEN;
	if ((outstr = malloc(bufsize)) == NULL) {
		fprintf(stderr, "%s: Out of memory\n", progname);
		exit(2);
	}
	outstr[0] = '\0';

	while ((c = getopt(argc, argv, goarg)) != EOF) {
		if (c == '?') {
			errflg++;
			continue;
		}

		tmpstr[0] = '-';
		tmpstr[1] = (char)c;
		tmpstr[2] = ' ';
		tmpstr[3] = '\0';

		/* If the buffer is full, expand it as appropriate */
		ALLOC_BUFMEM(outstr, bufsize, 3);

		strcat(outstr, tmpstr);

		if (*(strchr(goarg, c)+1) == ':') {
			ALLOC_BUFMEM(outstr, bufsize, strlen(optarg)+1)
			strcat(outstr, optarg);
			strcat(outstr, " ");
		}
	}

	if (errflg) {
		exit(2);
	}

	ALLOC_BUFMEM(outstr, bufsize, 3)
	strcat(outstr, "-- ");
	while (optind < argc) {
		ALLOC_BUFMEM(outstr, bufsize, strlen(argv[optind])+1)
		strcat(outstr, argv[optind++]);
		strcat(outstr, " ");
	}

	printf("%s\n", outstr);
	return 0;
}
