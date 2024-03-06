/*
 * Copyright (c) 2004 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)pathconf.h	1.2 (gritter) 5/1/04	*/

#ifdef	__dietlibc__
#include <unistd.h>

extern long	fpathconf(int, int);
extern long	pathconf(const char *, int);
#endif	/* __dietlibc__ */
