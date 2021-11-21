/*
 * tail - deliver the last part of a file
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, August 2002.
 *
 * Parts taken from /usr/src/cmd/tail.c, Unix 7th Edition:
 *
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   Redistributions of source code and documentation must retain the
 *    above copyright notice, this list of conditions and the following
 *    disclaimer.
 *   Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *   All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed or owned by Caldera
 *      International, Inc.
 *   Neither the name of Caldera International, Inc. nor the names of
 *    other contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE
 * LIABLE FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)tail.sl	1.25 (gritter) 6/3/05";

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/mman.h>
#include	<sys/resource.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<poll.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>
#include	<limits.h>
#include	<ctype.h>
#include	"atoll.h"

#define	check0(n)	((n) > 0 ? (n) : 512)

enum	from {
	FR_BEGIN,
	FR_END
};

struct	count {
	long long	c_off;		/* offset */
	enum from	c_frm;		/* count from start (0) or end (1) */
	int		c_typ;		/* 'b' 'c' 'l' */
};

static struct count	defcount = { 10, FR_END, 'l' };	/* default count */

static unsigned	errcnt;			/* count of errors */
static int	fflag;			/* follow file changes */
static int	rflag;			/* reverse count */
static char	*progname;		/* argv[0] to main() */
static int	newopt;			/* new option invocation */
static size_t	outblk;

static void *
srealloc(void *vp, size_t nbytes)
{
	void	*p;

	if ((p = realloc(vp, nbytes)) == NULL) {
		write(2, "no memory\n", 10);
		exit(077);
	}
	return p;
}

static void
usage(void)
{
	fprintf(stderr, "\
usage: %s [+/-[n][lbc][f]] [file]\n\
       %s [+/-[n][l][r|f]] [file]\n",
		progname, progname);
	exit(2);
}

static ssize_t
swrite(int fd, char *data, size_t size)
{
	ssize_t	wt, wo;

	wt = 0;
	do {
		if ((wo = write(fd, data + wt, size - wt)) < 0)
			return -1;
		wt += wo;
	} while (wt < size);
	return wt;
}

static ssize_t
bwrite(int fd, char *data, size_t size)
{
	size_t	i = 0;

	if (size > outblk)
		for ( ; i < size - outblk; i += outblk)
			if (swrite(fd, data + i, outblk) < 0)
				return -1;
	if (i < size)
		if (swrite(fd, data + i, size - i) < 0)
			return -1;
	return size;
}

static int
tailf(int fd, struct stat *sp)
{
	char	*buf;
	size_t	bufsize;
	ssize_t	sz;

	buf = srealloc(NULL, bufsize = check0(sp->st_blksize));
	for (;;) {
		poll(NULL, 0, 1000);
		while ((sz = read(fd, buf, bufsize)) > 0)
			bwrite(1, buf, sz);
	}
	/*NOTREACHED*/
	return 0;
}

static int
copy(int fd, struct stat *sp)
{
	char	*buf;
	size_t	bufsize;
	ssize_t	sz;

	buf = srealloc(NULL, bufsize = check0(sp->st_blksize));
	while ((sz = read(fd, buf, bufsize)) > 0)
		bwrite(1, buf, sz);
	free(buf);
	return 0;
}

static int
endtail(struct count *cnt, int fd, struct stat *sp)
{
	char	*buf = NULL, *map = MAP_FAILED;
	size_t	bufsize;
	long long	n, mapsize = 0;
	long	i, j, k, lastnl;
	int	partial, incomplete = 0;
	char	*spc;
	size_t	spcsize;

	if (cnt->c_off <= 0 && (!fflag || !newopt))
		return 0;
	n = (cnt->c_typ == 'b' ? (cnt->c_off << 9) : cnt->c_off);
	if (cnt->c_typ != 'l' &&
			(S_ISREG(sp->st_mode) || S_ISBLK(sp->st_mode))) {
		if (sp->st_size <= n || lseek(fd, -n, SEEK_END) != (off_t)-1)
			return copy(fd, sp);
	}
	if (fflag == 0 && S_ISREG(sp->st_mode) && sp->st_size > 0) {
#ifdef	RLIMIT_AS
		struct rlimit	rs;
#endif
		off_t	mapoffs;
		long	mapmax, pagesize;

		/*
		 * Start trying to mmap() at RLIMIT_AS and decrease
		 * in page size units. This takes typically a few
		 * hundred loops in practice if the file exceeds
		 * the limit.
		 */
		if ((pagesize = sysconf(_SC_PAGESIZE)) <= 0)
			pagesize = 512;
#ifdef	RLIMIT_AS
#ifndef	RLIM_SAVED_MAX
#define	RLIM_SAVED_MAX	RLIM_INFINITY
#endif
#ifndef	RLIM_SAVED_CUR
#define	RLIM_SAVED_CUR	RLIM_INFINITY
#endif
		if (getrlimit(RLIMIT_AS, &rs) == 0 && rs.rlim_cur < LONG_MAX &&
				rs.rlim_cur != RLIM_INFINITY &&
				rs.rlim_cur != RLIM_SAVED_MAX &&
				rs.rlim_cur != RLIM_SAVED_CUR)
			mapmax = rs.rlim_cur;
		else
#endif	/* RLIMIT_AS */
			mapmax = LONG_MAX;
		mapoffs = 0;
		if ((mapsize = sp->st_size) > mapmax) {
			mapoffs = mapsize - mapmax;
			mapoffs -= mapoffs % pagesize;
			mapsize -= mapoffs;
		}
		while ((map = mmap(NULL, mapsize, PROT_READ, MAP_SHARED,
				fd, mapoffs)) == MAP_FAILED) {
			mapsize -= pagesize;
			mapoffs += pagesize;
			if (errno != ENOMEM || mapsize < 0)
				goto rdin;
		}
		i = mapsize;
		spc = map;
		spcsize = i + 1;
		partial = 1;
		lseek(fd, sp->st_size, SEEK_SET);
	} else {
		long	alcmax;
		int	chunk;
rdin:
		chunk = 8192;
		partial = 1;
		bufsize = LINE_MAX * 10 + 1;
		buf = srealloc(NULL, bufsize);
		/*
		 * Limit the maximum amount of allocated memory to avoid
		 * huge wastes. A LINE_MAX limitation seems somewhat small
		 * as a fixed limit in real life. For -b|c, the limit can
		 * be based upon the demanded count.
		 */
		if (cnt->c_typ == 'l') {
			if (n / 65536 < LONG_MAX - chunk)
				alcmax = 65536 * n + 1;
			else
				alcmax = LONG_MAX - chunk;
		} else if (n < LONG_MAX - chunk)
			alcmax = n + 1;
		else
			alcmax = LONG_MAX - chunk;
		if (S_ISREG(sp->st_mode) || S_ISBLK(sp->st_mode))
			lseek(fd, -alcmax, SEEK_END);
		for (;;) {
			i = 0;
			do {
				j = read(fd, &buf[i], bufsize - i);
				if (j <= 0)
					goto brka;
				i += j;
				if (i == bufsize && i < alcmax) {
					char	*nbuf;

					nbuf = realloc(buf, bufsize + chunk);
					if (nbuf != NULL) {
						buf = nbuf;
						bufsize += chunk;
					}
				}
			} while (i < bufsize);
			partial = 0;
		}
brka:
		spc = buf;
		spcsize = bufsize;
	}
	if (cnt->c_typ != 'l') {
		k = n <= i ? i - n :
			partial ? 0 : 
			n >= spcsize ? i + 1 :
			i - n + spcsize;
		k--;
	} else {
		if (rflag && spc[i == 0 ? spcsize - 1 : i - 1] != '\n')
			incomplete = 1;
		k = i;
		j = 0;
		do {
			lastnl = k;
			do {
				if (--k < 0) {
					if (partial) {
						if (rflag) {
							bwrite(1, spc,
								lastnl + 1 -
								incomplete);
							incomplete = 0;
						}
						goto brkb;
					}
					k = spcsize - 1;
				}
			} while (spc[k] != '\n' && k != i);
			if (rflag && (j > 0 || incomplete)) {
				if (k < lastnl) {
					bwrite(1, &spc[k + 1], lastnl - k -
							incomplete);
				} else {
					bwrite(1, &spc[k + 1], spcsize - k - 1);
					bwrite(1, spc, lastnl + 1 - incomplete);
				}
				incomplete = 0;
			}
		} while (j++ < n && k != i);
brkb:
		if (rflag)
			goto out;
		if (k == i) {
			do {
				if (++k >= spcsize)
					k = 0;
			} while (spc[k] != '\n' && k != i);
		}
	}
	if (k < i) {
		bwrite(1, &spc[k + 1], i - k - 1);
	} else {
		bwrite(1, &spc[k + 1], spcsize - k - 1);
		bwrite(1, spc, i);
	}
out:
	if (buf)
		free(buf);
	if (map != MAP_FAILED)
		munmap(map, mapsize);
	return 0;
}

static int
begintail(struct count *cnt, int fd, struct stat *sp)
{
	char	*buf;
	size_t	bufsize;
	char	*p = NULL;
	int	sz;
	long long	n;

	buf = srealloc(NULL, bufsize = check0(sp->st_blksize));
	if (cnt->c_off > 0) {
		if (cnt->c_typ == 'l') {
			n = cnt->c_off;
			sz = 0;
			while (n-- > 0) {
				do {
					if (sz-- <= 0) {
						p = buf;
						sz = read(fd, p, bufsize);
						if (sz-- <= 0)
							return 0;
					}
				} while (*p++ != '\n');
			}
			bwrite(1, p, sz);
		} else {
			n = (cnt->c_typ == 'c' ? cnt->c_off : (cnt->c_off<<9));
			if ((!S_ISREG(sp->st_mode) && !S_ISBLK(sp->st_mode)) ||
					lseek(fd, n, SEEK_SET) == (off_t)-1) {
				while (n > 0) {
					sz = n > bufsize ? bufsize : (size_t)n;
					sz = read(fd, buf, sz);
					if (sz <= 0)
						return 0;
					n -= sz;
				}
			}
		}
	}
	return copy(fd, sp);
}

static struct count *
getcount(const char *arg, int type)
{
	static struct count	cnt;
	long long	off;
	char	*x;

	cnt.c_frm = FR_END;
	if (arg[0] == '+') {
		cnt.c_frm = FR_BEGIN;
		arg++;
	} else if (arg[0] == '-') {
		if (arg[1] == '-' && arg[2] == '\0')
			return NULL;
		arg++;
	}
	else if (type == '\0')
		return NULL;
	if (arg[0] == '+' || arg[0] == '-')
		return NULL;
	cnt.c_off = off = strtoull(arg, &x, 10);
	if (cnt.c_frm == FR_BEGIN && cnt.c_off > 0)
		cnt.c_off--;
	if (type == '\0') {
		cnt.c_typ = 'l';
		if (x == arg) {
			if (cnt.c_frm != FR_BEGIN && (*x == 'f' ||
						*x == 'c' && (x[1] == '\0' ||
							isdigit(x[1] & 0377))))
				return NULL;
			cnt.c_off = cnt.c_frm == FR_BEGIN ? 9 : 10;
		}
		while (*x != '\0') {
			switch (*x) {
			case 'l':
			case 'b':
			case 'c':
				cnt.c_typ = *x;
				break;
			case 'f':
				fflag = 1;
				break;
			case 'r':
				if (x == arg)
#ifndef	LLONG_MAX
#define	LLONG_MAX	9223372036854775807LL
#endif	/* GCC problem */
					cnt.c_off = LLONG_MAX;
				else if (cnt.c_frm == FR_BEGIN)
					cnt.c_off = off + 1;
				rflag = 1;
				break;
			default:
				return NULL;
			}
			x++;
		}
		if (rflag && (cnt.c_typ != 'l' || fflag))
			usage();
	} else {
		if (*x != '\0')
			return NULL;
		cnt.c_typ = type;
	}
	return &cnt;
}

int
main(int argc, char **argv)
{
	struct stat	st;
	struct count	*cnt = NULL;
	const char	optstring[] = ":c:fn:";
	int	i, fd;

#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	progname = basename(argv[0]);
	if (argc > 1 && (cnt = getcount(argv[1], '\0')) != NULL) {
		optind = 2;
		if (argc > 2 && argv[2][0] == '-' && argv[2][1] == '-' &&
				argv[2][2] == '\0')
			optind++;
		goto optend;
	}
	newopt = 1;
	while ((i = getopt(argc, argv, optstring)) != EOF) {
		switch (i) {
		case 'c':
		case 'n':
			if ((cnt = getcount(optarg, i == 'c' ?
							i : 'l')) == NULL) {
				fprintf(stderr,
					"%s: invalid argument: -%c %s\n",
					progname, i, optarg);
				usage();
			}
			break;
		case 'f':
			fflag = 1;
			break;
		default:
			usage();
		}
	}
	if (cnt == NULL)
		cnt = &defcount;
optend:
	if (fstat(1, &st) < 0) {
		fprintf(stderr, "%s: cannot stat stdout\n", progname);
		exit(8);
	}
	outblk = check0(st.st_blksize);
	if (optind < argc) {
		if ((fd = open(argv[optind], O_RDONLY)) < 0) {
			fprintf(stderr, "%s: cannot open input\n", progname);
			exit(1);
		}
	} else {
		if (fflag && lseek(0, 0, SEEK_CUR) == (off_t)-1 &&
				errno == ESPIPE)
			fflag = 0;
		fd = 0;
	}
	if (fstat(fd, &st) < 0) {
		fprintf(stderr, "%s: cannot stat input\n", progname);
		exit(4);
	}
	if (cnt->c_frm == FR_BEGIN && rflag == 0)
		errcnt |= begintail(cnt, fd, &st);
	else
		errcnt |= endtail(cnt, fd, &st);
	if (fflag)
		errcnt |= tailf(fd, &st);
	return errcnt;
}
