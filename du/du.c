/*
 * du - disk usage
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, December 2000.
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
static const char sccsid[] USED = "@(#)du_sus.sl	1.39 (gritter) 5/29/05";
#elif defined (UCB)
static const char sccsid[] USED = "@(#)/usr/ucb/du.sl	1.39 (gritter) 5/29/05";
#else
static const char sccsid[] USED = "@(#)du.sl	1.39 (gritter) 5/29/05";
#endif

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/resource.h>
#include	<sys/wait.h>
#include	<signal.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<errno.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<libgen.h>
#include	<dirent.h>
#include	<limits.h>
#include	<locale.h>

#include	<getdir.h>

#ifdef	__hpux
#define	hpfix(blocks)	((blocks) <<= 1)
#else
#define	hpfix(blocks)
#endif

static struct level {
	struct getdb	*l_db;
	int	l_fd;
} *levels;

static size_t	maxlevel;
static char	*path;
static size_t	pathsz;

static int	subproc(size_t, const char *, int);

/*
 * Files with multiple links may be counted once only. This structure
 * is used as a per-device tree of i-nodes.
 */
struct islot {
	struct islot	*i_lln;		/* left link */
	struct islot	*i_rln;		/* right link */
	ino_t		i_ino;		/* i-node number */
};

/*
 * The devices are kept in a linked list.
 */
struct dslot {
	struct dslot	*d_nxt;		/* next device */
	struct islot	*d_isl;		/* root of i-node tree */
	dev_t		d_dev;		/* device id */
};

static unsigned	errcnt;			/* count of errors */
static int		aflag;		/* report all files */
static int		hflag;		/* human-readable sizes */
static int		kflag;		/* sizes in kilobytes */
static int		sflag;		/* print summary only */
static int		xflag;		/* do not cross device boundaries */
#if defined (SUS) || defined (UCB)
static int		rflag = 1;	/* write error messages */
#else	/* !SUS, !UCB */
static int		rflag;		/* write error messages */
#endif	/* !SUS, !UCB */
static int		oflag;		/* do not add subdirs */
static int		HLflag;		/* -H or -L flag */
static long		maxdepth;	/* max depth of recursion */
static long long	*dtots;		/* size of directories on recursion */
static struct dslot	*d0;		/* start of devices table */
static char		*progname;	/* argv0 to main */
static struct islot	*inull;		/* i-node tree null element */

static void *
srealloc(void *vp, size_t sz)
{
	void *cp;

	if ((cp = realloc(vp, sz)) == NULL) {
		write(2, "No memory\n", 10);
		exit(1);
	}
	return cp;
}

static void *
smalloc(size_t sz)
{
	return srealloc(NULL, sz);
}

static void *
scalloc(size_t nelem, size_t nbytes)
{
	void	*cp;

	if ((cp = calloc(nelem, nbytes)) == NULL) {
		write(2, "No memory\n", 10);
		exit(1);
	}
	return cp;
}

/*
 * perror()-alike.
 */
static void
pnerror(int eno, const char *string)
{
	if (rflag)
		fprintf(stderr, "%s: %s: %s\n",
				progname, string, strerror(eno));
	errcnt |= 01;
}

/*
 * Show output.
 */
static void
report(unsigned long long sz, const char *fn)
{
	if (hflag || kflag) {
		if (sz & 01)
			sz++;
		sz >>= 01;
	}
	if (hflag) {
		const char	units[] = "KMGTPE", *up = units;
		int	rest = 0;

		while (sz > 1023) {
			rest = (sz % 1024) / 128;
			sz /= 1024;
			up++;
		}
		if (sz < 10 && rest)
			printf("%2llu.%u%c\t%s\n", sz, rest, *up, fn);
		else
			printf("%4llu%c\t%s\n", sz, *up, fn);
	} else
		printf("%llu\t%s\n", sz, fn);
}

/*
 * Free the given i-nodes tree.
 */
static void
freeislots(struct islot *ip)
{
	if (ip == inull)
		return;
	freeislots(ip->i_lln);
	freeislots(ip->i_rln);
	free(ip);
}

/*
 * Free the device tree.
 */
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

/*
 * Top-down splay function for inode tree.
 */
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

/*
 * Find the inode number ino.
 */
static struct islot *
ifind(ino_t ino, struct islot **it)
{
	if (*it == NULL)
		return NULL;
	*it = isplay(ino, *it);
	return (*it)->i_ino == ino ? *it : NULL;
}

/*
 * Put ik into the tree.
 */
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
/*
 * Handle multiple links. Return 0 if the i-node was visited the first time,
 * else 1.
 */
static int
multilink(const struct stat *sp)
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
		ip->i_ino = sp->st_ino;
		iput(ip, &ds->d_isl);
	} else
		return 1;
	return 0;
}

/*
 * Handle a file.
 */
static void
dufile(const char *fn, const struct stat *sp, int level)
{
	switch (sp->st_mode&S_IFMT) {
	default:
		if (sp->st_nlink > 1 && multilink(sp))
			break;
		if (aflag && !sflag)
			report(sp->st_blocks, fn);
		dtots[level] += sp->st_blocks;
		break;
	case S_IFDIR:
		dtots[level + 1] += sp->st_blocks;
		if (!sflag || level == 0)
			report(dtots[level + 1], fn);
		if (oflag == 0)
			dtots[level] += dtots[level + 1];
		dtots[level + 1] = 0;
	}
}

static void
setlevel(int level, struct getdb *db, int fd)
{
	if (level >= maxlevel)
		levels = srealloc(levels, (maxlevel=level+16) * sizeof *levels);
	levels[level].l_db = db;
	levels[level].l_fd = fd;
}

static size_t
catpath(size_t pend, const char *base)
{
	size_t	blen = strlen(base);

	if (pend + blen + 2 >= pathsz)
		path = srealloc(path, pathsz = pend + blen + 16);
	if (pend == 0 || path[pend-1] != '/')
		path[pend++] = '/';
	strcpy(&path[pend], base);
	return pend + blen;
}

static void
descend(size_t pend, const char *base, const int olddir, int ssub, int level)
{
	struct stat	st;
	int	(*statfn)(const char *, struct stat *);

	statfn = HLflag == 'L' || level == 0 && HLflag == 'H' ? stat : lstat;
	if (statfn(base, &st) < 0) {
		pnerror(errno, path);
		return;
	}
	hpfix(st.st_blocks);
	if (xflag) {
		static dev_t	curdev;

		if (level == 0)
			curdev = st.st_dev;
		else if (curdev != st.st_dev)
			return;
	}
	if ((st.st_mode&S_IFMT) == S_IFDIR) {
		struct direc	*dp;
		struct getdb	*db = NULL;
		int	df, err;
		int	oflag = O_RDONLY;

		if (HLflag == 'L' && multilink(&st)) {
			if (lstat(base, &st) < 0) {
				pnerror(errno, path);
				return;
			}
			hpfix(st.st_blocks);
			goto done;
		}
#ifdef	O_DIRECTORY
		oflag |= O_DIRECTORY;
#endif
#ifdef	O_NOFOLLOW
		if (statfn == lstat)
			oflag |= O_NOFOLLOW;
#endif
		if ((df = open(base, oflag)) < 0 ||
				(db = getdb_alloc(base, df)) == NULL) {
			if (errno == EMFILE) {
				int	sres;

				sres = subproc(pend, base, level);
				if (sres >= 0)
					goto done;
			}
			pnerror(errno, path);
			goto done;
		}
		if (fchdir(df) < 0) {
			pnerror(errno, path);
			getdb_free(db);
			close(df);
			goto done;
		}
		setlevel(level, db, df);
		while ((dp = getdir(db, &err)) != NULL) {
			if (dp->d_name[0] == '.' &&
					(dp->d_name[1] == '\0' ||
					 dp->d_name[1] == '.' &&
					 dp->d_name[2] == '\0'))
				continue;
			descend(catpath(pend, dp->d_name), dp->d_name,
					df, 0, level + 1);
			path[pend] = '\0';
		}
		if (err)
			pnerror(errno, path);
		if (olddir >= 0 && fchdir(olddir) < 0) {
			pnerror(errno, "cannot change backwards");
			exit(1);
		}
		getdb_free(db);
		close(df);
	}
	if (ssub == 0)
	done:	dufile(path, &st, level);
}

static int
subproc(size_t pend, const char *base, int level)
{
	pid_t	pid;

	switch (pid = fork()) {
	case 0: {
		int	i;

		errcnt = 0;
		for (i = 0; i < level - 1; i++) {
			getdb_free(levels[i].l_db);
			close(levels[i].l_fd);
		}
		descend(pend, base, -1, 1, 0);
		_exit(errcnt);
	}
	default: {
		int	status;

		while (waitpid(pid, &status, 0) != pid);
		if (status && WIFSIGNALED(status)) {
			struct rlimit	rl;

			rl.rlim_cur = rl.rlim_max = 0;
			setrlimit(RLIMIT_CORE, &rl);
			raise(WTERMSIG(status));
			pause();
		}
		if (status)
			errcnt |= 1;
		return 0;
	}
	case -1:
		 return -1;
	}
}

static void
du(const char *fn)
{
	struct stat st;
	int	i, startfd;

	if ((HLflag ? stat : lstat)(fn, &st) < 0) {
		pnerror(errno, fn);
		return;
	}
	hpfix(st.st_blocks);
	if (S_ISDIR(st.st_mode)) {
		if ((startfd = open(".", O_RDONLY)) < 0) {
			pnerror(errno, ".");
			return;
		}
		if (d0)
			freedslots();
		memset(dtots, 0, sizeof *dtots * (maxdepth + 1));
		if (path)
			free(path);
		i = strlen(fn);
		strcpy(path = smalloc(pathsz = i+1), fn);
		descend(i, fn, startfd, 0, 0);
	} else {
#ifndef	SUS
		if (aflag || sflag)
#endif
			report(st.st_blocks, fn);
	}
}

static void
usage(void)
{
#ifdef	UCB
	fprintf(stderr, "usage: %s [-as] [name ...]\n", progname);
#else
	fprintf(stderr, "usage: %s [-ars] [name ...]\n", progname);
#endif
	exit(2);
}

int
main(int argc, char **argv)
{
	int i;

#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	progname = basename(argv[0]);
	maxdepth = sysconf(_SC_OPEN_MAX);
	dtots = smalloc(sizeof *dtots * (maxdepth + 1));
	inull = scalloc(1, sizeof *inull);
	inull->i_lln = inull->i_rln = inull;
#ifdef	UCB
	kflag = 1;
#endif
	while ((i = getopt(argc, argv, "ahHkLosrx")) != EOF) {
		switch (i) {
		case 'a':
			aflag = 1;
			break;
		case 'h':
			hflag = 1;
			break;
		case 'H':
		case 'L':
			HLflag = i;
			break;
		case 'k':
			kflag = 1;
			break;
		case 'o':
			oflag = 1;
			break;
		case 'r':
			rflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 'x':
			xflag = 1;
			break;
		default:
			usage();
		}
	}
	if (sflag)
		oflag = 0;
	if (argv[optind])
		while (argv[optind])
			du(argv[optind++]);
	else
		du(".");
	return errcnt;
}
