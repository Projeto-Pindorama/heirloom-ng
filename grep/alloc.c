/*
 * grep - search a file for a pattern
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, April 2001.
 */
/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */

/*	Sccsid @(#)alloc.c	1.3 (gritter) 4/17/03>	*/

/*
 * Memory allocation routines.
 */

#include	<stdlib.h>
#include	<unistd.h>
#include	"alloc.h"

/*
 * Memory allocation with check.
 */
void *
smalloc(size_t nbytes)
{
	void *p;

	if ((p = (void *)malloc(nbytes)) == NULL) {
		write(2, "Out of memory\n", 14);
		exit(077);
	}
	return p;
}

/*
 * Memory reallocation with check.
 */
void *
srealloc(void *ptr, size_t nbytes)
{
	void *cp;

	if ((cp = (void *)realloc(ptr, nbytes)) == NULL) {
		write(2, "Out of memory\n", 14);
		exit(077);
	}
	return cp;
}

/*
 * Zero-filled allocation with check.
 */
void *
scalloc(size_t nelem, size_t elsize)
{
	void *cp;

	if ((cp = calloc(nelem, elsize)) == NULL) {
		write(2, "Out of memory\n", 14);
		exit(077);
	}
	return cp;
}
