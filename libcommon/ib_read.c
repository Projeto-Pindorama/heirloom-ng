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
/*	Sccsid @(#)ib_read.c	1.2 (gritter) 4/17/03	*/

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<string.h>
#include	<errno.h>
#include	<stdlib.h>

#include	"iblok.h"

int
ib_read(struct iblok *ip)
{
	ssize_t	sz;

	do {
		if ((sz = read(ip->ib_fd, ip->ib_blk, ip->ib_blksize)) > 0) {
			ip->ib_endoff += sz;
			ip->ib_cur = ip->ib_blk;
			ip->ib_end = &ip->ib_blk[sz];
			return *ip->ib_cur++ & 0377;
		}
	} while (sz < 0 && errno == EINTR);
	if (sz < 0)
		ip->ib_errno = errno;
	ip->ib_cur = ip->ib_end = NULL;
	return EOF;
}
