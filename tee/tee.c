/*
 * tee - pipe fitting
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
static const char sccsid[] USED = "@(#)tee.sl	1.10 (gritter) 5/29/05";

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>
#include	<signal.h>
#include	<limits.h>

struct	file {
	struct file	*f_nxt;
	const char	*f_nam;
	int		f_fd;
};

static unsigned		errcnt;			/* count of errors */
static char		*progname;		/* argv[0] to main() */
static struct file	*f0;			/* files to write to */
static int		totty;			/* any output file is a tty */

static void *
srealloc(void *vp, size_t nbytes)
{
	void *p;

	if ((p = (void *)realloc(vp, nbytes)) == NULL) {
		write(2, "no memory\n", 10);
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
	fprintf(stderr, "usage: %s [ -i ] [ -a ] [file ] ...\n", progname);
	exit(2);
}

static int
copyto(struct file *f, char *data, ssize_t size)
{
	ssize_t	wo, wt;

	wt = 0;
	do {
		if ((wo = write(f->f_fd, data + wt, size - wt)) < 0) {
			fprintf(stderr, "%s: write error on %s: %s\n",
					progname, f->f_nam, strerror(errno));
			return 4;
		}
		wt += wo;
	} while (wt < size);
	return 0;
}

static void
copy(void)
{
	struct stat	st;
	struct file	*f;
	char	*buf;
	size_t	bufsize;
	ssize_t	rsz;

	if (fstat(0, &st) < 0) {
		fprintf(stderr, "%s: cannot stat() standard input\n", progname);
		errcnt |= 010;
		return;
	}
	bufsize = totty ? 512 : st.st_blksize < 512 ? 512 :
		st.st_blksize > 32768 ? 32768 : st.st_blksize;
	buf = smalloc(bufsize);
	while ((rsz = read(0, buf, bufsize)) > 0)
		for (f = f0; f; f = f->f_nxt)
			errcnt |= copyto(f, buf, rsz);
	if (rsz < 0) {
		fprintf(stderr, "%s: read error on standard input: %s\n",
				progname, strerror(errno));
		errcnt |= 020;
	}
}

static int
make(const char *fn, mode_t mode, int fd)
{
	struct file	*f;

	if (fd < 0 && (fd = open(fn, mode, 0666)) < 0) {
		fprintf(stderr, "%s: cannot open %s\n", progname, fn);
		return 1;
	}
	if (totty == 0 && isatty(fd))
		totty = 1;
	f = smalloc(sizeof *f);
	f->f_nam = fn;
	f->f_fd = fd;
	f->f_nxt = f0;
	f0 = f;
	return 0;
}

int
main(int argc, char **argv)
{
	struct file	*f;
	int	i;
	mode_t	mode = O_WRONLY|O_CREAT|O_TRUNC;

#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	progname = basename(argv[0]);
	while ((i = getopt(argc, argv, "ai")) != EOF) {
		switch (i) {
		case 'a':
			mode = O_WRONLY|O_CREAT|O_APPEND;
			break;
		case 'i':
			signal(SIGINT, SIG_IGN);
			break;
		default:
			usage();
		}
	}
	for (i = optind; i < argc; i++)
		errcnt |= make(argv[i], mode, -1);
	errcnt |= make("standard output", mode, 1);
	copy();
	for (f = f0; f; f = f->f_nxt) {
		if (f->f_fd != 1) {
			if (close(f->f_fd) < 0) {
				fprintf(stderr, "%s: close error on %s: %s\n",
						progname, f->f_nam,
						strerror(errno));
				errcnt |= 040;
			}
		}
	}
	return errcnt;
}
