/*
 * cmp - compare two files
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
static const char sccsid[] USED = "@(#)cmp.sl	1.19 (gritter) 5/29/05";

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<malloc.h>
#include	<errno.h>
#include	<libgen.h>
#include	<limits.h>

#include	"atoll.h"
#include	"memalign.h"

#if defined (__GLIBC__) && defined (_IO_putc_unlocked)
#undef	putchar
#define	putchar(c)	_IO_putc_unlocked(c, stdout)
#endif	/* __GLIBC__ */

#define	BLKSIZE		8192
#define	IOSIZE		(SSIZE_MAX > BLKSIZE ? BLKSIZE : SSIZE_MAX)

struct	file {
	char	f_buf[IOSIZE];		/* input buffer */
	struct stat	f_st;		/* fstat(f_fd) */
	long long	f_off;		/* offset of f_buf in file */
	char	*f_cur;			/* current position in buffer */
	char	*f_max;			/* highest valid position in buffer+1 */
	const char	*f_nam;		/* file name */
	int	f_fd;			/* file descriptor */
	int	f_eof;			/* end of file reached */
};

static unsigned	errcnt;			/* count of errors */
static int	lflag;			/* write differing bytes */
static int	sflag;			/* write nothing */
static int	wflag;			/* word mode (Cray) */
static char	*progname;		/* argv[0] to main() */

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-l] [-s] file1 file2 [skip1 [skip2] ]\n",
			progname);
	exit(2);
}

static void
eofon(struct file *f)
{
	if (sflag == 0)
		fprintf(stderr, "%s: EOF on %s\n", progname, f->f_nam);
	exit(1);
}

static void
printbe64(const char *s, int max)
{
	int	i;
	unsigned long long	u = 0;

	for (i = 0; i < max; i++)
		u += (unsigned long long)(s[i]&0377) << 8*(7-i);
	printf("%022llo", u);
}

static void
printbytes(const char *s, int max)
{
	int	i, c;

	for (i = 0; i < 8; i++) {
		c = s[i] & 0377;
		if (i >= max || c < ' ' || c > '~')
			putchar('_');
		else
			putchar(c);
	}
}

static ssize_t
sread(int fd, char *data, size_t size)
{
	ssize_t	rd, ro;

	rd = 0;
	do {
		if ((ro = read(fd, data + rd, size - rd)) < 0)
			return -1;
		rd += ro;
	} while (ro > 0 && rd < size);
	return rd;
}

static int
bread(struct file *f)
{
	ssize_t	rsz;

	if ((rsz = sread(f->f_fd, f->f_buf, sizeof f->f_buf)) < 0)
		rsz = 0;
	if (rsz < sizeof f->f_buf)
		f->f_eof = 1;
	f->f_cur = f->f_buf;
	f->f_max = &f->f_buf[rsz];
	f->f_off += rsz;
	if (wflag && f->f_max < &f->f_buf[sizeof f->f_buf])
		memset(f->f_max, 0, &f->f_buf[sizeof f->f_buf] - f->f_max);
	return rsz > 0 ? *f->f_cur++ & 0377 : EOF;
}

#define	offset(f)	(f->f_off - (f->f_max - f->f_buf - 1) + \
				(f->f_cur - f->f_buf - 1))

#define	getchr(f)	(f->f_cur < f->f_max ? *f->f_cur++ & 0377 : \
				f->f_eof ? EOF : bread(f))

static int
wprnt(struct file *f1, struct file *f2)
{
	long long	offs = offset(f1) - 1;
	int	mod = offs % 8;
	int	eof = 0, diff, max;

	f1->f_cur -= mod + 1;
	f2->f_cur -= mod + 1;
	if (f1->f_cur + 9 > f1->f_max && f1->f_eof)
		eof |= 1;
	if (f2->f_cur + 9 > f2->f_max && f2->f_eof)
		eof |= 2;
	diff = eof ? (f1->f_max - f1->f_buf) - (f2->f_max - f2->f_buf) : 0;
	if (diff < 0)
		max = f1->f_max - f1->f_cur;
	else if (diff > 0)
		max = f2->f_max - f2->f_cur;
	else
		max = 8;
	printf("%011llo: ", offs / 8);
	printbe64(f1->f_cur, max);
	putchar(' ');
	printbe64(f2->f_cur, max);
	putchar(' ');
	printbytes(f1->f_cur, max);
	putchar(' ');
	printbytes(f2->f_cur, max);
	putchar('\n');
	if (eof) {
		if (diff > 0)
			eofon(f2);
		else if (diff < 0)
			eofon(f1);
	}
	f1->f_cur += 9;
	f2->f_cur += 9;
	return eof;
}

static void
cmp(struct file *f1, struct file *f2)
{
	int	c1, c2;
	unsigned	i;
	long long	line = 1;

	for (;;) {
		c1 = bread(f1);
		c2 = bread(f2);
		if (c1 != c2)
			goto different;
		if (f1->f_eof == 0 && f2->f_eof == 0) {
			for (i = 0; i < sizeof f1->f_buf; i++) {
				if (f1->f_buf[i] == f2->f_buf[i]) {
					if (f1->f_buf[i] == '\n')
						line++;
				} else {
					c1 = f1->f_buf[i] & 0377;
					c2 = f2->f_buf[i] & 0377;
					f1->f_cur = &f1->f_buf[i + 1];
					f2->f_cur = &f2->f_buf[i + 1];
					goto different;
				}
			}
		} else
			goto equal;
	}
	/*LINTED*/
	for (;;) {
		c1 = getchr(f1);
		c2 = getchr(f2);
		if (c1 == c2) {
equal:
			if (c1 == '\n')
				line++;
			else if (c1 == EOF)
				break;
		} else {
different:
			if (wflag) {
				errcnt = 1;
				if (wprnt(f1, f2) != 0)
					break;
			} else if (c1 == EOF)
				eofon(f1);
			else if (c2 == EOF)
				eofon(f2);
			else if (lflag) {
				printf("%6lld %3o %3o\n", (long long)offset(f1),
				c1, c2);
				errcnt = 1;
			} else {
				if (sflag == 0)
					printf("%s %s differ: char %lld,"
							" line %lld\n",
						f1->f_nam, f2->f_nam,
						(long long)offset(f1),
						line);
				exit(1);
			}
		}
	}
}

static struct file *
openfile(const char *fn)
{
	static long	pagesize;
	struct file	*f;

	if (pagesize == 0)
		if ((pagesize = sysconf(_SC_PAGESIZE)) < 0)
			pagesize = 4096;
	if ((f = memalign(pagesize, sizeof *f)) == NULL) {
		write(2, "no memory\n", 10);
		_exit(077);
	}
	if (fn[0] == '-' && fn[1] == '\0') {
		f->f_fd = 0;
		f->f_nam = "standard input";
	} else {
		if ((f->f_fd = open(fn, O_RDONLY)) < 0) {
			if (sflag == 0)
				fprintf(stderr, "%s: cannot open %s\n",
						progname, fn);
			exit(2);
		}
		f->f_nam = fn;
	}
	if (fstat(f->f_fd, &f->f_st) < 0) {
		if (sflag == 0)
			fprintf(stderr, "%s: cannot stat %s\n",
					progname, f->f_nam);
		exit(2);
	}
	f->f_off = 0;
	f->f_cur = f->f_max = &f->f_buf[sizeof f->f_buf];
	return f;
}

static void
setskip(struct file *f, const char *skipstring)
{
	long long	skip;
	ssize_t	rsz;

	skip = strtoll(skipstring, NULL, *skipstring == '0' ? 8 : 10);
	if (skip > sizeof f->f_buf)
		while (f->f_off < skip - sizeof f->f_buf)
			bread(f);
	if (f->f_off < skip) {
		rsz = sread(f->f_fd, f->f_buf, skip - f->f_off);
		if (rsz < skip - f->f_off)
			f->f_eof = 1;
	}
	if (f->f_eof)
		eofon(f);
	f->f_off = 0;
}

int
main(int argc, char **argv)
{
	struct file	*f1, *f2;
	const char	optstring[] = "lsw";
	int	i;

#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	progname = basename(argv[0]);
	while ((i = getopt(argc, argv, optstring)) != EOF) {
		switch (i) {
		case 'l':
			lflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 'w':
			wflag = 1;
			break;
		default:
			usage();
		}
	}
	if (argc - optind < 2 || argc - optind > 4)
		usage();
	f1 = openfile(argv[optind++]);
	f2 = openfile(argv[optind++]);
	if (optind < argc) {
		setskip(f1, argv[optind++]);
		if (optind < argc)
			setskip(f2, argv[optind]);
	}
	cmp(f1, f2);
	return errcnt;
}
