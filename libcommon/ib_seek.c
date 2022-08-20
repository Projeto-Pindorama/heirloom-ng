/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)ib_seek.c	1.4 (gritter) 5/8/03	*/

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<string.h>
#include	<errno.h>
#include	<stdlib.h>

#include	"iblok.h"

off_t
ib_seek(struct iblok *ip, off_t off, int whence)
{
	if (whence == SEEK_CUR) {
		off = ip->ib_endoff - (ip->ib_end - ip->ib_cur);
		whence = SEEK_SET;
	}
	if (ip->ib_seekable && whence == SEEK_SET && ip->ib_cur && ip->ib_end &&
			off < ip->ib_endoff &&
			off >= ip->ib_endoff - (ip->ib_end - ip->ib_blk)) {
		ip->ib_cur = ip->ib_end - (ip->ib_endoff - off);
		return off;
	}
	if ((off = lseek(ip->ib_fd, off, whence)) == (off_t)-1)
		return -1;
	ip->ib_cur = ip->ib_end = NULL;
	ip->ib_endoff = off;
	ip->ib_seekable = 1;
	return off;
}
