/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)ib_free.c	1.2 (gritter) 4/17/03	*/

#include	<stdlib.h>

#include	"iblok.h"

void
ib_free(struct iblok *ip)
{
	free(ip->ib_blk);
	free(ip);
}
