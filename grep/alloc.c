/*
 * grep - search a file for a pattern
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, April 2001.
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
