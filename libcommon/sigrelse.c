/*
 * Copyright (c) 2004 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)sigrelse.c	1.8 (gritter) 1/22/06	*/

#if defined (__FreeBSD__) || defined (__dietlibc__) || defined (__NetBSD__) || \
	defined (__OpenBSD__) || defined (__DragonFly__) || defined (__APPLE__)
#include <signal.h>
#include "sigset.h"

int
sigrelse(int sig)
{
	sigset_t	set, oset;

	if (sig <= 0)
		return -1;
	sigemptyset(&set);
	sigaddset(&set, sig);
	return sigprocmask(SIG_UNBLOCK, &set, &oset);
}
#endif	/* __FreeBSD__ || __dietlibc__ || __NetBSD__ || __OpenBSD__ ||
	__DragonFly__ || __APPLE__ */
