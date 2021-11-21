/*
 * nohup - run a command immune to hangups
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
#ifdef	SUS
static const char sccsid[] USED = "@(#)nohup_sus.sl	1.11 (gritter) 5/29/05";
#else
static const char sccsid[] USED = "@(#)nohup.sl	1.11 (gritter) 5/29/05";
#endif

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<signal.h>
#include	<libgen.h>
#include	<limits.h>

static char	*progname;		/* argv[0] to main() */
static int	errorfd = 2;

static void
fatal(int eno, const char *string, int val)
{
	char	buf[PATH_MAX];

	snprintf(buf, sizeof buf, "%s: %s: %s\n", progname, string,
				strerror(eno));
	write(errorfd, buf, strlen(buf));
	exit(val);
}

/*
 * Memory allocation with check.
 */
static void *
srealloc(void *vp, size_t nbytes)
{
	void *p;

	if ((p = realloc(vp, nbytes)) == NULL) {
		write(2, "Out of memory\n", 14);
		exit(077);
	}
	return p;
}

static void *
smalloc(size_t nbytes)
{
	return srealloc(NULL, nbytes);
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s command arg ...\n", progname);
	exit(0177);
}

static void
success(const char *fn)
{
	fprintf(stderr, "Sending output to %s\n", fn);
}

static void
failed(const char *fn)
{
	fprintf(stderr, "%s: cannot open/create %s\n", progname, fn);
	exit(0177);
}

static void
create_nohup_out(void)
{
	char	*nohup_out = "nohup.out";
	char	*fn;
	int	fd;
#ifdef	SUS
	const mode_t	mode = S_IRUSR | S_IWUSR;
#else
	const mode_t	mode = 0666;
#endif

	fn = nohup_out;
	if ((fd = open(fn, O_WRONLY|O_CREAT|O_APPEND, mode)) < 0) {
		char	*home;

		if ((home = getenv("HOME")) != NULL) {
			char	*cp, *cq;

			cp = fn = smalloc(strlen(home) + strlen(nohup_out) + 2);
			for (cq = home; *cq; cq++)
				*cp++ = *cq;
			*cp++ = '/';
			cq = nohup_out;
			while ((*cp++ = *cq++) != '\0');
			if ((fd = open(fn, O_WRONLY|O_CREAT|O_APPEND,
							mode)) < 0)
				failed(fn);
		} else
			failed(fn);
	}
	success(fn);
	if (dup2(fd, 1) < 0)
		fatal(errno, "dup2", 0177);
	close(fd);
}

int
main(int argc, char **argv)
{
	int i;

	progname = basename(argv[0]);
	if (argc > 1 && argv[1][0] == '-' && argv[1][1] == '-'
			&& argv[1][2] == '\0')
		argv++, argc--;
	if (argc <= 1)
		usage();
	if (isatty(1))
		create_nohup_out();
	errorfd = dup(2);
	fcntl(errorfd, F_SETFD, FD_CLOEXEC);
	if (isatty(2))
		if (dup2(1, 2) < 0)
			fatal(errno, "dup2", 0177);
	signal(SIGHUP, SIG_IGN);
#ifndef	SUS
	signal(SIGQUIT, SIG_IGN);
#endif
	execvp(argv[1], &argv[1]);
	i = errno;
	fatal(i, argv[1], i == ENOENT ? 0177 : 0176);
	/*NOTREACHED*/
	return 0177;
}
