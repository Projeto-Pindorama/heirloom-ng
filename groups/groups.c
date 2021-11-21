/*
 * groups - print group membership of user
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, August 2003.
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
#ifndef	UCB
static const char sccsid[] USED = "@(#)groups.sl	1.3 (gritter) 5/29/05";
#else
static const char sccsid[] USED = "@(#)/usr/ucb/groups.sl	1.3 (gritter) 5/29/05";
#endif

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <libgen.h>

static const char	*progname;
static int		status;

static void	groups(const struct passwd *, int);

int
main(int argc, char **argv)
{
	struct passwd	*pw;
	int	i;

	progname = basename(argv[0]);
	if (argc > 1) {
		for (i = 1; i < argc; i++) {
			if ((pw = getpwnam(argv[i])) != NULL)
				groups(pw,
#ifndef	UCB
						argc > 2
#else
						1
#endif
					);
			else {
				fprintf(stderr, "%s: %s : No such user\n",
						progname, argv[i]);
				status = 1;
			}
		}
	} else {
#ifndef	UCB
		if ((pw = getpwuid(getuid())) != NULL)
			groups(pw, 0);
		else {
			fprintf(stderr, "%s: No passwd entry\n", progname);
			status = 1;
		}
#else	/* UCB */
		int	i, size, spc = 0;
		struct group	*gp;
		gid_t	*list;

		size = getgroups(0, NULL);
		if ((list = malloc(size * sizeof *list)) == NULL) {
			write(2, "no memory\n", 10);
			_exit(077);
		}
		getgroups(size, list);
		for (i = 0; i < size; i++) {
			if (spc++)
				putchar(' ');
			if ((gp = getgrgid(list[i])) != NULL)
				printf("%s", gp->gr_name);
			else
				printf("%d", list[i]);
		}
		putchar('\n');
#endif	/* UCB */
	}
	return status;
}

static void
groups(const struct passwd *pw, int prnt)
{
	struct group	*gp;
	char	*prim = NULL;
	int	spc = 0, i;

	if (prnt) {
		printf("%s :", pw->pw_name);
		spc++;
	}
	if (spc++)
		putchar(' ');
	if ((gp = getgrgid(pw->pw_gid)) != NULL) {
		prim = strdup(gp->gr_name);
		printf("%s", prim);
	} else
		printf("%ld", (long)pw->pw_gid);
	setgrent();
	while ((gp = getgrent()) != NULL) {
		for (i = 0; gp->gr_mem[i]; i++)
			if (strcmp(pw->pw_name, gp->gr_mem[i]) == 0 && (
						prim == NULL ||
						strcmp(prim, gp->gr_name))) {
				if (spc++)
					putchar(' ');
				printf("%s", gp->gr_name);
				break;
			}
	}
	putchar('\n');
}
