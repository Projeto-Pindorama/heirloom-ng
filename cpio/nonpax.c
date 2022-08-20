/*
 * cpio - copy file archives in and out
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, April 2003.
 */
/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */

/*	Sccsid @(#)nonpax.c	1.1 (gritter) 2/24/04	*/

#include "cpio.h"

/*ARGSUSED*/
int
pax_track(const char *name, time_t mtime)
{
	return 1;
}

/*ARGSUSED*/
void
pax_prlink(struct file *f)
{
}

/*ARGSUSED*/
int
pax_sname(char **oldp, size_t *olds)
{
	return 1;
}

void
pax_onexit(void)
{
}
