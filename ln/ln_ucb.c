/*
 * This code contains changes by
 *      Gunnar Ritter, Freiburg i. Br., Germany, February 2003.
 *      All rights reserved.
 *
 *
 * Copyright (c) 1980, 1993
 * 	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 	This product includes software developed by the University of
 * 	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*	from 4.3BSD  ln.c 4.6 11/15/85	*/

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)/usr/ucb/ln.sl	1.6 (gritter) 5/29/05";

/*
 * ln
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static struct	stat stb;
static int	fflag;		/* force flag set? */
static int	sflag;
static char	*progname;

static int	linkit(const char *from, const char *to);
static void	*srealloc(void *, size_t);

int
main(int argc, register char **argv)
{
	register int i, r;

	progname = basename(argv[0]);
	argc--, argv++;
	if (argc && strcmp(argv[0], "-f") == 0) {
		fflag++;
		argv++;
		argc--;
	}
	if (argc && strcmp(argv[0], "-s") == 0) {
		sflag++;
		argv++;
		argc--;
	}
	if (argc == 0) 
		goto usage;
	else if (argc == 1) {
		argv[argc] = ".";
		argc++;
	}
	if (sflag == 0 && argc > 2) {
		if (stat(argv[argc-1], &stb) < 0)
			goto usage;
		if ((stb.st_mode&S_IFMT) != S_IFDIR) 
			goto usage;
	}
	r = 0;
	for(i = 0; i < argc-1; i++)
		r |= linkit(argv[i], argv[argc-1]);
	exit(r);
usage:
	fprintf(stderr,
		"Usage: %s [-s] f1 or %s [-s] f1 f2 or %s [-s] f1 ... fn d2\n",
		progname, progname, progname);
	exit(2);
}

static int
linkit(const char *from, const char *to)
{
	const char *tail, *cp;
	int (*linkf)(const char *, const char *) = sflag ? symlink : link;

	/* is target a directory? */
	if (sflag == 0 && fflag == 0 && stat(from, &stb) >= 0
	    && (stb.st_mode&S_IFMT) == S_IFDIR) {
		printf("%s is a directory\n", from);
		return (1);
	}
	if (stat(to, &stb) >= 0 && (stb.st_mode&S_IFMT) == S_IFDIR) {
		static char	*name;
		char	*np;

		tail = strrchr(from, '/');
		if (tail == 0)
			tail = from;
		else
			tail++;
		np = name = srealloc(name, strlen(to) + strlen(tail) + 2);
		for (cp = to; *cp; cp++)
			*np++ = *cp;
		*np++ = '/';
		for (cp = tail; *cp; cp++)
			*np++ = *cp;
		*np = '\0';
		to = name;
	}
	if ((*linkf)(from, to) < 0) {
		if (errno == EEXIST || sflag)
			perror(to);
		else if (access(from, 0) < 0)
			perror(from);
		else
			perror(to);
		return (1);
	}
	return (0);
}

static void *
srealloc(void *op, size_t size)
{
	void	*np;

	if ((np = realloc(op, size)) == NULL) {
		write(2, "no memory\n", 10);
		_exit(077);
	}
	return np;
}
