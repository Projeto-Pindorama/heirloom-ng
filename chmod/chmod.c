/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, March 2003.
 */
/*	from Unix 7th Edition /usr/src/cmd/chmod.c	*/
/*
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
#if defined (SUS)
static const char sccsid[] USED = "@(#)chmod_sus.sl	1.10 (gritter) 5/29/05";
#else
static const char sccsid[] USED = "@(#)chmod.sl	1.10 (gritter) 5/29/05";
#endif
/*
 * chmod [-R] [ugoa][+-=][rwxXlstugo] files
 *  change mode of files
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdlib.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define	USER	05700	/* user's bits */
#define	GROUP	02070	/* group's bits */
#define	OTHER	00007	/* other's bits */
#define	ALL	07777	/* all */

#define	READ	00444	/* read permit */
#define	WRITE	00222	/* write permit */
#define	EXEC	00111	/* exec permit */
#define	SETID	06000	/* set[ug]id */
#define	STICKY	01000	/* sticky bit */

#ifndef	S_ENFMT
#define	S_ENFMT	02000	/* mandatory locking bit */
#endif

static mode_t	um;
static struct	stat st;
static char	*progname;
static int	errcnt;
static int	Rflag;

static void	chng(const char *, const char *);
static mode_t	newmode(const char *, const mode_t, const char *);
static mode_t	absol(const char **);
static mode_t	who(const char **, mode_t *);
static int	what(const char **);
static mode_t	where(const char **, mode_t, int *, int *, const mode_t);
static void	*srealloc(void *, size_t);
static void	*smalloc(size_t);
static void	getpath(const char *, char **, char **, size_t *, size_t *);
static void	setpath(const char *, char **, char **,
			size_t, size_t *, size_t *);
static void	warning(int, const char *, ...);
static void	error(int, const char *, ...);
static void	usage(void);
static void	corresp(const char *);
static void	execreq(const char *);

int
main(int argc, char **argv)
{
	register int	i;
	const char	*ms;

	progname = basename(argv[0]);
	while ((i = getopt(argc, argv, ":R")) != EOF) {
		switch (i) {
		case 'R':
			Rflag = 1;
			break;
		default:
			optind--;
			goto end;
		}
	}
end:	if (argc - optind < 2)
		usage();
	ms = argv[optind];
#ifdef	SUS
	umask(um = umask(0));
#endif	/* SUS */
	newmode(ms, 0, NULL);
	for (i = optind + 1; i < argc; i++)
		chng(ms, argv[i]);
	exit(errcnt > 253 ? 253 : errcnt);
}

static void
chng(const char *ms, const char *fn)
{
	int	symlink;

	if (lstat(fn, &st) < 0 ||
			(symlink = ((st.st_mode&S_IFMT) == S_IFLNK)) &&
			stat(fn, &st) < 0) {
		warning(1, "can't access %s", fn);
		return;
	}
	if (chmod(fn, newmode(ms, st.st_mode, fn)) < 0) {
		warning(1, "can't change %s", fn);
		return;
	}
	if (Rflag && symlink == 0 && (st.st_mode&S_IFMT) == S_IFDIR) {
		DIR	*Dp;
		struct dirent	*dp;
		char	*copy, *cend;
		size_t	sz, slen, ss;

		if ((Dp = opendir(fn)) == NULL) {
			warning(1, "%s", strerror(errno));
			return;
		}
		getpath(fn, &copy, &cend, &sz, &slen);
		while ((dp = readdir(Dp)) != NULL) {
			if (dp->d_name[0] == '.' && (dp->d_name[1] == '\0' || (
					dp->d_name[1] == '.' &&
					dp->d_name[2] == '\0')))
				continue;
			setpath(dp->d_name, &copy, &cend, slen, &sz, &ss);
			chng(ms, copy);
		}
		free(copy);
		closedir(Dp);
	}
}

static mode_t
newmode(const char *ms, const mode_t pm, const char *fn)
{
	register mode_t	o, m, b;
	int	lock, setsgid = 0, didprc = 0, cleared = 0, copy = 0;
	mode_t	nm, om, mm;

	nm = om = pm;
	m = absol(&ms);
	if (!*ms) {
		nm = m;
		goto out;
	}
	if ((lock = (nm&S_IFMT) != S_IFDIR && (nm&(S_ENFMT|S_IXGRP)) == S_ENFMT)
			== 01)
		nm &= ~(mode_t)S_ENFMT;
	do {
		m = who(&ms, &mm);
		while (o = what(&ms)) {
			b = where(&ms, nm, &lock, &copy, pm);
			switch (o) {
			case '+':
				nm |= b & m & ~mm;
				if (b & S_ISGID)
					setsgid = 1;
				if (lock & 04)
					lock |= 02;
				break;
			case '-':
				nm &= ~(b & m & ~mm);
				if (b & S_ISGID)
					setsgid = 1;
				if (lock & 04)
					lock = 0;
				break;
			case '=':
				nm &= ~m;
				nm |= b & m & ~mm;
				lock &= ~01;
				if (lock & 04)
					lock |= 02;
				om = 0;
				if (copy == 0)
					cleared = 1;
				break;
			}
			lock &= ~04;
		}
	} while (*ms++ == ',');
	if (*--ms)
		error(255, "invalid mode");
	if (lock) {
		if (om & S_IXGRP) {
			warning(0, "Locking not permitted on "
				"%s, a group executable file",
				fn);
			return pm;
		} else if ((lock & 02) && (nm & S_IXGRP))
			error(3, "Group execution and locking "
				"not permitted together");
		else if ((lock & 01) && (nm & S_IXGRP)) {
			warning(0, "Group execution not "
				"permitted on %s, a "
				"lockable file", fn);
			return pm;
		} else if ((lock & 02) && (nm & S_ISGID))
			error(3, "Set-group-ID and locking not "
				"permitted together");
		else if ((lock & 01) && (nm & S_ISGID)) {
			warning(0, "Set-group-ID not permitted "
				"on %s, a lockable file", fn);
			return pm;
		}
		nm |= S_ENFMT;
	} else /* lock == 0 */ if ((pm&S_IFMT) != S_IFDIR) {
		if ((om & (S_IXGRP|S_ISGID)) == (S_IXGRP|S_ISGID) &&
				(nm & (S_IXGRP|S_ISGID)) == S_ISGID) {
			corresp(fn);
			didprc = 1;
			nm &= ~(mode_t)S_ISGID;
		} else if ((nm & (S_ISGID|S_IXGRP)) == S_ISGID) {
			if (fn || cleared) {
				execreq(fn);
				return pm;
			}
		}
	}
	if ((pm&S_IFMT) != S_IFDIR) {
		if ((om & (S_IXUSR|S_ISUID)) == (S_IXUSR|S_ISUID) &&
				(nm & (S_IXUSR|S_ISUID)) == S_ISUID) {
			if (didprc == 0)
				corresp(fn);
			nm &= ~(mode_t)S_ISUID;
		} else if ((nm & (S_ISUID|S_IXUSR)) == S_ISUID) {
			if (fn || cleared) {
				execreq(fn);
				return pm;
			}
		}
	}
out:	if ((pm&S_IFMT) == S_IFDIR) {
		if ((pm & S_ISGID) && setsgid == 0)
			nm |= S_ISGID;
		else if ((nm & S_ISGID) && setsgid == 0)
			nm &= ~(mode_t)S_ISGID;
	}
	return(nm);
}

static mode_t
absol(const char **ms)
{
	register int c, i;

	i = 0;
	while ((c = *(*ms)++) >= '0' && c <= '7')
		i = (i << 3) + (c - '0');
	(*ms)--;
	return(i);
}

static mode_t
who(const char **ms, mode_t *mp)
{
	register int m;

	m = 0;
	*mp = 0;
	for (;;) switch (*(*ms)++) {
	case 'u':
		m |= USER;
		continue;
	case 'g':
		m |= GROUP;
		continue;
	case 'o':
		m |= OTHER;
		continue;
	case 'a':
		m |= ALL;
		continue;
	default:
		(*ms)--;
		if (m == 0) {
			m = ALL;
			*mp = um;
		}
		return m;
	}
}

static int
what(const char **ms)
{
	switch (**ms) {
	case '+':
	case '-':
	case '=':
		return *(*ms)++;
	}
	return(0);
}

static mode_t
where(const char **ms, mode_t om, int *lock, int *copy, const mode_t pm)
{
	register mode_t m;

	m = 0;
	*copy = 0;
	switch (**ms) {
	case 'u':
		m = (om & USER) >> 6;
		goto dup;
	case 'g':
		m = (om & GROUP) >> 3;
		goto dup;
	case 'o':
		m = (om & OTHER);
	dup:
		*copy = 1;
		m &= (READ|WRITE|EXEC);
		m |= (m << 3) | (m << 6);
		++(*ms);
		return m;
	}
	for (;;) switch (*(*ms)++) {
	case 'r':
		m |= READ;
		continue;
	case 'w':
		m |= WRITE;
		continue;
	case 'x':
		m |= EXEC;
		continue;
	case 'X':
		if ((pm&S_IFMT) == S_IFDIR || (pm & EXEC))
			m |= EXEC;
		continue;
	case 'l':
		if ((pm&S_IFMT) != S_IFDIR)
			*lock |= 04;
		continue;
	case 's':
		m |= SETID;
		continue;
	case 't':
		m |= STICKY;
		continue;
	default:
		(*ms)--;
		return m;
	}
}

static void *
srealloc(void *op, size_t nbytes)
{
	void	*np;

	if ((np = realloc(op, nbytes)) == NULL) {
		write(2, "no memory\n", 10);
		_exit(254);
	}
	return np;
}

static void *
smalloc(size_t nbytes)
{
	return srealloc(NULL, nbytes);
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
		const char	*cp = path;
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
warning(int val, const char *fmt, ...)
{
	va_list	ap;

	fprintf(stderr, "%s: WARNING: ", progname);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
	errcnt += val;
}

static void
error(int val, const char *fmt, ...)
{
	va_list	ap;

	fprintf(stderr, "%s: ERROR: ", progname);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
	exit(val > 1 ? val : errcnt > 253 ? 253 : errcnt);
}

static void
usage(void)
{
	warning(2, "Usage: %s [-R] [ugoa][+-=][rwxXlstugo] file ...",
			progname);
	exit(255);
}

static void
corresp(const char *fn)
{
	warning(0, "Corresponding set-ID also disabled on %s "
			"since set-ID requires execute permission", fn);
}

static void
execreq(const char *fn)
{
	if (fn)
		warning(0, "Execute permission required for "
				"set-ID on execution for %s", fn);
	else
		error(2, "Execute permission required for set-ID on execution");
}
