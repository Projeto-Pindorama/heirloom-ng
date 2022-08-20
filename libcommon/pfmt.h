/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)pfmt.h	1.2 (gritter) 9/21/03	*/

#include <stdio.h>

extern int	pfmt(FILE *stream, long flags, const char *format, ...);

#include <stdarg.h>

extern int	vpfmt(FILE *stream, long flags, const char *format, va_list ap);

#define	MM_HALT		0x00000001
#define	MM_ERROR	0x00000000
#define	MM_WARNING	0x00000002
#define	MM_INFO		0x00000004
#define	MM_ACTION	0x00000100
#define	MM_NOSTD	0x00000200
#define	MM_STD		0x00000000
#define	MM_NOGET	0x00000400
#define	MM_GET		0x00000000

extern int	setlabel(const char *label);
extern int	setuxlabel(const char *label);

#define	setcat(s)	(s)
#define	gettxt(n, s)	(s)
