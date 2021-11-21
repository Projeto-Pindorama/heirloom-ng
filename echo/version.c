/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
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
