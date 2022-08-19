/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "hashmake.c	1.11	05/06/08 SMI"	 SVr4.0 1.2		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)hashmake.c	2.4 (gritter) 6/25/05
 */

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)hashmake.c	2.4 (gritter) 6/25/05";

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <locale.h>
#include <limits.h>
#include <string.h>

#include "hash.h"

static void *
srealloc(void *p, size_t size)
{
	if ((p = realloc(p, size)) == NULL) {
		write(2, "out of memory\n", 14);
		_exit(0177);
	}
	return p;
}

#if defined (__GLIBC__) && defined (_IO_getc_unlocked)
#undef	getchar
#define	getchar()	_IO_getc_unlocked(stdin)
#endif

int
main(void)
{
	char *word = NULL;
	size_t size = 0;
	int c, n;

	hashinit();
	do {
		n = 0;
		while ((c = getchar()) != EOF && c != '\n') {
			if (n >= size)
				word = srealloc(word, size += 128);
			word[n++] = c;
		}
		if (n == 0) {
			if (c == EOF)
				break;
			else
				continue;
		}
		if (n >= size)
			word = srealloc(word, size += 128);
		word[n] = '\0';
		printf("%.*lo\n", (HASHWIDTH+2)/3, (long)hash(word));
	} while (c != EOF);
	return 0;
}
