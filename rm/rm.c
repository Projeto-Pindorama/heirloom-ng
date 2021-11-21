/*
 * rm - remove directory entries
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
#ifdef	SUS
static const char sccsid[] USED = "@(#)rm_sus.sl	2.23 (gritter) 11/17/05";
#else
static const char sccsid[] USED = "@(#)rm.sl	2.23 (gritter) 11/17/05";
#endif

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/wait.h>
#include	<sys/resource.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>
#include	<dirent.h>
#include	<limits.h>
#include	<signal.h>
#include	<stdarg.h>

#include	"getdir.h"

struct	level {
	struct getdb	*l_db;
	int	l_fd;
};

static struct level	*levels;	/* visited directories */
static size_t	maxlevel;		/* maximum level visited */
static unsigned	errcnt;			/* count of errors */
static int	ontty;			/* stdin is a tty */
static int	fflag;			/* force */
static int	iflag;			/* ask for confirmation */
static int	rflag;			/* recursive */
static char	*progname;		/* argv[0] to main() */
static char	*path;			/* full path to current file */
static size_t	pathsz;			/* allocated size of path */
extern int	sysv3;			/* emulate SYSV3 behavior */

static int	subproc(size_t, const char *, int);

static void *
srealloc(void *vp, size_t nbytes)
{
	void *p;

	if ((p = realloc(vp, nbytes)) == NULL) {
		write(2, progname, strlen(progname));
		write(2, ": Insufficient memory.\n", 23);
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
	fprintf(stderr, "usage: %s [-fir] file ...\n", progname);
	exit(2);
}

static void
msg(const char *fmt, ...)
{
	va_list	ap;

	if (!sysv3)
		fprintf(stderr, "%s: ", progname);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

static int
confirm(void)
{
	int yes = 0;
	char c;

	if (read(0, &c, 1) == 1) {
		yes = (c == 'y' || c == 'Y');
		while (c != '\n' && read(0, &c, 1) == 1);
	}
	return yes;
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

static int
rmfile(const char *base, const struct stat *sp)
{
	if (iflag) {
		msg(sysv3 ? "%s ? " : "remove  %s: (y/n)? ", path);
		if (confirm() == 0)
			return 0;
	} else if ((sp->st_mode&S_IFMT) != S_IFDIR &&
			(sp->st_mode&S_IFMT) != S_IFLNK &&
			ontty && !fflag &&
			access(base, W_OK) < 0) {
		msg("%s: %o mode ? ", path, (int)(sp->st_mode & 0777));
		if (confirm() == 0)
			return 0;
	}
	if (((sp->st_mode&S_IFMT) == S_IFDIR ? rmdir : unlink)(base) < 0) {
#ifndef	SUS
		if ((sp->st_mode&S_IFMT) == S_IFDIR || !fflag || iflag)
#endif
			fprintf(stderr, (sp->st_mode&S_IFMT) == S_IFDIR ?
				"%s: Unable to remove directory %s\n%s\n" :
				"%s: %s not removed.\n%s\n",
				progname, path, strerror(errno));
		errcnt |= 1;
		return -1;
	}
	return 0;
}

static void
rm(size_t pend, const char *base, const int olddir, int ssub, int level)
{
	struct stat st;

	if (lstat(base, &st) < 0) {
		if (fflag == 0 || errno != ENOENT) {
			if (sysv3)
				fprintf(stderr, "%s: %s non-existent\n",
						progname, path);
			else
				fprintf(stderr, "%s: %s\n",
						path, strerror(errno));
			errcnt |= 4;
		}
		return;
	}
	if ((st.st_mode&S_IFMT) == S_IFDIR) {
		if (rflag) {
			struct direc *dp;
			struct getdb *db;
			int df, err;

			if (ssub == 0 && (iflag
#ifdef	SUS
					|| (!fflag && ontty &&
						access(base, W_OK) < 0)
#endif
					)) {
				msg("directory %s: ? ", path);
				if (confirm() == 0)
					return;
			}
			if ((df = open(base,
							O_RDONLY
#ifdef	O_DIRECTORY
							| O_DIRECTORY
#endif
#ifdef	O_NOFOLLOW
							| O_NOFOLLOW
#endif
						)) < 0 ||
					(db = getdb_alloc(base, df)) == NULL) {
				if (errno == EMFILE) {
					int	sres;

					sres = subproc(pend, base, level);
					if (sres >= 0) {
						errcnt |= sres;
						goto remove;
					}
				}
				/*
				 * Maybe the directory is empty and can just
				 * be removed.
				 */
				if (rmfile(base, &st) < 0) {
					fprintf(stderr,
						"%s: cannot read "
						"directory %s\n",
						progname, path);
					errcnt |= 4;
				}
				return;
			}
			if (fchdir(df) < 0) {
				if (rmfile(base, &st) < 0) {
					fprintf(stderr,
						"%s: cannot chdir to %s\n",
							progname, path);
					errcnt |= 4;
				}
				getdb_free(db);
				close(df);
				return;
			}
			setlevel(level, db, df);
			while ((dp = getdir(db, &err)) != NULL) {
				if (dp->d_name[0] == '.' &&
						(dp->d_name[1] == '\0' ||
						 dp->d_name[1] == '.' &&
						 dp->d_name[2] == '\0'))
					continue;
				rm(catpath(pend, dp->d_name), dp->d_name,
						df, 0, level + 1);
				path[pend] = '\0';
			}
			if (err) {
				fprintf(stderr,
					"%s: error reading directory %s\n",
					progname, path);
				errcnt |= 4;
			}
			if (olddir >= 0 && fchdir(olddir) < 0) {
				fprintf(stderr, "%s: cannot change backwards\n",
						progname);
				exit(1);
			}
			getdb_free(db);
			close(df);
		} else {
			fprintf(stderr, "%s: %s directory\n", progname, path);
			errcnt |= 1;
			return;
		}
	}
	if (ssub == 0)
	remove:	rmfile(base, &st);
}

/*
 * POSIX.2 specifies that rm shall not fail because it exceeds the
 * maximum number of open file descriptors. It is not possible to
 * use path names and chdir("..") instead since the tree might be
 * moved during operation and we'd then start to remove files outside.
 * Creating a new process makes it possible to circumvent the OPEN_MAX
 * limit in a safe way.
 */
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
		rm(pend, base, -1, 1, 0);
		_exit(errcnt);
	}
	default: {
		int status;

		while (waitpid(pid, &status, 0) != pid);
		if (status && WIFSIGNALED(status)) {
			/*
			 * If the signal was sent due to a tty keypress,
			 * we should be terminated automatically and
			 * never reach this point. Otherwise, we terminate
			 * with the same signal, but make sure that we do
			 * not overwrite a possibly generated core file.
			 * This results in nearly the usual behavior except
			 * that the shell never prints a 'core dumped'
			 * message.
			 */
			struct rlimit	rl;

			rl.rlim_cur = rl.rlim_max = 0;
			setrlimit(RLIMIT_CORE, &rl);
			raise(WTERMSIG(status));
			pause();
		}
		return status ? WEXITSTATUS(status) : 0;
	}
	case -1:
		return -1;
	}
}

static char *
dotdot(const char *s)
{
	const char	*sp;

	for (sp = s; *sp; sp++);
	sp--;
	while (sp > s && *sp == '/')
		sp--;
	if (sp[0] == '.' && (sp == s || (sp > s && sp[-1] == '/')))
		return ".";
	if (&sp[-1] >= s && sp[0] == '.' && sp[-1] == '.' && (&sp[-1] == s ||
				&sp[-1] > s && sp[-2] == '/'))
		return "..";
	return NULL;
}

int
main(int argc, char **argv)
{
	int i, startfd = -1, illegal = 0;

#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	progname = basename(argv[0]);
	if (getenv("SYSV3") != NULL)
		sysv3 = 1;
	while ((i = getopt(argc, argv, "fiRr")) != EOF) {
		switch (i) {
		case 'f':
			fflag = 1;
#ifdef	SUS
			iflag = 0;
#endif
			break;
		case 'i':
#ifdef	SUS
			fflag = 0;
#endif
			iflag = 1;
			break;
		case 'R':
		case 'r':
			rflag = 1;
			break;
		default:
			illegal = 1;
		}
	}
	if (illegal)
		usage();
#ifndef	SUS
	if (argv[optind] && argv[optind][0] == '-' && argv[optind][1] == '\0' &&
			(argv[optind-1][0] != '-' || argv[optind-1][1] != '-' ||
			 argv[optind-1][2] != '\0'))
		optind++;
#endif
	if (optind >= argc)
		usage();
	ontty = isatty(0);
	if (rflag && (startfd = open(".", O_RDONLY)) < 0) {
		fprintf(stderr, "%s: cannot open current directory\n",
				progname);
		exit(1);
	}
	while (optind < argc) {
		if (dotdot(argv[optind]) != NULL) {
			if (fflag == 0)
				fprintf(stderr, "%s of %s is not allowed\n",
					progname, argv[optind]);
			errcnt |= 1;
		} else {
			if (path)
				free(path);
			i = strlen(argv[optind]);
			strcpy(path = smalloc(pathsz = i + 1), argv[optind]);
			rm(i, argv[optind], startfd, 0, 0);
		}
		optind++;
	}
	return errcnt;
}
