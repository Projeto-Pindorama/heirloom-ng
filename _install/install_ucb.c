/*
 * install - (BSD style) install files
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, March 2003.
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
static const char sccsid[] USED = "@(#)/usr/ucb/install.sl	1.12 (gritter) 5/29/05";

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>
#include	<limits.h>
#include	<pwd.h>
#include	<grp.h>

enum	okay {
	OKAY = 0,
	STOP = 1
};

static int	mflag;			/* -m option present */
static int	sflag;			/* strip files */
static mode_t	mode = 0755;		/* mode to set */
static int	dflag;			/* create directories */
static int	gflag;			/* set group */
static gid_t	group;			/* group to set */
static int	oflag;			/* set owner */
static uid_t	owner;			/* owner to set */
static int	errcnt;			/* count of errors */
static char	*progname;		/* argv[0] to main */

void *
srealloc(void *op, size_t size)
{
	void	*np;

	if ((np = realloc(op, size)) == NULL) {
		write(2, "no memory\n", 10);
		_exit(077);
	}
	return np;
}

void *
smalloc(size_t size)
{
	return srealloc(NULL, size);
}

uid_t
getowner(const char *string)
{
	struct passwd	*pwd;
	char	*x;
	long	val;

	if ((pwd = getpwnam(string)) != NULL)
		return pwd->pw_uid;
	val = strtoul(string, &x, 10);
	if (*x != '\0' || *string == '+' || *string == '-') {
		fprintf(stderr, "%s: unknown user %s.\n", progname, string);
		exit(1);
	}
	return val;
}

gid_t
getgroup(const char *string)
{
	struct group	*grp;
	char	*x;
	long	val;

	if ((grp = getgrnam(string)) != NULL)
		return grp->gr_gid;
	val = strtoul(string, &x, 10);
	if (*x != '\0' || *string == '+' || *string == '-') {
		fprintf(stderr, "%s: unknown group %s.\n", progname, string);
		exit(1);
	}
	return val;
}

void
getpath(const char *path, char **file, char **filend, size_t *sz, size_t *slen)
{
	*sz = 14 + strlen(path) + 2;
	*file = smalloc(*sz);
	*filend = *file;
	if (path[0] == '/' && path[1] == '\0')
		*(*filend)++ = '/';
	else {
		const char	*cp = path;
		while ((*(*filend)++ = *cp++) != '\0');
		(*filend)[-1] = '/';
	}
	*slen = *filend - *file;
}

void
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

void
fdcopy(const char *src, const struct stat *ssp, const int sfd,
		const char *tgt, const struct stat *dsp, const int dfd)
{
	char	*buf;
	size_t	bufsize;
	ssize_t	rsz, wo, wt;

	if ((bufsize = ssp->st_blksize) < dsp->st_blksize)
		if ((bufsize = dsp->st_blksize) <= 0)
			bufsize = 512;
	buf = smalloc(bufsize);
	while ((rsz = read(sfd, buf, bufsize)) > 0) {
		wt = 0;
		do {
			if ((wo = write(dfd, buf + wt, rsz - wt)) < 0) {
				fprintf(stderr, "%s: write: %s: %s\n",
						progname, tgt,
						strerror(errno));
				errcnt |= 01;
				unlink(tgt);
				free(buf);
				return;
			}
			wt += wo;
		} while (wt < rsz);
	}
	if (rsz < 0) {
		fprintf(stderr, "%s: read: %s: %s\n",
				progname, src, strerror(errno));
		errcnt |= 01;
		unlink(tgt);
	}
	free(buf);
}

static void
usage(void)
{
	fprintf(stderr, "\
usage: %s [-cs] [-g group] [-m mode] [-o owner] file ...  destination\n\
       %s  -d   [-g group] [-m mode] [-o owner] dir\n",
       		progname, progname);
	exit(2);
}

static void
strip(const char *file)
{
	const char	cpr[] = "strip ";
	char	*cmd, *cp;
	const char	*sp;

	cp = cmd = smalloc(strlen(cpr) + strlen(file) + 1);
	for (sp = cpr; *sp; sp++)
		*cp++ = *sp;
	for (sp = file; *sp; sp++)
		*cp++ = *sp;
	*cp = '\0';
	system(cmd);
	free(cmd);
}

static enum okay
chgown(const char *fn, struct stat *sp)
{
	struct stat	st;

	if (sp == NULL) {
		if (stat(fn, &st) < 0) {
			fprintf(stderr, "%s: stat: %s: %s\n",
					progname, fn, strerror(errno));
			errcnt |= 1;
			return STOP;
		}
		sp = &st;
	}
	if (!oflag)
		owner = sp->st_uid;
	if (!gflag)
		group = sp->st_gid;
	if (chown(fn, owner, group) < 0) {
		fprintf(stderr, "%s: chown: %s: %s\n", progname, fn,
				strerror(errno));
		errcnt |= 01;
		return STOP;
	}
	return OKAY;
}

static enum okay
check(const char *src, const char *tgt, const struct stat *dsp,
		struct stat *ssp)
{
	if (stat(src, ssp) < 0) {
		fprintf(stderr, "%s: %s: %s\n", progname, src,
				strerror(errno));
		errcnt |= 01;
		return STOP;
	}
	if ((ssp->st_mode&S_IFMT) != S_IFREG && strcmp(src, "/dev/null")) {
		fprintf(stderr, "%s: %s isn't a regular file.\n",
				progname, src);
		errcnt |= 01;
		return STOP;
	}
	if (dsp && (ssp->st_dev == dsp->st_dev && ssp->st_ino == dsp->st_ino)) {
		fprintf(stderr, "%s: %s and %s are the same file.\n",
				progname, src, tgt);
		errcnt |= 01;
		return STOP;
	}
	return OKAY;
}

static void
cp(const char *src, const char *tgt, struct stat *dsp)
{
	struct stat	sst, nst;
	int	sfd, dfd;

	if (check(src, tgt, dsp, &sst) != OKAY)
		return;
	unlink(tgt);
	if ((dfd = creat(tgt, 0700)) < 0 || fchmod(dfd, 0700) < 0 ||
			fstat(dfd, &nst) < 0) {
		fprintf(stderr, "%s: %s: %s\n", progname, src,
				strerror(errno));
		errcnt |= 01;
		if (dfd >= 0)
			close(dfd);
		return;
	}
	if ((sfd = open(src, O_RDONLY)) < 0) {
		fprintf(stderr, "%s: open: %s: %s\n", progname, src,
				strerror(errno));
		errcnt |= 01;
		return;
	}
	fdcopy(src, &sst, sfd, tgt, &nst, dfd);
	close(dfd);
	close(sfd);
	if (sflag)
		strip(tgt);
	if (oflag || gflag)
		chgown(tgt, &nst);
	if (chmod(tgt, mode) < 0) {
		fprintf(stderr, "%s: %s: %s\n", progname, tgt, strerror(errno));
		errcnt |= 01;
	}
}

static void
installf(int ac, char **av)
{
	struct stat	dst, ust;

	if (lstat(av[ac-1], &dst) == 0) {
		if ((dst.st_mode&S_IFMT) != S_IFLNK ||
				stat(av[ac-1], &ust) < 0)
			ust = dst;
		if ((ust.st_mode&S_IFMT) == S_IFDIR) {
			char	*copy, *cend;
			size_t	sz, slen, ss;
			int	i;

			getpath(av[ac-1], &copy, &cend, &sz, &slen);
			for (i = 0; i < ac-1; i++) {
				setpath(basename(av[i]), &copy, &cend,
						slen, &sz, &ss);
				cp(av[i], copy, stat(copy, &dst) < 0 ?
						NULL : &dst);
			}
		} else if (ac > 2)
			usage();
		else
			cp(av[0], av[1], &ust);
	} else if (ac > 2)
		usage();
	else
		cp(av[0], av[1], NULL);
}

static enum okay
makedir(const char *dir)
{
	struct stat	st;

	if (mkdir(dir, 0777) < 0) {
		if (errno == EEXIST) {
			if (stat(dir, &st) < 0 ||
					(st.st_mode&S_IFMT) != S_IFDIR){
				fprintf(stderr, "%s: %s is not a directory\n",
						progname, dir);
				errcnt |= 01;
				return STOP;
			}
		} else {
			fprintf(stderr, "%s: mkdir: %s: %s\n",
					progname, dir, strerror(errno));
			errcnt |= 01;
			return STOP;
		}
	}
	return OKAY;
}
static void
installd(char *dir)
{
	struct stat	st;
	int	sgid_bit;
	char	*slash;
	char	c;

	slash = dir;
	do {
		while (*slash == '/')
			slash++;
		while (*slash != '/' && *slash != '\0')
			slash++;
		c = *slash;
		*slash = '\0';
		if (makedir(dir) != OKAY)
			return;
		if (c == '\0') {
			if (oflag || gflag)
				if (chgown(dir, NULL) != OKAY)
					return;
			if (mflag) {
				sgid_bit = stat(dir, &st) == 0 &&
					st.st_mode&S_ISGID ? S_ISGID : 0;
				if (chmod(dir, mode | sgid_bit) < 0) {
					fprintf(stderr, "%s: chmod: %s: %s\n",
							progname, dir,
							strerror(errno));
					errcnt |= 01;
					return;
				}
			}
		}
		*slash = c;
	} while (c != '\0');
}

int
main(int argc, char **argv)
{
	const char	optstring[] = "csg:m:o:d";
	int	i;

	progname = basename(argv[0]);
	while ((i = getopt(argc, argv, optstring)) != EOF) {
		switch (i) {
		case 'c':
			/* no-op */
			break;
		case 's':
			sflag = 1;
			break;
		case 'g':
			gflag = 1;
			group = getgroup(optarg);
			break;
		case 'm':
			mflag = 1;
			mode = strtol(optarg, NULL, 8);
			break;
		case 'o':
			oflag = 1;
			owner = getowner(optarg);
			break;
		case 'd':
			dflag = 1;
			break;
		default:
			usage();
		}
	}
	if (dflag) {
		if (argc == optind || argc > optind + 1)
			usage();
		if (mflag)
			mode &= ~(mode_t)S_ISGID;
		installd(argv[optind]);
	} else {
		if (argc < optind + 2)
			usage();
		installf(argc - optind, &argv[optind]);
	}
	return errcnt;
}
