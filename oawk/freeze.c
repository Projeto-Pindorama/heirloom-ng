/*-
 * This code contains changes by
 *      Gunnar Ritter, Freiburg i. Br., Germany, 2002. All rights reserved.
 *
 * Conditions 1, 2, and 4 and the no-warranty notice below apply
 * to these changes.
 *
 *
 * Copyright (c) 1991
 *      The Regents of the University of California.  All rights reserved.
 *
 * SPDX-Licence-Identifier: BSD-4-Clause-UC
 *
 *
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * SPDX-Licence-Identifier: Caldera
 */

/*	from 4.4BSD /usr/src/old/awk/freeze.c	4.3 (Berkeley) 4/17/91	*/
/*	Sccsid @(#)freeze.c	1.3 (gritter) 3/31/03>	*/

#include "stdio.h"
int
freeze(char *s)
{	int fd;
	unsigned int *len;
	len = (unsigned int *)sbrk(0);
	if((fd = creat(s, 0666)) < 0) {
		perror(s);
		return(1);
	}
	write(fd, &len, sizeof(len));
	write(fd, (char *)0, len);
	close(fd);
	return(0);
}

int
thaw(char *s)
{	int fd;
	unsigned int *len;
	if(*s == 0) {
		fprintf(stderr, "empty restore file\n");
		return(1);
	}
	if((fd = open(s, 0)) < 0) {
		perror(s);
		return(1);
	}
	read(fd, &len, sizeof(len));
	(void) brk(len);
	read(fd, (char *)0, len);
	close(fd);
	return(0);
}
