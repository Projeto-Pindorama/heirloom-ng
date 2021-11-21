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
