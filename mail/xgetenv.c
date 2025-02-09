/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*
 * Copyright (c) 1999, by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*	from OpenSolaris "xgetenv.c	1.10	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)xgetenv.c	1.6 (gritter) 7/3/05
 */
/*LINTLIBRARY*/

/*
 *  NAME
 *	xsetenv, xgetenv, Xgetenv - manage an alternate environment space
 *
 *  SYNOPSIS
 *	int ret = xsetenv(file)
 *	char *x = xgetenv("FOO");
 *	char *x = Xgetenv("FOO");
 *
 *  DESCRIPTION
 *	xsetenv() reads the given file into an internal buffer
 *	and sets up an alternate environment.
 *
 *	Return values:	1 - OKAY
 *			0 - troubles reading the file
 *			-1 - troubles opening the file
 *
 *	xgetenv() returns the environment value from the
 *	alternate environment.
 *
 *	Return values:	(char *)0 - no value for that variable
 *			pointer  - the value
 *
 *	Xgetenv() returns the environment value from the
 *	alternate environment.
 *
 *	Return values:	"" - no value for that variable
 *			pointer  - the value
 *
 *  LIMITATIONS
 *	Assumes the environment is < 5120 bytes, as in the UNIX
 *	System environment. Assumes < 512 lines in the file.
 *	These values may be adjusted below.
 */

#include <sys/types.h>
#include "libmail.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "asciitype.h"

#include <stdlib.h>
#include <unistd.h>

#define	MAXVARS  512
#define	MAXENV  5120

static char **xenv = 0;
static char *(xenvptrs[MAXVARS]);
static char xbuf[MAXENV];

static void reduce(char *);

/*
 *	set up an environment buffer
 *	and the pointers into it
 */
int
xsetenv(char *xfile)
{
	int envctr, infd;
	ssize_t i, nread;

	/* Open the file */
	infd = open(xfile, O_RDONLY);
	if (infd == -1) {
		return (-1);
	}

	/* Read in the entire file. */
	nread = read(infd, xbuf, sizeof (xbuf));
	if (nread < 0) {
		close(infd);
		return (0);
	}

	/*
	 * Set up pointers into the buffer.
	 * Replace \n with \0.
	 * Collapse white space around the = sign and at the
	 * beginning and end of the line.
	 */
	xenv = xenvptrs;
	xenv[0] = &xbuf[0];
	for (i = 0, envctr = 0; i < nread; i++) {
		if (xbuf[i] == '\n') {
			xbuf[i] = '\0';
			reduce(xenv[envctr]);
			xenv[++envctr] = &xbuf[i+1];
			if (envctr == MAXVARS) {
				break;
			}
		}
	}

	xenv[envctr] = 0;
	close(infd);
	return (1);
}

/*
 *	Let getenv() do the dirty work
 *	of looking up the variable. We
 *	do this by temporarily resetting
 *	environ to point to the local area.
 */
char *
xgetenv(char *env)
{
	extern char **environ;
	char *ret, **svenviron;

	if (xenv != NULL) {
		svenviron = environ;
		environ = xenv;
		ret = getenv(env);
		environ = svenviron;
		return (ret);
	} else
		return NULL;
}

/*
 *	Let xgetenv() do the dirty work
 *	of looking up the variable.
 */
char *
Xgetenv(char *env)
{
	char *ret = xgetenv(env);
	return (ret ? ret : "");
}

/*
 * Remove the spaces within the environment variable.
 * The variable can look like this:
 *
 * <sp1> variable <sp2> = <sp3> value <sp4> \0
 *
 * All spaces can be removed, except within
 * the variable name and the value.
 */

static void
reduce(char *from)
{
	char *to = from;
	char *svfrom = from;

	/* <sp1> */
	while (*from &&spacechar(*from&0377))
		from++;

	/* variable */
	while (*from && (*from != '=') && !spacechar(*from&0377))
		*to++ = *from++;

	/* <sp2> */
	while (*from && spacechar(*from&0377))
		from++;

	/* = */
	if (*from == '=')
		*to++ = *from++;

	/* <sp3> */
	while (*from && spacechar(*from&0377))
		from++;

	/* value */
	while (*from)
		*to++ = *from++;

	/* <sp4> */
	while ((to > svfrom) && spacechar(to[-1]&0377))
		to--;
	*to = '\0';
}
