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
/*	Sccsid @(#)ib_alloc.c	1.5 (gritter) 3/12/05	*/

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<string.h>
#include	<errno.h>
#include	<stdlib.h>
#include	<malloc.h>

#include	"memalign.h"
#include	"iblok.h"

struct iblok *
ib_alloc(int fd, unsigned blksize)
{
	static long	pagesize;
	struct iblok	*ip;
	struct stat	st;

	if (pagesize == 0)
		if ((pagesize = sysconf(_SC_PAGESIZE)) < 0)
			pagesize = 4096;
	if (blksize == 0) {
		if (fstat(fd, &st) < 0)
			return NULL;
		blksize = st.st_blksize > 0 ? st.st_blksize : 512;
	}
	if ((ip = calloc(1, sizeof *ip)) == NULL)
		return NULL;
	if ((ip->ib_blk = memalign(pagesize, blksize)) == NULL) {
		free(ip);
		return NULL;
	}
	ip->ib_blksize = blksize;
	ip->ib_fd = fd;
	ip->ib_mb_cur_max = MB_CUR_MAX;
	return ip;
}
