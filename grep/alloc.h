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

/*	Sccsid @(#)alloc.h	1.3 (gritter) 4/17/03>	*/

#include	<sys/types.h>

extern void		*smalloc(size_t);
extern void		*srealloc(void *, size_t);
extern void		*scalloc(size_t, size_t);
