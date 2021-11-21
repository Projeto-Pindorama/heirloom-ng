/*
 * env - set environment for command
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, August 2002.
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

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)env.sl	1.10 (gritter) 5/29/05";

#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>

static int	iflag;			/* ignore initial environment */
static char	*progname;		/* argv[0] to main() */

extern char	**environ;

int
main(int argc, char **argv)
{
	int	i;

	progname = basename(argv[0]);
	while ((i = getopt(argc, argv, ":i")) != EOF) {
		switch (i) {
		case 'i':
			iflag = 1;
			break;
		default:
			optind--;
			goto done;
		}
	}
done:	if (optind < argc && strcmp(argv[optind], "-") == 0 &&
			strcmp(argv[optind - 1], "--")) {
		iflag = 1;
		optind++;
	}
	if (iflag)
		environ = NULL;
	while (optind < argc && strchr(argv[optind], '=') != NULL)
		putenv(argv[optind++]);
	if (optind >= argc) {
		char	**cp;

		if (environ) {
			for (cp = environ; *cp; cp++)
				puts(*cp);
		}
		return 0;
	}
	execvp(argv[optind], &argv[optind]);
	i = errno;
	fprintf(stderr, "%s: %s\n", strerror(i), argv[optind]);
	return i == ENOENT ? 0177 : 0176;
}
