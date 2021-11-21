/*
 * Copyright (c) 2003 Gunnar Ritter
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
