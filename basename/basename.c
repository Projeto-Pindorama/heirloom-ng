/*
 * basename - return non-directory portion of a pathname
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, March 2003.
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
#if defined (SUS)
static const char sccsid[] USED = "@(#)basename_sus.sl	1.9 (gritter) 5/29/05";
#elif defined (UCB)
static const char sccsid[] USED = "@(#)/usr/ucb/basename.sl	1.9 (gritter) 5/29/05";
#else
static const char sccsid[] USED = "@(#)basename.sl	1.9 (gritter) 5/29/05";
#endif

#include <stdio.h>
#include <libgen.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <limits.h>
#include <wchar.h>
#if !defined (SUS) && !defined (UCB)
#include <regexpr.h>
#endif

static char	*progname;

#ifndef	UCB
static void
usage(void)
{
	fprintf(stderr, 
#ifdef	SUS
			"Usage:  %s [ path [ suffix ] ]\n",
#else
			"Usage:  %s [ path [ suffix-pattern ] ]\n",
#endif
			progname);
	exit(2);
}
#endif	/* !UCB */

int
main(int argc, char **argv)
{
	char	*path, *suffix = NULL;
	char	*cp, *sp;

	progname = basename(argv[0]);
	setlocale(LC_CTYPE, "");
	if (argc > 1 && argv[1][0] == '-' && argv[1][1] == '-' &&
			argv[1][2] == '\0')
		argv++, argc--;
	if (argc < 1)
		return 0;
	path = argv[1];
#ifndef	UCB
	if (argc == 1 || *path == '\0')
		path = ".";
	else if (argc == 3)
		suffix = argv[2];
	else if (argc > 3)
		usage();
#else	/* UCB */
	if (argc == 1)
		path = "";
	else if (argc >= 3)
		suffix = argv[2];
#endif	/* UCB */
	for (cp = path; *cp != '\0' && *cp == '/'; cp++);
	if (*cp == '\0') {
#ifndef	UCB
		path = "/";
#else	/* UCB */
		path = "";
#endif	/* UCB */
	} else {
		for (cp = path; *cp; cp++);
#ifndef	UCB
		while (*--cp == '/' && cp > path)
			*cp = '\0';
#else	/* UCB */
		if (*--cp == '/')
			cp = path = "";
#endif	/* UCB */
		while (cp > path && *cp != '/')
			cp--;
		if (*cp == '/')
			path = &cp[1];
#if defined (SUS) || defined (UCB)
		if (suffix
#ifdef	SUS
				&& strcmp(path, suffix)
#endif	/* SUS */
				) {
			for (cp = path; *cp; cp++);
			for (sp = suffix; *sp; sp++);
			while (cp > path && sp > suffix && cp[-1] == sp[-1])
				cp--, sp--;
			if (MB_CUR_MAX > 1) {
				/*
				 * Now check whether cp starts at a character
				 * boundary.
				 */
				int	n;
				wchar_t	wc;

				for (sp = path; *sp && sp < cp; sp += n)
					if ((n=mbtowc(&wc, sp, MB_LEN_MAX)) < 0)
						n = 1;
				if (cp == sp)
					*cp = '\0';
			} else
				*cp = '\0';
		}
#else	/* !SUS, !UCB */
		if (suffix) {
			sp = malloc(strlen(suffix) + 12);
			sprintf(sp, "\\(.*\\)\\(%s\\)$", suffix);
			suffix = sp;
			if ((sp = compile(suffix, NULL, NULL)) == NULL) {
				fprintf(stderr, "expr: RE error\n");
				exit(2);
			}
			if (advance(path, sp) && braslist[1] > path)
				*braslist[1] = '\0';
		}
#endif	/* !SUS, !UCB */
	}
	puts(path);
	return 0;
}
