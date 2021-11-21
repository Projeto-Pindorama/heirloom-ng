/*
 * This code contains changes by
 * Gunnar Ritter, Freiburg i. Br., Germany, March 2003. All rights reserved.
 *
 * Conditions 1, 2, and 4 and the no-warranty notice below apply
 * to these changes.
 *
 *
 * Copyright (c) 1991
 * 	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 	This product includes software developed by the University of
 * 	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
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

/*	Sccsid @(#)diffdir.c	1.30 (gritter) 1/22/06>	*/
/*	from 4.3BSD diffdir.c	4.9 (Berkeley) 8/28/84	*/

#include "diff.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include "sigset.h"
#include "pathconf.h"

#ifdef	__GLIBC__	/* old glibcs don't know _XOPEN_SOURCE=600L yet */
#ifndef	S_IFSOCK
#ifdef	__S_IFSOCK
#define	S_IFSOCK	__S_IFSOCK
#endif	/* __S_IFSOCK */
#endif	/* !S_IFSOCK */
#endif	/* __GLIBC__ */

/*
 * diff - directory comparison
 */
#define	d_flags	d_ino

#define	ONLY	1		/* Only in this directory */
#define	SAME	2		/* Both places and same */
#define	DIFFER	4		/* Both places and different */
#define	DIRECT	8		/* Directory */

struct dir {
	unsigned long long	d_ino;
	char	*d_entry;
};

static int	header;
static char	*title, *etitle;
static size_t	titlesize;
static char	procself[40];

static void	setfile(char **, char **, const char *);
static void	scanpr(register struct dir *, int, const char *, const char *,
			const char *, const char *, const char *);
static void	only(struct dir *, int);
static struct dir	*setupdir(const char *);
static int	entcmp(const struct dir *, const struct dir *);
static void	compare(struct dir *, char **);
static void	calldiff(const char *, char **);
static int	useless(register const char *);
static const char	*mtof(mode_t mode);
static void	putN(const char *, const char *, const char *, int);
static void	putNreg(const char *, const char *, time_t, int);
static void	putNnorm(FILE *, const char *, const char *,
			FILE *, long long, int);
static void	putNedit(FILE *, const char *, const char *,
			FILE *, long long, int, int);
static void	putNcntx(FILE *, const char *, const char *,
			time_t, FILE *, long long, int);
static void	putNunif(FILE *, const char *, const char *,
			time_t, FILE *, long long, int);
static void	putNhead(FILE *, const char *, const char *, time_t, int,
			const char *, const char *);
static void	putNdata(FILE *, FILE *, int, int);
static void	putNdir(const char *, const char *, int);
static long long	linec(const char *, FILE *);
static char	*mkpath(const char *, const char *);
static void	mktitle(void);
static int	xclude(const char *);

void
diffdir(char **argv)
{
	register struct dir *d1, *d2;
	struct dir *dir1, *dir2;
	register int i, n;
	int cmp;

	if (opt == D_IFDEF) {
		fprintf(stderr, "%s: can't specify -I with directories\n",
				progname);
		done();
	}
	status = 0;
	if (opt == D_EDIT && (sflag || lflag))
		fprintf(stderr,
		    "%s: warning: shouldn't give -s or -l with -e\n",
		    progname);
	for (n = 6, i = 1; diffargv[i+2]; i++)
		n += strlen(diffargv[i]) + 1;
	if (n > titlesize)
		title = ralloc(title, titlesize = n);
	title[0] = 0;
	strcpy(title, "diff ");
	for (i = 1; diffargv[i+2]; i++) {
		if (!strcmp(diffargv[i], "-"))
			continue;	/* was -S, dont look silly */
		strcat(title, diffargv[i]);
		strcat(title, " ");
	}
	for (etitle = title; *etitle; etitle++)
		;
	/*
	 * This works around a bug present in (at least) Solaris 8 and
	 * 9: If exec() is called with /proc/self/object/a.out, the
	 * process hangs. It is possible, though, to use the executable
	 * of another process. So the parent diff is used instead of the
	 * forked child.
	 */
	i = getpid();
	snprintf(procself, sizeof procself,
#if defined (__linux__)
			"/proc/%d/exe",
#elif defined (__FreeBSD__) || defined (__DragonFly__) || defined (__APPLE__)
			"/proc/%d/file",
#else	/* !__linux__, !__FreeBSD__, !__APPLE__ */
			"/proc/%d/object/a.out",
#endif	/* !__linux__, !__FreeBSD__, !__APPLE__ */
			i);
	setfile(&file1, &efile1, file1);
	setfile(&file2, &efile2, file2);
	argv[0] = file1;
	argv[1] = file2;
	dir1 = setupdir(file1);
	dir2 = setupdir(file2);
	d1 = dir1; d2 = dir2;
	while (d1->d_entry != 0 || d2->d_entry != 0) {
		if (d1->d_entry && useless(d1->d_entry)) {
			d1++;
			continue;
		}
		if (d2->d_entry && useless(d2->d_entry)) {
			d2++;
			continue;
		}
		if (d1->d_entry == 0)
			cmp = 1;
		else if (d2->d_entry == 0)
			cmp = -1;
		else
			cmp = strcmp(d1->d_entry, d2->d_entry);
		if (cmp < 0) {
			if (lflag && !(Nflag&1))
				d1->d_flags |= ONLY;
			else if (Nflag&1 || opt == D_NORMAL ||
					opt == D_CONTEXT || opt == D_UNIFIED)
				only(d1, 1);
			d1++;
		} else if (cmp == 0) {
			compare(d1, argv);
			d1++;
			d2++;
		} else {
			if (lflag && !(Nflag&2))
				d2->d_flags |= ONLY;
			else if (Nflag&2 || opt == D_NORMAL ||
					opt == D_CONTEXT || opt == D_UNIFIED)
				only(d2, 2);
			d2++;
		}
	}
	if (lflag) {
		scanpr(dir1, ONLY, "Only in %.*s", file1, efile1, 0, 0);
		scanpr(dir2, ONLY, "Only in %.*s", file2, efile2, 0, 0);
		scanpr(dir1, SAME, "Common identical files in %.*s and %.*s",
		    file1, efile1, file2, efile2);
		scanpr(dir1, DIFFER, "Binary files which differ in %.*s and %.*s",
		    file1, efile1, file2, efile2);
		scanpr(dir1, DIRECT, "Common subdirectories of %.*s and %.*s",
		    file1, efile1, file2, efile2);
	}
	if (rflag) {
		if (header && lflag)
			printf("\f");
		for (d1 = dir1; d1->d_entry; d1++)  {
			if ((d1->d_flags & DIRECT) == 0)
				continue;
			strcpy(efile1, d1->d_entry);
			strcpy(efile2, d1->d_entry);
			calldiff(0, argv);
		}
	}
}

static void
setfile(char **fpp, char **epp, const char *file)
{
	register char *cp;
	int	n;

	if ((n = pathconf(file, _PC_PATH_MAX)) < 1024)
		n = 1024;
	*fpp = dalloc(strlen(file) + 2 + n);
	if (*fpp == 0) {
		oomsg(": ran out of memory\n");
		exit(1);
	}
	strcpy(*fpp, file);
	for (cp = *fpp; *cp; cp++)
		continue;
	*cp++ = '/';
	*cp = '\0';
	*epp = cp;
}

static void
scanpr(register struct dir *dp, int test, const char *title,
		const char *file1, const char *efile1,
		const char *file2, const char *efile2)
{
	int titled = 0;

	for (; dp->d_entry; dp++) {
		if ((dp->d_flags & test) == 0)
			continue;
		if (titled == 0) {
			if (header == 0)
				header = 1;
			else
				printf("\n");
			printf(title,
			    efile1 - file1 - 1, file1,
			    efile2 - file2 - 1, file2);
			printf(":\n");
			titled = 1;
		}
		printf("\t%s\n", dp->d_entry);
	}
}

static void
only(struct dir *dp, int which)
{
	char *file = which == 1 ? file1 : file2;
	char *other = which == 1 ? file2 : file1;
	char *efile = which == 1 ? efile1 : efile2;
	char *eother = which == 1 ? efile2 : efile1;

	if (Nflag&which) {
		char	c = file[efile - file - 1];
		char	d = other[eother - other - 1];
		file[efile - file - 1] = '\0';
		other[eother - other - 1] = '\0';
		putN(file, other, dp->d_entry, which);
		file[efile - file - 1] = c;
		other[eother - other - 1] = d;
	} else
		printf("Only in %.*s: %s\n", (int)(efile - file - 1), file,
				dp->d_entry);
	status = 1;
}

static struct dir *
setupdir(const char *cp)
{
	register struct dir *dp = 0, *ep;
	register struct dirent *rp;
	register int nitems;
	DIR *dirp;

	dirp = opendir(cp);
	if (dirp == NULL) {
		fprintf(stderr, "%s: %s: %s\n", progname, cp, strerror(errno));
		done();
	}
	nitems = 0;
	dp = dalloc(sizeof (struct dir));
	if (dp == 0) {
		oomsg(": ran out of memory\n");
		status = 2;
		done();
	}
	while (rp = readdir(dirp)) {
		if (xflag && xclude(rp->d_name))
			continue;
		ep = &dp[nitems++];
		ep->d_entry = 0;
		ep->d_flags = 0;
		ep->d_entry = dalloc(strlen(rp->d_name) + 1);
		if (ep->d_entry == 0) {
			oomsg(": out of memory\n");
			status = 2;
			done();
		}
		strcpy(ep->d_entry, rp->d_name);
		dp = ralloc(dp, (nitems + 1) * sizeof (struct dir));
	}
	dp[nitems].d_entry = 0;		/* delimiter */
	closedir(dirp);
	qsort(dp, nitems, sizeof (struct dir),
			(int (*)(const void *, const void *))entcmp);
	return (dp);
}

static int
entcmp(const struct dir *d1, const struct dir *d2)
{
	return (strcmp(d1->d_entry, d2->d_entry));
}

static void
compare(struct dir *dp, char **argv)
{
	register int i, j;
	int f1 = -1, f2 = -1;
	mode_t fmt1, fmt2;
	struct stat stb1, stb2;
	char buf1[BUFSIZ], buf2[BUFSIZ];

	strcpy(efile1, dp->d_entry);
	strcpy(efile2, dp->d_entry);
	if (stat(file1, &stb1) < 0 || (fmt1 = stb1.st_mode&S_IFMT) == S_IFREG &&
			(f1 = open(file1, O_RDONLY)) < 0) {
		perror(file1);
		status = 2;
		return;
	}
	if (stat(file2, &stb2) < 0 || (fmt2 = stb2.st_mode&S_IFMT) == S_IFREG &&
			(f2 = open(file2, O_RDONLY)) < 0) {
		perror(file2);
		close(f1);
		status = 2;
		return;
	}
	if (fmt1 != S_IFREG || fmt2 != S_IFREG) {
		if (fmt1 == fmt2) {
			switch (fmt1) {
			case S_IFDIR:
				dp->d_flags = DIRECT;
				if (lflag || opt == D_EDIT)
					goto closem;
				if (opt != D_UNIFIED)
					printf("Common subdirectories: "
						"%s and %s\n",
				    		file1, file2);
				goto closem;
			case S_IFBLK:
			case S_IFCHR:
				if (stb1.st_rdev == stb2.st_rdev)
					goto same;
				printf("Special files %s and %s differ\n",
						file1, file2);
				break;
			case S_IFIFO:
				if (stb1.st_dev == stb2.st_dev &&
						stb1.st_ino == stb2.st_ino)
					goto same;
				printf("Named pipes %s and %s differ\n",
						file1, file2);
				break;
			default:
				printf("Don't know how to compare "
						"%ss %s and %s\n",
						mtof(fmt1), file1, file2);
			}
		} else
			printf("File %s is a %s while file %s is a %s\n",
					file1, mtof(fmt1), file2, mtof(fmt2));
		if (lflag)
			dp->d_flags |= DIFFER;
		status = 1;
		goto closem;
	}
	if (stb1.st_size != stb2.st_size)
		goto notsame;
	if (stb1.st_dev == stb2.st_dev && stb1.st_ino == stb2.st_ino)
		goto same;
	for (;;) {
		i = read(f1, buf1, BUFSIZ);
		j = read(f2, buf2, BUFSIZ);
		if (i < 0 || j < 0 || i != j)
			goto notsame;
		if (i == 0 && j == 0)
			goto same;
		for (j = 0; j < i; j++)
			if (buf1[j] != buf2[j])
				goto notsame;
	}
same:
	if (sflag == 0)
		goto closem;
	if (lflag)
		dp->d_flags = SAME;
	else
		printf("Files %s and %s are identical\n", file1, file2);
	goto closem;
notsame:
	if (!aflag && (!ascii(f1) || !ascii(f2))) {
		if (lflag)
			dp->d_flags |= DIFFER;
		else if (opt == D_NORMAL || opt == D_CONTEXT ||
				opt == D_UNIFIED)
			printf("Binary files %s and %s differ\n",
			    file1, file2);
		status = 1;
		goto closem;
	}
	close(f1); close(f2);
	anychange = 1;
	if (lflag)
		calldiff(title, argv);
	else {
		if (opt == D_EDIT) {
			printf("ed - %s << '-*-END-*-'\n", dp->d_entry);
			calldiff(0, argv);
		} else {
			printf("%s%s %s\n", title, file1, file2);
			calldiff(0, argv);
		}
		if (opt == D_EDIT)
			printf("w\nq\n-*-END-*-\n");
	}
	return;
closem:
	close(f1); close(f2);
}

static void
stackdiff(char **argv)
{
	int	oanychange;
	char	*ofile1, *ofile2, *oefile1, *oefile2;
	struct stat	ostb1, ostb2;
	struct stackblk	*ocurstack;
	char	*oargv[2];
	int	oheader;
	char	*otitle, *oetitle;
	size_t	otitlesize;
	jmp_buf	orecenv;

	(void)&oargv;
	recdepth++;
	oanychange = anychange;
	ofile1 = file1;
	ofile2 = file2;
	oefile1 = efile1;
	oefile2 = efile2;
	ostb1 = stb1;
	ostb2 = stb2;
	ocurstack = curstack;
	oargv[0] = argv[0];
	oargv[1] = argv[1];
	oheader = header;
	otitle = title;
	oetitle = etitle;
	otitlesize = titlesize;
	memcpy(orecenv, recenv, sizeof orecenv);

	anychange = 0;
	file1 = argv[0];
	file2 = argv[1];
	efile1 = NULL;
	efile2 = NULL;
	curstack = NULL;
	header = 0;
	title = NULL;
	etitle = NULL;
	titlesize = 0;

	if (setjmp(recenv) == 0)
		diffany(argv);
	purgestack();

	anychange = oanychange;
	file1 = ofile1;
	file2 = ofile2;
	efile1 = oefile1;
	efile2 = oefile2;
	stb1 = ostb1;
	stb2 = ostb2;
	curstack = ocurstack;
	argv[0] = oargv[0];
	argv[1] = oargv[1];
	header = oheader;
	title = otitle;
	etitle = oetitle;
	titlesize = otitlesize;
	memcpy(recenv, orecenv, sizeof recenv);
	recdepth--;
}

static const char	*prargs[] = { "pr", "-h", 0, "-f", 0, 0 };

static void
calldiff(const char *wantpr, char **argv)
{
	int pid, cstatus, cstatus2, pv[2];

	if (wantpr == NULL && hflag == 0) {
		stackdiff(argv);
		return;
	}
	prargs[2] = wantpr;
	fflush(stdout);
	if (wantpr) {
		mktitle();
		pipe(pv);
		pid = fork();
		if (pid == -1) {
			fprintf(stderr, "No more processes\n");
			done();
		}
		if (pid == 0) {
			close(0);
			dup(pv[0]);
			close(pv[0]);
			close(pv[1]);
			execvp(pr, (char **)prargs);
			perror(pr);
			done();
		}
	}
	pid = fork();
	if (pid == -1) {
		fprintf(stderr, "%s: No more processes\n", progname);
		done();
	}
	if (pid == 0) {
		if (wantpr) {
			close(1);
			dup(pv[1]);
			close(pv[0]);
			close(pv[1]);
		}
		execv(procself, diffargv);
		execv(argv0, diffargv);
		execvp(diff, diffargv);
		perror(diff);
		done();
	}
	if (wantpr) {
		close(pv[0]);
		close(pv[1]);
	}
	while (wait(&cstatus) != pid)
		continue;
	if (cstatus != 0) {
		if (WIFEXITED(cstatus) && WEXITSTATUS(cstatus) == 1)
			status = 1;
		else
			status = 2;
	}
	while (wait(&cstatus2) != -1)
		continue;
/*
	if ((status >> 8) >= 2)
		done();
*/
}

int
ascii(int f)
{
	char buf[BUFSIZ];
	register int cnt;
	register char *cp;

	lseek(f, 0, 0);
	cnt = read(f, buf, BUFSIZ);
	cp = buf;
	while (--cnt >= 0)
		if (*cp++ == '\0')
			return (0);
	return (1);
}

/*
 * THIS IS CRUDE.
 */
static int
useless(register const char *cp)
{

	if (cp[0] == '.') {
		if (cp[1] == '\0')
			return (1);	/* directory "." */
		if (cp[1] == '.' && cp[2] == '\0')
			return (1);	/* directory ".." */
	}
	if (start && strcmp(start, cp) > 0)
		return (1);
	return (0);
}

static const char *
mtof(mode_t mode)
{
	switch (mode) {
	case S_IFDIR:
		return "directory";
	case S_IFCHR:
		return "character special file";
	case S_IFBLK:
		return "block special file";
	case S_IFREG:
		return "plain file";
	case S_IFIFO:
		return "named pipe";
#ifdef	S_IFSOCK
	case S_IFSOCK:
		return "socket";
#endif	/* S_IFSOCK */
	default:
		return "unknown type";
	}
}

static void
putN(const char *dir, const char *odir, const char *file, int which)
{
	struct stat	st;
	char	*path;
	char	*opath;

	path = mkpath(dir, file);
	opath = mkpath(odir, file);
	if (stat(path, &st) < 0) {
		fprintf(stderr, "%s: %s: %s\n", progname, path,
				strerror(errno));
		status = 2;
		goto out;
	}
	switch (st.st_mode & S_IFMT) {
	case S_IFREG:
		putNreg(path, opath, st.st_mtime, which);
		break;
	case S_IFDIR:
		putNdir(path, opath, which);
		break;
	default:
		printf("Only in %s: %s\n", dir, file);
	}
out:	tfree(path);
	tfree(opath);
}

static void
putNreg(const char *fn, const char *on, time_t mtime, int which)
{
	long long	lines;
	FILE	*fp;
	FILE	*op;
	void	(*opipe)(int) = SIG_DFL;
	pid_t	pid = 0;

	if ((fp = fopen(fn, "r")) == NULL) {
		fprintf(stderr, "%s: %s: %s\n", progname, fn, strerror(errno));
		status = 2;
		return;
	}
	if ((lines = linec(fn, fp)) == 0 || fseek(fp, 0, SEEK_SET) != 0)
		goto out;
	if (lflag) {
		int	pv[2];
		opipe = sigset(SIGPIPE, SIG_IGN);
		fflush(stdout);
		prargs[2] = title;
		pipe(pv);
		switch (pid = fork()) {
		case -1:
			fprintf(stderr, "No more processes\n");
			done();
			/*NOTREACHED*/
		case 0:
			close(0);
			dup(pv[0]);
			close(pv[0]);
			close(pv[1]);
			execvp(pr, (char **)prargs);
			perror(pr);
			done();
		}
		close(pv[0]);
		op = fdopen(pv[1], "w");
	} else
		op = stdout;
	fprintf(op, "%.*s %s %s\n", (int)(etitle - title - 1), title,
			which == 1 ? fn : on,
			which == 1 ? on : fn);
	switch (opt) {
	case D_NORMAL:
		putNnorm(op, fn, on, fp, lines, which);
		break;
	case D_EDIT:
		putNedit(op, fn, on, fp, lines, which, 0);
		break;
	case D_REVERSE:
		putNedit(op, fn, on, fp, lines, which, 1);
		break;
	case D_CONTEXT:
		putNcntx(op, fn, on, mtime, fp, lines, which);
		break;
	case D_NREVERSE:
		putNedit(op, fn, on, fp, lines, which, 2);
		break;
	case D_UNIFIED:
		putNunif(op, fn, on, mtime, fp, lines, which);
		break;
	}
	if (lflag) {
		fclose(op);
		while (wait(NULL) != pid);
		sigset(SIGPIPE, opipe);
	}
out:	fclose(fp);
}

static void
putNnorm(FILE *op, const char *fn, const char *on,
		FILE *fp, long long lines, int which)
{
	int	pfx;

	if (which == 1) {
		fprintf(op, "1,%lldd0\n", lines);
		pfx = '<';
	} else {
		fprintf(op, "0a1,%lld\n", lines);
		pfx = '>';
	}
	putNdata(op, fp, pfx, ' ');
}

static void
putNedit(FILE *op, const char *fn, const char *on,
		FILE *fp, long long lines, int which, int reverse)
{
	switch (reverse) {
	case 0:
		if (which == 1)
			fprintf(op, "1,%lldd\n", lines);
		else {
			fprintf(op, "0a\n");
			putNdata(op, fp, 0, 0);
			fprintf(op, ".\n");
		}
		break;
	case 1:
		if (which == 1)
			fprintf(op, "d1 %lld\n", lines);
		else {
			fprintf(op, "a0\n");
			putNdata(op, fp, 0, 0);
			fprintf(op, ".\n");
		}
		break;
	case 2:
		if (which == 1)
			fprintf(op, "d1 %lld\n", lines);
		else {
			fprintf(op, "a0 %lld\n", lines);
			putNdata(op, fp, 0, 0);
		}
		break;
	}
}

static void
putNcntx(FILE *op, const char *fn, const char *on, time_t mtime,
		FILE *fp, long long lines, int which)
{
	putNhead(op, fn, on, mtime, which, "***", "---");
	fprintf(op, "***************\n*** ");
	if (which == 1)
		fprintf(op, "1,%lld", lines);
	else
		putc('0', op);
	fprintf(op, " ****\n");
	if (which != 1)
		fprintf(op, "--- 1,%lld ----\n", lines);
	putNdata(op, fp, which == 1 ? '-' : '+', ' ');
	if (which == 1)
		fprintf(op, "--- 0 ----\n");
}

static void
putNunif(FILE *op, const char *fn, const char *on, time_t mtime,
		FILE *fp, long long lines, int which)
{
	putNhead(op, fn, on, mtime, which, "---", "+++");
	fprintf(op, "@@ ");
	fprintf(op, which == 1 ? "-1,%lld +0,0" : "-0,0 +1,%lld", lines);
	fprintf(op, " @@\n");
	putNdata(op, fp, which == 1 ? '-' : '+', 0);
}

static void
putNhead(FILE *op, const char *fn, const char *on, time_t mtime, int which,
		const char *p1, const char *p2)
{
	time_t	t1, t2;
	const char	*f1, *f2;

	t1 = which == 1 ? mtime : 0;
	t2 = which == 1 ? 0 : mtime;
	f1 = which == 1 ? fn : on;
	f2 = which == 1 ? on : fn;
	fprintf(op, "%s %s\t%s", p1, f1, ctime(&t1));
	fprintf(op, "%s %s\t%s", p2, f2, ctime(&t2));
}

static void
putNdata(FILE *op, FILE *fp, int pfx, int sec)
{
	int	c, lastc = '\n', col = 0;

	while ((c = getc(fp)) != EOF) {
		if (lastc == '\n') {
			col = 0;
			if (pfx)
				putc(pfx, op);
			if (sec)
				putc(sec, op);
		}
		if (c == '\t' && tflag) {
			do
				putc(' ', op);
			while (++col & 7);
		} else {
			putc(c, op);
			col++;
		}
		lastc = c;
	}
	if (lastc != '\n') {
		if (aflag)
			fprintf(op, "\n\\ No newline at end of file\n");
		else
			putc('\n', op);
	}
}

static void
putNdir(const char *fn, const char *on, int which)
{
	DIR	*Dp;
	struct dirent	*dp;

	if ((Dp = opendir(fn)) == NULL) {
		fprintf(stderr, "%s: %s: %s\n", progname, fn, strerror(errno));
		status = 2;
		return;
	}
	while ((dp = readdir(Dp)) != NULL) {
		if (dp->d_name[0] == '.' && (dp->d_name[1] == '\0' ||
					dp->d_name[1] == '.' &&
					dp->d_name[2] == '\0'))
			continue;
		if (xflag && xclude(dp->d_name))
			continue;
		putN(fn, on, dp->d_name, which);
	}
	closedir(Dp);
}

static long long
linec(const char *fn, FILE *fp)
{
	int	c, lastc = '\n';
	long long	cnt = 0;

	while ((c = getc(fp)) != EOF) {
		if (c == '\n')
			cnt++;
		lastc = c;
	}
	if (lastc != '\n') {
		if (!aflag)
			fprintf(stderr,
				"Warning: missing newline at end of file %s\n",
				fn);
		cnt++;
	}
	return cnt;
}

static char *
mkpath(const char *dir, const char *file)
{
	char	*path, *pp;
	const char	*cp;

	pp = path = talloc(strlen(dir) + strlen(file) + 2);
	for (cp = dir; *cp; cp++)
		*pp++ = *cp;
	if (pp > path && pp[-1] != '/')
		*pp++ = '/';
	for (cp = file; *cp; cp++)
		*pp++ = *cp;
	*pp = '\0';
	return path;
}

static void
mktitle(void)
{
	int	n;

	n = strlen(file1) + strlen(file2) + 2;
	if (etitle - title + n < titlesize) {
		titlesize = n;
		n = etitle - title;
		title = ralloc(title, titlesize);
		etitle = &title[n];
	}
	sprintf(etitle, "%s %s", file1, file2);
}

static int
xclude(const char *fn)
{
	extern int	gmatch(const char *, const char *);
	struct xclusion	*xp;

	for (xp = xflag; xp; xp = xp->x_nxt)
		if (gmatch(fn, xp->x_pat))
			return 1;
	return 0;
}
