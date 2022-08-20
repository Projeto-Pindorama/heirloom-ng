/*
 * Copyright (c) 2004 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)sigset.c	1.7 (gritter) 1/22/06	*/

#if defined (__FreeBSD__) || defined (__dietlibc__) || defined (__NetBSD__) || \
	defined (__OpenBSD__) || defined (__DragonFly__) || defined (__APPLE__)
#include <signal.h>
#include "sigset.h"

void (*sigset(int sig, void (*func)(int)))(int)
{
	struct sigaction nact, oact;
	sigset_t	nset, oset;

	if (sig <= 0)
		return SIG_ERR;
	sigemptyset(&nset);
	sigaddset(&nset, sig);
	if (sigprocmask(func==SIG_HOLD?SIG_BLOCK:SIG_UNBLOCK, &nset, &oset) < 0)
		return SIG_ERR;
	nact.sa_handler = func;
	nact.sa_flags = 0;
	if (sig == SIGCHLD && func == SIG_IGN)
		nact.sa_flags |= SA_NOCLDSTOP|SA_NOCLDWAIT;
	sigemptyset(&nact.sa_mask);
	sigaddset(&nact.sa_mask, sig);
	if (sigaction(sig, func==SIG_HOLD?(struct sigaction *)0:&nact, &oact)
			== -1)
		return SIG_ERR;
	if (sigismember(&oset, sig))
		return SIG_HOLD;
	else
		return (oact.sa_handler);
}
#endif	/* __FreeBSD__ || __dietlibc__ || __NetBSD__ || __OpenBSD__ ||
	__DragonFly__ || __APPLE__ */
