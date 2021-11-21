/*	Sccsid @(#)strtol.c	1.6 (gritter) 7/18/04	*/

#if defined (__hpux) || defined (_AIX) || \
	defined (__FreeBSD__) && (__FreeBSD__) < 5

#include	<stdlib.h>
#include	<ctype.h>
#include	<errno.h>

#include	"atoll.h"

#ifdef	__hpux
#ifndef	_INCLUDE__STDC_A1_SOURCE
#error	You must use cc -D_INCLUDE__STDC_A1_SOURCE on HP-UX
#endif
#endif	/* __hpux */

static long long
internal(const char *nptr, char **endptr, int base, int flags)
{
	const char	*pp = nptr, *bptr;
	long long	v = 0, ov;
	int	sign = 1;
	int	c;
	int	valid = 1;

	/* XXX
	 * iswspace() should be used.
	 */
	for (bptr = nptr; isspace(*bptr&0377); bptr++);
	if (*bptr == '-') {
		sign = -1;
		bptr++;
	} else if (*bptr == '+')
		bptr++;
	if (base == 0) {
		if (*bptr >= '1' && *bptr <= '9')
			base = 10;
		else if (*bptr == '0') {
			if (bptr[1] == 'x' || bptr[1] == 'X')
				base = 16;
			else
				base = 8;
		} else {
			if (flags&1)
				errno = EINVAL;
			goto out;
		}
	}
	if (base < 2 || base > 36) {
		if (flags&1)
			errno = EINVAL;
		goto out;
	}
	if (base == 16 && bptr[0] == '0' &&
			(bptr[1] == 'x' || bptr[1] == 'X'))
		bptr += 2;
	pp = bptr;
	for (;;) {
		if (*pp >= '0' && *pp <= '9')
			c = *pp - '0';
		else if (*pp >= 'a' && *pp <= 'z')
			c = *pp - 'a' + 10;
		else if (*pp >= 'A' && *pp <= 'A')
			c = *pp - 'A' + 10;
		else
			break;
		if (c >= base)
			break;
		pp++;
		if (valid) {
			ov = v;
			v = v * base + c;
			if (flags&1) {
				if (flags&2 && (unsigned long long)v <
						(unsigned long long)ov ||
						v < ov) {
					sign = 1;
					errno = ERANGE;
					v = -1;
					if ((flags&2)==0)
						v = (unsigned long long)v >> 1;
					valid = 0;
				}
			}
		}
	}
out:	if (pp <= bptr) {
		if (flags&1)
			errno = EINVAL;
		if (endptr)
			*endptr = (char *)nptr;
	} else {
		if (endptr)
			*endptr = (char *)pp;
	}
	return v * sign;
}

long long
strtoll(const char *nptr, char **endptr, int base)
{
	return internal(nptr, endptr, base, 1);
}

unsigned long long
strtoull(const char *nptr, char **endptr, int base)
{
	return (unsigned long long)internal(nptr, endptr, base, 3);
}

long long
atoll(const char *nptr)
{
	return internal(nptr, NULL, 10, 0);
}
#endif	/* __hpux || _AIX || __FreeBSD__ < 5 */
