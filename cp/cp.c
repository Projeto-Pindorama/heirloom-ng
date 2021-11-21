/*
 * cp - copy files
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, July 2002.
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
#if defined (SUS)
static const char sccsid[] USED = "@(#)cp_sus.sl	1.84 (gritter) 3/4/06";
#elif defined (S42)
static const char sccsid[] USED = "@(#)cp_s42.sl	1.84 (gritter) 3/4/06";
#else
static const char sccsid[] USED = "@(#)cp.sl	1.84 (gritter) 3/4/06";
#endif

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/socket.h>
#include	<sys/un.h>
#include	<sys/time.h>
#include	<sys/resource.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<malloc.h>
#include	<errno.h>
#include	<libgen.h>
#include	<limits.h>
#include	<dirent.h>
#include	<utime.h>
#include	"sfile.h"
#include	"memalign.h"
#include	"alloca.h"

#ifndef	S_IFDOOR
#define	S_IFDOOR	0xD000		/* Solaris door */
#endif
#ifndef	S_IFNAM
#define	S_IFNAM		0x5000		/* XENIX special named file */
#endif
#ifndef	S_IFNWK
#define	S_IFNWK		0x9000		/* HP-UX network special file */
#endif

static enum {
	PERS_CP,
	PERS_MV,
	PERS_LN
} pers;

enum okay {
	OKAY = 0,
	STOP = 1
};

struct	islot {
	struct islot	*i_lln;
	struct islot	*i_rln;
	char		*i_name;
	ino_t		i_ino;
};

struct	dslot {
	struct dslot	*d_nxt;
	struct islot	*d_isl;
	dev_t		d_dev;
};

static struct dslot	*d0;

static unsigned	errcnt;			/* count of errors */
static long	bflag;			/* buffer size */
static int	dflag;			/* preserve hard links */
#ifdef	O_DIRECT
static int	Dflag;			/* use direct i/o */
#endif	/* O_DIRECT */
static int	fflag;			/* force */
static int	iflag;			/* ask before overwriting */
static int	nflag;			/* ln: do not remove links */
static int	pflag;			/* preserve owner and times */
static int	rflag;			/* recursive, read FIFOs */
static int	Rflag;			/* recursive, recreate FIFOs */
static int	sflag;			/* make symlinks / show statistics */
static int	HLPflag;		/* -H, -L, or -P */
static int	ontty;			/* stdin is a terminal */
static mode_t	umsk;			/* current umask */
static uid_t	myuid;			/* current uid */
static gid_t	mygid;			/* current gid */
static char	*progname;		/* argv[0] to main() */
static struct islot	*inull;		/* inode tree null element */
static void	(*go)(const char *, const char *, struct stat *, int,
			int (*statfn)(const char *, struct stat *));

static mode_t
check_suid(const struct stat *sp, mode_t mode)
{
	if (sp->st_uid != myuid || sp->st_gid != mygid) {
		mode &= ~(mode_t)S_ISUID;
		if ((sp->st_mode&S_IFMT) != S_IFDIR || sp->st_mode&0010)
			mode &= ~(mode_t)S_ISGID;
		if ((sp->st_mode&S_IFMT) == S_IFDIR || sp->st_gid != mygid)
			mode &= ~(mode_t)S_ISGID;
	}
	return mode;
}

static void
nomem(void)
{
	write(2, progname, strlen(progname));
	write(2, ": Insufficient memory space.\n", 29);
	_exit(077);
}

static void *
srealloc(void *vp, size_t nbytes)
{
	void *p;

	if ((p = realloc(vp, nbytes)) == NULL)
		nomem();
	return p;
}

static void *
smalloc(size_t nbytes)
{
	return srealloc(NULL, nbytes);
}

static void *
scalloc(size_t nelem, size_t nbytes)
{
	void *p;

	if ((p = calloc(nelem, nbytes)) == NULL)
		nomem();
	return p;
}

static void
usage(void)
{
	switch (pers) {
	case PERS_CP:
		fprintf(stderr, "\
Usage: %s [-i] [-p] f1 f2\n\
       %s [-i] [-p] f1 ... fn d1\n\
       %s [-i] [-p] [-r] d1 d2\n",
       			progname, progname, progname);
		break;
	case PERS_MV:
		fprintf(stderr, "\
Usage: %s [-f] [-i] f1 f2\n\
       %s [-f] [-i] f1 ... fn d1\n\
       %s [-f] [-i] d1 d2\n",
       			progname, progname, progname);
		break;
	case PERS_LN:
		{
#if defined (SUS)
			const char nstr[] = "";
#else	/* !SUS */
			const char nstr[] = "[-n] ";
#endif	/* !SUS */
			fprintf(stderr, "\
Usage: %s [-f] %s[-s] f1 f2\n\
       %s [-f] %s[-s] f1 ... fn d1\n\
       %s [-f] %s[-s] d1 d2\n",
       				progname, nstr, progname, nstr,
				progname, nstr);
		}
		break;
	}
	exit(2);
}

static void
freeislots(struct islot *ip)
{
	if (ip == inull)
		return;
	freeislots(ip->i_lln);
	freeislots(ip->i_rln);
	free(ip->i_name);
	free(ip);
}

static void
freedslots(void)
{
	struct dslot *dp, *dn;
	
	for (dp = d0; dp; dp = dn) {
		dn = dp->d_nxt;
		freeislots(dp->d_isl);
		free(dp);
	}
	d0 = NULL;
}

static struct islot *
isplay(ino_t ino, struct islot *x)
{
	struct islot	hdr;
	struct islot	*leftmax, *rightmin;
	struct islot	*y;

	hdr.i_lln = hdr.i_rln = inull;
	leftmax = rightmin = &hdr;
	inull->i_ino = ino;
	while (ino != x->i_ino) {
		if (ino < x->i_ino) {
			if (ino < x->i_lln->i_ino) {
				y = x->i_lln;
				x->i_lln = y->i_rln;
				y->i_rln = x;
				x = y;
			}
			if (x->i_lln == inull)
				break;
			rightmin->i_lln = x;
			rightmin = x;
			x = x->i_lln;
		} else {
			if (ino > x->i_rln->i_ino) {
				y = x->i_rln;
				x->i_rln = y->i_lln;
				y->i_lln = x;
				x = y;
			}
			if (x->i_rln == inull)
				break;
			leftmax->i_rln = x;
			leftmax = x;
			x = x->i_rln;
		}
	}
	leftmax->i_rln = x->i_lln;
	rightmin->i_lln = x->i_rln;
	x->i_lln = hdr.i_rln;
	x->i_rln = hdr.i_lln;
	inull->i_ino = !ino;
	return x;
}

static struct islot *
ifind(ino_t ino, struct islot **it)
{
	if (*it == NULL)
		return NULL;
	*it = isplay(ino, *it);
	return (*it)->i_ino == ino ? *it : NULL;
}

static void
iput(struct islot *ik, struct islot **it)
{
	if ((*it) == NULL) {
		ik->i_lln = ik->i_rln = inull;
		(*it) = ik;
	} else {
		/*(*it) = isplay(ik->i_ino, (*it));*/
		/* ifind() is always called before */
		if (ik->i_ino < (*it)->i_ino) {
			ik->i_lln = (*it)->i_lln;
			ik->i_rln = (*it);
			(*it)->i_lln = inull;
			(*it) = ik;
		} else if ((*it)->i_ino < ik->i_ino) {
			ik->i_rln = (*it)->i_rln;
			ik->i_lln = (*it);
			(*it)->i_rln = inull;
			(*it) = ik;
		}
	}
}
static int
canlink(const char *path, const struct stat *sp)
{
	struct dslot *ds, *dp;
	struct islot *ip;

	for (ds = d0, dp = NULL; ds; dp = ds, ds = ds->d_nxt)
		if (ds->d_dev == sp->st_dev)
			break;
	if (ds == NULL) {
		ds = scalloc(1, sizeof *ds);
		ds->d_dev = sp->st_dev;
		if (d0 == NULL)
			d0 = ds;
		else
			dp->d_nxt = ds;
	}
	if ((ip = ifind(sp->st_ino, &ds->d_isl)) == NULL) {
		ip = scalloc(1, sizeof *ip);
		ip->i_name = smalloc(strlen(path) + 1);
		strcpy(ip->i_name, path);
		ip->i_ino = sp->st_ino;
		iput(ip, &ds->d_isl);
	} else {
		if (link(ip->i_name, path) == 0)
			return 1;
	}
	return 0;
}

static enum okay
confirm(void)
{
	enum okay yes = STOP;
	char c;

	if (read(0, &c, 1) == 1) {
		yes = (c == 'y' || c == 'Y') ? OKAY : STOP;
		while (c != '\n' && read(0, &c, 1) == 1);
	}
	return yes;
}

static void
permissions(const char *path, const struct stat *ssp)
{
	mode_t mode;

	mode = ssp->st_mode & 07777;
	if (pflag) {
		struct utimbuf ut;
		ut.actime = ssp->st_atime;
		ut.modtime = ssp->st_mtime;
		if (utime(path, &ut) < 0) {
#if defined (SUS) || defined (S42)
			fprintf(stderr, "%s: cannot set times for %s\n%s: %s\n",
					progname, path,
					progname, strerror(errno));
#endif /* SUS || S42 */
			if (pers != PERS_MV)
				errcnt |= 010;
		}
		if (myuid == 0) {
			if (chown(path, ssp->st_uid, ssp->st_gid) < 0) {
#if defined (SUS) || defined (S42)
				fprintf(stderr,
			"%s: cannot change owner and group of %s\n%s: %s\n",
					progname, path,
					progname, strerror(errno));
#endif	/* SUS || S42 */
				if (pers != PERS_MV)
					errcnt |= 010;
				mode &= ~(mode_t)(S_ISUID|S_ISGID);
			}
		} else
			mode = check_suid(ssp, mode);
	} else
		mode = check_suid(ssp, mode & ~umsk);
	if (chmod(path, mode) < 0) {
#if defined (SUS) || defined (S42)
		fprintf(stderr, "%s: cannot set permissions for %s\n%s: %s\n",
				progname, path,
				progname, strerror(errno));
#endif	/* SUS || S42 */
		if (pers != PERS_MV)
			errcnt |= 010;
	}
}

static size_t
balign(const struct stat *ssp, const struct stat *dsp,
		long long size, size_t prefd)
{
	int	n, m;
	size_t	s;

	n = (ssp->st_mode&S_IFMT) == S_IFREG && ssp->st_blksize >= 0 ?
		ssp->st_blksize : 512;
	m = (dsp->st_mode&S_IFMT) == S_IFREG && dsp->st_blksize >= 0 ?
		dsp->st_blksize : 512;
	if (prefd <= size && prefd % n == 0 && prefd % m == 0)
		return prefd;
	else if (n % m == 0)
		return n;
	else if (m % n == 0)
		return m;
	else {
		s = n;
		while (s % m)
			s *= 2;
		return s;
	}
}

/*ARGSUSED*/
void
writerr(void *vp, int count, int written)
{
}

static long long
fdcopy(const char *src, const struct stat *ssp, const int sfd,
		const char *tgt, const struct stat *dsp, const int dfd)
{
	static long pagesize;
	static char *buf = NULL;
	static size_t bufsize;
	ssize_t rsz, wo, wt;
	size_t	blksize;
	long long	copied = 0;
#ifdef	O_DIRECT
	int	sfl = 0, dfl = 0, haverest = 0, dioen = 0;
	off_t	remsz = 0;
#endif

#ifdef	__linux__
	if (!bflag && !Dflag && ssp->st_size > 0) {
		long long	sent;

		if ((sent = sfile(dfd, sfd, ssp->st_mode, ssp->st_size)) ==
				ssp->st_size)
			return sent;
		if (sent < 0)
			goto err;
	}
#endif	/* __linux__ */
	if (pagesize == 0)
		if ((pagesize = sysconf(_SC_PAGESIZE)) < 0)
			pagesize = 4096;
	if (bflag)
		blksize = bflag;
#ifdef	O_DIRECT
	else if (Dflag)
		blksize = balign(ssp, dsp, ssp->st_size, 1048576);
#endif	/* O_DIRECT */
	else
		blksize = balign(ssp, dsp, ssp->st_size, 4096);
	if (blksize > bufsize) {
		if (buf)
			free(buf);
		if ((buf = memalign(pagesize, bufsize = blksize)) == 0)
			nomem();
	}
#ifdef	O_DIRECT
	if (Dflag) {
		if ((ssp->st_mode&S_IFMT) == S_IFREG &&
					ssp->st_size > blksize ||
				(ssp->st_mode&S_IFMT) == S_IFBLK) {
			sfl = fcntl(sfd, F_GETFL);
			fcntl(sfd, F_SETFL, sfl | O_DIRECT);
			remsz = ssp->st_size;
		}
		if ((dsp->st_mode&S_IFMT) == S_IFREG ||
				(dsp->st_mode&S_IFMT) == S_IFBLK) {
			dfl = fcntl(dfd, F_GETFL);
			fcntl(dfd, F_SETFL, dfl | O_DIRECT);
			dioen = 1;
		}
	}
#endif	/* O_DIRECT */
	while ((rsz = read(sfd, buf, blksize)) > 0) {
#ifdef	O_DIRECT
		if (Dflag && rsz < blksize && dioen != 0) {
			fcntl(dfd, F_SETFL, dfl);
			dioen = 0;
		}
#endif	/* O_DIRECT */
		wt = 0;
		do {
			if ((wo = write(dfd, buf + wt, rsz - wt)) < 0) {
#ifdef	__linux__
			err:
#endif	/* __linux__ */
				fprintf(stderr, "%s: %s: write: %s\n",
						progname, tgt,
						strerror(errno));
				errcnt |= 04;
#ifdef	notdef
				if ((dsp->st_mode&S_IFMT) == S_IFREG)
					unlink(tgt);
#endif	/* notdef */
				return copied;
			}
			wt += wo;
			copied += wo;
		} while (wt < rsz);
#ifdef	O_DIRECT
		if (Dflag && ssp->st_size > blksize &&
				(ssp->st_mode&S_IFMT) == S_IFREG) {
			remsz -= rsz;
			if (remsz > 0 && remsz < blksize &&
					haverest == 0 && (bflag ||
					 remsz<(blksize=balign(ssp, dsp,
							ssp->st_size, 4096)))) {
				fcntl(sfd, F_SETFL, sfl);
				haverest = 1;
			}
		}
#endif	/* O_DIRECT */
	}
	if (rsz < 0) {
		fprintf(stderr, "%s: %s: read: %s\n",
				progname, src, strerror(errno));
		errcnt |= 04;
#ifdef	notdef
		if ((dsp->st_mode&S_IFMT) == S_IFREG)
			unlink(tgt);
#endif	/* notdef */
	}
#ifdef	O_DIRECT
	if (haverest) {
#if !defined (__FreeBSD__) && !defined (__DragonFly__) && !defined (__APPLE__)
		fdatasync(dfd);
#else	/* __FreeBSD__, __DragonFly__, __APPLE__ */
		fsync(dfd);
#endif	/* __FreeBSD_, __DragonFly__, __APPLE__ */
	}
#endif	/* O_DIRECT */
	return copied;
}

static void
filecopy(const char *src, const struct stat *ssp,
		const char *tgt, const struct stat *dsp)
{
	struct stat stbuf;
	mode_t mode;
	int sfd, dfd;
	float	f, s, t;
	struct timeval	tv1, tv2;
	struct rusage	ru1, ru2;
	long long	copied = 0;

	if (sflag) {
		gettimeofday(&tv1, NULL);
		getrusage(RUSAGE_SELF, &ru1);
	}
	if ((sfd = open(src, O_RDONLY)) < 0) {
		fprintf(stderr, "%s: cannot open %s\n%s: %s\n",
				progname, src,
				src, strerror(errno));
		errcnt |= 01;
		return;
	}
	mode = check_suid(ssp, ssp->st_mode & 07777);
	if ((dfd = creat(tgt, mode)) < 0)
		if (pers != PERS_MV && dsp != NULL && fflag && unlink(tgt) == 0)
			dfd = creat(tgt, mode);
	if (dfd < 0) {
		fprintf(stderr, "%s: cannot create %s\n%s: %s\n",
				progname, tgt,
				progname, strerror(errno));
		errcnt |= 01;
		goto end1;
	}
	if (fstat(dfd, &stbuf) < 0) {
		fprintf(stderr, "%s: fstat for %s failed: %s\n",
				progname, tgt, strerror(errno));
		errcnt |= 04;
		goto end2;
	}
	copied = fdcopy(src, ssp, sfd, tgt, &stbuf, dfd);
end2:
	if (pflag)
		permissions(tgt, ssp);
	if (close(dfd) < 0) {
		fprintf(stderr, "%s: close error on %s: %s\n",
				progname, tgt, strerror(errno));
		errcnt |= 04;
	}
	if (sflag) {
		gettimeofday(&tv2, NULL);
		getrusage(RUSAGE_SELF, &ru2);
#define	tv2f(tv)	((tv).tv_sec + (float)(tv).tv_usec / 1000000)
		f = tv2f(tv2) - tv2f(tv1);
		s = (float)copied / (2<<19);
		t = f ? s / f : s;
		printf("                 ****** %s File Information ******\n"
		       "        Input file              :       %s\n"
		       "        Output file             :       %s\n"
		       "        Real Time (secs)        : %14.6f\n"
		       "        User Time (secs)        : %14.6f\n"
		       "        System Time (secs)      : %14.6f\n"
		       "        File Size (MB)          : %14.6f\n"
		       "        Transfer Rate (MB/s)    : %14.6f\n",
		       progname, src, tgt,
		       f,
		       tv2f(ru2.ru_utime) - tv2f(ru1.ru_utime),
		       tv2f(ru2.ru_stime) - tv2f(ru1.ru_stime),
		       s,
		       t);
	}
end1:
	close(sfd);
}

static void
ignoring(const char *type, const char *path)
{
	fprintf(stderr, "%s: %signoring %s %s\n", progname,
#if defined (SUS)
			"",
#else	/* !SUS */
			"warning: ",
#endif	/* !SUS */
			type, path);
#if defined (SUS)
	if (pers == PERS_MV)
		errcnt |= 020;
#endif	/* SUS */
}

static enum okay
do_unlink(const char *tgt, const struct stat *dsp)
{
	if (dsp && unlink(tgt) < 0) {
		fprintf(stderr, "%s: cannot unlink %s\n%s: %s\n",
				progname, tgt,
				progname, strerror(errno));
		errcnt |= 01;
		return STOP;
	}
	return OKAY;
}

static void
devicecopy(const char *src, const struct stat *ssp,
		const char *tgt, const struct stat *dsp)
{
	if (do_unlink(tgt, dsp) != OKAY)
		return;
	if (mknod(tgt, check_suid(ssp, ssp->st_mode & (07777|S_IFMT)),
				ssp->st_rdev) < 0) {
		fprintf(stderr, "%s: cannot create special file %s\n%s: %s\n",
				progname, tgt,
				progname, strerror(errno));
		errcnt |= 01;
		return;
	}
	if (pflag)
		permissions(tgt, ssp);
}

static void
symlinkcopy(const char *src, const struct stat *ssp,
		const char *tgt, const struct stat *dsp)
{
	static char *buf;
	static size_t bufsize;
	ssize_t sz;

	if (buf == NULL)
		buf = smalloc(bufsize = 256);
	for (;;) {
		sz = readlink(src, buf, bufsize - 1);
		if (sz < 0) {
			fprintf(stderr,
				"%s: cannot read symbolic link %s\n%s: %s\n",
					progname, src,
					progname, strerror(errno));
			errcnt |= 01;
			return;
		}
		if (sz == bufsize - 1) {
			buf = srealloc(buf, bufsize += 256);
			continue;
		}
		buf[sz] = '\0';
		break;
	}
	if (do_unlink(tgt, dsp) != OKAY)
		return;
	if (symlink(buf, tgt) < 0) {
		fprintf(stderr, "%s: cannot create symbolic link %s\n%s: %s\n",
				progname, tgt,
				progname, strerror(errno));
		errcnt |= 01;
		return;
	}
	if (myuid == 0 && lchown(tgt, ssp->st_uid, ssp->st_gid) < 0) {
#if defined (SUS)
		fprintf(stderr,
			"%s: cannot change owner and group of %s\n%s: %s\n",
				progname, tgt,
				progname, strerror(errno));
#endif	/* SUS */
		if (pers != PERS_MV)
			errcnt |= 010;
	}
}

static void
socketcopy(const char *src, const struct stat *ssp,
		const char *tgt, const struct stat *dsp)
{
	int	fd, addrsz;
	struct sockaddr_un	addr;
	size_t	len;

	if (do_unlink(tgt, dsp) != OKAY)
		return;
	len = strlen(tgt);
	memset(&addr, 0, sizeof addr);
	addr.sun_family = AF_UNIX;
	addrsz = sizeof addr - sizeof addr.sun_path + len;
	if ((len >= sizeof addr.sun_path ? errno = ENAMETOOLONG, fd = -1, 1 :
			(strncpy(addr.sun_path,tgt,sizeof addr.sun_path), 0)) ||
			(fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0 ||
			bind(fd, (struct sockaddr *)&addr, addrsz) < 0) {
		fprintf(stderr, "%s: cannot create socket %s\n%s: %s\n",
				progname, tgt,
				progname, strerror(errno));
		if (fd >= 0)
			close(fd);
		errcnt |= 01;
		return;
	}
	close(fd);
	if (pflag)
		permissions(tgt, ssp);
}

static void
specialcopy(const char *src, const struct stat *ssp,
		const char *tgt, const struct stat *dsp)
{
	switch (ssp->st_mode & S_IFMT) {
	case S_IFIFO:
	case S_IFCHR:
	case S_IFBLK:
	case S_IFNAM:
	case S_IFNWK:
		devicecopy(src, ssp, tgt, dsp);
		break;
	case S_IFLNK:
		symlinkcopy(src, ssp, tgt, dsp);
		break;
	case S_IFSOCK:
		socketcopy(src, ssp, tgt, dsp);
		break;
	case S_IFDOOR:
		ignoring("door", src);
		break;
	default:
		fprintf(stderr, "%s: %s: unknown file type %o\n",
				progname, src, (int)ssp->st_mode);
		if (pers == PERS_MV)
			errcnt |= 020;
	}
}

static void
getpath(const char *path, char **file, char **filend, size_t *sz, size_t *slen)
{
	*sz = 14 + strlen(path) + 2;
	*file = smalloc(*sz);
	*filend = *file;
	if (path[0] == '/' && path[1] == '\0')
		*(*filend)++ = '/';
	else {
		register const char *cp = path;
		while ((*(*filend)++ = *cp++) != '\0');
		(*filend)[-1] = '/';
	}
	*slen = *filend - *file;
}

static void
setpath(const char *base, char **file, char **filend,
		size_t slen, size_t *sz, size_t *ss)
{
	if (slen + (*ss = strlen(base)) >= *sz) {
		*sz += slen + *ss + 15;
		*file = srealloc(*file, *sz);
		*filend = &(*file)[slen];
	}
	strcpy(*filend, base);
}

static enum okay
trydelete(const char *path, int recursive)
{
	struct stat st;
	enum okay val = OKAY;

	if (lstat(path, &st) < 0) {
		fprintf(stderr, "%s: cannot stat %s for removal\n%s: %s\n",
				progname, path,
				progname, strerror(errno));
		errcnt |= 040;
		val = STOP;
	} else if ((st.st_mode & S_IFMT) == S_IFDIR) {
		DIR *Dp;

		if (recursive == 0)
			goto do_rmdir;
		if ((Dp = opendir(path)) != NULL) {
			struct dirent *dp;
			char *copy, *cend;
			size_t sz, slen, ss;

			getpath(path, &copy, &cend, &sz, &slen);
			while ((dp = readdir(Dp)) != NULL) {
				if (dp->d_name[0] == '.' &&
						(dp->d_name[1] == '\0' ||
					 	(dp->d_name[1] == '.' &&
					 	dp->d_name[2] == '\0')))
					continue;
				setpath(dp->d_name, &copy, &cend,
						slen, &sz, &ss);
				if ((val = trydelete(copy, recursive)) == STOP)
					break;
			}
			free(copy);
			closedir(Dp);
			if (val != STOP) {
do_rmdir:
				if (rmdir(path) < 0) {
					fprintf(stderr,
				"%s: cannot remove directory %s\n%s: %s\n",
					progname, path,
					progname, strerror(errno));
					val = STOP;
				}
			}
		} else {
			fprintf(stderr,
			"%s: cannot open directory %s for removal\n%s: %s\n",
					progname, path,
					progname, strerror(errno));
			errcnt |= 040;
			val = STOP;
		}
	} else {
		if (unlink(path) < 0) {
			fprintf(stderr, "%s: cannot unlink %s\n%s: %s\n",
					progname, path,
					progname, strerror(errno));
			errcnt |= 040;
			val = STOP;
		}
	}
	return val;
}

static enum okay
tryrename(const char *src, const struct stat *ssp,
		const char *tgt, const struct stat *dsp)
{
	if (dsp && !fflag) {
		if (iflag) {
			fprintf(stderr, "%s: overwrite %s? ",
					progname, tgt);
			if (confirm() != OKAY)
				return STOP;
		} else if (ontty && (dsp->st_mode&S_IFMT) != S_IFLNK &&
				access(tgt, W_OK) < 0) {
			fprintf(stderr, "%s: %s: %o mode? ",
					progname, tgt,
					(int)(dsp->st_mode & 0777));
			if (confirm() != OKAY)
				return STOP;
		}
	}
	if (rename(src, tgt) == 0)
		return STOP;
	if (errno != EXDEV) {
		fprintf(stderr, "%s: cannot rename %s to %s\n%s: %s\n",
				progname, src, tgt,
				progname, strerror(errno));
		errcnt |= 01;
		return STOP;
	}
	if (dsp) {
		if ((dsp->st_mode & S_IFMT) == S_IFDIR &&
				(ssp->st_mode & S_IFMT) != S_IFDIR) {
			fprintf(stderr, "%s: <%s> directory\n",
					progname, tgt);
			errcnt |= 01;
			return STOP;
		}
		if ((dsp->st_mode & S_IFMT) != S_IFDIR &&
				(ssp->st_mode & S_IFMT) == S_IFDIR) {
			fprintf(stderr, "%s: Target must be directory\n",
					progname);
			errcnt |= 01;
			return STOP;
		}
	}
	if (dsp == NULL || trydelete(tgt, 0) == OKAY)
		return OKAY;
	return STOP;
}

static enum okay
commoncheck(const char *src, const char *tgt, const struct stat *dsp,
	struct stat *ssp,
	int (*statfn)(const char *, struct stat *))
{
	if (statfn(src, ssp) < 0) {
		if (pers == PERS_LN && sflag)
			return OKAY;
		fprintf(stderr, "%s: cannot access %s\n", progname, src);
		errcnt |= 01;
		return STOP;
	}
	if (dsp && (ssp->st_dev == dsp->st_dev && ssp->st_ino == dsp->st_ino)) {
		fprintf(stderr, "%s: %s and %s are identical\n",
				progname, src, tgt);
		errcnt |= 01;
		return STOP;
	}
	return OKAY;
}

static void
cpmv(const char *src, const char *tgt, struct stat *dsp, int level,
	int (*statfn)(const char *, struct stat *))
{
	struct stat sst;

	if (commoncheck(src, tgt, dsp, &sst,
			Rflag && level == 0 ?
				pers == PERS_MV || HLPflag == 'P' ?
					lstat : stat :
			statfn) != OKAY)
		return;
	if (pers == PERS_MV && level == 0) {
		if (tryrename(src, &sst, tgt, dsp) == STOP)
			return;
		dsp = NULL;
	}
	if ((sst.st_mode & S_IFMT) == S_IFDIR) {
		DIR *Dp;
		struct dirent *dp;
		char *scp, *send, *dcp, *dend;
		size_t ssz, slen, sss, dsz, dlen, dss;
		int destcreat = 0;

		if (rflag == 0) {
			fprintf(stderr, "%s: <%s> directory\n",
					progname, src);
			errcnt |= 01;
			return;
		}
		if (dsp && (dsp->st_mode & S_IFMT) != S_IFDIR) {
			fprintf(stderr, "%s: %s: Not a directory.\n",
					progname, tgt);
			errcnt |= 01;
			return;
		}
#if !defined (SUS)
		if (pers == PERS_CP && dsp != NULL && iflag) {
			fprintf(stderr, "%s: overwrite %s? ",
					progname, tgt);
			if (confirm() != OKAY)
				return;
		}
#endif	/* !SUS */
		if (dsp == NULL) {
			if (mkdir(tgt, check_suid(&sst,
					sst.st_mode&07777 | S_IRWXU)) < 0) {
				fprintf(stderr, "%s: %s: %s\n",
						progname, tgt, strerror(errno));
				errcnt |= 01;
				return;
			}
			destcreat = 1;
		}
		if ((Dp = opendir(src)) == NULL) {
			fprintf(stderr, "%s: %s: %s\n",
					progname, src,
					strerror(errno));
			errcnt |= 01;
			return;
		}
		getpath(src, &scp, &send, &ssz, &slen);
		getpath(tgt, &dcp, &dend, &dsz, &dlen);
		while ((dp = readdir(Dp)) != NULL) {
			struct stat xst;
			if (dp->d_name[0] == '.' &&
					(dp->d_name[1] == '\0' ||
					 (dp->d_name[1] == '.' &&
					  dp->d_name[2] == '\0')))
				continue;
			setpath(dp->d_name, &scp, &send, slen, &ssz, &sss);
			setpath(dp->d_name, &dcp, &dend, dlen, &dsz, &dss);
			go(scp, dcp, stat(dcp, &xst) < 0 ? NULL : &xst,
					level + 1, statfn);
		}
		free(scp);
		free(dcp);
		if (destcreat)
			permissions(tgt, &sst);
		closedir(Dp);
	} else {
		if (dsp != NULL && iflag) {
			fprintf(stderr, "%s: overwrite %s? ",
					progname, tgt);
			if (confirm() != OKAY)
				return;
		}
		if (dflag && sst.st_nlink > 1) {
			if (canlink(tgt, &sst))
				return;
		}
		if ((sst.st_mode & S_IFMT) == S_IFREG || Rflag == 0)
			filecopy(src, &sst, tgt, dsp);
		else
			specialcopy(src, &sst, tgt, dsp);
	}
	if (pers == PERS_MV && errcnt == 0 && level == 0)
		trydelete(src, 1);
	if ((pers == PERS_CP || pers == PERS_MV) && level == 0 && d0)
		freedslots();
}

/*ARGSUSED3*/
static void
ln(const char *src, const char *tgt, struct stat *dsp, int level,
	int (*statfn)(const char *, struct stat *))
{
	struct stat sst;
	int (*how)(const char *, const char *) = sflag ? symlink : link;

	if (commoncheck(src, tgt, dsp, &sst, statfn) != OKAY)
		return;
	if ((sst.st_mode&S_IFMT) == S_IFDIR && !sflag) {
		fprintf(stderr, "%s: <%s> directory\n", progname, src);
		errcnt |= 01;
		return;
	}
#if (defined (SUS) || defined (S42)) && (defined (__linux__) || defined (__sun))
	if (sflag == 0) {
		char	*rpbuf = alloca(PATH_MAX+1);
		if (realpath(src, rpbuf) == NULL) {
			fprintf(stderr, "%s: cannot access %s\n",
					progname, src);
			errcnt |= 01;
			return;
		}
		src = rpbuf;
	}
#endif	/* (SUS || S42) && (__linux__ || __sun) */
	if (dsp
#if !defined (SUS)
			&& !sflag
#endif	/* !SUS */
			) {
		if (nflag && !fflag) {
			fprintf(stderr, "%s: %s: File exists\n",
					progname, tgt);
			errcnt |= 01;
			return;
		}
		if (!fflag && ontty && (dsp->st_mode&S_IFMT) != S_IFLNK &&
				access(tgt, W_OK) < 0) {
			fprintf(stderr, "%s: %s: %o mode? ",
				progname, tgt, (int)(dsp->st_mode & 0777));
			if (confirm() != OKAY)
				return;
		}
		if (unlink(tgt) < 0) {
			fprintf(stderr, "%s: cannot unlink %s\n%s: %s\n",
					progname, tgt,
					progname, strerror(errno));
			errcnt |= 01;
			return;
		}
	}
	if (how(src, tgt) < 0) {
		if (sflag)
			fprintf(stderr, "%s: cannot create %s\n%s: %s\n",
					progname, tgt,
					progname, strerror(errno));
		else if (errno == EXDEV)
			fprintf(stderr, "%s: different file system\n",
					progname);
		else
			fprintf(stderr, "%s: errno: %d no permission for %s\n",
					progname, errno, tgt);
		errcnt |= 01;
	}
}

static const char *
getfl(void)
{
	const char	*optstring;

	if (progname[0] == 'm' && progname[1] == 'v') {
		pers = PERS_MV;
		optstring = "b:fi";
		dflag = pflag = rflag = Rflag = 1;
		go = cpmv;
	} else if (progname[0] == 'l' && progname[1] == 'n') {
		pers = PERS_LN;
		optstring = "fns";
#if defined (SUS)
		nflag = 1;
#endif	/* SUS */
		go = ln;
	} else {
		pers = PERS_CP;
		optstring = "ab:dDfiHLpPrRs";
		go = cpmv;
	}
	return optstring;
}

int
main(int argc, char **argv)
{
	struct stat dst, ust;
	const char *optstring;
	int (*statfn)(const char *, struct stat *);
	int i, illegal = 0;

#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	progname = basename(argv[0]);
	optstring = getfl();
	while ((i = getopt(argc, argv, optstring)) != EOF) {
		switch (i) {
		case 'b':
			bflag = atol(optarg);
			break;
#ifdef	O_DIRECT
		case 'D':
			Dflag = 1;
			break;
#endif	/* O_DIRECT */
		case 'd':
			dflag = 1;
			break;
		case 'f':
			fflag = 1;
#if defined (SUS)
			if (pers == PERS_MV)
				iflag = 0;
#endif	/* SUS */
			break;
		case 'i':
			iflag = 1;
#if defined (SUS)
			if (pers == PERS_MV)
				fflag = 0;
#endif	/* SUS */
			break;
		case 'n':
			nflag = 1;
			break;
		case 'p':
			pflag = 1;
			break;
		case 'a':
			dflag = pflag = 1;
			/*FALLTHRU*/
		case 'R':
			Rflag = 1;
			/*FALLTHRU*/
		case 'r':
			rflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 'H':
		case 'L':
		case 'P':
			HLPflag = i;
			break;
		default:
			illegal = 1;
		}
	}
	argv += optind, argc -= optind;
	if (argc < 2) {
		fprintf(stderr, "%s: Insufficient arguments (%d)\n",
				progname, argc);
		illegal = 1;
	}
	if (illegal)
		usage();
	umask(umsk = umask(0));
	ontty = isatty(0);
#if defined (SUS)
	/* nothing */
#elif defined (S42)
	if (pers == PERS_MV && !ontty)
		iflag = 0;
#else	/* !SUS, !S42 */
	if (!ontty)
		iflag = 0;
#endif	/* !SUS, !S42 */
	myuid = geteuid();
	mygid = getegid();
	inull = scalloc(1, sizeof *inull);
	inull->i_lln = inull->i_rln = inull;
	statfn = (Rflag && HLPflag != 'L'
#if !defined (SUS) && !defined (S42)
			|| pers == PERS_LN
#endif	/* !SUS && !S42 */
			? lstat : stat);
	if (lstat(argv[argc-1], &dst) == 0) {
		if ((dst.st_mode&S_IFMT) != S_IFLNK ||
				stat(argv[argc-1], &ust) < 0)
			ust = dst;
		if ((ust.st_mode&S_IFMT) == S_IFDIR) {
			char *copy, *cend;
			size_t sz, slen, ss;
			unsigned saverrs = errcnt;

			getpath(argv[argc-1], &copy, &cend, &sz, &slen);
			for (i = 0; i < argc-1; i++) {
				errcnt = 0;
				setpath(basename(argv[i]), &copy, &cend,
						slen, &sz, &ss);
				go(argv[i], copy, statfn(copy, &dst) < 0 ?
						NULL : &dst, 0, statfn);
				saverrs |= errcnt;
			}
			errcnt = saverrs;
		} else if (argc > 2) {
			fprintf(stderr, "%s: Target must be directory\n",
					progname);
			usage();
		} else
			go(argv[0], argv[1], pers == PERS_CP ? &ust : &dst,
					0, statfn);
	} else if (argc > 2) {
		fprintf(stderr, "%s: %s not found\n", progname, argv[argc-1]);
		errcnt |= 01;
	} else
		go(argv[0], argv[1], NULL, 0, statfn);
	return errcnt;
}
