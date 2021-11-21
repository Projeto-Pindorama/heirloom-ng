/*
 * uname - get system name
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
static const char sccsid[] USED = "@(#)uname.sl	1.16 (gritter) 5/29/05";

#include	<sys/utsname.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>

#if defined (__i386__)
#define	PROCESSOR	"ia32"
#elif defined (__x86_64__)
#define	PROCESSOR	"x86_64"
#elif defined (__itanium__)
#define	PROCESSOR	"ia64"
#elif defined (__sparc)
#define	PROCESSOR	"sparc"
#elif defined (__R5900)
#define	PROCESSOR	"r5900"
#else
#define	PROCESSOR	"unknown"
#endif

static enum {
	FL_M = 0001,
	FL_N = 0002,
	FL_R = 0004,
	FL_S = 0010,
	FL_V = 0020,
	FL_P = 0040,
	FL_A = 0777
} flags;

static unsigned	errcnt;			/* count of errors */
static char	*progname;		/* argv[0] to main() */
static char	*system_name;		/* new system name */

static void
usage(void)
{
	fprintf(stderr, "\
usage:  %s [-snrvmap]\n\
        %s [-S system name]\n",
       		progname, progname);
	exit(2);
}

#define	FIELD(x)	printf("%s%s", hadfield++ ? " " : "", x)

static int
go(void)
{
	struct utsname	u;
	char	*sysv3, *token;
	int	hadfield = 0;

	if (uname(&u) < 0) {
		fprintf(stderr, "%s: uname: %s\n", progname, strerror(errno));
		return 1;
	}
	if ((sysv3 = getenv("SYSV3")) != NULL && *sysv3 != '\0') {
		token = strtok(sysv3, ",");
		if (token = strtok(0, ","))
			snprintf(u.sysname, sizeof u.sysname, token);
		if (token = strtok(0, ","))
			snprintf(u.nodename, sizeof u.nodename, token);
		if (token = strtok(0, ","))
			snprintf(u.release, sizeof u.release, token);
		if (token = strtok(0, ","))
			snprintf(u.version, sizeof u.version, token);
		if (token = strtok(0, ","))
			snprintf(u.machine, sizeof u.machine, token);
	} else if (sysv3) {
		snprintf(u.sysname, sizeof u.sysname, u.nodename);
		snprintf(u.release, sizeof u.release, "3.2");
		snprintf(u.version, sizeof u.version, "2");
		snprintf(u.machine, sizeof u.machine, "i386");
	}
	if (flags & FL_S)
		FIELD(u.sysname);
	if (flags & FL_N)
		FIELD(u.nodename);
	if (flags & FL_R)
		FIELD(u.release);
	if (flags & FL_V)
		FIELD(u.version);
	if (flags & FL_M)
		FIELD(u.machine);
	if (flags & FL_P && (sysv3 == 0 || flags != FL_A))
		FIELD(PROCESSOR);
	if (hadfield)
		putchar('\n');
	return 0;
}

int
main(int argc, char **argv)
{
	const char	optstring[] = "amnprsvS:";
	int	i;

#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	progname = basename(argv[0]);
	while ((i = getopt(argc, argv, optstring)) != EOF) {
		switch (i) {
		case 'a':
			flags |= FL_A;
			break;
		case 'm':
			flags |= FL_M;
			break;
		case 'n':
			flags |= FL_N;
			break;
		case 'p':
			flags |= FL_P;
			break;
		case 'r':
			flags |= FL_R;
			break;
		case 's':
			flags |= FL_S;
			break;
		case 'v':
			flags |= FL_V;
			break;
		case 'S':
			system_name = optarg;
			break;
		default:
			usage();
		}
	}
	if (optind < argc)
		usage();
	if (system_name) {
		if (flags)
			usage();
		if (sethostname(system_name, strlen(system_name)) < 0) {
			fprintf(stderr, "%s: sethostname: %s\n",
					progname, strerror(errno));
			errcnt |= 010;
		}
	} else {
		if (flags == 0)
			flags = FL_S;
		errcnt |= go();
	}
	return errcnt;
}
