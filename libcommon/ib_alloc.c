/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
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
