/*
 * isblank() and iswblank() are not available with many pre-XSH6
 * systems. Check whether isblank was defined, and assume it is
 * not available if not.
 */
/*	Sccsid @(#)blank.h	1.3 (gritter) 5/1/04	*/

#ifndef	__dietlibc__
#ifndef	LIBCOMMON_BLANK_H
#define	LIBCOMMON_BLANK_H	1

#include <ctype.h>
#include <wctype.h>

#ifndef	isblank

static
#ifdef	__GNUC__
__inline__
#endif	/* __GNUC__ */
int
my_isblank(int c)
{
	return c == ' ' || c == '\t';
}
#define	isblank(c)	my_isblank(c)

static int
my_iswblank(wint_t c)
{
	return c == L' ' || c == L'\t';
}
#undef	iswblank
#define	iswblank(c)	my_iswblank(c)

#endif	/* !isblank */
#endif	/* !LIBCOMMON_BLANK_H */
#endif	/* !__dietlibc__ */
