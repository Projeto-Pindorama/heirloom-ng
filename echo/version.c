/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*
 * Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 */
#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
#if defined (UCB)
static const char id[] = "@(#)/usr/ucb/echo.sl	1.7 (gritter) 7/2/05";
#elif defined (SUS)
static const char id[] = "@(#)echo_sus.sl	1.7 (gritter) 7/2/05";
#else	/* !SUS */
static const char id[] = "@(#)echo.sl	1.7 (gritter) 7/2/05";
#endif	/* !SUS */
/* SLIST */
/*
defs.h: * Sccsid @(#)defs.h	1.3 (gritter) 7/2/05
echo.c: * Sccsid @(#)echo.c	1.9 (gritter) 7/2/05
main.c: * Sccsid @(#)main.c	1.3 (gritter) 7/2/05
*/
