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
static const char id[] = "@(#)fmtmsg.sl	1.3 (gritter) 7/1/05";
/* SLIST */
/*
fmtmsg.c: * Sccsid @(#)fmtmsg.c	1.3 (gritter) 6/26/05
fmtmsg.h: * Sccsid @(#)fmtmsg.h	1.3 (gritter) 6/26/05
main.c: * Sccsid @(#)main.c	1.4 (gritter) 7/1/05
*/
