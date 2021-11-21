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
/*	Sccsid @(#)_utmpx.h	1.9 (gritter) 1/22/06	*/

#if defined (__FreeBSD__) || defined (__dietlibc__) || defined (__NetBSD__) || \
	defined (__UCLIBC__) || defined (__OpenBSD__) || \
	defined (__DragonFly__) || defined (__APPLE__)
#include <sys/types.h>
#include <sys/time.h>
#include <utmp.h>

#ifndef	__dietlibc__
struct utmpx {
	char	ut_user[UT_NAMESIZE];
	char	ut_id[UT_LINESIZE];
	char	ut_line[UT_LINESIZE];
	char	ut_host[UT_HOSTSIZE];
	pid_t	ut_pid;
	short	ut_type;
	struct timeval	ut_tv;
	struct {
		int	e_termination;
		int	e_exit;
	} ut_exit;
};

#ifndef EMPTY
#define	EMPTY		0
#endif
#ifndef BOOT_TIME
#define	BOOT_TIME	1
#endif
#ifndef OLD_TIME
#define	OLD_TIME	2
#endif
#ifndef NEW_TIME
#define	NEW_TIME	3
#endif
#ifndef USER_PROCESS
#define	USER_PROCESS	4
#endif
#ifndef INIT_PROCESS
#define	INIT_PROCESS	5
#endif
#ifndef LOGIN_PROCESS
#define	LOGIN_PROCESS	6
#endif
#ifndef DEAD_PROCESS
#define	DEAD_PROCESS	7
#endif
#ifndef RUN_LVL
#define	RUN_LVL		8
#endif
#ifndef ACCOUNTING
#define	ACCOUNTING	9
#endif
#else	/* __dietlibc__ */
#define	utmpx	utmp
#endif	/* __dietlibc__ */

extern void		endutxent(void);
extern struct utmpx	*getutxent(void);
extern struct utmpx	*getutxid(const struct utmpx *);
extern struct utmpx	*getutxline(const struct utmpx *);
extern struct utmpx	*pututxline(const struct utmpx *);
extern void		setutxent(void);
extern int		utmpxname(const char *);
extern void		updwtmpx(const char *, const struct utmpx *);
#endif	/* __FreeBSD__ || __dietlibc__ || __NetBSD__ || __UCLIBC__ ||
	 	__OpenBSD__ || __DragonFly__ || __APPLE__ */
