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
