/*
 * nice - run a command at low priority
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
static const char sccsid[] USED = "@(#)nice.sl	1.10 (gritter) 5/29/05";

#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>

static unsigned	errcnt;			/* count of errors */
static char	*progname;		/* argv[0] to main() */

/*
 * perror()-alike.
 */
static void
pnerror(int eno, const char *string)
{
	fprintf(stderr, "%s: %s\n", strerror(eno), string);
	errcnt++;
}

static void
usage(void)
{
	fprintf(stderr, "%s: usage: %s [-num] command\n",
			progname, progname);
	exit(2);
}

static void
notnumeric(void)
{
	fprintf(stderr, "%s: argument must be numeric.\n", progname);
	exit(2);
}

static int
get_niceval(char *arg)
{
	char	*x;
	int	n;

	n = strtol(arg, &x, 10);
	if (*x != '\0')
		notnumeric();
	return n;
}

int
main(int argc, char **argv)
{
	int	e, i;
	int	niceval = 10;

	progname = basename(argv[0]);
	i = 0;
	while (++i < argc && argv[i][0] == '-') {
		switch (argv[i][1]) {
		case 'n':
			if (argv[i][2])
				niceval = get_niceval(&argv[i][2]);
			else if (argv[i+1])
				niceval = get_niceval(argv[++i]);
			else
				notnumeric();
			break;
		case '-':
			if (argv[i][2] == '\0') {
				i++;
				goto optend;
			}
			/*FALLTHRU*/
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			niceval = get_niceval(&argv[i][1]);
			break;
		default:
			notnumeric();
		}
	}
optend:
	if (i >= argc)
		usage();
	nice(niceval);
	execvp(argv[i], &argv[i]);
	e = errno;
	pnerror(e, argv[i]);
	return e == ENOENT ? 0177 : 0176;
}
