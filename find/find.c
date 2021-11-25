/*	find	COMPILE:	cc -o find -s -O -i find.c -lS	*/
/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, September 2003.
 */
/*	from Unix 7th Edition /usr/src/cmd/find.c	*/
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
#if defined (SU3)
static const char sccsid[] USED = "@(#)find_su3.sl	1.45 (gritter) 5/8/06";
#elif defined (SUS)
static const char sccsid[] USED = "@(#)find_sus.sl	1.45 (gritter) 5/8/06";
#else
static const char sccsid[] USED = "@(#)find.sl	1.45 (gritter) 5/8/06";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <time.h>
#include <grp.h>
#include <stdarg.h>
#include <libgen.h>
#include <errno.h>
#include <locale.h>
#include <signal.h>
#if defined (SUS) || defined (SU3)
#include <fnmatch.h>
#endif
#if defined (__linux__) || defined (_AIX) || defined (__hpux)
#include <mntent.h>
#endif
#if defined (__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) || \
	defined (__DragonFly__) || defined (__APPLE__)
#include <sys/param.h>
#include <sys/mount.h>
#endif
#ifdef	_AIX
#include <sys/sysmacros.h>
#endif
#ifndef	major
#include <sys/sysmacros.h>
#endif
#if __NetBSD_Version__>= 300000000
#include <sys/statvfs.h>
#define statfs statvfs
#endif
#include "getdir.h"
#include "atoll.h"
#define A_DAY	86400L /* a day full of seconds */
#define EQ(x, y)	(strcmp(x, y)==0)

#ifndef	MNTTYPE_IGNORE
#define	MNTTYPE_IGNORE	""
#endif

#ifndef	S_IFDOOR
#define	S_IFDOOR	0xD000
#endif

#ifndef	S_IFNWK
#define	S_IFNWK		0x9000
#endif

#undef	ctime
#define	ctime	find_ctime

static char	*Pathname;

struct aggregate {		/* for exec ... {} + */
	long	a_cnt;		/* count of arguments */
	long	a_cur;		/* current position in aggregate */
	long	a_csz;		/* aggregate current length */
	long	a_msz;		/* aggregate maximum length */
	char	**a_vec;	/* arguments */
	char	*a_spc;		/* aggregate space */
	long	a_maxarg;	/* maximum arguments in e_vec */
};

struct anode {
	int	(*F)(struct anode *);
	union anode_l {
		struct anode	*L;
		char *pat;
		time_t t;
		uid_t u;
		gid_t g;
		ino_t i;
		nlink_t link;
		off_t sz;
		mode_t per;
		int com;
		FILE *fp;
		char *fstype;
	} l;
	union anode_r {
		struct anode	*R;
		int	s;
		pid_t pid;
		struct aggregate *a;
	} r;
};
static char	*Fname;
static time_t	Now;
static int	Argc,
	Ai,
	Pi;
static char	**Argv;
/* cpio stuff */
static int	Cpio;

static struct stat Statb;

/*
 * Keep track of all visited directories, to avoid loops caused by
 * symbolic links and to free storage and close files after fork().
 */
static struct visit {
	struct getdb	*v_db;	/* getdb struct for this level */
	ino_t	v_ino;		/* inode number */
	int	v_fd;		/* file descriptor */
	dev_t	v_dev;		/* device id */
} *visited;
static int	vismax;		/* number of members in visited */

/*
 * For -fstype, keep track of all filesystem types known to the system. If
 * we had st_fstype in struct stat as SVR4 does, this would be far more
 * reliable.
 */
#if defined (__linux__) || defined (_AIX) || defined (__hpux)
static struct fstype {
	dev_t	fsdev;		/* device id of filesystem */
	char	*fstype;	/* filesystem type */
} *fstypes, *fscur;
#endif	/* __linux__ || _AIX || __hpux */

static int	Home = -1;
static int	wanthome;
static mode_t	um;		/* user's umask */
static const char	*progname;
static int	status;		/* exit status */
static int	depth;		/* -depth flag */
static int	Print = 1;	/* implicit -print */
static int	Prune;		/* -prune at this point */
static int	Mount;		/* -mount, -xdev */
static int	Execplus;	/* have a -exec command {} + node */
static int	HLflag;		/* -H or -L option given */
static char	*Statfs;	/* result of statfs() on FreeBSD */
static int	incomplete;	/* encountered an incomplete statement */
extern int	sysv3;

static int	(*statfn)(const char *, struct stat *) = lstat;

static struct anode *expr(void);
static struct anode *e1(void);
static struct anode *e2(void);
static struct anode *e3(void);
static struct anode *mk(struct anode *);
static void	oper(const char **);
static char	*nxtarg(int);
static int	and(struct anode *);
static int	or(struct anode *);
static int	not(struct anode *);
static int	glob(struct anode *);
static int	print(struct anode *);
static int	prune(struct anode *);
static int	null(struct anode *);
static int	mtime(struct anode *);
static int	atime(struct anode *);
static int	ctime(struct anode *);
static int	user(struct anode *);
static int	ino(struct anode *);
static int	group(struct anode *);
static int	nogroup(struct anode *);
static int	nouser(struct anode *);
static int	links(struct anode *);
static int	size(struct anode *);
static int	sizec(struct anode *);
static int	perm(struct anode *);
static int	type(struct anode *);
static int	exeq(struct anode *);
static int	ok(struct anode *);
static int	cpio(struct anode *);
static int	newer(struct anode *);
static int	cnewer(struct anode *);
static int	anewer(struct anode *);
static int	fstype(struct anode *);
static int	local(struct anode *);
static int	scomp(long long, long long, char);
static int	doex(int, struct aggregate *);
static struct aggregate *mkagg(long);
static uid_t	getunum(const char *);
static gid_t	getgnum(const char *);
static const char	*getuser(uid_t);
static const char	*getgroup(gid_t);
#if defined (__linux__) || defined (_AIX) || defined (__hpux)
static void	getfscur(dev_t);
static void	getfstypes(void);
#endif	/* __linux__ || _AIX || __hpux */
static int	descend(char *, struct anode *, int);
static int	descend1(char *, struct anode *, int);
static int	descend2(char *, struct anode *, int);
static void	setpath(char *, const char *, int);
static void	pr(const char *, ...);
static void	er(const char *, ...);
static void	usage(void);
static void	*srealloc(void *, size_t);
static void	mkcpio(struct anode *, const char *, int);
static void	trailer(struct anode *, int);
static void	mknewer(struct anode *, const char *, int (*)(struct anode *));
static mode_t	newmode(const char *ms, const mode_t pm);

int
main(int argc, char **argv)
{
	struct anode *exlist;
	struct anode nlist = { null, { 0 }, { 0 } };
	int paths;
	register char *sp = 0;
	int	i, j;

	time(&Now);
	umask(um = umask(0));
	progname = basename(argv[0]);
	setlocale(LC_COLLATE, "");
	setlocale(LC_CTYPE, "");
	if (getenv("SYSV3") != NULL)
		sysv3 = 1;
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-' || argv[i][1] == '\0')
			break;
		if (argv[i][1] == '-') {
			i++;
			break;
		}
		for (j = 1; argv[i][j]; j++)
			if (argv[i][j] != 'H' && argv[i][j] != 'L')
				goto brk;
		for (j = 1; argv[i][j]; j++)
			HLflag = argv[i][j];
	}
brk:	if (HLflag == 'L')
		statfn = stat;
	argc -= i - 1;
	argv += i - 1;
	Argc = argc; Argv = argv;
	if(argc<2) {
		pr("insufficient number of arguments");
		usage();
	}
	for(Ai = paths = 1; Ai < argc; ++Ai, ++paths)
		if(*Argv[Ai] == '-' || EQ(Argv[Ai], "(") || EQ(Argv[Ai], "!"))
			break;
	if(paths == 1) /* no path-list */
		usage();
	if(Ai<argc) {
		if(!(exlist = expr())) /* parse and compile the arguments */
			er("find: parsing error");
		if(Ai<argc) {
			pr("bad option %s", argv[Ai]);
			usage();
		}
	} else
		exlist = &nlist;
	if (paths > 2)
		wanthome = 1;
	if (wanthome && (Home = open(".", O_RDONLY)) < 0)
		er("bad starting directory");
	for(Pi = 1; Pi < paths; ++Pi) {
		if (Pi > 1 && Home >= 0 && fchdir(Home) < 0)
			er("bad starting directory");
		setpath(Pathname, Argv[Pi], 0);
		Fname = sp = Pathname;
		do
			if (sp[0] == '/')
				Fname = &sp[1];
		while (*sp++);
		descend(Pathname, exlist, 0); /* to find files that match  */
	}
	if(Cpio || Execplus)
		trailer(exlist, 1);
	exit(status);
}

/* compile time functions:  priority is  expr()<e1()<e2()<e3()  */

/*ARGSUSED*/
static struct anode *expr(void) { /* parse ALTERNATION (-o)  */
	register struct anode * p1;
	struct anode n = { 0, { 0 }, { 0 } };

	p1 = e1() /* get left operand */ ;
	if(EQ(nxtarg(0), "-o")) {
		const char	*ops[] = { "-o", "-a", 0 };
		oper(ops);
		n.F = or, n.l.L = p1, n.r.R = expr();
		return(mk(&n));
	}
	else if(Ai <= Argc) --Ai;
	return(p1);
}
static struct anode *e1(void) { /* parse CONCATENATION (formerly -a) */
	register struct anode * p1;
	register char *a;
	struct anode n = { 0, { 0 }, { 0 } };

	p1 = e2();
	a = nxtarg(0);
	if(EQ(a, "-a")) {
		const char	*ops[] = { "-o", "-a", 0 };
		oper(ops);
And:
		n.F = and, n.l.L = p1, n.r.R = e1();
		return(mk(&n));
	} else if(EQ(a, "(") || EQ(a, "!") || (*a=='-' && !EQ(a, "-o"))) {
		--Ai;
		goto And;
	} else if(Ai <= Argc) --Ai;
	return(p1);
}
static struct anode *e2(void) { /* parse NOT (!) */
	struct anode n = { 0, { 0 }, { 0 } };
	if(EQ(nxtarg(0), "!")) {
		const char	*ops[] = { "-o", "-a", "!", 0 };
		oper(ops);
		n.F = not, n.l.L = e3();
		return(mk(&n));
	}
	else if(Ai <= Argc) --Ai;
	return(e3());
}
static struct anode *e3(void) { /* parse parens and predicates */
	struct anode *p1;
	struct anode n = { 0, { 0 }, { 0 } };
	long i, k;
	register char *a, *b, s, *p, *q;

	a = nxtarg(0);
	if(EQ(a, "(")) {
		const char	*ops[] = { "-o", "-a", 0 };
		oper(ops);
		p1 = expr();
		a = nxtarg(1);
		if(!EQ(a, ")")) goto err;
		return(p1);
	}
	else if(EQ(a, "-depth")) {
		depth = 1;
		n.F = null;
	} else if(EQ(a, "-follow")) {
		statfn = stat;
		n.F = null;
	} else if(EQ(a, "-mount") || EQ(a, "-xdev")) {
		Mount = 1;
		n.F = null;
	} else if(EQ(a, "-print")) {
		Print = 0;
		n.F = print;
	} else if(EQ(a, "-prune"))
		n.F = prune;
	else if(EQ(a, "-nogroup"))
		n.F = nogroup;
	else if(EQ(a, "-nouser"))
		n.F = nouser;
	else if(EQ(a, "-local")) {
#if defined (__linux__) || defined (_AIX) || defined (__hpux)
		getfstypes();
#endif	/* __linux__ || _AIX || __hpux */
		n.F = local;
		Statfs = a;
	}
	if (n.F)
		return mk(&n);
	b = nxtarg(2);
	s = *b;
	/*if(s=='+') b++;*/
	if(EQ(a, "-name"))
		n.F = glob, n.l.pat = b;
	else if(EQ(a, "-mtime"))
		n.F = mtime, n.l.t = atol(b), n.r.s = s;
	else if(EQ(a, "-atime"))
		n.F = atime, n.l.t = atol(b), n.r.s = s;
	else if(EQ(a, "-ctime"))
		n.F = ctime, n.l.t = atol(b), n.r.s = s;
	else if(EQ(a, "-user"))
		n.F = user, n.l.u = getunum(b), n.r.s = s;
	else if(EQ(a, "-inum"))
		n.F = ino, n.l.i = atoll(b), n.r.s = s;
	else if(EQ(a, "-group"))
		n.F = group, n.l.g = getgnum(b), n.r.s = s;
	else if(EQ(a, "-size")) {
		n.l.sz = atoll(b), n.r.s = s;
		while (b[0] && b[1])
			b++;
		if (b[0] == 'c')
			n.F = sizec;
		else
			n.F = size;
	}
	else if(EQ(a, "-links"))
		n.F = links, n.l.link = atol(b), n.r.s = s;
	else if(EQ(a, "-perm")) {
		while (*b == '-')
			b++;
		n.F = perm, n.l.per = newmode(b, 0), n.r.s = s;
#if defined (SUS) || defined (SU3)
		if (s == '-')
			n.l.per &= 07777;
#endif
	}
	else if(EQ(a, "-type")) {
		i = b[0] == '-' || b[0] == '+' ? b[1] : b[0];
		i = i=='d' ? S_IFDIR :
		    i=='b' ? S_IFBLK :
		    i=='c' ? S_IFCHR :
		    i=='D' ? S_IFDOOR :
		    i=='f' ? S_IFREG :
		    i=='l' ? S_IFLNK :
		    i=='n' ? S_IFNWK :
		    i=='p' ? S_IFIFO :
		    i=='s' ? S_IFSOCK :
		    0;
		n.F = type, n.l.per = i;
	}
	else if (EQ(a, "-exec")) {
		Print = 0;
		wanthome = 1;
		i = Ai - 1;
		q = "";
		k = 0;
		while(!EQ(p = nxtarg(1), ";")) {
			if (EQ(p, "+") && EQ(q, "{}")) {
				n.r.a = mkagg(k);
				break;
			}
			q = p;
			k += strlen(p) + 1;
		}
		n.F = exeq, n.l.com = i;
	}
	else if (EQ(a, "-ok")) {
		Print = 0;
		wanthome = 1;
		i = Ai - 1;
		while(!EQ(p = nxtarg(1), ";"));
		n.F = ok, n.l.com = i;
	}
	else if(EQ(a, "-cpio"))
		mkcpio(&n, b, 0);
	else if(EQ(a, "-ncpio"))
		mkcpio(&n, b, 1);
	else if(EQ(a, "-newer"))
		mknewer(&n, b, newer);
	else if(EQ(a, "-anewer"))
		mknewer(&n, b, anewer);
	else if(EQ(a, "-cnewer"))
		mknewer(&n, b, cnewer);
	else if(EQ(a, "-fstype")) {
#if defined (__linux__) || defined (_AIX) || defined (__hpux)
		getfstypes();
#endif	/* __linux__ || _AIX || __hpux */
		n.F = fstype, n.l.fstype = b;
		Statfs = a;
	}
	if (n.F) {
		if (incomplete)
			nxtarg(1);
		return mk(&n);
	}
err:	pr("bad option %s", a);
	usage();
	/*NOTREACHED*/
	return 0;
}
static struct anode *mk(struct anode *p)
{
	struct anode	*n;

	n = srealloc(NULL, sizeof *n);
	*n = *p;
	return(n);
}
static void oper(const char **ops)
{
	char	*a;

	a = nxtarg(-1);
	while (*ops)
		if (EQ(a, *ops++))
			er("operand follows operand");
	Ai--;
}

static char *nxtarg(int must) { /* get next arg from command line */
	static int strikes = 0;

	if(must==1 && Ai>=Argc || strikes==3)
		er("incomplete statement");
	if(Ai>=Argc) {
		if (must >= 0)
			strikes++;
		incomplete = 1;
		Ai = Argc + 1;
		return("");
	}
	return(Argv[Ai++]);
}

/* execution time functions */
static int and(register struct anode *p)
{
	return(((*p->l.L->F)(p->l.L)) && ((*p->r.R->F)(p->r.R))?1:0);
}
static int or(register struct anode *p)
{
	 return(((*p->l.L->F)(p->l.L)) || ((*p->r.R->F)(p->r.R))?1:0);
}
static int not(register struct anode *p)
{
	return( !((*p->l.L->F)(p->l.L)));
}
#if !defined (SUS) && !defined (SU3)
static int glob(register struct anode *p)
{
	extern int gmatch(const char *, const char *);
	return(gmatch(Fname, p->l.pat));
}
#else	/* SUS, SU3 */
static int glob(register struct anode *p)
{
	int	val;
#ifdef	__GLIBC__
	/* avoid glibc's broken [^...] */
	extern char	**environ;
	char	**savenv = environ;
	char	*newenv[] = { "POSIXLY_CORRECT=", NULL };
	environ = newenv;
#endif	/* __GLIBC__ */
	val = fnmatch(p->l.pat, Fname, FNM_PATHNAME) == 0;
#ifdef	__GLIBC__
	environ = savenv;
#endif	/* __GLIBC__ */
	return val;
}
#endif	/* SUS, SU3 */
/*ARGSUSED*/
static int print(register struct anode *p)
{
	puts(Pathname);
	return(1);
}
/*ARGSUSED*/
static int prune(register struct anode *p)
{
	if (!depth)
		Prune = 1;
	return(1);
}
/*ARGSUSED*/
static int null(register struct anode *p)
{
	return(1);
}
static int mtime(register struct anode *p)
{
	return(scomp((Now - Statb.st_mtime) / A_DAY, p->l.t, p->r.s));
}
static int atime(register struct anode *p)
{
	return(scomp((Now - Statb.st_atime) / A_DAY, p->l.t, p->r.s));
}
static int ctime(register struct anode *p)
{
	return(scomp((Now - Statb.st_ctime) / A_DAY, p->l.t, p->r.s));
}
static int user(register struct anode *p)
{
	return(scomp(Statb.st_uid, p->l.u, p->r.s));
}
static int ino(register struct anode *p)
{
	return(scomp(Statb.st_ino, p->l.u, p->r.s));
}
static int group(register struct anode *p)
{
	return(p->l.u == Statb.st_gid);
}
static int nogroup(register struct anode *p)
{
	return(getgroup(Statb.st_gid) == NULL);
}
static int nouser(register struct anode *p)
{
	return(getuser(Statb.st_uid) == NULL);
}
static int links(register struct anode *p)
{
	return(scomp(Statb.st_nlink, p->l.link, p->r.s));
}
static int size(register struct anode *p)
{
	return(scomp(Statb.st_size?(Statb.st_size+511)>>9:0, p->l.sz, p->r.s));
}
static int sizec(register struct anode *p)
{
	return(scomp(Statb.st_size, p->l.sz, p->r.s));
}
static int perm(register struct anode *p)
{
	register int i;
	i = (p->r.s=='-') ? p->l.per : 07777; /* '-' means only arg bits */
	return((Statb.st_mode & i & 07777) == p->l.per);
}
static int type(register struct anode *p)
{
	return((Statb.st_mode&S_IFMT)==p->l.per);
}
static int exeq(register struct anode *p)
{
	if (p->r.a) {
		if (Pathname) {
			size_t	sz = strlen(Pathname) + 1;
			if (p->r.a->a_csz + sz <= p->r.a->a_msz &&
					p->r.a->a_cur < p->r.a->a_maxarg-1) {
				strcpy(p->r.a->a_vec[p->r.a->a_cur++] =
						&p->r.a->a_spc[p->r.a->a_csz],
						Pathname);
				p->r.a->a_csz += sz;
				return 1;
			} else {
				if (p->r.a->a_cur == 0) {
					p->r.a->a_vec[p->r.a->a_cur++] =
						Pathname;
					p->r.a->a_vec[p->r.a->a_cur] = NULL;
				}
				else {
					p->r.a->a_vec[p->r.a->a_cur] = NULL;
					fflush(stdout);
					doex(p->l.com, p->r.a);
					return exeq(p);
				}
			}
		} else {
			if (p->r.a->a_cur == 0)
				return 1;
			p->r.a->a_vec[p->r.a->a_cur] = NULL;
		}
	}
	fflush(stdout); /* to flush possible `-print' */
	return(doex(p->l.com, p->r.a));
}
static int ok(struct anode *p)
{
	char c;  int yes;
	yes = 0;
	fflush(stdout); /* to flush possible `-print' */
	fprintf(stderr, "< %s ... %s >?   ", Argv[p->l.com], Pathname);
	if (read(0, &c, 1) != 1)
		exit(2);
	yes = c == 'y';
	if (c != '\n')
		while (read(0, &c, 1) == 1 && c != '\n');
	if(yes) return(doex(p->l.com, 0));
	return(0);
}

static int cpio(struct anode *p)
{
	if (strchr(Pathname, '\n')) {
		pr("file name \"%s\" contains a newline character; "
			"file not archived", Pathname);
		status |= 1;
	} else
		fprintf(p->l.fp, "%s\n", Pathname);
	return(1);
}
static int newer(register struct anode *p)
{
	return Statb.st_mtime > p->l.t;
}
static int anewer(register struct anode *p)
{
	return Statb.st_atime > p->l.t;
}
static int cnewer(register struct anode *p)
{
	return Statb.st_ctime > p->l.t;
}
static int fstype(register struct anode *p)
{
#if defined (__linux__) || defined (_AIX) || defined (__hpux)
	return(EQ(fscur->fstype, p->l.fstype));
#elif defined (__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) \
		|| defined (__DragonFly__) || defined (__APPLE__)
	return(EQ(Statfs, p->l.fstype));
#else
	return(EQ(Statb.st_fstype, p->l.fstype));
#endif
}
static int local(register struct anode *p)
{
#if defined (__linux__) || defined (_AIX) || defined (__hpux)
	return(strcmp(fscur->fstype, "nfs") && strcmp(fscur->fstype, "smbfs"));
#elif defined (__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) \
		|| defined (__DragonFly__) || defined (__APPLE__)
	return(strcmp(Statfs, "nfs") != 0);
#else
	return(strcmp(Statb.st_fstype, "nfs") != 0);
#endif
}

/* support functions */
/* funny signed compare */
static int scomp(register long long a, register long long b, register char s)
{
	if(s == '+')
		return(a > b);
	if(s == '-')
		return(a < (b * -1));
	return(a == b);
}

static int
doex(int com, struct aggregate *a)
{
	register int np;
	register char *na;
	char **oargv;
	int oargc;
	static char **nargv;
	static int narga;
	static int ccode;
	pid_t	pid;

	ccode = np = 0;
	oargv = Argv;
	oargc = com;
	while (na=oargv[oargc++]) {
		if (np >= narga-1)
			nargv = srealloc(nargv, (narga+=20) * sizeof *nargv);
		if(strcmp(na, ";")==0 && oargv == Argv) break;
		if(strcmp(na, "{}")==0 && oargv == Argv) {
			if (a) {
				oargv = a->a_vec;
				oargc = 0;
			} else
				nargv[np++] = Pathname;
		}
		else nargv[np++] = na;
	}
	if (a) {
		a->a_cur = 0;
		a->a_csz = 0;
	}
	if (np==0) return(9);
	nargv[np] = 0;
	if(pid = fork()) /*parent*/ while (wait(&ccode) != pid);
	else { /*child*/
		if (fchdir(Home) < 0) {
			pr("bad starting directory");
			_exit(1);
		}
		execvp(nargv[0], nargv);
		_exit(1);
	}
	if (a && ccode) {
		if (WIFSIGNALED(ccode))
			status |= WTERMSIG(ccode) | 0200;
		else if (WIFEXITED(ccode))
			status |= WEXITSTATUS(ccode);
	}
	return(ccode && a==NULL ? 0:1);
}

static struct aggregate *mkagg(long baselen)
{
	static size_t	envsz;
	extern char	**environ;
	register int	i;
	struct aggregate	*a;

	a = srealloc(NULL, sizeof *a);
	if (envsz == 0)
		for (i = 0; environ[i]; i++)
			envsz += strlen(environ[i]) + 1;
	a->a_msz = sysconf(_SC_ARG_MAX) - baselen - envsz - 2048;
	a->a_spc = srealloc(NULL, a->a_msz);
	a->a_maxarg = 8192;
	a->a_vec = srealloc(NULL, a->a_maxarg * sizeof *a->a_vec);
	a->a_csz = 0;
	a->a_cur = 0;
	Execplus = 1;
	return a;
}

static uid_t getunum(const char *s) { /* find user name and return number */
	struct passwd *pwd;
	char *x;
	uid_t u;

	if ((pwd = getpwnam(s)) != NULL)
		return pwd->pw_uid;
	u = strtol(s, &x, 10);
	if (*x == '\0')
		return u;
	er("cannot find %s name", s);
	/*NOTREACHED*/
	return 0;
}

static gid_t getgnum(const char *s) { /* find group name and return number */
	struct group *grp;
	char *x;
	gid_t g;

	if ((grp = getgrnam(s)) != NULL)
		return grp->gr_gid;
	g = strtol(s, &x, 10);
	if (*x == '\0')
		return g;
	er("cannot find %s name", s);
	/*NOTREACHED*/
	return 0;
}

#define	CACHESIZE	16

static const char *getuser(uid_t uid)
{
	static struct {
		char	*name;
		uid_t	uid;
	} cache[CACHESIZE];
	static int	last;
	int	i;
	struct passwd	*pwd;
	const char	*name;

	for (i = 0; i < CACHESIZE && cache[i].name; i++)
		if (cache[i].uid == uid)
			goto found;
	if ((pwd = getpwuid(uid)) != NULL)
		name = pwd->pw_name;
	else
		name = "";
	if (i >= CACHESIZE) {
		if (last >= CACHESIZE)
			last = 0;
		i = last++;
	}
	if (cache[i].name)
		free(cache[i].name);
	cache[i].name = strdup(name);
	cache[i].uid = uid;
found:	return cache[i].name[0] ? cache[i].name : NULL;
}

static const char *getgroup(gid_t gid)
{
	static struct {
		char	*name;
		gid_t	gid;
	} cache[CACHESIZE];
	static int	last;
	int	i;
	struct group	*grp;
	const char	*name;

	for (i = 0; i < CACHESIZE && cache[i].name; i++)
		if (cache[i].gid == gid)
			goto found;
	if ((grp = getgrgid(gid)) != NULL)
		name = grp->gr_name;
	else
		name = "";
	if (i >= CACHESIZE) {
		if (last >= CACHESIZE)
			last = 0;
		i = last++;
	}
	if (cache[i].name)
		free(cache[i].name);
	cache[i].name = strdup(name);
	cache[i].gid = gid;
found:	return cache[i].name[0] ? cache[i].name : NULL;
}

#if defined (__linux__) || defined (_AIX) || defined (__hpux)
static void getfscur(dev_t dev)
{
	int	i;

	for (i = 0; fstypes[i].fstype; i++)
		if (fstypes[i].fsdev == dev) {
			fscur = &fstypes[i];
			return;
		}
	er("filesystem type for %s unknown", Pathname);
}

static void getfstypes(void)
{
	struct stat	st;
	FILE	*fp;
	struct mntent	*mp;
#ifdef	__hpux
	const char mtab[] = "/etc/mnttab";
#else	/* __linux__, _AIX */
	const char mtab[] = "/etc/mtab";
#endif	/* __linux__, _AIX */
	int	i = 0;

	if (fstypes)
		return;
	if ((fp = setmntent(mtab, "r")) == NULL)
		er("cannot open %s: %s", mtab, strerror(errno));
	while ((mp = getmntent(fp)) != NULL) {
		if (EQ(mp->mnt_type, MNTTYPE_IGNORE))
			continue;
		if (stat(mp->mnt_dir, &st) < 0)
			continue;
		fstypes = srealloc(fstypes, (i+1) * sizeof *fstypes);
		fstypes[i].fsdev = st.st_dev;
		fstypes[i].fstype = strdup(mp->mnt_type);
		i++;
	}
	endmntent(fp);
}
#endif	/* __linux__ || _AIX || __hpux */

/*
 * First part of descend, called for any file found.
 */
static int descend(char *fname, struct anode *exlist, int level)
{
	struct stat	ost;
	register char *c1;
	int i;
	int rv = 0;

	if(statfn(fname, &Statb)<0) {
		if (statfn != lstat && lstat(fname, &Statb) == 0)
		nof:	c1 = "cannot follow symbolic link %s: %s";
		else if (sysv3)
			c1 = "stat() failed: %s: %s";
		else if (errno == ENOENT || errno == ENOTDIR)
			c1 = "cannot open %s: %s";
		else
			c1 = "stat() error %s: %s";
		pr(c1, Pathname, strerror(errno));
		status = 18;
		return(0);
	}
	if (level == 0 && HLflag == 'H' && (Statb.st_mode&S_IFMT) == S_IFLNK) {
		struct stat	nst;
		if (stat(fname, &nst) == 0)
			Statb = nst;
		else if (errno == ELOOP)
			goto nof;
	}
#if defined (__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) || \
		defined (__DragonFly__) || defined (__APPLE__)
	if (Statfs != NULL) {
		static struct statfs	sf;
		if (statfs(fname, &sf) < 0) {
			pr("statfs() error %s: %s", Pathname, strerror(errno));
			status = 18;
			return(0);
		}
		Statfs = sf.f_fstypename;
	}
#endif	/* __FreeBSD__, __NetBSD__, __OpenBSD__, __DragonFly__, __APPLE__ */
	if (Mount) {
		static dev_t	curdev;
		if (level == 0)
			curdev = Statb.st_dev;
		else if (curdev != Statb.st_dev)
			return(0);
	}
	Prune = 0;
	if (!depth) {
#if defined (__linux__) || defined (_AIX) || defined (__hpux)
		if (fstypes)
			getfscur(Statb.st_dev);
#endif	/* __linux__ || _AIX || __hpux */
		if((*exlist->F)(exlist) && Print)
		puts(Pathname);
	} else
		ost = Statb;
	if(Prune || (Statb.st_mode&S_IFMT)!=S_IFDIR)
		goto reg;
	if (statfn != lstat) {
		for (i = 0; i < level; i++)
			if (Statb.st_dev == visited[i].v_dev &&
					Statb.st_ino == visited[i].v_ino) {
#ifdef	SU3
				pr("Symbolic link loop at %s", Pathname);
				status = 18;
#endif	/* SU3 */
				goto reg;
			}
	}
	if (level >= vismax) {
		vismax += 20;
		visited = srealloc(visited, sizeof *visited * vismax);
	}
	visited[level].v_dev = Statb.st_dev;
	visited[level].v_ino = Statb.st_ino;

	rv = descend1(fname, exlist, level);

reg:
	if (depth) {
		Statb = ost;
#if defined (__linux__) || defined (_AIX) || defined (__hpux)
		if (fstypes)
			getfscur(Statb.st_dev);
#endif	/* __linux__ || _AIX || __hpux */
		if ((*exlist->F)(exlist) && Print)
			puts(Pathname);
	}
	return(rv);
}

/*
 * Second part of descend, called for any directory found.
 */
static int descend1(char *fname, struct anode *exlist, int level)
{
	int	dir = 0; /* open directory */
	register char *c1;
	struct getdb *db;
	register struct direc *dp;
	int endofname;
	int err;
	int oflags = O_RDONLY;

#ifdef	O_DIRECTORY
	oflags |= O_DIRECTORY;
#endif
#ifdef	O_NOFOLLOW
	if (statfn == lstat && (HLflag != 'H' || level > 0))
		oflags |= O_NOFOLLOW;
#endif
	if ((dir = open(fname, oflags)) < 0 ||
			fcntl(dir, F_SETFD, FD_CLOEXEC) < 0 ||
			fchdir(dir) < 0) {
		if (dir >= 0)
			close(dir);
		else if (errno == EMFILE && descend2(fname, exlist, level))
			/*
			 * A possible performance improvement would be to
			 * call descend2() in the directory above, since
			 * the current method involves one fork() call per
			 * subdirectory at this level. The condition occurs
			 * so rarely that it seems hardly worth optimization
			 * though.
			 */
			return 0;
		pr("cannot open %s: %s", Pathname, strerror(errno));
		status = 18;
		return 0;
	}
	if ((db = getdb_alloc(Pathname, dir)) == NULL) {
		write(2, "no memory\n", 10);
		_exit(077);
	}
	visited[level].v_db = db;
	visited[level].v_fd = dir;
	for(c1 = Pathname; *c1; ++c1);
	if(*(c1-1) == '/')
		--c1;
	endofname = c1 - Pathname;

	while ((dp = getdir(db, &err)) != NULL) {
		if((dp->d_name[0]=='.' && dp->d_name[1]=='\0') ||
				(dp->d_name[0]=='.' &&
				 dp->d_name[1]=='.' && dp->d_name[2]=='\0'))
			continue;
		setpath(&Pathname[endofname], dp->d_name, 1);
		Fname = &Pathname[endofname+1];
		if(descend(Fname, exlist, level+1)) {
			if (fchdir(dir) < 0)
				er("bad directory tree");
		}
	}
	Pathname[endofname] = '\0';
	getdb_free(db);
	if (err) {
		pr("cannot read dir %s: %s", Pathname, strerror(errno));
		status = 18;
	}
	close(dir);
	visited[level].v_fd = -1;
	return 1;
}

/*
 * Third part of descend, called if the limit of open file descriptors
 * is exceeded (EMFILE).
 */
static int descend2(char *fname, struct anode *exlist, int level)
{
	pid_t	pid;
	int	i;

	if (Cpio || Execplus)
		trailer(exlist, 0);
	fflush(stdout);
	switch (pid = fork()) {
	case 0:
		for (i = 0; i < level-1; i++) {
			if (visited[i].v_fd >= 0) {
				getdb_free(visited[i].v_db);
				close(visited[i].v_fd);
				visited[i].v_fd = -1;
			}
		}
		status |= 0;
		descend1(fname, exlist, level);
		if (Cpio || Execplus)
			trailer(exlist, 0);
		exit(status);
		/*NOTREACHED*/
	default:
		while (waitpid(pid, &i, 0) != pid);
		if (i && WIFSIGNALED(i)) {
			struct rlimit	rl;

			rl.rlim_cur = rl.rlim_max = 0;
			setrlimit(RLIMIT_CORE, &rl);
			raise(WTERMSIG(i));
			pause();
		}
		if (i)
			status |= WEXITSTATUS(i);
		return 1;
	case -1:
		return 0;
	}
}
static void setpath(char *eos, const char *fn, int slash)
{
	static char	*pathend;
	char	*opath;

	for (;;) {
		if (eos >= pathend) {
			pathend += 14;
			opath = Pathname;
			Pathname = srealloc(Pathname, pathend - Pathname);
			eos += Pathname - opath;
			pathend += Pathname - opath;
		}
		if (slash) {
			*eos++ = '/';
			slash = 0;
		} else
			if ((*eos++ = *fn++) == '\0')
				break;
	}
}

static void pr(const char *s, ...)
{
	va_list	ap;

	fprintf(stderr, "%s: ", progname);
	va_start(ap, s);
	vfprintf(stderr, s, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

static void er(const char *s, ...)
{
	va_list	ap;

	fprintf(stderr, "%s: ", progname);
	va_start(ap, s);
	vfprintf(stderr, s, ap);
	va_end(ap);
	fprintf(stderr, "\n");
	exit(1);
}

static void usage(void)
{
	er("path-list predicate-list");
}

static void *srealloc(void *op, size_t n)
{
	void	*np;

	if ((np = realloc(op, n)) == NULL) {
		write(2, "no memory\n", 10);
		_exit(077);
	}
	return np;
}

static void mkcpio(struct anode *p, const char *b, int ascii)
{
	int	fd, pd[2];
	char	flags[20], *cp;

	p->F = cpio;
	if (*b == '\0')
		return;
	depth = 1;
	Print = 0;
	Cpio = 1;
	if (pipe(pd) < 0 || (p->l.fp = fdopen(pd[1], "w")) == NULL)
		er("pipe() %s", strerror(errno));
	if ((fd = creat(b, 0666)) < 0)
		er("cannot create %s", b);
	switch (p->r.pid = fork()) {
	case -1:
		er("can't fork");
		/*NOTREACHED*/
	case 0:
		dup2(pd[0], 0);
		close(pd[0]);
		close(pd[1]);
		dup2(fd, 1);
		close(fd);
		cp = flags;
		*cp++ = '-';
		*cp++ = 'o';
		*cp++ = 'B';
		if (ascii)
			*cp++ = 'c';
		if (statfn == stat)
			*cp++ = 'L';
		*cp = '\0';
		execlp("cpio", "cpio", flags, NULL);
		pr("cannot exec cpio: %s", strerror(errno));
		_exit(0177);
		/*NOTREACHED*/
	}
	close(pd[0]);
	close(fd);
}

static void
trailer(register struct anode *p, int termcpio)
{
	char	*Opath = Pathname;
	Pathname = 0;
	if (p->F == or || p->F == and) {
		trailer(p->l.L, termcpio);
		trailer(p->r.R, termcpio);
	} else if (p->F == not)
		trailer(p->l.L, termcpio);
	else if (p->F == cpio) {
		if (termcpio) {
			int	s;

			fclose(p->l.fp);
			while (waitpid(p->r.pid, &s, 0) != p->r.pid);
			if (s) {
				if (WIFEXITED(s))
					status |= WEXITSTATUS(s);
				else if (WIFSIGNALED(s))
					status |= WTERMSIG(s) | 0200;
			}
		} else
			fflush(p->l.fp);
	} else if (p->F == exeq && p->r.a)
		exeq(p);
	Pathname = Opath;
}

static void
mknewer(struct anode *p, const char *b, int (*f)(struct anode *))
{
	if (*b && stat(b, &Statb) < 0)
		er("cannot access %s", b);
	p->l.t = Statb.st_mtime;
	p->F = f;
}

/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, September 2003.
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

static mode_t	absol(const char **);
static mode_t	who(const char **, mode_t *);
static int	what(const char **);
static mode_t	where(const char **, mode_t, int *, int *, const mode_t);

static mode_t
newmode(const char *ms, const mode_t pm)
{
	register mode_t	o, m, b;
	int	lock, setsgid = 0, cleared = 0, copy = 0;
	mode_t	nm, om, mm;
	const char *mo = ms;

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
		er("bad permissions: %s", mo);
out:	if (pm & S_IFDIR) {
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
