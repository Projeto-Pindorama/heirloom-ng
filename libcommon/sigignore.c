/*
 * Copyright (c) 2004 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)sigignore.c	1.6 (gritter) 1/22/06	*/

#if defined (__FreeBSD__) || defined (__dietlibc__) || defined (__NetBSD__) || \
	defined (__OpenBSD__) || defined (__DragonFly__) || defined (__APPLE__)
#include <signal.h>
#include "sigset.h"

int
sigignore(int sig)
{
	struct sigaction	act;

	if (sig <= 0)
		return -1;
	act.sa_handler = SIG_IGN;
	act.sa_flags = 0;
	if (sig == SIGCHLD)
		act.sa_flags |= SA_NOCLDSTOP|SA_NOCLDWAIT;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, sig);
	return sigaction(sig, &act, (struct sigaction *)0);
}
#endif	/* __FreeBSD__ || __dietlibc__ || __NetBSD__ || __OpenBSD__ ||
	__DragonFly__ || __APPLE__ */
