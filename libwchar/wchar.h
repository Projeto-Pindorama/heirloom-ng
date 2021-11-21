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
/*	Sccsid @(#)wchar.h	1.4 (gritter) 8/3/05	*/

#ifndef	LIBWCHAR_WCHAR_H
#define	LIBWCHAR_WCHAR_H	1

#include <inttypes.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

typedef	int		wint_t;
typedef	intptr_t	wctype_t;
typedef	int		mbstate_t;

extern int	mbtowc(wchar_t *, const char *, size_t);
extern int	wctomb(char *, wchar_t);
extern size_t	mbstowcs(wchar_t *, const char *, size_t);
extern int	mblen(const char *, size_t);

/*wint_t		btowc(int);*/
#define	btowc(c)	(c)
wint_t		fgetwc(FILE *);
wchar_t		*fgetws(wchar_t *, int, FILE *);
wint_t		fputwc(wchar_t, FILE *);
int		fputws(const wchar_t *, FILE *);
int		fwide(FILE *, int);
int		fwprintf(FILE *, const wchar_t *, ...);
int		fwscanf(FILE *, const wchar_t *, ...);
wint_t		getwc(FILE *);
wint_t		getwchar(void);
int		iswalnum(wint_t);
int		iswalpha(wint_t);
int		iswcntrl(wint_t);
int		iswctype(wint_t, wctype_t);
int		iswdigit(wint_t);
int		iswgraph(wint_t);
int		iswlower(wint_t);
int		iswprint(wint_t);
int		iswpunct(wint_t);
int		iswspace(wint_t);
int		iswupper(wint_t);
int		iswxdigit(wint_t);
size_t		mbrlen(const char *, size_t, mbstate_t *);
size_t		mbrtowc(wchar_t *, const char *, size_t, mbstate_t *);
int		mbsinit(const mbstate_t *);
size_t		mbsrtowcs(wchar_t *, const char **, size_t, mbstate_t *);
wint_t		putwc(wchar_t, FILE *);
wint_t		putwchar(wchar_t);
int		swprintf(wchar_t *, size_t, const wchar_t *, ...);
int		swscanf(const wchar_t *, const wchar_t *, ...);
wint_t		towlower(wint_t);
wint_t		towupper(wint_t);
wint_t		ungetwc(wint_t, FILE *);
int		vfwprintf(FILE *, const wchar_t *, va_list);
int		vfwscanf(FILE *, const wchar_t *, va_list);
int		vwprintf(const wchar_t *, va_list);
int		vswprintf(wchar_t *, size_t, const wchar_t *, va_list);
int		vswscanf(const wchar_t *, const wchar_t *, va_list);
int		vwscanf(const wchar_t *, va_list);
size_t		wcrtomb(char *, wchar_t, mbstate_t *);
wchar_t		*wcscat(wchar_t *, const wchar_t *);
wchar_t		*wcschr(const wchar_t *, wchar_t);
int		wcscmp(const wchar_t *, const wchar_t *);
int		wcscoll(const wchar_t *, const wchar_t *);
wchar_t		*wcscpy(wchar_t *, const wchar_t *);
size_t		wcscspn(const wchar_t *, const wchar_t *);
/*size_t		wcsftime(wchar_t *, size_t, const wchar_t *, const struct tm *);*/
size_t		wcslen(const wchar_t *);
wchar_t		*wcsncat(wchar_t *, const wchar_t *, size_t);
int		wcsncmp(const wchar_t *, const wchar_t *, size_t);
wchar_t		*wcsncpy(wchar_t *, const wchar_t *, size_t);
wchar_t		*wcspbrk(const wchar_t *, const wchar_t *);
wchar_t		*wcsrchr(const wchar_t *, wchar_t);
size_t		wcsrtombs(char *, const wchar_t **, size_t, mbstate_t *);
size_t		wcsspn(const wchar_t *, const wchar_t *);
wchar_t		*wcsstr(const wchar_t *, const wchar_t *);
double		wcstod(const wchar_t *, wchar_t **);
float		wcstof(const wchar_t *, wchar_t **);
wchar_t		*wcstok(wchar_t *, const wchar_t *, wchar_t **);
long		wcstol(const wchar_t *, wchar_t **, int);
long double	wcstold(const wchar_t *, wchar_t **);
long long	wcstoll(const wchar_t *, wchar_t **, int);
unsigned long	wcstoul(const wchar_t *, wchar_t **, int);
unsigned long long	wcstoull(const wchar_t *, wchar_t **, int);
wchar_t		*wcswcs(const wchar_t *, const wchar_t *);
int		wcswidth(const wchar_t *, size_t);
size_t		wcsxfrm(wchar_t *, const wchar_t *, size_t);
/*int		wctob(wint_t);*/
#define	wctob(c)	(c)
wctype_t	wctype(const char *);
int		wcwidth(wchar_t);
wchar_t		*wmemchr(const wchar_t *, wchar_t, size_t);
int		wmemcmp(const wchar_t *, const wchar_t *, size_t);
wchar_t		*wmemcpy(wchar_t *, const wchar_t *, size_t);
wchar_t		*wmemmove(wchar_t *, const wchar_t *, size_t);
wchar_t		*wmemset(wchar_t *, wchar_t, size_t);
int		wprintf(const wchar_t *, ...);
int		wscanf(const wchar_t *, ...);

#ifndef	WEOF
#define	WEOF	((wint_t)-1)
#endif

#ifndef	EILSEQ
#define	EILSEQ	(errno)
#endif

#endif	/* !LIBWCHAR_WCHAR_H */
