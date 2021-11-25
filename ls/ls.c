/*	ls 4.1 - List files.				Author: Kees J. Bot
 *								25 Apr 1989
 */

/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, October 2001. Taken
 * from the MINIX sources:
 */

/*
 * Copyright (c) 1987,1997, Prentice Hall All rights reserved.
 * 
 * Redistribution and use of the MINIX operating system in source and binary
 * forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * Neither the name of Prentice Hall nor the names of the software authors or
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS, AUTHORS, AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL PRENTICE HALL OR ANY
 * AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
#if defined (SU3)
static const char sccsid[] USED = "@(#)ls_su3.sl	1.78 (gritter) 4/17/07>";
#elif defined (SUS)
static const char sccsid[] USED = "@(#)ls_sus.sl	1.78 (gritter) 4/17/07>";
#elif defined (UCB)
static const char sccsid[] USED = "@(#)/usr/ucb/ls.sl	1.78 (gritter) 4/17/07>";
#else
static const char sccsid[] USED = "@(#)ls.sl	1.78 (gritter) 4/17/07>";
#endif

/*
 * About the amount of bytes for heap + stack under Minix:
 * Ls needs a average amount of 42 bytes per unserviced directory entry, so
 * scanning 10 directory levels deep in an ls -R with 100 entries per directory
 * takes 42000 bytes of heap.  So giving ls 10000 bytes is tight, 20000 is
 * usually enough, 40000 is pessimistic.
 */

/* The array ifmt_c[] is used in an 'ls -l' to map the type of a file to a
 * letter.  This is done so that ls can list any future file or device type
 * other than symlinks, without recompilation.  (Yes it's dirty.)
 */
static char ifmt_c[] = "-pc-d-b--nl-SD--";
/*                       S_IFIFO
 *                        S_IFCHR
 *                          S_IFDIR
 *                            S_IFBLK
 *                              S_IFREG
 *                               S_IFNWK
 *                                S_IFLNK
 *                                  S_IFSOCK
 *                                   S_IFDOOR
 */

#define ifmt(mode)	ifmt_c[((mode) >> 12) & 0xF]

#ifndef	USE_TERMCAP
#ifdef	sun
#include <curses.h>
#include <term.h>
#endif
#endif
#define nil 0
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <locale.h>
#include <limits.h>
#include <ctype.h>
#include <libgen.h>
#include <wchar.h>
#include <wctype.h>
#include "config.h"
#ifndef	USE_TERMCAP
#ifndef	sun
#include <curses.h>
#include <term.h>
#endif
#else	/* USE_TERMCAP */
#include <termcap.h>
#endif	/* USE_TERMCAP */

#ifdef	_AIX
#include <sys/sysmacros.h>
#endif

#ifndef	S_IFNAM
#define	S_IFNAM		0x5000	/* XENIX special named file */
#endif
#ifndef	S_INSEM
#define	S_INSEM		0x1	/* XENIX semaphore subtype of IFNAM */
#endif
#ifndef	S_INSHD
#define	S_INSHD		0x2	/* XENIX shared data subtype of IFNAM */
#endif
#ifndef	S_IFDOOR
#define	S_IFDOOR	0xD000	/* Solaris door */
#endif
#ifndef	S_IFNWK
#define	S_IFNWK		0x9000	/* HP-UX network special */
#endif

#if !__minix
#define SUPER_ID	uid	/* Let -A flag be default for SUPER_ID == 0. */
#else
#define SUPER_ID	gid
#endif

#if __minix
#define BLOCK	1024
#else
#define BLOCK	 512
#endif

/* Assume other systems have st_blocks. */
#if !__minix
#define ST_BLOCKS 1
#endif

/* Some terminals ignore more than 80 characters on a line.  Dumb ones wrap
 * when the cursor hits the side.  Nice terminals don't wrap until they have
 * to print the 81st character.  Wether we like it or not, no column 80.
 */
static int ncols = 79;

#define NSEP	2	/* # spaces between columns. */

#define MAXCOLS	150	/* Max # of files per line. */

static char *arg0;	/* Last component of argv[0]. */
static int uid, gid;	/* callers id. */
static int ex = 0;	/* Exit status to be. */
static int istty;	/* Output is on a terminal. */
static int tinfostat = -1;	/* terminfo is initalized */
extern int sysv3;	/* emulate SYSV3 behavior */

#ifdef	SU3
static struct	visit {
	ino_t	v_ino;
	dev_t	v_dev;
} *visited;
static int	vismax;	/* number of members in visited */
#endif	/* SU3 */

static enum {
	PER_LS	= 0,
	PER_DIR	= 1
} personality = PER_LS;

static struct {
	const char	*per_opt;
} personalities[] = {
	/* orig: acdfgilnqrstu1ACFLMRTX */
#ifdef	UCB
	{ ":1RaAdClgrtucFqisfLSUXhH" },	/* PER_LS */
#else	/* !UCB */
	{ "1RadCxmnlogrtucpFbqisfLSUXhH" },	/* PER_LS */
#endif	/* !UCB */
	{ "1RadCxmnlogrtucpFbqiOfLSXhH" }	/* PER_DIR */
};

/* Safer versions of malloc and realloc: */

static void
heaperr(void)
{
	write(2, "out of memory\n", 14);
	exit(077);
}

/*
 * Deliver or die.
 */
static void *
salloc(size_t n)
{
	void *a;

	if ((a = malloc(n)) == nil)
		heaperr();
	return a;
}

static void *
srealloc(void *a, size_t n)
{
	if ((a = realloc(a, n)) == nil)
		heaperr();
	return a;
}

static char flags[0477];

static int
present(int f)
{
	return f == 0 || flags[f] != 0;
}

/*
 * Like perror(3), but in the style: "ls: junk: No such file or directory.
 */
static void
report(const char *f)
{
#ifdef	UCB
	fprintf(stderr, "%s not found\n", f);
#else	/* !UCB */
	fprintf(stderr, "%s: %s\n", f, strerror(errno));
#endif	/* !UCB */
	ex = 1;
}

#ifdef	UCB
#define	blockcount(n)	(((n) & 1 ? (n)+1 : (n)) >> 1)
#else
#define	blockcount(n)	(n)
#endif

/*
 * Two functions, uidname and gidname, translate id's to readable names.
 * All names are remembered to avoid searching the password file.
 */
#define NNAMES	(1 << (sizeof(int) + sizeof(char *)))
enum whatmap { PASSWD, GROUP };

static struct idname {		/* Hash list of names. */
	struct idname	*next;
	char		*name;
	uid_t		id;
} *uids[NNAMES], *gids[NNAMES];

/*
 * Return name for a given user/group id.
 */
static char *
idname(unsigned id, enum whatmap map)
{
	struct idname *i;
	struct idname **ids = &(map == PASSWD ? uids : gids)[id % NNAMES];

	while ((i = *ids) != nil && id < i->id)
		ids = &i->next;
	if (i == nil || id != i->id) {
		/* Not found, go look in the password or group map. */
		char *name = nil;
		char noname[3 * sizeof(uid_t)];

		if (!present('n')) {
			if (map == PASSWD) {
				struct passwd *pw = getpwuid(id);

				if (pw != nil) name = pw->pw_name;
			} else {
				struct group *gr = getgrgid(id);

				if (gr != nil) name = gr->gr_name;
			}
		}
		if (name == nil) {
			/* Can't find it, weird.  Use numerical "name." */
			sprintf(noname, "%lu", (long)id);
			name = noname;
		}
		/* Add a new id-to-name cell. */
		i = salloc(sizeof(*i));
		i->id = id;
		i->name = salloc(strlen(name) + 1);
		strcpy(i->name, name);
		i->next = *ids;
		*ids = i;
	}
	return i->name;
}

#define uidname(uid)	idname((uid), PASSWD)
#define gidname(gid)	idname((gid), GROUP)

/*
 * Path name construction, addpath adds a component, delpath removes it.
 * The string path is used throughout the program as the file under examination.
 */

static char *path;	/* Path name constructed in path[]. */
static int plen = 0, pidx = 0;	/* Lenght/index for path[]. */

/*
 * Add a component to path. (name may also be a full path at the first call)
 * The index where the current path ends is stored in *pdi.
 */
static void
addpath(int *didx, char *name)
{
	if (plen == 0)
		path = salloc((plen = 32) * sizeof(path[0]));
	if (pidx == 1 && path[0] == '.')
		pidx = 0;	/* Remove "." */
	*didx = pidx;	/* Record point to go back to for delpath. */
	if (pidx > 0 && path[pidx-1] != '/')
		path[pidx++] = '/';
	do {
		if (*name != '/' || pidx == 0 || path[pidx-1] != '/') {
			if (pidx == plen) {
				path = srealloc(path,
						(plen *= 2) * sizeof(path[0]));
			}
			path[pidx++] = *name;
		}
	} while (*name++ != 0);
	--pidx;		/*
			 * Put pidx back at the null.  The path[pidx++] = '/'
			 * statement will overwrite it at the next call.
			 */
}

#define delpath(didx)	(path[pidx = didx] = 0)	/* Remove component. */

static int field = 0;	/* (used to be) Fields that must be printed. */
		/* (now) Effects triggered by certain flags. */

#define FL_INODE	0x001	/* -i */
#define FL_BLOCKS	0x002	/* -s */
#define FL_UNUSED	0x004
#define FL_MODE		0x008	/* -lMX */
#define FL_LONG		0x010	/* -l */
#define FL_GROUP	0x020	/* -g */
#define FL_BYTIME	0x040	/* -tuc */
#define FL_ATIME	0x080	/* -u */
#define FL_CTIME	0x100	/* -c */
#define FL_MARK		0x200	/* -F */
/*#define FL_UNUSED	0x400	*/
#define FL_DIR		0x800	/* -d */
#define	FL_OWNER	0x1000	/* -o */
#define	FL_STATUS	0x2000

#define	ENDCOL		0x001	/* last printable column in -x mode */

struct file {		/* A file plus stat(2) information. */
	off_t		size;
#if ST_BLOCKS
	blkcnt_t	blocks;
#endif
	ino_t		ino;
	struct file	*next;	/* Lists are made of them. */
	struct file	*sord;	/* Saved order of list. */
	char		*name;	/* Null terminated name. */
	time_t		mtime;
	time_t		atime;
	time_t		ctime;
	mode_t		mode;
	int		flag;
	uid_t		uid;
	gid_t		gid;
	dev_t		rdev;
	nlink_t		nlink;
	unsigned short	namlen;
};

static void
setstat(struct file *f, struct stat *stp)
{
	f->ino =	stp->st_ino;
	f->mode =	stp->st_mode;
	f->nlink =	stp->st_nlink;
	f->uid =	stp->st_uid;
	f->gid =	stp->st_gid;
	f->rdev =	stp->st_rdev;
	f->size =	stp->st_size;
	f->mtime =	stp->st_mtime;
	f->atime =	stp->st_atime;
	f->ctime =	stp->st_ctime;
#if ST_BLOCKS
	f->blocks =	stp->st_blocks;
#ifdef	__hpux
	f->blocks <<= 1;
#endif
#endif
}

#define	PAST	(26*7*24*3600L)	/* Half a year ago. */
/* Between PAST and FUTURE from now a time is printed, otherwise a year. */
#define FUTURE	(15*60L)	/* Fifteen minutes. */

/*
 * Transform the right time field into something readable.
 */
static char *
timestamp(struct file *f)
{
	struct tm *tm;
	time_t t;
	static time_t now;
	static int drift = 0;
	static char date[32];

	t = f->mtime;
	if (field & FL_ATIME)
		t = f->atime;
	if (field & FL_CTIME)
		t = f->ctime;
	tm = localtime(&t);
	if (--drift < 0) {
		time(&now);
		drift = 50;
	}	/* limit time() calls */
	if (t < now - PAST || t > now + FUTURE) {
		strftime(date, sizeof date - 1, "%b %e  %Y", tm);
	} else {
		strftime(date, sizeof date - 1, "%b %e %H:%M", tm);
	}
	return date;
}

/*
 * Compute long or short rwx bits.
 */
static char *
permissions(struct file *f)
{
	/*
	 * Note that rwx[0] is a guess for the more alien file types.  It is
	 * correct for BSD4.3 and derived systems.  I just don't know how
	 * "standardized" these numbers are.
	 */
	static char rwx[] = "drwxr-x--x ";
	char *p = rwx+1;
	int mode = f->mode;

	rwx[0] = ifmt(f->mode);
	if ((f->mode & S_IFMT) == S_IFNAM) {
		if (f->rdev == S_INSEM)
			rwx[0] = 's';
		else if (f->rdev == S_INSHD)
			rwx[0] = 'm';
		else
			rwx[0] = '?';
	}
	do {
		p[0] = (mode & S_IRUSR) ? 'r' : '-';
		p[1] = (mode & S_IWUSR) ? 'w' : '-';
		p[2] = (mode & S_IXUSR) ? 'x' : '-';
		mode <<= 3;
	} while ((p += 3) <= rwx + 7);
	if (f->mode&S_ISUID)
		rwx[3] = f->mode&(S_IXUSR>>0) ? 's' : 'S';
	if (f->mode&S_ISGID)
		rwx[6] = f->mode&(S_IXGRP>>0) ? 's' :
#ifndef	UCB
			(f->mode & S_IFMT) != S_IFDIR ?
#if defined (SUS) || defined (SU3)
			'L'
#else	/* !SUS, !SU3 */
			'l'
#endif	/* !SUS, !SU3 */
			:
#endif	/* !UCB */
			'S'
			;
	if (f->mode&S_ISVTX)
		rwx[9] = f->mode&(S_IXUSR>>6) ? 't' : 'T';
	/*
	 * rwx[10] would be the "optional alternate access method flag",
	 * leave as a space for now.
	 */
	return rwx;
}

#ifdef	notdef
void
numeral(int i, char **pp)
{
	char itoa[3*sizeof(int)], *a = itoa;

	do
		*a++ = i % 10 + '0';
	while ((i /= 10) > 0);
	do
		*(*pp)++ = *--a;
	while (a > itoa);
}
#endif

static char *
extension(const char *fn)
{
	const char	*ep = "";

	while (*fn++)
		if (*fn == '.')
			ep = &fn[1];
	return (char *)ep;
}

static int (*CMP)(struct file *f1, struct file *f2);
static int (*rCMP)(struct file *f1, struct file *f2);

/*
 * This is either a stable mergesort, or thermal noise, I'm no longer sure.
 * It must be called like this: if (L != nil && L->next != nil) mergesort(&L);
 */
static void
_mergesort(struct file **al)
{
	/* static */ struct file *l1, **mid;  /* Need not be local */
	struct file *l2;

	l1 = *(mid = &(*al)->next);
	do {
		if ((l1 = l1->next) == nil)
			break;
		mid = &(*mid)->next;
	} while ((l1 = l1->next) != nil);
	l2 = *mid;
	*mid = nil;
	if ((*al)->next != nil)
		_mergesort(al);
	if (l2->next != nil)
		_mergesort(&l2);
	l1 = *al;
	for (;;) {
		if ((*CMP)(l1, l2) <= 0) {
			if ((l1 = *(al = &l1->next)) == nil) {
				*al = l2;
				break;
			}
		} else {
			*al = l2;
			l2 = *(al = &l2->next);
			*al = l1;
			if (l2 == nil)
				break;
		}
	}
}

static int
namecmp(struct file *f1, struct file *f2)
{
	return strcoll(f1->name, f2->name);
}

static int
extcmp(struct file *f1, struct file *f2)
{
	return strcoll(extension(f1->name), extension(f2->name));
}

static int
mtimecmp(struct file *f1, struct file *f2)
{
	return f1->mtime == f2->mtime ? 0 : f1->mtime > f2->mtime ? -1 : 1;
}

static int
atimecmp(struct file *f1, struct file *f2)
{
	return f1->atime == f2->atime ? 0 : f1->atime > f2->atime ? -1 : 1;
}

static int
ctimecmp(struct file *f1, struct file *f2)
{
	return f1->ctime == f2->ctime ? 0 : f1->ctime > f2->ctime ? -1 : 1;
}

static int
sizecmp(struct file *f1, struct file *f2)
{
	return f1->size == f2->size ? 0 : f1->size > f2->size ? -1 : 1;
}

static int
revcmp(struct file *f1, struct file *f2) {
	return (*rCMP)(f2, f1);
}

/*
 * Sort the files according to the flags.
 */
static void
sort(struct file **al)
{
	if (present('U'))
		return;
	if (!present('f') && *al != nil && (*al)->next != nil) {
		CMP = namecmp;
		if (!(field & FL_BYTIME)) {
			/* Sort on name */

			if (present('r')) {
				rCMP = CMP;
				CMP = revcmp;
			}
			_mergesort(al);
		} else {
			/* Sort on name first, then sort on time. */
			_mergesort(al);
			if (field & FL_CTIME) {
				CMP = ctimecmp;
			} else if (field & FL_ATIME) {
				CMP = atimecmp;
			} else {
				CMP = mtimecmp;
			}
			if (present('r')) {
				rCMP = CMP;
				CMP = revcmp;
			}
			_mergesort(al);
		}
		if (present('X')) {
			CMP = extcmp;
			if (present('r')) {
				rCMP = CMP;
				CMP = revcmp;
			}
			_mergesort(al);
		}
		if (present('S')) {
			CMP = sizecmp;
			if (present('r')) {
				rCMP = CMP;
				CMP = revcmp;
			}
			_mergesort(al);
		}
	}
}

/*
 * Create file structure for given name.
 */
static struct file *
newfile(char *name)
{
	struct file *new;

	new = salloc(sizeof(*new));
	new->name = strcpy(salloc(strlen(name)+1), name);
	new->namlen = 0;
	new->flag = 0;
	return new;
}

/*
 * Add file to the head of a list.
 */
static void
pushfile(struct file **flist, struct file *new)
{
	new->next = *flist;
	*flist = new;
}

/*
 * Release old file structure.
 */
static void
delfile(struct file *old)
{
	free(old->name);
	free(old);
}

/*
 * Pop file off top of file list.
 */
static struct file *
popfile(struct file **flist)
{
	struct file *f;

	f = *flist;
	*flist = f->next;
	return f;
}

/*
 * Save the current file order.
 */
static void
savord(struct file *f)
{
	while (f) {
		f->sord = f->next;
		f = f->next;
	}
}

/*
 * Restore the saved file order.
 */
static void
resord(struct file *f)
{
	while (f) {
		f->next = f->sord;
		f = f->sord;
	}
}

/*
 * Return flag that would make ls list this name: -a or -A.
 */
static int
dotflag(char *name)
{
	if (*name++ != '.')
		return 0;
	switch (*name++) {
	case 0:	
		return 'a';			/* "." */
	case '.':
		if (*name == 0)
			return 'a';	/* ".." */
	default:
		return 'A';			/* ".*" */
	}
}

/*
 * Add directory entries of directory name to a file list.
 */
static int
adddir(struct file **aflist, char *name)
{
	DIR *d;
	struct dirent *e;

	if (access(name, 0) < 0) {
		report(name);
		return 0;
	}
	if ((d = opendir(name)) == nil) {
#ifdef	UCB
		fprintf(stderr, "%s unreadable\n", name);
#else
		fprintf(stderr, "can not access directory %s\n", name);
#endif
		ex = 1;
		return 0;
	}
	while ((e = readdir(d)) != nil) {
		if (e->d_ino != 0 && present(dotflag(e->d_name))) {
			pushfile(aflist, newfile(e->d_name));
			aflist = &(*aflist)->next;
		}
	}
	closedir(d);
	return 1;
}

/*
 * Compute total block count for a list of files.
 */
static off_t
countblocks(struct file *flist)
{
	off_t cb = 0;

	while (flist != nil) {
		switch (flist->mode & S_IFMT) {
		case S_IFDIR:
		case S_IFREG:
#ifdef S_IFLNK
		case S_IFLNK:
#endif
			cb += flist->blocks;
		}
		flist = flist->next;
	}
	return cb;
}

static char *
#ifdef	LONGLONG
hfmt(long long n, int fill)
#else
hfmt(long n, int fill)
#endif
{
	static char	b[10];
	const char	units[] = " KMGTPE", *up = units;
	int	rest = 0;

	while (n > 1023) {
		rest = (n % 1024) / 128;
		n /= 1024;
		up++;
	}
#ifdef	LONGLONG
	if (up == units)
		snprintf(b, sizeof b, "%*llu", fill ? 5 : 1, n);
	else if (n < 10 && rest)
		snprintf(b, sizeof b, "%*llu.%u%c", fill ? 2 : 1, n, rest, *up);
	else
		snprintf(b, sizeof b, "%*llu%c", fill ? 4 : 1, n, *up);
#else	/* !LONGLONG */
	if (up == units)
		snprintf(b, sizeof b, "%*lu", fill ? 5 : 1, n);
	else if (n < 10 && rest)
		snprintf(b, sizeof b, "%*lu.%u%c", fill ? 2 : 1, n, rest, *up);
	else
		snprintf(b, sizeof b, "%*lu%c", fill ? 4 : 1, n, *up);
#endif	/* !LONGLONG */
	return b;
}

#define	FC_NORMAL	0
#define	FC_BLACK	30
#define	FC_RED		31
#define	FC_GREEN	32
#define	FC_YELLOW	33
#define	FC_BLUE		34
#define	FC_MAGENTA	35
#define	FC_CYAN		36
#define	FC_WHITE	37

/*
 * The curses color interface is nearly unusable since 1) it interferes
 * with bold attributes and 2) there is no clean possibility to reset
 * all colors to the previous (NOT black/white) values. This uses curses
 * to check whether running on an ANSI-style color terminal, and if yes,
 * returns appropriate attributes after.
 */
static char *
fc_get(int color)
{
	static int status;
	static char sequence[8];

	if (status == 0) {
#if defined (COLOR_BLACK) && !defined (USE_TERMCAP)
		char *cp = tparm(set_a_foreground, COLOR_BLACK);

		if (cp && strcmp(cp, "\33[30m") == 0)
			status = 1;
		else
			status = -1;
#else	/* !COLOR_BLACK */
		char *cp = getenv("TERM");
                status = strncmp(cp, "rxvt", 4) == 0 ||
				strncmp(cp, "dtterm", 6) == 0 ||
	                        strncmp(cp, "xterm", 5) == 0;
#endif	/* !COLOR_BLACK */
	}
	if (status == -1)
		return "";
	snprintf(sequence, sizeof sequence, "\33[%um", color);
	return sequence;
}

/*
 * Display a nonprintable character.
 */
static unsigned
nonprint(register int c, int doit)
{
	register int d;
	unsigned n;

	if (present('b')) {
		n = 4;
		if (doit) {
			char *nums = "01234567";

			putchar('\\');
			putchar(nums[(c & ~077) >> 6]);
			c &= 077;
			d = c & 07;
			if (c > d)
				putchar(nums[(c - d) >> 3]);
			else
				putchar(nums[0]);
			putchar(nums[d]);
		}
	} else {
		n = 1;
		if (doit)
			putchar('?');
	}
	return n;
}

/*
 * Print a name with control characters as '?' (unless -q).  The terminal is
 * assumed to be eight bit clean.
 */
static unsigned
printname(const char *name, struct file *f, int doit)
{
	int c, q = present('q'), bold = 0;
	unsigned n = 0;
	char *color = NULL;
#if defined (USE_TERMCAP)
	static char	tspace[1000];
	char	*tptr = tspace;
	static char	*Bold, *Normal;
	static int	washere;

	if (washere == 0 && tinfostat == 1) {
		washere = 1;
		Bold = tgetstr("md", &tptr);
		Normal = tgetstr("me", &tptr);
	}
#endif	/* USE_TERMCAP */

	if (f != NULL && doit == 0 && f->namlen != 0)
		return f->namlen;
	if (doit && f != NULL && tinfostat == 1) {
		if ((f->mode & S_IFMT) == S_IFDIR) {
			color = fc_get(FC_BLUE);
			bold++;
		} else if ((f->mode & S_IFMT) == S_IFCHR ||
				(f->mode & S_IFMT) == S_IFBLK) {
			color = fc_get(FC_YELLOW);
			bold++;
#ifdef	S_IFLNK
		} else if ((f->mode & S_IFMT) == S_IFLNK) {
			color = fc_get(FC_CYAN);
			bold++;
#endif
#ifdef	S_IFSOCK
		} else if ((f->mode & S_IFMT) == S_IFSOCK) {
			color = fc_get(FC_MAGENTA);
			bold++;
#endif
#ifdef	S_IFDOOR
		} else if ((f->mode & S_IFMT) == S_IFDOOR) {
			color = fc_get(FC_MAGENTA);
			bold++;
#endif
#ifdef	S_IFNAM
		} else if ((f->mode & S_IFMT) == S_IFNAM) {
			color = fc_get(FC_MAGENTA);
#endif
#ifdef	S_IFNWK
		} else if ((f->mode & S_IFMT) == S_IFNWK) {
			color = fc_get(FC_MAGENTA);
#endif
#ifdef	S_IFIFO
		} else if ((f->mode & S_IFMT) == S_IFIFO) {
			color = fc_get(FC_YELLOW);
#endif
		} else if (f->mode & (S_IXUSR|S_IXGRP|S_IXOTH)) {
			color = fc_get(FC_GREEN);
			bold++;
		} else if ((f->mode & S_IFMT) == 0) {
			color = fc_get(FC_RED);
			bold++;
		}
		if (color) {
#ifndef	USE_TERMCAP
			if (bold)
				vidattr(A_BOLD);
#else	/* USE_TERMCAP */
			if (Bold)
				tputs(Bold, 1, putchar);
#endif	/* USE_TERMCAP */
			printf(color);
		}
	}
	if (q || istty) {
#ifdef	MB_CUR_MAX
		wchar_t wc;

		while (
#define	ASCII
#ifdef	ASCII
				(*name & 0200) == 0 ?
					c = 1, (wc = *name) != '\0' :
#endif
				(c = mbtowc(&wc, name, MB_CUR_MAX)) != 0) {
			if (c != -1) {
				if (iswprint(wc)
#ifdef	UCB
						|| wc == '\t' || wc == '\n'
#else	/* !UCB */
						&& wc != '\t'
#endif	/* !UCB */
						) {
					if (doit) {
						while (c-- > 0)
							putchar(*name++ & 0377);
					} else
						name += c;
					n += wcwidth(wc);
				} else {
					while (c-- > 0)
						n += nonprint(*name++ & 0377,
								doit);
				}
			} else
				n += nonprint(*name++ & 0377, doit);
		}
#else	/* !MB_CUR_MAX */
		while ((c = (*name++ & 0377)) != '\0') {
			if (isprint(c)
#ifdef	UCB
					|| c == '\t' || c == '\n'
#else	/* !UCB */
					&& c != '\t'
#endif	/* !UCB */
					) {
				if (doit)
					putchar(c);
				n++;
			} else
				n += nonprint(c, doit);
		}
#endif	/* !MB_CUR_MAX */
	} else {
		while ((c = (*name++ & 0377)) != '\0') {
			if (doit)
				putchar(c);
			n++;
		}
	}
	if (doit && color) {
#if !defined (USE_TERMCAP)
		if (bold)
			vidattr(A_NORMAL);
#else	/* USE_TERMCAP */
		if (Normal)
			tputs(Normal, 1, putchar);
#endif	/* USE_TERMCAP */
		printf(fc_get(FC_NORMAL));
	}
	if (f)
		f->namlen = n;
	return n;
}

static int
mark(struct file *f, int doit)
{
	int c;

	if (!(field & FL_MARK))
		return 0;
	switch (f->mode & S_IFMT) {
	case S_IFDIR:	c = '/'; break;
#ifdef S_IFIFO
	case S_IFIFO:	c = '|'; break;
#endif
#ifdef S_IFLNK
	case S_IFLNK:	c = '@'; break;
#endif
#ifdef S_IFSOCK
	case S_IFSOCK:	c = '='; break;
#endif
#ifdef S_IFDOOR
	case S_IFDOOR:	c = '>'; break;
#endif
	case S_IFREG:
		if (f->mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
			c = '*';
			break;
		}
	default:
		c = 0;
	}
	if (doit && c != 0)
		putchar(c);
	return c;
}

static int colwidth[MAXCOLS];	/* Need colwidth[i] spaces to print column i. */
static int sizwidth[MAXCOLS];	/* Spaces for the size field in a -X print. */
static int namwidth[MAXCOLS];	/* Name field. */

/*
 * Set *aw to the larger of it and w.  Then return it.
 */
static int
maxise(int *aw, int w)
{
	if (w > *aw) *aw = w;
	return *aw;
}

static int nsp = 0;	/* This many spaces have not been printed yet. */
#define spaces(n)	(nsp = (n))
#define terpri()	(nsp = 0, putchar('\n'))		/* No trailing spaces */

/*
 * Either compute the number of spaces needed to print file f (doit == 0) or
 * really print it (doit == 1).
 */
static int
print1(struct file *f, int col, int doit)
{
	int width = 0, n;

	n = printname(f->name, f, 0);
	if (present('m')) {
		if (present('p') && (f->mode & S_IFMT) == S_IFDIR)
			width++;
		if (present('F'))
			width++;
	}
	while (nsp > 0) {
		putchar(' ');
		nsp--;
	}/* Fill gap between two columns */
	if (field & FL_INODE) {
		char	dummy[2];
		if (doit)
#ifdef	LONGLONG
			printf("%*llu ", present('m') ? 1 : 5,
					(long long)f->ino);
#else
			printf("%*lu ", present('m') ? 1 : 5,
					(long)f->ino);
#endif
		else
#ifdef	LONGLONG
			width += snprintf(dummy, sizeof dummy,
					"%*llu ", present('m') ? 1 : 5,
					(long long)f->ino);
#else
			width += snprintf(dummy, sizeof dummy,
					"%*lu ", present('m') ? 1 : 5,
					(long)f->ino);
#endif
	}
	if (field & FL_BLOCKS) {
		char	dummy[2];
		if (doit) {
			if (present('h'))
				printf("%s ", hfmt(f->blocks * 512,
							!present('m')));
			else
#ifdef	LONGLONG
				printf("%*llu ", present('m') ? 1 : 4,
					(long long)blockcount(f->blocks));
#else
				printf("%*lu ", present('m') ? 1 : 4,
					(long)blockcount(f->blocks));
#endif
		} else {
			if (present('h'))
				width += 6;
			else
#ifdef	LONGLONG
				width += snprintf(dummy, sizeof dummy, "%*llu ",
					present('m') ? 1 : 4,
					(long long)f->blocks);
#else
				width += snprintf(dummy, sizeof dummy, "%*lu ",
					present('m') ? 1 : 4,
					(long)f->blocks);
#endif
		}
	}
	if (field & FL_MODE) {
		if (doit) {
			printf("%s ", permissions(f));
		} else {
			width += 11;
		}
	}
	if (field & FL_LONG) {
		if (doit) {
			printf("%2u ", (unsigned) f->nlink);
			if (!(field & FL_GROUP)) {
				printf("%-8s ", uidname(f->uid));
			}
			if (!(field & FL_OWNER)) {
				printf("%-8s ", gidname(f->gid));
			}
			switch (f->mode & S_IFMT) {
			case S_IFBLK:
			case S_IFCHR:
#ifdef S_IFMPB
			case S_IFMPB:
#endif
#ifdef S_IFMPC
			case S_IFMPC:
#endif
				printf("%3lu,%3lu ",
					(long)major(f->rdev),
					(long)minor(f->rdev));
				break;
			default:
				if (present('h'))
					printf("%5s ", hfmt(f->size, 1));
				else
#ifdef	LONGLONG
					printf("%7llu ", (long long)f->size);
#else
					printf("%7lu ", (long)f->size);
#endif
			}
			printf("%s ", timestamp(f));
		} else {
			width += (field & FL_GROUP) ? 34 : 43;
		}
	}
	if (doit) {
		printname(f->name, f, 1);
		if (mark(f, 1) != 0)
			n++;
		if (present('p') && (f->mode & S_IFMT) == S_IFDIR) {
			n++;
			if (doit)
				putchar('/');
		}
#ifdef S_IFLNK
		if ((field & FL_LONG) && (f->mode & S_IFMT) == S_IFLNK) {
			char *buf;
			int sz, r, didx;

			sz = f->size ? f->size : PATH_MAX;
			buf = salloc(sz + 1);
			addpath(&didx, f->name);
			if ((r = readlink(path, buf, sz)) < 0)
				r = 0;
			delpath(didx);
			buf[r] = 0;
			printf(" -> ");
			printname(buf, NULL, 1);
			free(buf);
			n += 4 + r;
		}
#endif
		if (!present('m'))
			spaces(namwidth[col] - n);
	} else {
		if (mark(f, 0) != 0)
			n++;
#ifdef S_IFLNK
		if ((field & FL_LONG) && (f->mode & S_IFMT) == S_IFLNK) {
			n += 4 + (int) f->size;
		}
#endif
		if (!present('m')) {
			width += maxise(&namwidth[col], n + NSEP);
			maxise(&colwidth[col], width);
		}
	}
	return n + width;
}

/*
 * Return number of files in the list.
 */
static int
countfiles(struct file *flist)
{
	int n = 0;

	while (flist != nil) {
		n++;
		flist = flist->next;
	}
	return n;
}

static struct file *filecol[MAXCOLS];	/*
					 * filecol[i] is list of files
					 * for column i.
					 */
static int nfiles, nlines;	/* # files to print, # of lines needed. */

/*
 * Chop list of files up in columns.  Note that 3 columns are used for 5 files
 * even though nplin may be 4, filecol[3] will simply be nil.
 */
static void
columnise(struct file **flist, struct file *fsav, int nplin)
{
	struct file *f = *flist;
	int i, j;

	nlines = (nfiles + nplin - 1) / nplin;	/* nlines needed for nfiles */
	filecol[0] = f;
	if (!present('x')) {
		for (i = 1; i < nplin; i++) {
			/* Give nlines files to each column. */
			for (j = 0; j < nlines && f != nil; j++)
				f = f->next;
			filecol[i] = f;
		}
	} else {
		/*
		 * Ok, this is an ugly hack. We use the mechanisms for '-C'
		 * and thus have to change the file list order.
		 */
		struct file *curcol[MAXCOLS];

		resord(fsav);
		*flist = fsav;
		f = *flist;
		for (i = 0; i < nplin && f; i++) {
			filecol[i] = curcol[i] = f;
			f->flag &= ~ENDCOL;
			f = f->next;
		}
		while (f != NULL) {
			for (i = 0; i < nplin && f; i++) {
				curcol[i]->next = f;
				curcol[i] = f;
				f->flag &= ~ENDCOL;
				f = f->next;
			}
		}
		for (i = 1; i < nplin; i++) {
			curcol[i - 1]->next = filecol[i];
			curcol[i - 1]->flag |= ENDCOL;
		}
		curcol[nplin - 1]->next = NULL;
	}
}

/*
 * Try (doit == 0), or really print the list of files over nplin columns.
 * Return true if it can be done in nplin columns or if nplin == 1.
 */
static int
print(struct file **flist, struct file *fsav, int nplin, int doit)
{
	register struct file *f;
	register int i, totlen;

	if (present('m')) {
		if (!doit)
			return 1;
		totlen = 0;
		for (f = *flist; f; f = f->next) {
			i = print1(f, 0, 0) + 2;
			totlen += i;
			if (totlen > ncols+1) {
				putchar('\n');
				totlen = i;
			} else if (f != *flist)
				putchar(' ');
			print1(f, 0, 1);
			if (f->next)
				putchar(',');
		}
		putchar('\n');
		return 1;
	}
	columnise(flist, fsav, nplin);
	if (!doit) {
		if (nplin == 1)
			return 1;	/* No need to try 1 column. */
		for (i = 0; i < nplin; i++) {
			colwidth[i] = sizwidth[i] = namwidth[i] = 0;
		}
	}
	while (--nlines >= 0) {
		totlen = 0;
		for (i = 0; i < nplin; i++) {
			if ((f = filecol[i]) != nil) {
				if (f->flag & ENDCOL)
					filecol[i] = NULL;
				else
					filecol[i] = f->next;
				print1(f, i, doit);
			}
			if (!doit && nplin>1) {
				/* See if this line is not too long. */
				totlen += colwidth[i];
				if (totlen > ncols+NSEP)
					return 0;
			}
		}
		if (doit && !present('m'))
			terpri();
	}
	return 1;
}

enum depth { SURFACE, SURFACE1, SUBMERGED };
enum state { BOTTOM, SINKING, FLOATING };

/*
 * Main workhorse of ls, it sorts and prints the list of files.  Flags:
 * depth: working with the command line / just one file / listing dir.
 * state: How "recursive" do we have to be.
 */
static void
listfiles(struct file *flist, enum depth depth, enum state state, int level)
{
	struct file *dlist = nil, **afl = &flist, **adl = &dlist, *fsav;
	int nplin, t;
	static int white = 1;	/* Nothing printed yet. */

	/*
	 * Flush everything previously printed, so new error output will
	 * not intermix with files listed earlier.
	 */
	fflush(stdout);
	if (field != 0 || state != BOTTOM) {	/* Need stat(2) info. */
		while (*afl != nil) {
			static struct stat st;
			int didx;
#ifdef S_IFLNK
			int (*status)(const char *file, struct stat *stp) =
				depth == SURFACE1 && (field & FL_LONG) == 0
#ifdef	SU3
						&& !present('F')
#endif	/* SU3 */
					|| present('L')
					|| present('H') && depth != SUBMERGED ?
						stat : lstat;
#else
#define status	stat
#endif

/* Basic disk block size is 512 except for one niche O.S. */

			addpath(&didx, (*afl)->name);
			if ((t = status(path, &st)) < 0
#ifdef S_IFLNK
				&& (status == lstat || lstat(path, &st) < 0)
#endif
			) {
				if (depth != SUBMERGED || errno != ENOENT)
					report((*afl)->name);
#ifdef	SU3
			fail:
#endif	/* SU3 */
				delfile(popfile(afl));
			} else {
#ifdef	SU3
				if (t < 0 && errno == ELOOP && (present('H') ||
							present('L'))) {
					report((*afl)->name);
					goto fail;
				}
				if (present('L')) {
					int	i;
					for (i = 0; i < level; i++) {
						if (st.st_dev==visited[i].v_dev
						    && st.st_ino==
						      visited[i].v_ino) {
							fprintf(stderr, "link "
								"loop at %s\n",
								path);
							ex = 1;
							goto fail;
						}
					}
					if (level >= vismax) {
						vismax += 20;
						visited = srealloc(visited,
								sizeof *visited
								* vismax);
					}
					visited[level].v_dev = st.st_dev;
					visited[level].v_ino = st.st_ino;
				}
#endif	/* SU3 */
				if (((field & FL_MARK) || tinfostat == 1 ||
						present('H')) &&
						!present('L') &&
						status != lstat &&
						(st.st_mode & S_IFMT)
							!= S_IFDIR) {
					struct stat	lst;

					if (lstat(path, &lst) == 0)
						st = lst;
				}
				setstat(*afl, &st);
				afl = &(*afl)->next;
			}
			delpath(didx);
		}
	}
	sort(&flist);
	if (depth == SUBMERGED && (field & (FL_BLOCKS | FL_LONG))) {
		printf("total ");
		if (present('h'))
			printf("%s\n", hfmt(countblocks(flist) * 512, 0));
		else
#ifdef	LONGLONG
			printf("%lld\n", (long long)blockcount(countblocks(flist)));
#else
			printf("%ld\n", (long)blockcount(countblocks(flist)));
#endif
	}
	if (state == SINKING || depth == SURFACE1) {
	/* Don't list directories themselves, list their contents later. */
		afl = &flist;
		while (*afl != nil) {
			if (((*afl)->mode & S_IFMT) == S_IFDIR) {
				pushfile(adl, popfile(afl));
				adl = &(*adl)->next;
			} else {
				afl = &(*afl)->next;
			}
		}
	}
	if ((nfiles = countfiles(flist)) > 0) {
		/* Print files in how many columns? */
		nplin = !present('C') && !present('x') ?
			1 : nfiles < MAXCOLS ? nfiles : MAXCOLS;
		fsav = flist;
		if (present('x'))
			savord(flist);
		while (!print(&flist, fsav, nplin, 0))
			nplin--;	/* Try first */
		print(&flist, fsav, nplin, 1);		/* Then do it! */
		white = 0;
	}
	while (flist != nil) {	/* Destroy file list */
		if (state == FLOATING && (flist->mode & S_IFMT) == S_IFDIR) {
			/* But keep these directories for ls -R. */
			pushfile(adl, popfile(&flist));
			adl = &(*adl)->next;
		} else {
			delfile(popfile(&flist));
		}
	}
	while (dlist != nil) {	/* List directories */
		if (dotflag(dlist->name) != 'a' || depth != SUBMERGED) {
			int didx;

			addpath(&didx, dlist->name);

			flist = nil;
			if (adddir(&flist, path)) {
				if (depth != SURFACE1) {
					if (!white)
						putchar('\n');
					white = 0;
				}
				if (depth != SURFACE1 || present('R'))
					printf("%s:\n", path);
				listfiles(flist, SUBMERGED,
					state == FLOATING ? FLOATING : BOTTOM,
					level + 1);
			}
			delpath(didx);
		}
		delfile(popfile(&dlist));
	}
}

#ifndef	UCB
static void
usage(void)
{
	if (personality == PER_LS)
		fprintf(stderr, "usage: %s -1RadCxmnlogrtucpFbqisfL [files]\n",
				arg0);
	exit(2);
}
#else	/* UCB */
#define	usage()
#endif	/* UCB */

int
main(int argc, char **argv)
{
	struct file *flist = nil, **aflist = &flist;
	enum depth depth;
	struct winsize ws;
	int i;
	char *cp;

#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	setlocale(LC_COLLATE, "");
	setlocale(LC_CTYPE, "");
	setlocale(LC_TIME, "");
#ifndef	UCB
	if (getenv("SYSV3") != NULL)
		sysv3 = 1;
#endif	/* !UCB */
	uid = geteuid();
	gid = getegid();
	arg0 = basename(argv[0]);
	if (strcmp(arg0, "dir") == 0) {
		flags['s'] = flags['U'] = 1;
		personality = PER_DIR;
	} else if (strcmp(arg0, "lc") == 0) {
		flags['C'] = 1;
		istty = 1;
	}
	if (istty || isatty(1)) {
		istty = 1;
#if !defined (USE_TERMCAP)
		setupterm(NULL, 1, &tinfostat);
#else	/* USE_TERMCAP */
		{
			char	buf[2048];
			if ((cp = getenv("TERM")) != NULL)
				if (tgetent(buf, cp) > 0)
					tinfostat = 1;
		}
#endif	/* USE_TERMCAP */
		field |= FL_STATUS;
	}
	while ((i = getopt(argc, argv, personalities[personality].per_opt))
			!= EOF) {
		switch (i) {
		case '?':
			usage();
			break;

		case 'O':
			flags['U'] = 0;
			break;
		case '1':
			flags[i] = 1;
			flags['C'] = 0;
			break;
		case 'L':
			flags[i] = 1;
			flags['H'] = 0;
			break;
		case 'H':
			flags[i] = 1;
			flags['L'] = 0;
			break;
#if defined (SUS) || defined (SU3)
		case 'C':
			flags[i] = 1;
			flags['l'] = flags['m'] = flags['x'] = flags ['o'] =
				flags['g'] = flags['n'] = flags['1'] = 0;
			break;
		case 'm':
			flags[i] = 1;
			flags['l'] = flags['C'] = flags['x'] = flags ['o'] =
				flags['g'] = flags['n'] = flags['1'] = 0;
			break;
		case 'l':
			flags[i] = 1;
			flags['m'] = flags['C'] = flags['x'] = flags ['o'] =
				flags['g'] = 0;
			break;
		case 'x':
			flags[i] = 1;
			flags['m'] = flags['C'] = flags['l'] = flags ['o'] =
				flags['g'] = flags['n'] = flags['1'] = 0;
			break;
		case 'g':
			flags[i] = 1;
			flags['m'] = flags['C'] = flags['l'] = flags ['x'] =
				flags['o'] = 0;
			break;
		case 'o':
			flags[i] = 1;
			flags['m'] = flags['C'] = flags['l'] = flags ['x'] =
				flags['g'] = 0;
			break;
		case 'n':
			flags[i] = 1;
			flags['m'] = flags['C'] = flags ['x'] = 0;
			break;
#endif	/* !SUS, !SU3 */
		default:
			flags[i] = 1;
		}
	}
#ifdef	UCB
	if (present('l')) {
		if (present('g'))
			flags['g'] = 0;
		else
			flags['o'] = 1;
	} else if (present('g'))
		flags['g'] = 0;
#endif
#ifdef	UCB
	if (SUPER_ID == 0 || present('a'))
		flags['A'] = 1;
#else	/* !UCB */
	if (present('a'))
		flags['A'] = 1;
#endif	/* !UCB */
	if (present('i'))
		field |= FL_INODE;
	if (present('s'))
		field |= FL_BLOCKS;
	if (present('t'))
		field |= FL_BYTIME;
	if (present('u'))
		field |= FL_ATIME;
	if (present('c'))
		field |= FL_CTIME;
	if (present('l') || present('n') || present('g') || present('o')) {
		field = field | FL_MODE | FL_LONG;
		flags['m'] = flags['C'] = flags['x'] = 0;
	}
	if (present('g'))
		field = field | FL_MODE | FL_LONG | FL_GROUP;
	if (present('o'))
		field = field | FL_MODE | FL_LONG | FL_OWNER;
	if (present('F'))
		field |= FL_MARK;
	if (present('d'))
		field |= FL_DIR;
	if (present('f')) {
		field &= ~(FL_LONG|FL_BYTIME|FL_BLOCKS|FL_MODE|FL_MARK|FL_DIR);
		flags['o'] = flags['g'] = flags['l'] = flags['t'] = flags['s'] =
			flags['r'] = flags['d'] = flags['F'] = flags['R'] = 
			flags['p'] = 0;
		flags['a'] = flags['A'] = 1;
		field |= FL_STATUS;
	}
	if (!present('1') && !present('C') && !present('l') &&
			!present('o') && !present('g') && !present('m') &&
			(istty && !sysv3 || present('x')))
		flags['C'] = 1;
	if (istty)
		flags['q'] = 1;
	if ((cp = getenv("COLUMNS")) != NULL) {
		ncols = atoi(cp);
	} else if ((present('C') || present('x') || present('m')) && istty) {
		if (ioctl(1, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
			ncols = ws.ws_col - 1;
#if !defined (USE_TERMCAP)
		else if (tinfostat == 1 && columns > 0)
			ncols = columns;
#endif	/* !USE_TERMCAP */
	}
	depth = SURFACE;
	if (optind == argc) {
		if (!(field & FL_DIR))
			depth = SURFACE1;
		pushfile(aflist, newfile("."));
	} else {
		if (optind+1 == argc && !(field & FL_DIR))
			depth = SURFACE1;
		while (optind < argc) {
			if (present('f')) {
				struct stat st;

				if (stat(argv[optind], &st) == 0 &&
						(st.st_mode & S_IFMT)
							!= S_IFDIR) {
#ifdef	UCB
					fprintf(stderr, "%s unreadable\n",
							argv[optind]);
#else
					fprintf(stderr, "%s: Not a directory\n",
						argv[optind]);
#endif
					ex = 1;
					optind++;
					continue;
				}
			}
			pushfile(aflist, newfile(argv[optind++]));
			aflist = &(*aflist)->next;
		}
	}
	listfiles(flist, depth,
		(field & FL_DIR) ? BOTTOM : present('R') ? FLOATING : SINKING,
		0);
	return ex;
}
