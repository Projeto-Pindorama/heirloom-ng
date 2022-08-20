/*
 * Copyright (c) 2004 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
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
