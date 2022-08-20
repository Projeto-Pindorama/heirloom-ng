/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)ib_close.c	1.2 (gritter) 4/17/03	*/

#include	<unistd.h>

#include	"iblok.h"

int
ib_close(struct iblok *ip)
{
	int	fd;

	fd = ip->ib_fd;
	ib_free(ip);
	return close(fd);
}
