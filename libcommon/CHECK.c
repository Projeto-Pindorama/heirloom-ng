/*
 * Copyright (c) 2004 Gunnar Ritter
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
/*	Sccsid @(#)CHECK.c	1.7 (gritter) 1/22/06	*/

#include <stdlib.h>

#ifdef	__FreeBSD__
#define	NEED_ALLOCA_H	1
#define	NEED_MALLOC_H	1
#define	NEED_UTMPX_H	1
#endif

#ifdef	__APPLE__
#define	NEED_ALLOCA_H	1
#define	NEED_MALLOC_H	1
#define	NEED_UTMPX_H	1
#endif

#ifdef	__DragonFly__
#define	NEED_ALLOCA_H	1
#define	NEED_MALLOC_H	1
#define	NEED_UTMPX_H	1
#endif

#ifdef	__OpenBSD__
#define	NEED_ALLOCA_H	1
#define	NEED_MALLOC_H	1
#define	NEED_UTMPX_H	1
#endif

#ifdef	__NetBSD__
#define	NEED_ALLOCA_H	1
#define	NEED_MALLOC_H	1
#define	NEED_UTMPX_H	1
#endif

#ifdef	__dietlibc__
#define	NEED_MALLOC_H	1
#define	NEED_UTMPX_H	1
#endif

#ifdef	__UCLIBC__
#define	NEED_UTMPX_H	1
#endif

#ifndef	NEED_ALLOCA_H
#define	NEED_ALLOCA_H	0
#endif

#ifndef	NEED_MALLOC_H
#define	NEED_MALLOC_H	0
#endif

#ifndef	NEED_UTMPX_H
#define	NEED_UTMPX_H	0
#endif

int	alloca_h = NEED_ALLOCA_H;
int	malloc_h = NEED_MALLOC_H;
int	utmpx_h = NEED_UTMPX_H;
