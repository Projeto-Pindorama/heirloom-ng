/*
 * Copyright (c) 2004 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)sigset.h	1.9 (gritter) 1/22/06	*/

#if defined (__FreeBSD__) || defined (__dietlibc__) || defined (__NetBSD__) || \
	defined (__OpenBSD__) || defined (__DragonFly__) || defined (__APPLE__)

#ifndef	SIG_HOLD
#define	SIG_HOLD	((void (*)(int))2)
#endif	/* !SIG_HOLD */

extern int	sighold(int);
extern int	sigignore(int);
extern int	sigpause(int);
extern int	sigrelse(int);
extern void	(*sigset(int, void (*)(int)))(int);
extern void	(*signal(int, void (*)(int)))(int);
#endif	/* __FreeBSD__ || __dietlibc__ || __NetBSD__ || __OpenBSD__ ||
	__DragonFly__ || __APPLE__ */
