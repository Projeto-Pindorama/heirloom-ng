/*
 * copy - copy groups of files
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, August 2003.
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
static const char sccsid[] USED = "@(#)copy.sl	1.15 (gritter) 5/29/05";

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <dirent.h>
#include <utime.h>
#include <stdarg.h>
#include "sfile.h"
#include "memalign.h"

#ifndef	S_IFDOOR
#define	S_IFDOOR	0xD000
#endif
#ifndef	S_IFNAM
#define	S_IFNAM		0x5000
#endif
#ifndef	S_IFNWK
#define	S_IFNWK		0x9000
#endif

#ifdef	__GLIBC__	/* old glibcs don't know _XOPEN_SOURCE=600L yet */
#ifndef	S_IFSOCK
#ifdef	__S_IFSOCK
#define	S_IFSOCK	__S_IFSOCK
#endif	/* __S_IFSOCK */
#endif	/* !S_IFSOCK */
#endif	/* __GLIBC__ */

enum okay {
	OKAY = 0,
	STOP = 1
};

static unsigned	errcnt;		/* count of errors */
static int	aflag;		/* ask before copy */
static int	lflag;		/* hard link instead of copying */
static int	sflag;		/* soft link instead of copying */
static int	nflag;		/* do not overwrite destination ("new") */
static int	oflag;		/* copy owner and group */
static int	mflag;		/* copy access and modification times */
static int	rflag;		/* copy subdirectories */
static int	adflag;		/* ask before copying subdirectories */
static int	vflag;		/* verbose */
static uid_t	myuid;		/* current uid */
static gid_t	mygid;		/* current gid */
static const char	*progname;	/* argv[0] to main() */
static char	*cwd;	/* working directory */
static int	(*statfn)(const char *, struct stat *) = stat;

static void	copy(const char *, const char *, int);
static enum okay	filecopy(const char *, const struct stat *,
				const char *);
static enum okay	fdcopy(const char *, const struct stat *, int,
			const char *, int);
static enum okay	speccopy(const char *, const struct stat *,
				const char *);
static void	getpath(const char *, char **, char **, size_t *, size_t *);
static void	setpath(const char *, char **, char **, size_t, size_t *,
			size_t *);
static mode_t	check_suid(const struct stat *, mode_t);
static void	attribs(const char *, const struct stat *);
static enum okay	overwrite(const char *, const struct stat *,
				const char *, struct stat *);
static void	complain(const char *, ...);
static void	verbose(const char *, ...);
static enum okay	confirm(const char *, ...);
static void	usage(int);
static void	*srealloc(void *, size_t);
static void	*smalloc(size_t);
static void	nomem(void);
static const char	*basen(const char *);
static const char	*adapt(const char *, const char *);
static int	dots(const char *);

int
main(int argc, char **argv)
{
	int	i, j, argn;
	const char	*dst = NULL;
	char	rp[PATH_MAX+1];

	progname = basen(argv[0]);
	argn = argc;
	myuid = geteuid();
	mygid = getegid();
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			for (j = 1; argv[i][j]; j++) {
				switch (argv[i][j]) {
				case 'a':
					if (argv[i][j+1] != 'd')
						aflag = 1;
					else {
						j++;
						rflag = 1;
					}
					adflag = 1;
					break;
				case 'l':
					lflag = 1;
					break;
				case 's':
					sflag = 1;
					break;
				case 'n':
					nflag = 1;
					break;
				case 'o':
					oflag = 1;
					break;
				case 'm':
					mflag = 1;
					break;
				case 'r':
					rflag = 1;
					break;
				case 'v':
					vflag = 1;
					break;
				case 'h':
					statfn = lstat;
					break;
				default:
					usage(argv[i][j]);
				}
			}
			argv[i] = NULL;
			argn--;
		} else
			dst = argv[i];
	}
	if (argn <= 1 || lflag && sflag)
		usage(0);
	else if (argn == 2)
		dst = ".";
	else {
		if (argn > 3) {
			struct stat	st;

			if (stat(dst, &st) < 0 ||
					(st.st_mode&S_IFMT) != S_IFDIR) {
				complain("destination must be directory");
				return 1;
			}
		}
		argv[i-1] = NULL;
	}
	if (sflag) {
		if (dots(dst) && (dst = realpath(dst, rp)) == NULL) {
			complain("cannot resolve destination");
			return 1;
		}
		if (dst[0] == '/') {
			i = 0;
			do
				cwd = srealloc(cwd, i += 64);
			while (getcwd(cwd, i) == 0 && errno == ERANGE);
			if (cwd == NULL) {
				complain("cannot determine current directory");
				return 1;
			}
		}
	}
	for (i = 1; i < argc; i++) {
		if (argv[i])
			copy(argv[i], dst, 0);
	}
	return errcnt;
}

static void
copy(const char *src, const char *dst, int level)
{
	DIR	*Dp;
	struct dirent	*dp;
	struct stat	st, dt;
	char	*dcpy, *dend, *scpy, *send;
	size_t	dsz, dslen, dss, ssz, sslen, sss;

	if ((level?statfn:stat)(src, &st) < 0) {
		st.st_mode = S_IFREG|0666;
		goto reg;
	}
	switch (st.st_mode&S_IFMT) {
	default:
		complain("%s: funny file type %06o", src,
				(int)st.st_mode&S_IFMT);
		return;
	case S_IFSOCK:
	case S_IFDOOR:
		return;
	case S_IFDIR:
		if (!rflag && level != 0)
			return;
		if (adflag && level > 0) {
			if (confirm("examine directory %s? ", src) != OKAY)
				return;
		} else if (level > 0)
			verbose("examine directory %s", src);
		if ((Dp = opendir(src)) == NULL) {
			complain("could not open directory %s", src);
			return;
		}
		if (level > 0) {
			getpath(dst, &dcpy, &dend, &dsz, &dslen);
			setpath(basen(src), &dcpy, &dend, dslen, &dsz, &dss);
		} else
			dcpy = strdup(dst);
		if (stat(dcpy, &dt) == 0) {
			if ((dt.st_mode&S_IFMT) != S_IFDIR) {
				if (level == 0)
				    complain("destination must be directory");
				else
				    complain("%s is not a directory", dcpy);
				free(dcpy);
				return;
			}
		} else {
			if (mkdir(dcpy, st.st_mode&07000|0777) < 0) {
				complain("could not mkdir %s", dcpy);
				free(dcpy);
				return;
			}
		}
		getpath(src, &scpy, &send, &ssz, &sslen);
		while ((dp = readdir(Dp)) != NULL) {
			if (dp->d_name[0] == '.' &&
					(dp->d_name[1] == '\0' ||
					 (dp->d_name[1] == '.' &&
					  dp->d_name[2] == '\0')))
				continue;
			setpath(dp->d_name, &scpy, &send, sslen, &ssz, &sss);
			copy(scpy, dcpy, level+1);
		}
		closedir(Dp);
		attribs(dcpy, &st);
		chmod(dcpy, st.st_mode&07777);
		free(scpy);
		free(dcpy);
		break;
	case S_IFIFO:
	case S_IFBLK:
	case S_IFCHR:
	case S_IFNAM:
	case S_IFNWK:
	case S_IFREG:
	case S_IFLNK:
	reg:	if (aflag) {
			if (confirm("copy file %s? ") != OKAY)
				return;
		} else
			verbose("copy file %s", src);
		if (stat(dst, &dt) == 0 && (dt.st_mode&S_IFMT) == S_IFDIR) {
			getpath(dst, &dcpy, &dend, &dsz, &dslen);
			setpath(basen(src), &dcpy, &dend, dslen, &dsz, &dss);
		} else
			dcpy = strdup(dst);
		if (overwrite(src, &st, dcpy, &dt) != OKAY)
			return;
		if (lflag && (st.st_mode&S_IFMT) == S_IFREG &&
				(unlink(dcpy), link(src, dcpy)) == 0)
			/*EMPTY*/;
		else if (sflag && (st.st_mode&S_IFMT) == S_IFREG &&
				(unlink(dcpy),
				 symlink(adapt(src, dcpy), dcpy)) == 0)
			/*EMPTY*/;
		else if (((st.st_mode&S_IFMT)==S_IFREG?
				filecopy:speccopy)(src, &st, dcpy) == OKAY)
			attribs(dcpy, &st);
		free(dcpy);
	}
}

/*ARGSUSED*/
void
writerr(void *vp, int count, int written)
{
}

static enum okay
filecopy(const char *src, const struct stat *sp, const char *dst)
{
	int	sfd, dfd;
	mode_t	mode;
	enum okay	ok;

	if ((sfd = open(src, O_RDONLY)) < 0) {
		complain("cannot open %s", src);
		return STOP;
	}
	mode = check_suid(sp, sp->st_mode&07777);
	if ((dfd = creat(dst, mode)) < 0) {
		complain("cannot create %s", dst);
		return STOP;
	}
	ok = fdcopy(src, sp, sfd, dst, dfd);
	close(dfd);
	close(sfd);
	return ok;
}

static enum okay
fdcopy(const char *src, const struct stat *sp, int sfd,
		const char *dst, int dfd)
{
	static long	pagesize;
	static char	*buf = NULL;
	static size_t	bufsize;
	long long	blksize;
	ssize_t	rsz, wo, wt;

#ifdef	__linux__
	if (sp->st_size > 0) {
		long long	sent;

		if ((sent = sfile(dfd, sfd, sp->st_mode, sp->st_size))
				== sp->st_size)
			return OKAY;
		if (sent < 0)
			goto err;
	}
#endif	/* __linux__ */
	if (pagesize == 0 && (pagesize = sysconf(_SC_PAGESIZE)) <= 0)
		pagesize = 4096;
	if ((blksize = sp->st_blksize) <= 0)
		blksize = 512;
	if (bufsize < blksize) {
		free(buf);
		if ((buf = memalign(pagesize, blksize)) == NULL)
			nomem();
	}
	while ((rsz = read(sfd, buf, blksize)) > 0) {
		wt = 0;
		do {
			if ((wo = write(dfd, buf+wt, rsz-wt)) < 0) {
#ifdef	__linux__
			err:
#endif	/* __linux__ */
				complain("write error on %s", dst);
				return STOP;
			}
			wt += wo;
		} while (wt < rsz);
	}
	if (rsz < 0) {
		complain("read error on %s", src);
		return STOP;
	}
	return OKAY;
}

static enum okay
speccopy(const char *src, const struct stat *sp, const char *dst)
{
	if (myuid && (sp->st_mode&S_IFMT) != S_IFIFO &&
			(sp->st_mode&S_IFMT) != S_IFLNK) {
		complain("Only super user can copy special files.");
		return STOP;
	}
	if ((sp->st_mode&S_IFMT) == S_IFLNK) {
		char	*ln;
		ssize_t	sz = sp->st_size ? sp->st_size : PATH_MAX;

		ln = smalloc(sz+1);
		if ((sz = readlink(src, ln, sz)) < 0) {
			complain("cannot read symbolic link %s", src);
			return STOP;
		}
		ln[sz] = '\0';
		if (symlink(ln, dst) < 0)
			goto err;
	} else if (mknod(dst, check_suid(sp, sp->st_mode&(07777|S_IFMT)),
				sp->st_rdev) < 0) {
	err:	complain("Unable to create %s", dst);
		return STOP;
	}
	return OKAY;
}

static enum okay
overwrite(const char *src, const struct stat *sp,
		const char *dst, struct stat *dp)
{
	if (stat(dst, dp) == 0) {
		if (nflag) {
			complain("cannot overwrite %s", dst);
			return STOP;
		}
		if (sp->st_dev == dp->st_dev && sp->st_ino == dp->st_ino)
			return STOP;
	}
	return OKAY;
}

static void
attribs(const char *dst, const struct stat *sp)
{
	if (oflag && ((sp->st_mode&S_IFMT) == S_IFLNK ?
			lchown:chown)(dst, sp->st_uid, sp->st_gid) < 0)
		complain("Unable to chown %s", dst);
	if (mflag && (sp->st_mode&S_IFMT) != S_IFLNK) {
		struct utimbuf	ut;
		ut.actime = sp->st_atime;
		ut.modtime = sp->st_mtime;
		utime(dst, &ut);
	}
}

static void
complain(const char *fmt, ...)
{
	va_list	ap;

	fprintf(stderr, "%s: ", progname);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	putc('\n', stderr);
	errcnt++;
}

static void
verbose(const char *fmt, ...)
{
	va_list	ap;

	if (vflag) {
		va_start(ap, fmt);
		vfprintf(stdout, fmt, ap);
		va_end(ap);
		putc('\n', stdout);
	}
}

static enum okay
confirm(const char *fmt, ...)
{
	va_list	ap;
	enum okay yes = STOP;
	char c;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	if (read(0, &c, 1) == 1) {
		yes = (c == 'y' || c == 'Y') ? OKAY : STOP;
		while (c != '\n' && read(0, &c, 1) == 1);
	}
	return yes;
}

static void
usage(int c)
{
	if (c)
		fprintf(stderr, "Bad option - %c\n", c);
	fprintf(stderr,
"Usage: %s [-n] [-l] [-a[d]] [-m] [-o] [-r] [-v] src ... [dst]\n",
		progname);
	exit(1);
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

static void *
srealloc(void *vp, size_t nbytes)
{
	void	*np;

	if ((np = realloc(vp, nbytes)) == NULL)
		nomem();
	return np;
}

static void *
smalloc(size_t nbytes)
{
	return srealloc(NULL, nbytes);
}

static void
nomem(void)
{
	write(2, "no memory\n", 10);
	_exit(077);
}

/*
 * Basename that does not modify the string pointed to by its argument.
 */
static const char *
basen(const char *s)
{
	const char	*b;

	if (s == NULL)
		return ".";
	b = s;
	do
		if (s[0] == '/' && s[1] && s[1] != '/')
			b = &s[1];
	while (*++s);
	return b;
}

#define	fncat(c)	(*(fp < &fn[sz] ? fp : \
	(ofn = fn, fn = srealloc(fn, sz+=64), fp += fn - ofn)) = (c), fp++)

/*
 * Prepend the necessary number of "../" elements to the target
 * of a symbolic link.
 */
static const char *
adapt(const char *s, const char *n)
{
	static char	*fn;
	static size_t	sz;
	char	*fp, *ofn;
	const char	*cp;

	if (*s == '/')
		return s;
	fp = fn;
	if (*n == '/' && cwd) {
		for (cp = cwd; *cp && *n == *cp; cp++, n++);
		if (*n != '/') {
			if (sz < PATH_MAX+1)
				fn = srealloc(fn, sz=PATH_MAX+1);
			if (realpath(s, fn) == NULL) {
				complain("cannot resolve source path");
				exit(1);
			}
			return fn;
		}
		/*
		 * That's fine - the destination path is located below
		 * our working directory, so we can use relative symbolic
		 * links.
		 */
		while (*n == '/')
			n++;
		while (*n && *n != '/')
			n++;
	}
	while (*n) {
		if (*n == '/') {
			for (cp = "../"; *cp; cp++)
				fncat(*cp);
			while (*n == '/')
				n++;
		} else
			n++;
	}
	for (cp = s; *cp; cp++)
		fncat(*cp);
	fncat('\0');
	return fn;
}

/*
 * Return 1 if the passed string contains a '.' or '..' path name component.
 */
static int
dots(const char *s)
{
	do {
		if (s[0] == '.' && (s[1] == '/' || s[1] == '\0' ||
					s[1] == '.' &&
					(s[2] == '/' || s[2] == '\0')))
			return 1;
		while (*s && *s != '/')
			s++;
		while (*s && *s == '/')
			s++;
	} while (*s);
	return 0;
}
