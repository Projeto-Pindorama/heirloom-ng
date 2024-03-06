/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
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
