/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)memalign.c	1.7 (gritter) 1/22/06	*/

#if defined (__FreeBSD__) || defined (__dietlibc__) || defined (_AIX) || \
	defined (__NetBSD__) || defined (__OpenBSD__) || \
	defined (__DragonFly__) || defined (__APPLE__)
/*
 * FreeBSD malloc(3) promises to page-align the return of malloc() calls
 * if size is at least a page. This serves for a poor man's memalign() 
 * implementation that matches our needs.
 */
#include <unistd.h>
#include <stdlib.h>

#include "memalign.h"

void *
memalign(size_t alignment, size_t size)
{
	static long	pagesize;

	if (pagesize == 0)
		pagesize = sysconf(_SC_PAGESIZE);
	if (alignment != pagesize)
		return NULL;
	if (size < pagesize)
		size = pagesize;
	return malloc(size);
}
#endif	/* __FreeBSD__ || __dietlibc__ || _AIX || __NetBSD__ || __OpenBSD__ ||
	__DragonFly__ || __APPLE__ */
