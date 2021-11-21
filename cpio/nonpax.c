/*
 * cpio - copy file archives in and out
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, April 2003.
 */
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
