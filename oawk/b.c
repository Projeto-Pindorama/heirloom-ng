/*-
 * This code contains changes by
 *      Gunnar Ritter, Freiburg i. Br., Germany, 2002. All rights reserved.
 *
 * Conditions 1, 2, and 4 and the no-warranty notice below apply
 * to these changes.
 *
 *
 * Copyright (c) 1991
 *      The Regents of the University of California.  All rights reserved.
 *
 * SPDX-Licence-Identifier: BSD-4-Clause-UC
 *
 *
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * SPDX-Licence-Identifier: Caldera
*/

/*	from 4.4BSD /usr/src/old/awk/b.c	4.4 (Berkeley) 4/17/91	*/
/*	Sccsid @(#)b.c	1.10 (gritter) 3/31/03>	*/

#include "awk.def"
#include "stdio.h"
#include "awk.h"
#include <regex.h>

struct fa {
	char *pat;
	regex_t	re;
};

void
overflo(void)
{
	error(FATAL, "regular expression too long");
}

struct fa *makedfa(const char *p)	/* returns dfa for re pointed to by p */
{
	struct fa *fap;
	int i;

	dprintf("makedfa: re = /%s/\n", p);
	if ((fap = malloc(sizeof *fap)) == NULL)
		overflo();
	if ((i = regcomp(&fap->re, p, REG_OLDERE | REG_NOSUB |
			REG_BKTEMPTY | REG_BKTESCAPE | REG_ESCSEQ |
			REG_OLDESC | REG_NOI18N)) != 0) {
		switch (i) {
		case REG_ESPACE:
			overflo();
			/*NOTREACHED*/
		default:
			yyerror("syntax error");
			yyerror("bailing out");
			exit(2);
		}
	}
	fap->pat = tostring(p);
	return fap;
}

int
match(void *v, const char *p)
{
	struct fa *pfa = v;
	int i;

	i = regexec(&pfa->re, p, 0, NULL, 0);
	dprintf("match: re = /%s/ in = |%s| val=|%d|\n", pfa->pat, p, i);
	return i == 0;
}

int
member(char c, const char *s)	/* is c in s? */
{
	while (*s)
		if (c == *s++)
			return(1);
	return(0);
}
