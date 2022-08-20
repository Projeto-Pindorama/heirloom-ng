/*
 * Copyright (c) 2004 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
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
