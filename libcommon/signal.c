/*
 * Copyright (c) 2004 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)signal.c	1.6 (gritter) 1/22/06	*/

#if defined (__FreeBSD__) || defined (__dietlibc__) || defined (__NetBSD__) || \
	defined (__OpenBSD__) || defined (__DragonFly__) || defined (__APPLE__)
#include <signal.h>
#include "sigset.h"

void (*signal(int sig, void (*func)(int)))(int)
{
	struct sigaction nact, oact;

	if (sig <= 0)
		return SIG_ERR;
	nact.sa_handler = func;
	nact.sa_flags = SA_RESETHAND|SA_NODEFER;
	if (sig == SIGCHLD && func == SIG_IGN)
		nact.sa_flags |= SA_NOCLDSTOP|SA_NOCLDWAIT;
	sigemptyset(&nact.sa_mask);
	if (sigaction(sig, &nact, &oact) == -1)
		return SIG_ERR;
	return oact.sa_handler;
}
#endif	/* __FreeBSD__ || __dietlibc__ || __NetBSD__ || __OpenBSD__ ||
	__DragonFly__ || __APPLE__ */
