/*
 * whoami - display the effective current username
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, October 2003.
 */
/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)whoami.sl	1.3 (gritter) 5/29/05";

#include <stdio.h>
#include <pwd.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
	struct passwd	*pw;

	if ((pw = getpwuid(geteuid())) != NULL) {
		puts(pw->pw_name);
		return 0;
	} else {
		puts("Intruder alert.");
		return 1;
	}
}
