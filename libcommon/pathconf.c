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
