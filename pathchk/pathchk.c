/*
 * pathchk - check pathnames
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, July 2005.
 */
/*
 * Copyright (c) 2005 Gunnar Ritter
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
static const char sccsid[] USED = "@(#)pathchk.c	1.3 (gritter) 8/15/05";

#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <asciitype.h>

#ifndef	_POSIX_NAME_MAX
#define	_POSIX_NAME_MAX	14
#endif	/* !_POSIX_NAME_MAX */
#ifndef	NAME_MAX
#define	NAME_MAX	255
#endif

static const char	*progname;		/* argv[0] to main() */
static int		status;			/* exit status */
static int		pflag;			/* -p option */
static long		path_max = PATH_MAX;
static long		name_max = NAME_MAX;

static void
usage(void)
{
	fprintf(stderr, "Usage: %s [-p] name ...\n", progname);
	exit(2);
}

static void
pathchk(char *name)
{
	char	*np = name, *nq = name;
	int	c, ac = 0;

	do {
		if (*np == '/') {
			if (pflag == 0) {
				*np = '\0';
				c = access(name, X_OK);
				*np = '/';
				if (c < 0 && errno != ENOENT && ac == 0) {
					fprintf(stderr, "%s: component "
							"\"%.*s\" of name "
							"\"%s\" is not "
							"searchable\n",
						progname, 
						np - name, name,
						name);
					status |= 1;
					ac = 1;
				}
			}
		}
		if (*np == '/' || *np == '\0') {
			if (np - nq >= name_max) {
				fprintf(stderr, "%s: component \"%.*s\" of "
						"name \"%s\" is longer than "
						"%ld bytes\n",
					progname, np - nq, nq, name, name_max);
				status |= 1;
			}
			nq = np;
		} else if (pflag) {
			if (!alnumchar(*np & 0377) && *np != '.' &&
					*np != '_' && *np != '-') {
				fprintf(stderr, "%s: character '\\%o' of name "
						"\"%s\" is not in the portable "
						"filename character set\n",
					progname, *np&0377, name);
				status |= 1;
			}
		}
	} while (*np++ != '\0');
	if (np - name >= path_max) {
		fprintf(stderr, "%s: name \"%s\" is longer than %ld bytes\n",
			progname, name, path_max);
		status |= 1;
	}
}

int
main(int argc, char **argv)
{
	int	c;

	progname = basename(argv[0]);
	while ((c = getopt(argc, argv, "p")) != EOF) {
		switch (c) {
		case 'p':
			pflag = 1;
			path_max = _POSIX_PATH_MAX;
			name_max = _POSIX_NAME_MAX;
			break;
		default:
			usage();
		}
	}
	if (optind == argc)
		usage();
	do
		pathchk(argv[optind]);
	while (++optind < argc);
	return status;
}
