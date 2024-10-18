/*
 * Copyright (c) 2004 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)CHECK.c	1.8 (gritter) 12/16/07	*/

#include <stdlib.h>

#ifdef	__FreeBSD__
#define	NEED_ALLOCA_H	1
#define	NEED_MALLOC_H	1
#define	NEED_UTMPX_H	1
#endif

#ifdef	__APPLE__
#include <available.h>
#if __MAC_OS_X_VERSION_MIN_REQUIRED < __MAC_OS_X_VERSION_10_5
#define	NEED_ALLOCA_H	1
#endif
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
