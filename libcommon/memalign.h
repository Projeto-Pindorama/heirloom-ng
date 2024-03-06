/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)memalign.h	1.7 (gritter) 1/22/06	*/

#ifndef	LIBCOMMON_MEMALIGN_H
#define	LIBCOMMON_MEMALIGN_H

#if defined (__FreeBSD__) || defined (__dietlibc__) || defined (_AIX) || \
	defined (__NetBSD__) || defined (__OpenBSD__) || \
	defined (__DragonFly__) || defined (__APPLE__)
#include	<stdlib.h>

extern void	*memalign(size_t, size_t);
#endif	/* __FreeBSD__ || __dietlibc__ || _AIX || __NetBSD__ || __OpenBSD__ ||
	__DragonFly__ || __APPLE__ */
#endif	/* !LIBCOMMON_MEMALIGN_H */
