/*
 * head - display first few lines of files
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
static const char sccsid[] USED = "@(#)head.sl	1.13 (gritter) 5/29/05";

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>
#include	"atoll.h"

#include	"iblok.h"

static unsigned	errcnt;			/* count of errors */
static unsigned long long	count = 10;	/* lines to display */
static char	*progname;		/* argv[0] to main() */

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-n] [filename...]\n", progname);
	exit(2);
}

#if defined (__GLIBC__) && defined (_IO_putc_unlocked)
#undef	putchar
#define	putchar(c)	_IO_putc_unlocked(c, stdout)
#endif

static int
head(const char *fn, int putheader)
{
	struct iblok	*ip;
	static int	printed;
	const char	*errfn;
	register int	c;
	unsigned long long	n = 1;

	if (fn[0] == '-' && fn[1] == '\0') {
		ip = ib_alloc(0, 0);
		errfn = "standard input";
	} else {
		if ((ip = ib_open(fn, 0)) == NULL) {
			fprintf(stderr, "%s: %s\n", fn, strerror(errno));
			return 1;
		}
		errfn = fn;
	}
	if (putheader)
		printf("%s==> %s <==\n", printed++ ? "\n" : "", errfn);
	if (count > 0) {
		while ((c = ib_get(ip)) != EOF) {
			putchar(c);
			if (c == '\n' && ++n > count)
				break;
		}
	}
	if (ip->ib_fd)
		ib_close(ip);
	else
		ib_free(ip);
	return 0;
}

static unsigned long long
get_count(char *arg)
{
	char	*x;
	unsigned long long	n;

	if (arg == NULL || *arg == '\0') {
		fprintf(stderr, "%s: missing number\n", progname);
		usage();
	}
	n = strtoull(arg, &x, 10);
	if (*arg == '+' || *arg == '-' || *x != '\0') {
		fprintf(stderr, "%s: argument must be numeric\n", progname);
		usage();
	}
	return n;
}

int
main(int argc, char **argv)
{
	int	i;

	progname = basename(argv[0]);
	i = 0;
	while (++i < argc && argv[i][0] == '-') {
		switch (argv[i][1]) {
		case 'n':
			if (argv[i][2])
				count = get_count(&argv[i][2]);
			else
				count = get_count(argv[++i]);
			break;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			count = get_count(&argv[i][1]);
			break;
		case '\0':
			count = 0;
			break;
		case '-':
			if (argv[i][2] == '\0') {
				i++;
				goto optend;
			}
			/*FALLTHRU*/
		default:
			usage();
		}
	}
optend:
	if (i == argc)
		errcnt |= head("-", 0);
	else if (i == argc - 1)
		errcnt |= head(argv[i], 0);
	else {
		for ( ; i < argc; i++)
			errcnt |= head(argv[i], 1);
	}
	return errcnt < 0100 ? errcnt : 077;
}
