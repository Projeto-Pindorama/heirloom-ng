/*
 * kill - terminate a process
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, June 2005.
 */
/*
 * Copyright (c) 2005 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
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
static const char id[] = "@(#)kill.sl	1.2 (gritter) 6/30/05";
/* SLIST */
/*
kill.c: * Sccsid @(#)kill.c	1.2 (gritter) 6/30/05
strsig.c: * Sccsid @(#)strsig.c	1.9 (gritter) 6/30/05
*/
