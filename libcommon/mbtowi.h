/*	Sccsid @(#)mbtowi.h	1.2 (gritter) 7/16/04	*/

#ifndef	LIBCOMMON_MBTOWI_H
#define	LIBCOMMON_MBTOWI_H

static
#if defined (__GNUC__) || defined (__USLC__) || defined (__INTEL_COMPILER) || \
		defined (__IBMC__) || defined (__SUNPRO_C)
	inline
#endif
	int
mbtowi(wint_t *pwi, const char *s, size_t n)
{
	wchar_t	wc;
	int	i;

	i = mbtowc(&wc, s, n);
	*pwi = wc;
	return i;
}

#endif	/* !LIBCOMMON_MBTOWI_H */
