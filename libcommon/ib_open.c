/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)ib_open.c	1.2 (gritter) 4/17/03	*/

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<string.h>
#include	<errno.h>
#include	<stdlib.h>

#include	"iblok.h"

struct iblok *
ib_open(const char *name, unsigned blksize)
{
	struct iblok	*ip;
	int	fd, err;

	if ((fd = open(name, O_RDONLY)) < 0)
		return NULL;
	if ((ip = ib_alloc(fd, blksize)) == NULL) {
		err = errno;
		close(fd);
		errno = err;
	}
	return ip;
}
