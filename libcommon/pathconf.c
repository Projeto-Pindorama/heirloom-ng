/*
 * Copyright (c) 2004 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)pathconf.c	1.2 (gritter) 5/1/04	*/

#ifdef	__dietlibc__
#include <unistd.h>
#include "pathconf.h"

static long
pc(int name)
{
	switch (name) {
	case _PC_PATH_MAX:
		return 1024;
	case _PC_VDISABLE:
		return 0;
	default:
		return -1;
	}
}

long
fpathconf(int fildes, int name)
{
	return pc(name);
}

long
pathconf(const char *path, int name) {
	return pc(name);
}
#endif	/* __dietlibc__ */
