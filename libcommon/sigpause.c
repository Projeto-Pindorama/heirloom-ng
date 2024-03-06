/*
 * Copyright (c) 2004 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)sigpause.c	1.6 (gritter) 1/22/06	*/

#if defined (__FreeBSD__) || defined (__dietlibc__) || defined (__NetBSD__) || \
	defined (__OpenBSD__) || defined (__DragonFly__) || defined (__APPLE__)
#include <signal.h>
#include "sigset.h"

int
sigpause(int sig)
{
	sigset_t	nset, oset;
	int	ret;

	if (sig <= 0)
		return -1;
	sigemptyset(&nset);
	sigaddset(&nset, sig);
	if (sigprocmask(SIG_UNBLOCK, &nset, &oset) < 0)
		return -1;
	sigemptyset(&nset);
	ret = sigsuspend(&nset);
	if (sigprocmask(SIG_SETMASK, &oset, (sigset_t *)0) < 0)
		ret = -1;
	return ret;
}
#endif	/* __FreeBSD__ || __dietlibc__ || __NetBSD__ || __OpenBSD__ ||
	__DragonFly__ || __APPLE__ */
