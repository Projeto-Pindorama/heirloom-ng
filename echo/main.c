/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */

/*
 * Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)main.c	1.3 (gritter) 7/2/05
 */

#include "defs.h"
#include <locale.h>

int	mb_cur_max;

int
main(int argc, char **argv)
{
	extern int	func(int, char **);
	extern int	sysv3;

	if (sysv3)
		putenv("SYSV3=set");
	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	return func(argc, argv);
}

struct namnod *
findnam(const char *name)
{
	static struct namnod	n;
	char	*cp;

	if ((cp = getenv(name)) != NULL) {
		n.value = cp;
		n.namflg = N_EXPORT|N_ENVNAM;
		return &n;
	}
	return NULL;
}
