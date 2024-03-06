/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)getdir.h	1.4 (gritter) 10/19/03	*/

#include	<sys/types.h>

struct	direc {
	unsigned long long	d_ino;
	char	*d_name;
};

extern struct getdb	*getdb_alloc(const char *, int);
extern void		getdb_free(struct getdb *);
extern struct direc	*getdir(struct getdb *, int *);
