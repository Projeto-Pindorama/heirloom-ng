
/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
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
/* Copyright (c) 1988 AT&T */
/* All Rights Reserved */
/*
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * from what.c 1.11 06/12/12
 */

/*	from OpenSolaris "what.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)what.c	1.12 (gritter) 4/14/07
 */
/*	from OpenSolaris "sccs:cmd/what.c"	*/

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)what.sl	1.12 (gritter) 4/14/07";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MINUS '-'
#define MINUS_S "-s"
#undef	TRUE
#define TRUE  1
#undef	FALSE
#define FALSE 0


static int found = FALSE;
static int silent = FALSE;

static char	pattern[]  =  "@(#)";

static void	dowhat(FILE *);
static int	trypat(FILE *,char *);


int 
main(int argc, register char **argv)
{
	register int i;
	register FILE *iop;
	int current_optind, c;

	if (argc < 2)
		dowhat(stdin);
	else {

		current_optind = 1;
		optind = 1;
		opterr = 0;
		i = 1;
		/*CONSTCOND*/
		while (1) {
			if(current_optind < optind) {
			   current_optind = optind;
			   argv[i] = 0;
			   if (optind > i+1 ) {
			      argv[i+1] = NULL;
			   }
			}
			i = current_optind;
		        c = getopt(argc, argv, "-s");

				/* this takes care of options given after
				** file names.
				*/
			if((c == EOF)) {
			   if (optind < argc) {
				/* if it's due to -- then break; */
			       if(argv[i][0] == '-' &&
				      argv[i][1] == '-') {
			          argv[i] = 0;
			          break;
			       }
			       optind++;
			       current_optind = optind;
			       continue;
			   } else {
			       break;
			   }
			}
			switch (c) {
			case 's' :
				silent = TRUE;
				break;
			default  :
				fprintf(stderr,
					"Usage: what [ -s ] filename...\n");
				exit(2);
			}
		}
		for(i=1;(i<argc );i++) {
			if(!argv[i]) {
			   continue;
			}
			if ((iop = fopen(argv[i],"r")) == NULL)
				fprintf(stderr,"can't open %s (26)\n",
					argv[i]);
			else {
				printf("%s:\n",argv[i]);
				dowhat(iop);
			}
		}
	}
	return (!found);				/* shell return code */
}


static void
dowhat(register FILE *iop)
{
	register int c;

	while ((c = getc(iop)) != EOF) {
		if (c == pattern[0])
			if(trypat(iop, &pattern[1]) && silent) break;
	}
	fclose(iop);
}


static int
trypat(register FILE *iop,register char *pat)
{
	register int c = 0;

	for (; *pat; pat++)
		if ((c = getc(iop)) != *pat)
			break;
	if (!*pat) {
		found = TRUE;
		putchar('\t');
		while ((c = getc(iop)) != EOF && c &&
				!strchr("\"\\>\n", c&0377))
			putchar(c);
		putchar('\n');
		if(silent)
			return(TRUE);
	}
	else if (c != EOF)
		ungetc(c, iop);
	return(FALSE);
}
