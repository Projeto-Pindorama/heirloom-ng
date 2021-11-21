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
