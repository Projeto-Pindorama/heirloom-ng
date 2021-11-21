/*
 * chown - change file owner
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
#ifdef	UCB
static const char sccsid[] USED = "@(#)/usr/ucb/chown.sl	1.14 (gritter) 5/29/05";
#else
static const char sccsid[] USED = "@(#)chown.sl	1.14 (gritter) 5/29/05";
#endif

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>
#include	<limits.h>
#include	<stdarg.h>
#include	<dirent.h>
#include	<pwd.h>
#include	<grp.h>

#ifdef	UCB
#define	SEPCHAR	'.'
#else	/* !UCB */
#define	SEPCHAR	':'
#endif	/* !UCB */

static enum {
	PERS_CHOWN,
	PERS_CHGRP
} pers = PERS_CHOWN;

static unsigned	errcnt;			/* count of errors */
static int	fflag;			/* force (don't report errors) */
static int	hflag;			/* use lchown */
static int	Rflag;			/* recursive */
static int	HLPflag;		/* -H, -L, -P */
static char	*progname;		/* argv[0] to main() */
static char	*owner;
static char	*group;
static uid_t	myeuid;
static uid_t	new_uid;
static gid_t	new_gid;

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

static int
eprintf(int always, const char *fmt, ...)
{
	va_list	ap;
	int	ret = 0;

	if (always || fflag == 0) {
		va_start(ap, fmt);
		ret = vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
	errcnt |= 1;
	return ret;
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

static void
dochown(const char *path, int level)
{
	struct stat ost, nst, lst;
	int	(*chownfn)(const char *, uid_t, gid_t);
	int	(*statfn)(const char *, struct stat *);

	chownfn = hflag || Rflag && HLPflag == 'P' ? lchown : chown;
	statfn = hflag || Rflag && HLPflag == 'P' ? lstat : stat;
	if (statfn(path, &ost) < 0) {
		eprintf(0, "%s: cannot access %s: %s\n",
				progname, path, strerror(errno));
		return;
	}
	if (chownfn(path, owner ? new_uid : ost.st_uid,
				group ? new_gid : ost.st_gid) < 0) {
		eprintf(0, "%s: cannot chown %s: %s\n",
				progname, path, strerror(errno));
		goto failed;
	}
	if ((ost.st_mode & S_ISUID) || (ost.st_mode & (S_ISGID|S_IXGRP))
				== (S_ISGID|S_IXGRP)) {
		if (statfn(path, &nst) < 0) {
			eprintf(0, "%s: cannot access %s after chmod: %s\n",
				progname, path, strerror(errno));
			return;
		}
		if (myeuid && ((nst.st_mode & S_ISUID) ||
					(nst.st_mode & (S_ISGID|S_IXGRP))
					== (S_ISGID|S_IXGRP))) {
			if (chmod(path, ost.st_mode & 07777 &
						~(S_ISUID|S_ISGID)) < 0) {
				eprintf(0,
				"%s: cannot clear suid/sgid bits of %s: %s\n",
					progname, path, strerror(errno));
			}
		} else if (myeuid == 0 && (ost.st_mode & 07777) !=
				(nst.st_mode & 07777)) {
			if (chmod(path, ost.st_mode & 07777) < 0) {
				eprintf(0, "%s: cannot reset modes of %s: %s\n",
					progname, path, strerror(errno));
			}
		}
	} else
	failed:	nst = ost;
	if (Rflag == 0)
		return;
	if (statfn != lstat && (HLPflag != 'H' || level > 0)
			&& HLPflag != 'L') {
		if (lstat(path, &lst) < 0) {
			eprintf(0, "%s: cannot lstat %s: %s\n",
					progname, path, strerror(errno));
			return;
		}
	} else
		lst = nst;
	if (S_ISDIR(lst.st_mode)) {
		DIR *Dp;
		struct dirent *dp;
		char *copy, *cend;
		size_t sz, slen, ss;

		if ((Dp = opendir(path)) == NULL) {
			eprintf(0, "%s: cannot open directory %s: %s\n",
					progname, path, strerror(errno));
			return;
		}
		getpath(path, &copy, &cend, &sz, &slen);
		while ((dp = readdir(Dp)) != NULL) {
			if (dp->d_name[0] == '.' && (dp->d_name[1] == '\0' ||
						(dp->d_name[1] == '.' &&
						 dp->d_name[2] == '\0')))
				continue;
			setpath(dp->d_name, &copy, &cend, slen, &sz, &ss);
			dochown(copy, level + 1);
		}
		free(copy);
		closedir(Dp);
	}
}

static long
numeric(const char *name, const char *type)
{
	char *x;
	long n;

	n = strtol(name, &x, 10);
	if (*x != '\0' || *name == '-' || *name == '+') {
		eprintf(1, "%s: unknown %s %s\n", progname, type, name);
		exit(2);
	}
	return n;
}

static void
getowner(char *name)
{
	if (pers == PERS_CHOWN) {
		owner = name;
		if ((group = strchr(name, SEPCHAR)) != 0)
			*group++ = '\0';
	} else if (pers == PERS_CHGRP)
		group = name;
	if (owner) {
		struct passwd *pwd;

		if ((pwd = getpwnam(owner)) == NULL)
			new_uid = numeric(owner, "user id");
		else
			new_uid = pwd->pw_uid;
	}
	if (group) {
		struct group *grp;

		if ((grp = getgrnam(group)) == NULL)
			new_gid = numeric(group, "group:");
		else
			new_gid = grp->gr_gid;
	}
}

static void
usage(void)
{
	switch (pers) {
	case PERS_CHOWN:
#ifdef	UCB
		eprintf(1, "usage: %s [-fhR] owner[.group] file ...\n",
				progname);
#else	/* !UCB */
		eprintf(1, "usage: %s [-h] [-R] uid file ...\n",
				progname);
#endif	/* !UCB */
		break;
	case PERS_CHGRP:
		eprintf(1, "%s: usage: %s [-h] [-R] gid file...\n",
				progname, progname);
		break;
	}
	exit(2);
}

int
main(int argc, char **argv)
{
#ifdef	UCB
	const char	optstring[] = "fhHLPR";
#else
	const char	optstring[] = "hHLPR";
#endif
	int i;

#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	progname = basename(argv[0]);
	if (!strncmp(progname, "chgrp", 5))
		pers = PERS_CHGRP;
	while ((i = getopt(argc, argv, optstring)) != EOF) {
		switch (i) {
		case 'f':
			fflag = 1;
			break;
		case 'h':
			hflag = 1;
			break;
		case 'R':
			Rflag = 1;
			break;
		case 'H':
		case 'L':
		case 'P':
			HLPflag = i;
			break;
		default:
			usage();
		}
	}
	if (optind >= argc)
		usage();
	getowner(argv[optind++]);
	if (optind >= argc)
		usage();
	myeuid = geteuid();
	while (optind < argc)
		dochown(argv[optind++], 0);
	return errcnt;
}
