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
/*	Sccsid @(#)wctype.h	1.1 (gritter) 5/1/04	*/

#ifndef	LIBWCHAR_WCTYPE_H
#define	LIBWCHAR_WCTYPE_H	1

#include <stdlib.h>
#include <ctype.h>
#include "wchar.h"

typedef	int	wctrans_t;

#define	iswalnum(c)	isalnum(c)
#define	iswalpha(c)	isalpha(c)
#define	iswblank(c)	isblank(c)
#define	iswcntrl(c)	iscntrl(c)
#define	iswdigit(c)	isdigit(c)
#define	iswgraph(c)	isgraph(c)
#define	iswlower(c)	islower(c)
#define	iswprint(c)	isprint(c)
#define	iswpunct(c)	ispunct(c)
#define	iswspace(c)	isspace(c)
#define	iswupper(c)	isupper(c)
#define	iswxdigit(c)	isxdigit(c)

#define	towlower(c)	(tolower(c) & 0377)
#define	towupper(c)	(toupper(c) & 0377)

extern int		iswctype(wint_t, wctype_t);
extern wint_t		towctrans(wint_t, wctrans_t);
extern wctrans_t	wctrans(const char *);
extern wctype_t		wctype(const char *);

#endif	/* !LIBWCHAR_WCTYPE_H */
