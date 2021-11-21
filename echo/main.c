/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
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
