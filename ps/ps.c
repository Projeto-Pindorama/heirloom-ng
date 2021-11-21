/*
 * ps - process status
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, August 2002.
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
#if defined (S42)
static const char sccsid[] USED = "@(#)ps_s42.sl	2.114 (gritter) 1/12/07";
#elif defined (SUS)
static const char sccsid[] USED = "@(#)ps_sus.sl	2.114 (gritter) 1/12/07";
#elif defined (UCB)
static const char sccsid[] USED = "@(#)/usr/ucb/ps.sl	2.114 (gritter) 1/12/07";
#else
static const char sccsid[] USED = "@(#)ps.sl	2.114 (gritter) 1/12/07";
#endif

static const char cacheid[] = "@(#)/tmp/ps_cache	2.114 (gritter) 1/12/07";

#if !defined (__linux__) && !defined (__sun) && !defined (__FreeBSD__) \
	&& !defined (__DragonFly__)
#define	_KMEMUSER
#endif	/* !__linux__, !__sun, !__FreeBSD__, !__DragonFly__ */
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/utsname.h>
#ifdef	__GLIBC__
#include	<sys/sysmacros.h>
#endif
#include	<fcntl.h>
#include	<time.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>
#include	<alloca.h>
#include	<dirent.h>
#include	<limits.h>
#include	<sched.h>
#include	<pwd.h>
#include	<grp.h>
#include	<langinfo.h>
#include	<locale.h>
#include	<ctype.h>
#include	<blank.h>
#include	<inttypes.h>
#include 	<termios.h>
#if defined (__linux__)
#include	<mntent.h>
#elif defined (__FreeBSD__) || defined (__DragonFly__)
#include	<kvm.h>
#include	<sys/param.h>
#include	<sys/ucred.h>
#include	<sys/mount.h>
#include	<sys/time.h>
#include	<sys/resource.h>
#include	<sys/sysctl.h>
#include	<sys/user.h>
#define	proc	process
#undef	p_pgid
#undef	p_pctcpu
#if defined (__DragonFly__)
#endif	/* __DragonFly__ */
#elif defined (__hpux)
#include	<mntent.h>
#include	<sys/param.h>
#include	<sys/pstat.h>
#elif defined (_AIX)
#include	<mntent.h>
#include	<procinfo.h>
#define	proc	process
#ifndef	MNTTYPE_IGNORE
#define	MNTTYPE_IGNORE	""
#endif
#elif defined (__NetBSD__) || defined (__OpenBSD__) 
#include	<kvm.h>
#include	<sys/param.h>
#include	<sys/sysctl.h>
#include	<sys/mount.h>
#define	proc	process
#undef	p_pgid
#if !defined (SRUN) && defined (LSRUN)
#define	SRUN	LSRUN
#endif
#if !defined (SSLEEP) && defined (LSSLEEP)
#define	SSLEEP	LSSLEEP
#endif
#if !defined (SDEAD) && defined (LSDEAD)
#define	SDEAD	LSDEAD
#endif
#if !defined (SONPROC) && defined (LSONPROC)
#define	SONPROC	LSONPROC
#endif
#if !defined (P_INMEM) && defined (L_INMEM)
#define	P_INMEM	L_INMEM
#endif
#if !defined (P_SINTR) && defined (L_SINTR)
#define	P_SINTR	L_SINTR
#endif
#ifndef	SCHED_OTHER
#define	SCHED_OTHER	1
#endif
#elif defined (__APPLE__)
#include	<sys/proc.h>
#include        <sys/sysctl.h>
#include        <sys/mount.h>
#include	<sys/resource.h>
#include	<mach/mach_types.h>
#include	<mach/task_info.h>
#include	<mach/shared_memory_server.h>
#define	proc	process
#undef	p_pgid
#else	/* SVR4 */
#include	<sys/mnttab.h>
#ifdef	__sun
#define	_STRUCTURED_PROC	1
#endif	/* __sun */
#include	<sys/procfs.h>
#include	<sys/proc.h>
#undef	p_pid
#undef	p_wchan
#define	proc	process
#endif	/* SVR4 */
#include	<wchar.h>
#include	<wctype.h>
#ifndef	TIOCGWINSZ
#include	<sys/ioctl.h>
#endif

#if __NetBSD_Version__ >= 300000000
#include	<sys/statvfs.h>
#define	statfs	statvfs
#endif

#include	<mbtowi.h>

#ifdef	__linux__
#ifndef	SCHED_BATCH
#define	SCHED_BATCH	3
#endif
#ifndef	SCHED_ISO
#define	SCHED_ISO	4
#endif
#endif	/* __linux__ */

#define	PROCDIR		"/proc"
#ifndef	UCB
#define	DEFUNCT		"<defunct>"
#else	/* UCB */
#define	DEFUNCT		" <defunct>"
#endif	/* UCB */
#ifndef	PRNODEV
#define	PRNODEV		0
#endif	/* !PRNODEV */
#define	eq(a, b)	(strcmp(a, b) == 0)

#ifdef	__GLIBC__
#ifdef	_IO_getc_unlocked
#undef	getc
#define	getc(f)		_IO_getc_unlocked(f)
#endif	/* _IO_getc_unlocked */
#ifdef	_IO_putc_unlocked
#undef	putchar
#define	putchar(c)	_IO_putc_unlocked(c, stdout)
#endif	/* _IO_putc_unlocked */
#endif	/* __GLIBC__ */

#define	next(wc, s, n)	(mb_cur_max > 1 && *(s) & 0200 ? \
		((n) = mbtowi(&(wc), (s), mb_cur_max), \
		 (n) = ((n) > 0 ? (n) : (n) < 0 ? (wc=WEOF, 1) : 1)) :\
		((wc) = *(s) & 0377, (n) = 1))

#ifndef	_AIX
typedef	uint32_t	dev_type;
#else
typedef	uint64_t	dev_type;
#endif

enum	okay {
	OKAY,
	STOP
};

enum	crtype {
	CR_ALL,				/* -e, -A */
	CR_ALL_WITH_TTY,		/* -a */
	CR_ALL_BUT_SESSION_LEADERS,	/* -d */
	CR_WITHOUT_TTY,			/* UCB -gx */
	CR_NO_TTY_NO_SESSION_LEADER,	/* UCB -x */
	CR_PROCESS_GROUP,		/* traditional -g ... */
	CR_REAL_GID,			/* -G ... */
	CR_PROCESS_ID,			/* -p ... */
	CR_TTY_DEVICE,			/* -t ... */
	CR_SESSION_LEADER,		/* -s ..., SUS -g ... */
	CR_EFF_UID,			/* SUS -u ... */
	CR_REAL_UID,			/* -U ..., traditional -u ... */
	CR_ADD_UNINTERESTING,		/* UCB -g */
	CR_INVALID_EFF_UID,		/* invalid eff. uid but look for more */
	CR_INVALID_REAL_UID,		/* invalid real uid but look for more */
	CR_INVALID_REAL_GID,		/* invalid group but look for more */
	CR_INVALID_TTY_DEVICE,		/* invalid tty, ignore */
	CR_INVALID_STOP,		/* invalid criterion but stop later */
	CR_DEFAULT
};

enum	outype {
	OU_USER,
	OU_RUSER,
	OU_GROUP,
	OU_RGROUP,
	OU_PID,
	OU_PPID,
	OU_PGID,
	OU_PCPU,
	OU_VSZ,
	OU_NICE,
	OU_ETIME,
	OU_OTIME,
	OU_TIME,
	OU_ACCUTIME,
	OU_TTY,
	OU_COMM,
	OU_ARGS,
	OU_F,
	OU_S,
	OU_C,
	OU_UID,
	OU_RUID,
	OU_GID,
	OU_RGID,
	OU_SID,
	OU_CLASS,
	OU_PRI,
	OU_OPRI,
	OU_PSR,
	OU_ADDR,
	OU_OSZ,
	OU_WCHAN,
	OU_STIME,
	OU_RSS,
	OU_ORSS,
	OU_PMEM,
	OU_FNAME,
	OU_LWP,
	OU_NLWP,
	OU_LTIME,
	OU_STID,
	OU_TID,
	OU_NTP,
	OU_MRSZ,
	OU_PFLTS,
	OU_BUFR,
	OU_BUFW,
	OU_MRCV,
	OU_MSND,
	OU_UTIME,
	OU_KTIME,
	OU_SPACE
};

enum {
	FL_LOAD	= 001,
	FL_SYS	= 002,
	FL_LOCK	= 004,
	FL_SWAP	= 010,
	FL_TRC	= 020,
	FL_WTED	= 040
};

enum	valtype {
	VT_CHAR,
	VT_INT,
	VT_UINT,
	VT_LONG,
	VT_ULONG
};

union	value {
	char			v_char;
	int			v_int;
	unsigned int		v_uint;
	long			v_long;
	unsigned long		v_ulong;
};

struct	trenod {
	struct trenod		*t_lln;
	struct trenod		*t_rln;
	char			*t_str;
	unsigned long		t_num;
};

struct	ditem {
	struct ditem		*d_lnk;
	char			*d_str;
	dev_type		d_rdev;
};

struct	criterion {
	struct criterion	*c_nxt;
	enum crtype		c_typ;
	unsigned long		c_val;
};

struct	output {
	struct output		*o_nxt;
	enum outype		o_typ;
	char			*o_nam;
	int			o_len;
};

static const struct	{
	enum outype		os_typ;
	char			*os_fmt;
	char			*os_def;
	enum {
		OS_Lflag = 01
	}			os_flags;
} outspec[] = {
	{ OU_USER,	"user",		"    USER",	0		},
	{ OU_RUSER,	"ruser",	"   RUSER",	0		},
	{ OU_GROUP,	"group",	"   GROUP",	0		},
	{ OU_RGROUP,	"rgroup",	"  RGROUP",	0		},
	{ OU_PID,	"pid",		"  PID",	0		},
	{ OU_PPID,	"ppid",		" PPID",	0		},
	{ OU_PGID,	"pgid",		" PGID",	0		},
	{ OU_PCPU,	"pcpu",		"%CPU",		0		},
	{ OU_VSZ,	"vsz",		"   VSZ",	0		},
	{ OU_NICE,	"nice",		"NI",		0		},
	{ OU_ETIME,	"etime",	"    ELAPSED",	0		},
	{ OU_TIME,	"time",		"    TIME",	0		},
	{ OU_ACCUTIME,	"accutime",	" TIME",	0		},
	{ OU_OTIME,	"otime",	" TIME",	0		},
	{ OU_TTY,	"tty",		"TT     ",	0		},
	{ OU_COMM,	"comm",		"COMMAND",	0		},
	{ OU_ARGS,	"args",		"COMMAND",	0		},
	{ OU_F,		"f",		" F",		0		},
	{ OU_S,		"s",		"S",		0		},
	{ OU_C,		"c",		" C",		0		},
	{ OU_UID,	"uid",		"  UID",	0		},
	{ OU_RUID,	"ruid",		" RUID",	0		},
	{ OU_GID,	"gid",		"  GID",	0		},
	{ OU_RGID,	"rgid",		" RGID",	0		},
	{ OU_SID,	"sid",		"  SID",	0		},
	{ OU_CLASS,	"class",	" CLS",		0		},
	{ OU_PRI,	"pri",		"PRI",		0		},
	{ OU_OPRI,	"opri",		"PRI",		0		},
	{ OU_PSR,	"psr",		"PSR",		0		},
	{ OU_ADDR,	"addr",		"    ADDR",	0		},
	{ OU_OSZ,	"osz",		"    SZ",	0		},
	{ OU_WCHAN,	"wchan",	"   WCHAN",	0		},
	{ OU_STIME,	"stime",	"   STIME",	0		},
	{ OU_RSS,	"rss",		"  RSS",	0		},
	{ OU_ORSS,	"orss",		"  RSS",	0		},
	{ OU_PMEM,	"pmem",		"%MEM",		0		},
	{ OU_FNAME,	"fname",	"COMMAND",	0		},
	{ OU_LWP,	"lwp",		"  LWP",	OS_Lflag	},
	{ OU_NLWP,	"nlwp",		" NLWP",	0		},
	{ OU_LTIME,	"ltime",	"LTIME",	OS_Lflag	},
	{ OU_STID,	"stid",		" STID",	OS_Lflag	},
	{ OU_TID,	"tid",		"TID",		OS_Lflag	},
	{ OU_NTP,	"ntp",		"NTP",		0		},
	{ OU_MRSZ,	"mrsz",		" MRSZ",	0		},
	{ OU_PFLTS,	"pflts",	"PFLTS",	0		},
	{ OU_BUFR,	"bufr",		"  BUFR",	0		},
	{ OU_BUFW,	"bufw",		"  BUFW",	0		},
	{ OU_MRCV,	"mrcv",		"  MRCV",	0		},
	{ OU_MSND,	"msnd",		"  MSND",	0		},
	{ OU_UTIME,	"utime",	"   UTIME",	0		},
	{ OU_KTIME,	"ktime",	"   KTIME",	0		},
	{ OU_SPACE,	NULL,		" ",	 	0		}
};

struct	proc {
	pid_t		p_pid;		/* process id */
	char		p_fname[19];	/* executable name */
	char		p_state[2];	/* process state */
	char		p_lstate[2];	/* linux state */
	pid_t		p_ppid;		/* parent process id */
	pid_t		p_pgid;		/* process group */
	pid_t		p_sid;		/* session */
	pid_t		p_lwp;		/* LWP id */
	dev_type	p_ttydev;	/* tty device */
	unsigned long	p_flag;		/* process flags */
	unsigned long	p_lflag;	/* linux flags */
	time_t		p_time;		/* cpu time */
	time_t		p_accutime;	/* accumulated cpu time */
	time_t		p_utime;	/* user time */
	time_t		p_ktime;	/* kernel time */
	long		p_intpri;	/* priority value from /proc */
	long		p_rtpri;	/* rt_priority value from /proc */
	long		p_policy;	/* scheduling policy */
	int		p_c;		/* cpu usage for scheduling */
	int		p_oldpri;	/* old priority */
	int		p_pri;		/* new priority */
	int		p_nice;		/* nice value */
	int		p_nlwp;		/* number of LWPs */
	time_t		p_start;	/* start time */
	unsigned long	p_size;		/* size in kilobytes */
	unsigned long	p_osz;		/* size in pages */
	unsigned long	p_rssize;	/* rss size in kbytes */
	unsigned long	p_orss;		/* rss size in pages */
	unsigned long	p_pflts;	/* page faults */
	unsigned long	p_bufr;		/* buffer reads */
	unsigned long	p_bufw;		/* buffer writes */
	unsigned long	p_mrcv;		/* messages received */
	unsigned long	p_msnd;		/* messages sent */
	unsigned long	p_addr;		/* address */
	unsigned long	p_wchan;	/* wait channel */
	int		p_psr;		/* processor */
	double		p_pctcpu;	/* cpu percent */
	double		p_pctmem;	/* mem percent */
	char		*p_clname;	/* scheduling class */
	char		p_comm[80];	/* first argument */
	char		p_psargs[80];	/* process arguments */
	uid_t		p_uid;		/* real uid */
	uid_t		p_euid;		/* effective uid */
	gid_t		p_gid;		/* real gid */
	gid_t		p_egid;		/* effective gid */
};

static unsigned	errcnt;			/* count of errors */
static int		Lflag;		/* show LWPs */
static int		oflag;		/* had -o switch */
static const char	*rflag;		/* change root directory */
static int		ucb_rflag;	/* running processes only */
static int		dohdr;		/* output header */
#undef	hz
static long		hz;		/* clock ticks per second */
static time_t		now;		/* current time */
#ifdef	__linux__
static time_t		uptime;
#endif	/* __linux__ */
#ifndef	__sun
static unsigned long	totalmem;
#endif	/* !__sun */
static unsigned long	kbytes_per_page;
static unsigned long	pagesize;
static uid_t		myuid;		/* real uid of ps */
static uid_t		myeuid;		/* effective uid of ps */
static int		sched_selection;
static int		maxcolumn;	/* maximum terminal size */
static int		mb_cur_max;	/* MB_CUR_MAX acceleration */
static int		ontty;		/* running on a tty */
static char		*progname;	/* argv[0] to main() */
static struct proc	myproc;		/* struct proc for this ps instance */

static struct ditem	**d0;		/* dev_t to device name mapping */
static struct criterion	*c0;		/* criteria list */
static struct output	*o0;		/* output field list */

#ifdef	__linux__
static int		linux_version[3] = { 2, 4, 0 };
#endif	/* !__linux__ */

#ifdef	USE_PS_CACHE
static FILE		*devfp;
static char		*ps_cache_file = "/tmp/ps_cache";
static mode_t		ps_cache_mode = 0664;
static gid_t		ps_cache_gid = 3;
#endif	/* USE_PS_CACHE */
static int		dropprivs;

static void		postproc(struct proc *);
static enum okay	selectproc(struct proc *);

/************************************************************************
 * 			Utility functions				*
 ************************************************************************/

static void *
srealloc(void *vp, size_t nbytes)
{
	void	*p;

	if ((p = realloc(vp, nbytes)) == NULL) {
		write(2, "no memory\n", 10);
		exit(077);
	}
	return p;
}

static void *
smalloc(size_t nbytes)
{
	return srealloc(NULL, nbytes);
}

static char *
sstrdup(const char *op)
{
	char	*np;

	np = smalloc(strlen(op) + 1);
	strcpy(np, op);
	return np;
}

static void *
scalloc(size_t nmemb, size_t size)
{
	void	*p;

	if ((p = (void *)calloc(nmemb, size)) == NULL) {
		write(2, "no memory\n", 10);
		exit(077);
	}
	return p;
}

static FILE *
wopen(const char *fn)
{
	int	fd;
	char	*tl, *dn, *fc;

	dn = dirname(fc = sstrdup(fn));
	tl = smalloc(strlen(dn) + 10);
	strcpy(tl, dn);
	strcat(tl, "/psXXXXXX");
	free(fc);
	if ((fd = mkstemp(tl)) < 0)
		return NULL;
	if (rename(tl, fn) < 0) {
		unlink(tl);
		free(tl);
		close(fd);
		return NULL;
	}
	free(tl);
	return fdopen(fd, "w");
}

static struct trenod *
treget(unsigned long num, struct trenod **troot)
{
	long long	c;
	struct trenod	*tp = *troot;

	while (tp != NULL) {
		if ((c = num - tp->t_num) == 0)
			break;
		else if (c < 0)
			tp = tp->t_lln;
		else
			tp = tp->t_rln;
	}
	return tp;
}

static void
treput(struct trenod *tk, struct trenod **troot)
{
	if (*troot) {
		long long	c;
		struct trenod	*tp = *troot, *tq = NULL;

		while (tp != NULL) {
			tq = tp;
			if ((c = tk->t_num - tp->t_num) == 0)
				return;
			else if (c < 0)
				tp = tp->t_lln;
			else
				tp = tp->t_rln;
		}
		if (tq != NULL) {
			if ((c = tk->t_num - tq->t_num) < 0)
				tq->t_lln = tk;
			else
				tq->t_rln = tk;
		}
	} else
		*troot = tk;
}

#define	dhash(c)	((uint32_t)(2654435769U * (uint32_t)(c) >> 24))

static struct ditem *
dlook(dev_type rdev, struct ditem **dt, char *str)
{
	struct ditem	*dp;
	int	h;

	dp = dt[h = dhash(rdev)];
	while (dp != NULL) {
		if (dp->d_rdev == rdev)
			break;
		dp = dp->d_lnk;
	}
	if (str != NULL && dp == NULL) {
		dp = scalloc(1, sizeof *dp);
		dp->d_rdev = rdev;
		dp->d_str = str;
		dp->d_lnk = dt[h];
		dt[h] = dp;
	}
	return dp;
}

#if !defined (__hpux) && !defined (_AIX) && !defined (__NetBSD__) && \
	!defined (__OpenBSD__) && !defined (__APPLE__)
static void
chdir_to_proc(void)
{
	static int	fd = -1;

	if (fd == -1 && (fd = open(PROCDIR, O_RDONLY)) < 0) {
		fprintf(stderr, "%s: cannot open %s\n", progname, PROCDIR);
		exit(075);
	}
	if (fchdir(fd) < 0) {
		fprintf(stderr, "%s: cannot chdir to %s\n", progname, PROCDIR);
		exit(074);
	}
}
#endif	/* !__hpux, !_AIX, !__NetBSD__, !__OpenBSD__, !__APPLE__ */

static union value *
getval(char **listp, enum valtype type, int separator, int sep2)
{
	char	*buf;
	static union value	v;
	const char	*cp, *op;
	char	*cq, *x;

	if (**listp == '\0')
		return NULL;
	op = *listp;
	while (**listp != '\0') {
		if ((separator == ' ' ? isspace(**listp) : **listp == separator)
				|| **listp == sep2)
			break;
		(*listp)++;
	}
	buf = alloca(*listp - op + 1);
	for (cp = op, cq = buf; cp < *listp; cp++, cq++)
		*cq = *cp;
	*cq = '\0';
	if (**listp) {
		while ((separator == ' ' ?
				isspace(**listp) : **listp == separator) ||
				**listp == sep2)
			(*listp)++;
	}
	switch (type) {
	case VT_CHAR:
		if (buf[1] != '\0')
			return NULL;
		v.v_char = buf[0];
		break;
	case VT_INT:
		v.v_int = strtol(buf, &x, 10);
		if (*x != '\0')
			return NULL;
		break;
	case VT_UINT:
		v.v_uint = strtoul(buf, &x, 10);
		if (*x != '\0')
			return NULL;
		break;
	case VT_LONG:
		v.v_long = strtol(buf, &x, 10);
		if (*x != '\0')
			return NULL;
		break;
	case VT_ULONG:
		v.v_ulong = strtoul(buf, &x, 10);
		if (*x != '\0')
			return NULL;
		break;
	}
	return &v;
}

#ifdef	__linux__
static int
linux_version_lt(int version, int patchlevel, int sublevel)
{
	if (linux_version[0] < version)
		return 1;
	if (linux_version[0] == version) {
		if (linux_version[1] < patchlevel)
			return 1;
		if (patchlevel == linux_version[1] &&
				linux_version[2] < sublevel)
			return 1;
	}
	return 0;
}

static int
has_o1_sched(void)
{
	struct stat	st;

	if (sched_selection)
		return sched_selection > 0;
	return stat("/proc/sys/sched", &st) == 0;
}
#endif	/* __linux__ */

static int
hasnonprint(const char *s)
{
	wint_t	wc;
	int	n;

	while (*s) {
		next(wc, s, n);
		if (mb_cur_max > 1 ? !iswprint(wc) : !isprint(wc))
			return 1;
		s += n;
	}
	return 0;
}

static int
colwidth(const char *s)
{
	wint_t	wc;
	int	i, n, w = 0;

	while (*s) {
		next(wc, s, n);
		s += n;
		if (mb_cur_max > 1)
			i = iswprint(wc) ? wcwidth(wc) : 0;
		else
			i = isprint(wc) != 0;
		w += i;
	}
	return w;
}

static void
cleanline(struct proc *p)
{
	/*
	 * If the argument list contains a nonprintable character,
	 * replace it with the file name even if output is not a
	 * terminal.
	 */
	if (*p->p_psargs == '\0' || hasnonprint(p->p_psargs)) {
		if (p->p_size == 0 && *p->p_psargs == '\0')
			strcpy(p->p_psargs, p->p_fname);
		else
			snprintf(p->p_psargs, sizeof p->p_psargs, "[ %.8s ]",
				p->p_fname);
		strcpy(p->p_comm, p->p_psargs);
	}
}

/************************************************************************
 * 				Execution				*
 ************************************************************************/

static void
putheader(void)
{
	struct output	*o;
	unsigned	i;

	for (o = o0; o; o = o->o_nxt) {
		if (*o->o_nam == '\0') {
			for (i = 0; i < o->o_len; i++)
				putchar(' ');
		} else
			fputs(o->o_nam, stdout);
		if (o->o_nxt && o->o_typ != OU_SPACE)
			putchar(' ');
	}
	putchar('\n');
}

/*
 * Print a string, not exceeding the maximum output width, but with at least
 * minimum columns. Drop nonprintable characters if printing to a terminal.
 */
static int
putstr(int width, int minimum, int maximum, const char *s)
{
	wint_t	wc;
	int	written = 0, n, cw;

	while (next(wc, s, n), cw = wcwidth(wc), cw = cw >= 0 ? cw : 1,
			wc != '\0' &&
			(maxcolumn == 0 || width + cw <= maxcolumn) &&
			(maximum == 0 || written + cw <= maximum)) {
		if (!ontty || (mb_cur_max > 1 ? iswprint(wc) : isprint(wc))) {
			while (n--) {
				putchar(*s);
				s++;
			}
			written += cw;
			width += cw;
		} else
			s += n;
	}
	while ((maxcolumn == 0 || width < maxcolumn) && written < minimum &&
			(maximum == 0 || written < maximum)) {
		putchar(' ');
		written++;
	}
	return written;
}

/*
 * Print a hexadecimal value with a maximum width, preceded by spaces
 * if it is short.
 *
 * This is used for ADDR and WCHAN. Truncating the addresses to keep
 * the display columns in order makes sense here since ADDR serves no
 * known purpose anymore, and for WCHAN only the lower part of the
 * address is relevant.
 */
static int
putxd(int width, unsigned long val)
{
	const char	digits[] = "0123456789abcdef";
	char	*buf = alloca(width);
	int	m, n = width;

	do {
		buf[--n] = digits[val & 0xf];
		val >>= 4;
	} while (val != 0 && n > 0);
	for (m = 0; m < n; m++)
		putchar(' ');
	do
		putchar(buf[n]);
	while (++n < width);
	return width;
}

static int
putid(unsigned long val, unsigned len, struct trenod **troot,
		char *(*func)(unsigned long))
{
	struct trenod	*tp;
	char	*str;

	if ((tp = treget(val, troot)) == NULL) {
		if ((str = func(val)) != NULL) {
			tp = scalloc(1, sizeof *tp);
			tp->t_str = smalloc(strlen(str) + 1);
			strcpy(tp->t_str, str);
			tp->t_num = val;
			treput(tp, troot);
		} else
		numeric:
#ifdef	UCB
			return printf("%-*lu", len, val);
#else
			return printf("%*lu", len, val);
#endif
	}
	if (oflag && colwidth(tp->t_str) > len)
		goto numeric;
#ifdef	UCB
	return printf("%-*s", len, tp->t_str);
#else
	return printf("%*s", len, tp->t_str);
#endif
}

static char *
get_username_from_pwd(unsigned long uid)
{
	struct passwd	*pwd;

	if ((pwd = getpwuid(uid)) != NULL)
		return pwd->pw_name;
	return NULL;
}

static char *
get_groupname_from_grp(unsigned long gid)
{
	struct group	*grp;

	if ((grp = getgrgid(gid)) != NULL)
		return grp->gr_name;
	return NULL;
}

static int
putuser(uid_t uid, unsigned len)
{
	static struct trenod	*u0;

	return putid(uid, len, &u0, get_username_from_pwd);
}

static int
putgroup(gid_t gid, unsigned len)
{
	static struct trenod	*g0;

	return putid(gid, len, &g0, get_groupname_from_grp);
}

static int
putdev(dev_type dev, unsigned len)
{
	struct ditem	*d;
	char	*nam;

	if (dev != (dev_type)PRNODEV) {
		if ((d = dlook(dev, d0, NULL)) != NULL)
			nam = d->d_str;
		else
			nam = "??";
	} else
		nam = "?";
	return printf("%-*s", len, nam);
}

static int
time2(long t, unsigned len, int format)
{
	char	buf[40];
	int	days, hours, minutes, seconds;

	if (t < 0)
		t = 0;
	if (format == 2)
		snprintf(buf, sizeof buf, "%2lu:%02lu.%ld", t / 600,
				(t/10) % 60,
				t % 10);
	else if (format == 1)
		snprintf(buf, sizeof buf, "%2lu:%02lu", t / 60, t % 60);
	else {
		days = t / 86400;
		t %= 86400;
		hours = t / 3600;
		t %= 3600;
		minutes = t / 60;
		t %= 60;
		seconds = t;
		if (days)
			snprintf(buf, sizeof buf, "%02u-:%02u:%02u:%02u",
					days, hours, minutes, seconds);
		else
			snprintf(buf, sizeof buf, "%02u:%02u:%02u",
					hours, minutes, seconds);
	}
	return printf("%*s", len, buf);
}

static int
time3(time_t t, unsigned len)
{
	struct tm	*tp;
	int	sz = 8, width = 0;

	while (sz++ < len) {
		putchar(' ');
		width++;
	}
	tp = localtime(&t);
	if (now > t && now - t > 86400) {
		nl_item	val;

		switch (tp->tm_mon) {
		case 0:		val = ABMON_1;	break;
		case 1:		val = ABMON_2;	break;
		case 2:		val = ABMON_3;	break;
		case 3:		val = ABMON_4;	break;
		case 4:		val = ABMON_5;	break;
		case 5:		val = ABMON_6;	break;
		case 6:		val = ABMON_7;	break;
		case 7:		val = ABMON_8;	break;
		case 8:		val = ABMON_9;	break;
		case 9:		val = ABMON_10;	break;
		case 10:	val = ABMON_11;	break;
		case 11:	val = ABMON_12;	break;
		default:	val = ABMON_12;	/* won't happen anyway */
		}
		width += printf("  %s %02u", nl_langinfo(val), tp->tm_mday);
	} else
		width += printf("%02u:%02u:%02u",
				tp->tm_hour, tp->tm_min, tp->tm_sec);
	return width;
}

#define	ZOMBIE(a)	(p->p_lstate[0] != 'Z' ? (a) : \
				printf("%-*s", o->o_len, oflag ? "-" : " "))

static void
outproc(struct proc *p)
{
	struct output	*o;
	int	width = 0;

	for (o = o0; o; o = o->o_nxt) {
		switch (o->o_typ) {
		case OU_USER:
			width += putuser(p->p_euid, o->o_len);
			break;
		case OU_RUSER:
			width += putuser(p->p_uid, o->o_len);
			break;
		case OU_RGROUP:
			width += putgroup(p->p_gid, o->o_len);
			break;
		case OU_GROUP:
			width += putgroup(p->p_egid, o->o_len);
			break;
		case OU_PID:
			width += printf("%*u", o->o_len, (int)p->p_pid);
			break;
		case OU_PPID:
			width += printf("%*u", o->o_len, (int)p->p_ppid);
			break;
		case OU_PGID:
			width += printf("%*u", o->o_len, (int)p->p_pgid);
			break;
		case OU_LWP:
		case OU_STID:
			width += ZOMBIE(printf("%*u", o->o_len, (int)p->p_lwp));
			break;
		case OU_PCPU:
			width += printf("%*.1f", o->o_len, p->p_pctcpu);
			break;
		case OU_VSZ:
			width += ZOMBIE(printf("%*lu", o->o_len,
						(long)p->p_size));
			break;
		case OU_NICE:
			if (p->p_policy == SCHED_OTHER && p->p_pid != 0) {
				width += ZOMBIE(printf("%*d", o->o_len,
							(int)p->p_nice));
			} else {
				width += ZOMBIE(printf("%*.*s",
							o->o_len, o->o_len,
							p->p_clname));
			}
			break;
		case OU_NLWP:
			width += ZOMBIE(printf("%*u", o->o_len, p->p_nlwp));
			break;
		case OU_NTP:
			width += ZOMBIE(printf("%*u", o->o_len,
					p->p_nlwp > 1 ? p->p_nlwp : 0));
			break;
		case OU_TID:
			width += ZOMBIE(printf("%*s", o->o_len, "-"));
			break;
		case OU_ETIME:
			width += time2(now - p->p_start, o->o_len, 0);
			break;
		case OU_TTY:
			width += ZOMBIE(putdev(p->p_ttydev, o->o_len));
			break;
		case OU_LTIME:
		case OU_OTIME:
			width += time2(p->p_time, o->o_len, 1);
			break;
		case OU_TIME:
			width += time2(p->p_time, o->o_len, 0);
			break;
		case OU_ACCUTIME:
			width += time2(p->p_accutime, o->o_len, 1);
			break;
		case OU_UTIME:
			width += time2(p->p_utime, o->o_len, 2);
			break;
		case OU_KTIME:
			width += time2(p->p_ktime, o->o_len, 2);
			break;
		case OU_COMM:
			width += putstr(width, o->o_nxt ? o->o_len : 0, 0,
					p->p_lstate[0] != 'Z' ?
					p->p_comm : DEFUNCT);
			break;
		case OU_ARGS:
			width += putstr(width, o->o_nxt ? o->o_len : 0, 0,
					p->p_lstate[0] != 'Z' ? 
					p->p_psargs : DEFUNCT);
			break;
		case OU_F:
			width += printf("%*o", o->o_len,
					(int)(p->p_flag & 077));
			break;
		case OU_S:
			width += printf("%*s", o->o_len, p->p_state);
			break;
		case OU_C:
			width += printf("%*d", o->o_len, p->p_c);
			break;
		case OU_UID:
			width += printf("%*u", o->o_len, (int)p->p_euid);
			break;
		case OU_RUID:
			width += printf("%*u", o->o_len, (int)p->p_uid);
			break;
		case OU_GID:
			width += printf("%*u", o->o_len, (int)p->p_egid);
			break;
		case OU_RGID:
			width += printf("%*u", o->o_len, (int)p->p_gid);
			break;
		case OU_SID:
			width += printf("%*u", o->o_len, (int)p->p_sid);
			break;
		case OU_CLASS:
			width += ZOMBIE(printf("%*s", o->o_len, p->p_clname));
			break;
		case OU_PRI:
			width += ZOMBIE(printf("%*d", o->o_len, (int)p->p_pri));
			break;
		case OU_OPRI:
			width += ZOMBIE(printf("%*d", o->o_len,
						(int)p->p_oldpri));
			break;
		case OU_PSR:
			width += printf("%*d", o->o_len, (int)p->p_psr);
			break;
		case OU_ADDR:
			width += ZOMBIE(putxd(o->o_len, (long)p->p_addr));
			break;
		case OU_OSZ:
			width += ZOMBIE(printf("%*lu", o->o_len,
						(long)p->p_osz));
			break;
		case OU_WCHAN:
			if (p->p_lstate[0] == 'S' || p->p_lstate[0] == 'X' ||
					p->p_lstate[0] == 'D')
				width += putxd(o->o_len, (long)p->p_wchan);
			else
				width += printf("%*s", o->o_len, " ");
			break;
		case OU_STIME:
			width += ZOMBIE(time3(p->p_start, o->o_len));
			break;
		case OU_RSS:
			width += ZOMBIE(printf("%*lu", o->o_len,
						(long)p->p_rssize));
			break;
		case OU_ORSS:
		case OU_MRSZ:
			width += ZOMBIE(printf("%*lu", o->o_len,
						(long)p->p_orss));
			break;
		case OU_PMEM:
			width += printf("%*.1f", o->o_len, p->p_pctmem);
			break;
		case OU_PFLTS:
			width += printf("%*lu", o->o_len, p->p_pflts);
			break;
		case OU_BUFW:
			width += printf("%*lu", o->o_len, p->p_bufw);
			break;
		case OU_BUFR:
			width += printf("%*lu", o->o_len, p->p_bufr);
			break;
		case OU_MRCV:
			width += printf("%*lu", o->o_len, p->p_mrcv);
			break;
		case OU_MSND:
			width += printf("%*lu", o->o_len, p->p_msnd);
			break;
		case OU_FNAME:
			width += putstr(width, o->o_nxt ? o->o_len : 0,
#ifndef	UCB
					p->p_lstate[0] != 'Z' ? 8 : 9,
#else	/* UCB */
					16,
#endif	/* UCB */
				p->p_lstate[0] != 'Z' ? 
				p->p_fname : DEFUNCT);
			break;
		case OU_SPACE:
			if (o->o_len > 1)
				width += printf("%*s", o->o_len - 1, "");
			break;
		}
		if (o->o_nxt) {
			putchar(' ');
			width++;
		}
	}
	putchar('\n');
}

#if !defined (__hpux) && !defined (_AIX) && !defined (__NetBSD__) && \
	!defined (__OpenBSD__) && !defined (__APPLE__)

#if defined (__linux__) || defined (__FreeBSD__) || defined (__DragonFly__)
#define	GETVAL_REQ(a)		if ((v = getval(&cp, (a), ' ', 0)) == NULL) \
					return STOP

#define	GETVAL_OPT(a)		if ((v = getval(&cp, (a), ' ', 0)) == NULL) \
					goto complete

#define	GETVAL_COMMA(a)		if ((v = getval(&cp, (a), ' ', ',')) == NULL) \
					return STOP
#endif	/* __linux__ || __FreeBSD__ || __DragonFly__ */

#if defined (__linux__)
static void
get_linux_version(void)
{
	struct utsname	ut;
	char	*x;
	long	val;

	if (uname(&ut) == 0) {
		if ((val = strtol(ut.release, &x, 10)) > 0 &&
				(*x == '.' || *x == '\0')) {
			linux_version[0] = val;
			if (*x && (val = strtol(&x[1], &x, 10)) >= 0 &&
					(*x == '.' || *x == '\0')) {
				linux_version[1] = val;
				if (*x && (val = strtol(&x[1], &x, 10)) >= 0)
					linux_version[2] = val;
			}
		}
	}
}

static time_t
sysup(void)
{
	FILE	*fp;
	char	buf[32];
	char	*cp;
	union value	*v;
	time_t	s = 0;

	if ((fp = fopen("uptime", "r")) == NULL)
		return 0;
	if (fread(buf, 1, sizeof buf, fp) > 0) {
		cp = buf;
		if ((v = getval(&cp, VT_ULONG, '.', 0)) != NULL)
			s = v->v_ulong;
	}
	fclose(fp);
	return s;
}

static unsigned long
getmem(void)
{
	FILE	*fp;
	char	line[LINE_MAX];
	char	*cp;
	union value	*v;
	unsigned long	mem = 1;

	if ((fp = fopen("meminfo", "r")) == NULL)
		return 0;
	while (fgets(line, sizeof line, fp) != NULL) {
		if (strncmp(line, "MemTotal:", 9) == 0) {
			cp = &line[9];
			while (isspace(*cp))
				cp++;
			if ((v = getval(&cp, VT_ULONG, ' ', 0)) != NULL)
				mem = v->v_ulong;
			break;
		}
	}
	fclose(fp);
	return mem;
}

static time_t
hz2time(long val, int mult)
{
	long long	t;

	t = val * mult / hz;
	if ((val * mult) % hz >= (hz >> 1))
		t++;
	return t;
}

static void	(*compute_priority)(struct proc *);

/*
 * Calculate reasonable values for priority fields using all we can get
 * from /proc in Linux 2.4: a crippled counter (in p->intpri) and the
 * nice value.
 */
static void
compute_priority_old(struct proc *p)
{
	static int	def_counter, scale, max_goodness;
	int	full_counter, counter, goodness;

	/*
	 * This is based on the computations in linux/sched.c, 2.4.19.
	 */
	if (def_counter == 0) {
		def_counter = 10 * hz / 100;
		if (hz < 200)
			scale = 4;
		else if (hz < 400)
			scale = 3;
		else if (hz < 800)
			scale = 2;
		else if (hz < 1600)
			scale = 1;
		else
			scale = 0;
		max_goodness = (((40 << 3) >> scale) + 2) + 40;
	}
	full_counter = (((40 - p->p_nice) << 3) >> scale) + 2;
	/*
	 * Try to reverse the computation in linux/fs/proc/array.c,
	 * 2.4.19.
	 */
	counter = (def_counter * (20 - p->p_intpri)) / 10;
	/*
	 * This can apparently happen if the command is in its first
	 * timeslice after a lower nice value has been set.
	 */
	if (counter > full_counter)
		counter = full_counter;
	/*
	 * This approximation is even worse, as we cannot know about
	 * PROC_CHANGE_PENALTY and MM.
	 */
	if ((goodness = counter) > 0)
		goodness += 40 - p->p_nice;
	/*
	 * Keep all priorities for -c below 60 and with higher
	 * priorities for higher numbers.
	 */
	p->p_pri = goodness * 59 / max_goodness;
	/*
	 * Old-style priorities start at 60 and have lower numbers
	 * for higher priorities.
	 */
	p->p_oldpri = 119 - p->p_pri;
	/*
	 * Our counter emulation can be wrong by 2 in the worst
	 * case. If the process is not currently on a run queue,
	 * assume it did not use the CPU at all.
	 */
	p->p_c = full_counter - counter;
	if (p->p_lstate[0] != 'R' && p->p_c <= 2)
		p->p_c = 0;
	/*
	 * The value for C still depends on the nice value. Make 80
	 * the highest possible C value for all nice values.
	 */
	p->p_c *= 80 / full_counter;
}

/*
 * Priority calculation for Linux 2.5 and (hopefully) above, based
 * on 2.5.31. This supplies a sensible priority value, but originally
 * nothing we could use to compute "CPU usage for scheduling". More
 * recent 2.6 versions have a SleepAVG field in the "status" file.
 */
static void
compute_priority_new(struct proc *p)
{
	if (p->p_rtpri) {
		p->p_pri = 100 + p->p_rtpri;
		p->p_oldpri = 60 - (p->p_rtpri >> 1);
	} else {
		p->p_pri = 40 - p->p_intpri;
		p->p_oldpri = 60 + p->p_intpri + (p->p_intpri >> 1);
	}
}

static void
compute_various(struct proc *p)
{
	/*
	 * All dead processes are considered zombies by us.
	 */
	if (p->p_lstate[0] == 'X')
		p->p_lstate[0] = 'Z';
	/*
	 * Set System V style status. There seems no method to
	 * determine 'O' (not only on run queue, but actually
	 * running).
	 */
	if (p->p_lstate[0] == 'D' || p->p_lstate[0] == 'W')
		p->p_state[0] = 'S';
	else
		p->p_state[0] = p->p_lstate[0];
#ifdef	notdef
	/*
	 * Process flags vary too much between real and vendor kernels
	 * and there's no method to distinguish them - don't use.
	 */
	if (p->p_lflag & 0x00000002)		/* PF_STARTING */
		p->p_state[0] = 'I';
	else if (p->p_lflag & 0x00000800)	/* PF_MEMALLOC */
		p->p_state[0] = 'X';
#endif	/* notdef */
	/*
	 * Set v7 / System III style flags.
	 */
	if (p->p_lstate[0] != 'Z') {
		if (p->p_flag & FL_SYS || p->p_rssize != 0)
			p->p_flag |= FL_LOAD;	/* cf. statm processing */
		else
			p->p_flag |= FL_SWAP;	/* no rss -> swapped */
		if (p->p_lstate[0] == 'D') {
			p->p_flag |= FL_LOCK;
			p->p_flag &= ~FL_SWAP;
		} else if (p->p_lstate[0] == 'W')
			p->p_flag |= FL_SWAP;
		/*if (p->p_lflag & 0x10)		obsolete, doesn't work
			p->p_flag |= FL_TRC;*/
	}
}

static enum okay
getproc_stat(struct proc *p, pid_t expected_pid)
{
	static char	*buf;
	static size_t	buflen;
	union value	*v;
	FILE	*fp;
	char	*cp, *cq, *ce;
	size_t	sz, sc;
	unsigned long	lval;
	/*
	 * There is no direct method to determine if something is a system
	 * process. We consider a process a system process if a certain set
	 * of criteria is entirely zero.
	 */
	unsigned long	sysfl = 0;

	if ((fp = fopen("stat", "r")) == NULL)
		return STOP;
	for (cp = buf; ;) {
		const unsigned	chunk = 32;

		if (buflen < (sz = cp - buf + chunk)) {
			sc = cp - buf;
			buf = srealloc(buf, buflen = sz);
			cp = &buf[sc];
		}
		if ((sz = fread(cp, 1, chunk, fp)) < chunk) {
			ce = &cp[sz - 1];
			break;
		}
		cp += chunk;
	}
	fclose(fp);
	if (*ce != '\n')
		return STOP;
	*ce-- = '\0';
	cp = buf;
	/* pid */
	GETVAL_REQ(VT_INT);
	if ((p->p_pid = v->v_int) != expected_pid)
		return STOP;
	if (*cp++ != '(')
		return STOP;
	for (cq = ce; cq >= cp && *cq != ')'; cq--);
	if (cq < cp)
		return STOP;
	*cq = '\0';
	strncpy(p->p_fname, cp, sizeof p->p_fname);
	p->p_fname[sizeof p->p_fname - 1] = '\0';
	cp = &cq[1];
	while (isspace(*cp))
		cp++;
	/* state */
	GETVAL_REQ(VT_CHAR);
	p->p_lstate[0] = v->v_char;
	sysfl |= v->v_char == 'Z';
	/* ppid */
	GETVAL_REQ(VT_INT);
	p->p_ppid = v->v_int;
	/* pgrp */
	GETVAL_REQ(VT_INT);
	p->p_pgid = v->v_int;
	/* session */
	GETVAL_REQ(VT_INT);
	p->p_sid = v->v_int;
	/* tty_nr */
	GETVAL_REQ(VT_INT);
	p->p_ttydev = v->v_int;
	sysfl |= v->v_int;
	/* tty_pgrp */
	GETVAL_REQ(VT_INT);
	/* flags */
	GETVAL_REQ(VT_ULONG);
	p->p_lflag = v->v_ulong;
	/* minflt */
	GETVAL_REQ(VT_ULONG);
	/* cminflt */
	GETVAL_REQ(VT_ULONG);
	/* majflt */
	GETVAL_REQ(VT_ULONG);
	p->p_pflts = v->v_ulong;
	/* cmajflt */
	GETVAL_REQ(VT_ULONG);
	/* utime */
	GETVAL_REQ(VT_ULONG);
	lval = v->v_ulong;
	p->p_utime = hz2time(lval, 10);
	sysfl |= v->v_ulong;
	/* stime */
	GETVAL_REQ(VT_ULONG);
	p->p_ktime = hz2time(v->v_ulong, 10);
	lval += v->v_ulong;
	p->p_time = hz2time(lval, 1);
	/* cutime */
	GETVAL_REQ(VT_LONG);
	lval += v->v_ulong;
	/* cstime */
	GETVAL_REQ(VT_LONG);
	lval += v->v_ulong;
	p->p_accutime += hz2time(lval, 1);
	/* priority */
	GETVAL_REQ(VT_LONG);
	p->p_intpri = v->v_long;
	/* nice */
	GETVAL_REQ(VT_LONG);
	p->p_nice = v->v_long + 20;
	/* timeout */
	GETVAL_REQ(VT_LONG);
	/* itrealvalue */
	GETVAL_REQ(VT_LONG);
	/* starttime */
	GETVAL_REQ(VT_ULONG);
	p->p_start = hz2time(v->v_ulong, 1) + now - uptime;
	/* vsize */
	GETVAL_REQ(VT_ULONG);
	p->p_size = (v->v_ulong >> 10);
	p->p_osz = v->v_ulong / pagesize;
	sysfl |= v->v_ulong;
	/* rss */
	GETVAL_REQ(VT_LONG);
	p->p_orss = v->v_long;
	p->p_rssize = v->v_long * kbytes_per_page;
	sysfl |= v->v_ulong;
	/* rlim */
	GETVAL_REQ(VT_ULONG);
	/* startcode */
	GETVAL_REQ(VT_ULONG);
	p->p_addr = v->v_ulong;
	sysfl |= v->v_ulong;
	/* endcode */
	GETVAL_REQ(VT_ULONG);
	sysfl |= v->v_ulong;
	/* startstack */
	GETVAL_REQ(VT_ULONG);
	sysfl |= v->v_ulong;
	/* kstkesp */
	GETVAL_REQ(VT_ULONG);
	/* kstkeip */
	GETVAL_REQ(VT_ULONG);
	/* signal */
	GETVAL_REQ(VT_ULONG);
	/* blocked */
	GETVAL_REQ(VT_ULONG);
	/* sigignore */
	GETVAL_REQ(VT_ULONG);
	/* sigcatch */
	GETVAL_REQ(VT_ULONG);
	/* wchan */
	GETVAL_REQ(VT_ULONG);
	p->p_wchan = v->v_ulong;
	/*
	 * These appeared in later Linux versions, so they are not
	 * required to be present.
	 */
	p->p_policy = -1;	/* initialize to invalid values */
	/* nswap */
	GETVAL_OPT(VT_ULONG);
	/* cnswap */
	GETVAL_OPT(VT_ULONG);
	/* exit_signal */
	GETVAL_OPT(VT_INT);
	/* processor */
	GETVAL_OPT(VT_INT);
	p->p_psr = v->v_int;
	/* rt_priority */
	GETVAL_OPT(VT_ULONG);
	p->p_rtpri = v->v_ulong;
	/* policy */
	GETVAL_OPT(VT_ULONG);
	p->p_policy = v->v_ulong;
complete:
	if (sysfl == 0)
		p->p_flag |= FL_SYS;
	compute_various(p);
	return OKAY;
}

static enum okay
getproc_scheduler(struct proc *p)
{
	struct sched_param	s;

	if (p->p_policy == -1)	/* Linux 2.4 and below */
		p->p_policy = sched_getscheduler(p->p_pid);
	switch (p->p_policy) {
	case SCHED_FIFO:
	case SCHED_RR:
		switch (p->p_policy) {
		case SCHED_FIFO:	p->p_clname = "FF"; break;
#ifdef	S42
		case SCHED_RR:		p->p_clname = "FP"; break;
#else
		case SCHED_RR:		p->p_clname = "RT"; break;
#endif
		}
		if (p->p_rtpri == 0 && sched_getparam(p->p_pid, &s) == 0) {
			p->p_rtpri = s.sched_priority;
			/* Linux 2.4 and below */
			p->p_pri = 100 + s.sched_priority;
		}
		break;
	case SCHED_OTHER:
		p->p_clname = "TS";
		break;
#ifdef	SCHED_BATCH
	case SCHED_BATCH:
		p->p_clname = "B";
		break;
#endif	/* SCHED_BATCH */
#ifdef	SCHED_ISO
	case SCHED_ISO:
		p->p_clname = "ISO";
		break;
#endif	/* SCHED_ISO */
	default:
		p->p_clname =  "??";
	}
	compute_priority(p);
	return OKAY;
}

static enum okay
getproc_cmdline(struct proc *p)
{
	FILE	*fp;
	char	*cp, *cq, *ce;
	int	hadzero = 0, c;

	if ((fp = fopen("cmdline", "r")) != NULL) {
		cp = p->p_psargs;
		cq = p->p_comm;
		ce = cp + sizeof p->p_psargs - 1;
		while (cp < ce && (c = getc(fp)) != EOF) {
			if (c != '\0') {
				if (hadzero) {
					*cp++ = ' ';
					if (cp == ce)
						break;
					hadzero = 0;
				}
				*cp++ = c;
				if (cq)
					*cq++ = c;
			} else {
				hadzero = 1;
				if (cq) {
					*cq = c;
					cq = NULL;
				}
			}
		}
		*cp = '\0';
		if (cq)
			*cq = '\0';
		fclose(fp);
	}
	return OKAY;
}

static enum okay
getproc_status(struct proc *p)
{
	char	line[LINE_MAX];
	union value	*v;
	FILE	*fp;
	char	*cp;
	int	scanr;

	if ((fp = fopen("status", "r")) == NULL)
		return STOP;
	scanr = 0;
	while (fgets(line, sizeof line, fp) != NULL) {
		if (strncmp(line, "Uid:", 4) == 0) {
			cp = &line[4];
			while (isspace(*cp))
				cp++;
			if ((v = getval(&cp, VT_INT, ' ', 0)) == NULL) {
				fclose(fp);
				return STOP;
			}
			p->p_uid = v->v_int;
			if ((v = getval(&cp, VT_INT, ' ', 0)) == NULL) {
				fclose(fp);
				return STOP;
			}
			p->p_euid = v->v_int;
			scanr++;
		} else if (strncmp(line, "Gid:", 4) == 0) {
			cp = &line[4];
			while (isspace(*cp))
				cp++;
			if ((v = getval(&cp, VT_INT, ' ', 0)) == NULL) {
				fclose(fp);
				return STOP;
			}
			p->p_gid = v->v_int;
			if ((v = getval(&cp, VT_INT, ' ', 0)) == NULL) {
				fclose(fp);
				return STOP;
			}
			p->p_egid = v->v_int;
			scanr++;
		} else if (strncmp(line, "Threads:", 8) == 0) {
			cp = &line[8];
			while (isspace(*cp))
				cp++;
			if ((v = getval(&cp, VT_INT, ' ', 0)) == NULL) {
				fclose(fp);
				return STOP;
			}
			p->p_nlwp = v->v_int;
		} else if (strncmp(line, "Pid:", 4) == 0) {
			cp = &line[4];
			while (isspace(*cp))
				cp++;
			if ((v = getval(&cp, VT_INT, ' ', 0)) == NULL) {
				fclose(fp);
				return STOP;
			}
			p->p_lwp = v->v_int;
		} else if (strncmp(line, "SleepAVG:", 9) == 0) {
			cp = &line[9];
			while (isspace(*cp))
				cp++;
			if ((v = getval(&cp, VT_INT, '%', 0)) == NULL) {
				fclose(fp);
				return STOP;
			}
			p->p_c = (100 - v->v_int) * 80 / 100;
		}
	}
	fclose(fp);
	if (scanr != 2)
		return STOP;
	return OKAY;
}

static enum okay
getproc_statm(struct proc *p)
{
	char	line[LINE_MAX];
	union value	*v;
	FILE	*fp;
	char	*cp;
	unsigned long	trs, drs, dt;

	if ((fp = fopen("statm", "r")) == NULL)
		return OKAY;	/* not crucial */
	if (fgets(line, sizeof line, fp) != NULL) {
		cp = line;
		if ((v = getval(&cp, VT_LONG, ' ', 0)) == NULL)	/* size */
			goto out;
		if ((v = getval(&cp, VT_LONG, ' ', 0)) == NULL)	/* resident */
			goto out;
		if ((v = getval(&cp, VT_LONG, ' ', 0)) == NULL)	/* share */
			goto out;
		if ((v = getval(&cp, VT_LONG, ' ', 0)) == NULL)	/* trs */
			goto out;
		trs = v->v_long;
		if ((v = getval(&cp, VT_LONG, ' ', 0)) == NULL)	/* drs */
			goto out;
		drs = v->v_long;
		if ((v = getval(&cp, VT_LONG, ' ', 0)) == NULL)	/* lrs */
			goto out;
		if ((v = getval(&cp, VT_LONG, ' ', 0)) == NULL)	/* dt */
			goto out;
		dt = v->v_long;
		/*
		 * A process is considered to be swapped out if it has
		 * neither resident non-library text, data, nor dirty
		 * pages. A system process is always considered to be
		 * in core.
		 */
		if (trs + drs + dt == 0 &&
				(p->p_flag&(FL_LOAD|FL_SYS|FL_LOCK))==FL_LOAD) {
			p->p_flag &= ~FL_LOAD;
			p->p_flag |= FL_SWAP;
		}
	}
out:	fclose(fp);
	return OKAY;
}

static enum okay
getproc(const char *dir, struct proc *p, pid_t expected_pid, pid_t lwp)
{
	enum okay	result;

	memset(p, 0, sizeof *p);
	if (chdir(dir) == 0) {
		if ((result = getproc_stat(p, expected_pid)) == OKAY)
			if ((result = getproc_scheduler(p)) == OKAY)
				if ((result = getproc_cmdline(p)) == OKAY)
					if ((result= getproc_status(p)) == OKAY)
						result = getproc_statm(p);
		chdir_to_proc();
	} else
		result = STOP;
	return result;
}

static enum okay
getLWPs(const char *dir, struct proc *p, pid_t expected_pid)
{
	DIR	*Dp;
	struct dirent	*dp;
	unsigned long	val;
	char	*x;
	int	fd;

	if (chdir(dir) == 0 &&
			(fd = open("task", O_RDONLY)) >= 0 &&
			fchdir(fd) == 0 &&
			(Dp = opendir(".")) != NULL) {
		while ((dp = readdir(Dp)) != NULL) {
			if (dp->d_name[0] == '.' && (dp->d_name[1]=='\0' ||
					(dp->d_name[1]=='.' &&
					 dp->d_name[2]=='\0')))
			continue;
			val = strtoul(dp->d_name, &x, 10);
			if (*x != 0)
				continue;
			if (fchdir(fd) < 0) {
				fprintf(stderr,
					"%s: cannot chdir to %s/%s/task\n",
					progname, PROCDIR, dir);
				errcnt = 1;
				break;
			}
			if (getproc(dp->d_name, p, val, val) == OKAY) {
				postproc(p);
				if (selectproc(p) == OKAY) {
					p->p_pid = expected_pid;
					outproc(p);
				}
			}
		}
		closedir(Dp);
		close(fd);
		return OKAY;
	} else {
		chdir_to_proc();
		return STOP;
	}
}

#elif defined (__FreeBSD__) || defined (__DragonFly__)

static unsigned long
getmem(void)
{
	return 0;
}

static enum okay
getproc_status(struct proc *p, pid_t expected_pid)
{
	static char	*buf;
	static size_t	buflen;
	union value	*v;
	FILE	*fp;
	char	*cp, *cq, *ce;
	size_t	sz, sc;
	int	mj, mi;

	if ((fp = fopen("status", "r")) == NULL)
		return STOP;
	for (cp = buf; ;) {
		const unsigned	chunk = 32;

		if (buflen < (sz = cp - buf + chunk)) {
			sc = cp - buf;
			buf = srealloc(buf, buflen = sz);
			cp = &buf[sc];
		}
		if ((sz = fread(cp, 1, chunk, fp)) < chunk) {
			ce = &cp[sz - 1];
			break;
		}
		cp += chunk;
	}
	fclose(fp);
	if (*ce != '\n')
		return STOP;
	*ce-- = '\0';
	cp = buf;
	cq = p->p_fname;
	while (*cp != ' ') {
		if (cq - p->p_fname < sizeof p->p_fname - 1) {
			if (cp[0] == '\\' && isdigit(cp[1]) &&
					isdigit(cp[2]) && isdigit(cp[3])) {
				*cq++ = cp[3] - '0' +
					(cp[2] - '0' << 3) +
					(cp[1] - '0' << 6);
				cp += 4;
			} else
				*cq++ = *cp++;
		} else
			cp++;
	}
	*cq = '\0';
	while (*cp == ' ')
		cp++;
	GETVAL_REQ(VT_INT);
	p->p_pid = v->v_int;
	GETVAL_REQ(VT_INT);
	p->p_ppid = v->v_int;
	GETVAL_REQ(VT_INT);
	p->p_pgid = v->v_int;
	GETVAL_REQ(VT_INT);
	p->p_sid = v->v_int;
	if (isdigit(*cp)) {
		GETVAL_COMMA(VT_INT);
		mj = v->v_int;
		GETVAL_REQ(VT_INT);
		mi = v->v_int;
		if (mj != -1 || mi != -1)
			p->p_ttydev = makedev(mj, mi);
	} else {
		struct stat	st;
		char	*dev;
		cq = cp;
		while (*cp != ' ') cp++;
		*cp = '\0';
		dev = smalloc(cp - cq + 8);
		strcpy(dev, "/dev/");
		strcpy(&dev[5], cq);
		if (stat(dev, &st) < 0)
			p->p_ttydev = PRNODEV;
		else
			p->p_ttydev = st.st_rdev;
		free(dev);
		*cp = ' ';
		while (*cp == ' ') cp++;
	}
	while (*cp != ' ') cp++; while (*cp == ' ') cp++;
	/* skip flags */
	GETVAL_COMMA(VT_LONG);
	p->p_start = v->v_long;
	/* microseconds */
	GETVAL_REQ(VT_LONG);
	GETVAL_COMMA(VT_LONG);
	p->p_time = v->v_long;
	p->p_utime = v->v_long;
	/* microseconds */
	GETVAL_REQ(VT_LONG);
	p->p_utime += v->v_long / 100000;
	GETVAL_COMMA(VT_LONG);
	p->p_time += v->v_long;
	p->p_ktime = v->v_long;
	p->p_accutime = p->p_time;
	/* microseconds */
	GETVAL_REQ(VT_LONG);
	p->p_ktime += v->v_long / 100000;
	if (strncmp(cp, "nochan ", 7) == 0)
		p->p_state[0] = p->p_lstate[0] = 'R';
	else
		p->p_state[0] = p->p_lstate[0] = 'S';
	while (*cp != ' ') {
		if (p->p_state[0] == 'S')
			p->p_wchan |= *cp << (*cp&30);	/* fake */
		cp++;
	}
	while (*cp == ' ') cp++;
	GETVAL_REQ(VT_INT);
	p->p_euid = v->v_int;
	GETVAL_REQ(VT_INT);
	p->p_uid = v->v_int;
	GETVAL_COMMA(VT_INT);
	p->p_gid = v->v_int;
	GETVAL_COMMA(VT_INT);
	p->p_egid = v->v_int;
	return OKAY;
}

static enum okay
getproc_cmdline(struct proc *p)
{
	FILE	*fp;
	char	*cp, *ce;
	int	hadzero = 0, c;

	if ((fp = fopen("cmdline", "r")) != NULL) {
		cp = p->p_psargs;
		ce = cp + sizeof p->p_psargs - 1;
		while (cp < ce && (c = getc(fp)) != EOF) {
			if (c != '\0') {
				if (hadzero) {
					*cp++ = ' ';
					if (cp == ce)
						break;
					hadzero = 0;
				}
				*cp++ = c;
			} else {
				hadzero = 1;
			}
		}
		*cp = '\0';
		fclose(fp);
	}
	if (*p->p_psargs == '\0' && p->p_size == 0)
		strcpy(p->p_psargs, p->p_fname);
	return OKAY;
}

static void
priocomp(struct proc *p)
{
	static int	once;
	static int	ranges[3][2];
	int	*cur;

	if (once++ == 0) {
		ranges[0][0] = sched_get_priority_min(SCHED_OTHER);
		ranges[0][1] = sched_get_priority_max(SCHED_OTHER);
		ranges[1][0] = sched_get_priority_min(SCHED_FIFO);
		ranges[1][1] = sched_get_priority_max(SCHED_FIFO);
		ranges[2][0] = sched_get_priority_min(SCHED_RR);
		ranges[3][1] = sched_get_priority_max(SCHED_RR);
	}
	switch (p->p_policy) {
	case SCHED_OTHER:
		cur = ranges[0];
		break;
	case SCHED_FIFO:
		cur = ranges[1];
		break;
	case SCHED_RR:
		cur = ranges[2];
		break;
	default:
		return;
	}
	switch (p->p_policy) {
	case SCHED_OTHER:
		p->p_nice = getpriority(PRIO_PROCESS, p->p_pid) + 20;
		break;
	case SCHED_FIFO:
	case SCHED_RR:
		p->p_pri = ((double)p->p_intpri - cur[0]) / (cur[1] - cur[0]) *
			100 + 60;
	}
}

static enum okay
getproc_map(struct proc *p)
{
	FILE	*fp;
	long	start, end, resident;
	int	c;

	if ((fp = fopen("map", "r")) == NULL)
		return OKAY;
	while (fscanf(fp, "0x%lx 0x%lx %ld", &start, &end, &resident) == 3) {
		if (p->p_addr == 0)
			p->p_addr = start;
		while ((c = getc(fp)) != EOF && c != '\n');
		p->p_size += (end - start) / 1024;
		p->p_orss += resident;
	}
	p->p_osz = p->p_size / (pagesize / 1024);
	p->p_rssize = p->p_orss * (pagesize / 1024);
	fclose(fp);
	return OKAY;
}

static enum okay
getproc_scheduler(struct proc *p)
{
	struct sched_param	s;

	switch (p->p_policy = sched_getscheduler(p->p_pid)) {
	case SCHED_FIFO:
		p->p_clname = "FF";
		break;
	case SCHED_RR:
#ifdef	S42
		p->p_clname = "FP";
#else
		p->p_clname = "RT";
#endif
		break;
	case SCHED_OTHER:
		p->p_clname = "TS";
		break;
	default:
		p->p_clname = "??";
	}
	if (sched_getparam(p->p_pid, &s) == 0)
		p->p_intpri = s.sched_priority;
	priocomp(p);
	return OKAY;
}

static enum okay
getproc_kvm(struct proc *p)
{
	static kvm_t	*kv;
	struct kinfo_proc	*kp;
	int	c;

	if (myeuid != 0)
		return OKAY;
	if (kv == NULL) {
		char	err[_POSIX2_LINE_MAX];
		if ((kv = kvm_open(NULL, NULL, NULL, O_RDONLY, err)) == NULL)
			return OKAY;
	}
	if ((kp = kvm_getprocs(kv, KERN_PROC_PID, p->p_pid, &c)) == NULL)
		return OKAY;
#if (__FreeBSD__) < 5 || defined (__DragonFly__)
	switch (kp->kp_proc.p_stat) {
#else	/* __FreeBSD__ >= 5 */
	switch (kp->ki_stat) {
#endif	/* __FreeBSD__ >= 5 */
	case SIDL:
		p->p_state[0] = 'I';
		break;
	case SRUN:
		p->p_state[0] = 'R';
		break;
#if defined (SWAIT) || defined (SLOCK)
#ifdef	SWAIT
	case SWAIT:
#endif	/* SWAIT */
#ifdef	SLOCK
	case SLOCK:
#endif	/* SLOCK */
		p->p_flag |= FL_LOCK;
		/*FALLTHRU*/
#endif	/* SWAIT || SLOCK */
	case SSLEEP:
		p->p_state[0] = 'S';
		break;
	case SSTOP:
		p->p_state[0] = 'T';
		break;
	case SZOMB:
		p->p_state[0] = 'Z';
		break;
	}
	p->p_lstate[0] = p->p_state[0];
#if (__FreeBSD__) < 5 || defined (__DragonFly__)
#define	ki_flag		kp_proc.p_flag
#define	ki_oncpu	kp_proc.p_oncpu
#define	ki_wchan	kp_proc.p_wchan
#define	ki_pri		kp_proc.p_pri
#endif	/* __FreeBSD__ < 5 */
	if (kp->ki_flag & P_SYSTEM)
		p->p_flag |= FL_SYS;
	if (kp->ki_flag & P_TRACED)
		p->p_flag |= FL_TRC;
#if (__FreeBSD__) < 5 || defined (__DragonFly__)
#ifndef	__DragonFly__
	p->p_intpri = kp->kp_proc.p_usrpri;
	p->p_oldpri = kp->kp_proc.p_usrpri;
	p->p_pri = kp->kp_proc.p_priority;
#endif	/* !__DragonFly__ */
	p->p_policy = SCHED_OTHER;
	p->p_clname = "TS";
#else	/* __FreeBSD__ >= 5 */
	if (kp->ki_sflag & PS_INMEM)
		p->p_flag |= FL_LOAD;
	if (kp->ki_sflag & PS_SWAPPINGOUT)
		p->p_flag |= FL_SWAP;
	p->p_oldpri = ((double)kp->ki_pri.pri_user - PRI_MIN) /
		(PRI_MAX - PRI_MIN) * 60 + 60;
	p->p_pri = 40 - ((double)kp->ki_pri.pri_user - PRI_MIN) /
		(PRI_MAX - PRI_MIN) * 40;
	if (p->p_policy != SCHED_OTHER)
		p->p_pri += 100;
#endif	/* __FreeBSD__ >= 5 */
#ifndef	__DragonFly__
	p->p_psr = kp->ki_oncpu;
	p->p_wchan = (unsigned long)kp->ki_wchan;
#endif	/* !__DragonFly__ */
	return OKAY;
}

static enum okay
getproc(const char *dir, struct proc *p, pid_t expected_pid, pid_t lwp)
{
	enum okay	result;

	memset(p, 0, sizeof *p);
	if (chdir(dir) == 0) {
		if ((result = getproc_status(p, expected_pid)) == OKAY)
			if ((result = getproc_cmdline(p)) == OKAY)
				if ((result = getproc_map(p)) == OKAY)
					if ((result = getproc_scheduler(p)) ==
							OKAY)
						result = getproc_kvm(p);
		chdir_to_proc();
	} else
		result = STOP;
	return result;
}

#else	/* !__linux__, !__FreeBSD__, !__DragonFly__ */

#ifndef	__sun
static unsigned long
getmem(void)
{
#ifdef	_SC_USEABLE_MEMORY
	long	usm;

	if ((usm = sysconf(_SC_USEABLE_MEMORY)) > 0)
		return usm * (pagesize / 1024);
#endif	/* _SC_USEABLE_MEMORY */
	return 0;
}
#endif	/* !__sun */

static const char *
concat(const char *dir, const char *base)
{
	static char	*name;
	static long	size;
	long	length;
	char	*np;
	const char	*cp;

	if ((length = strlen(dir) + strlen(base) + 2) > size)
		name = srealloc(name, size = length);
	np = name;
	for (cp = dir; *cp; cp++)
		*np++ = *cp;
	*np++ = '/';
	for (cp = base; *cp; cp++)
		*np++ = *cp;
	*np = '\0';
	return name;
}

static time_t
tv2sec(timestruc_t tv, int mult)
{
	return tv.tv_sec*mult + (tv.tv_nsec >= 500000000/mult);
}

static enum okay
getproc_psinfo(const char *dir, struct proc *p, pid_t expected_pid)
{
	FILE	*fp;
	struct psinfo	pi;
	const char	*cp;
	char	*np;

	if ((fp = fopen(concat(dir, "psinfo"), "r")) == NULL)
		return STOP;
	if (fread(&pi, 1, sizeof pi, fp) != sizeof pi ||
			pi.pr_pid != expected_pid) {
		fclose(fp);
		return STOP;
	}
	fclose(fp);
	p->p_pid = pi.pr_pid;
	strncpy(p->p_fname, pi.pr_fname, sizeof p->p_fname);
	p->p_fname[sizeof p->p_fname - 1] = '\0';
	p->p_ppid = pi.pr_ppid;
	p->p_pgid = pi.pr_pgid;
	p->p_sid = pi.pr_sid;
	p->p_nlwp = pi.pr_nlwp;
	p->p_ttydev = pi.pr_ttydev;
	p->p_time = tv2sec(pi.pr_time, 1);
#ifdef	__sun
	p->p_accutime = tv2sec(pi.pr_ctime, 1);
#endif	/* __sun */
	p->p_start = tv2sec(pi.pr_start, 1);
	p->p_size = pi.pr_size;
	p->p_osz = pi.pr_size / kbytes_per_page;
	p->p_rssize = pi.pr_rssize;
	p->p_orss = pi.pr_rssize / kbytes_per_page;
	p->p_addr = (unsigned long)pi.pr_addr;
#ifdef	__sun
	p->p_pctcpu = (double)pi.pr_pctcpu / 0x8000 * 100;
	p->p_pctmem = (double)pi.pr_pctmem / 0x8000 * 100;
#endif	/* __sun */
	strncpy(p->p_psargs, pi.pr_psargs, sizeof p->p_psargs);
	p->p_psargs[sizeof p->p_psargs - 1] = '\0';
	for (np = p->p_comm, cp = p->p_psargs; *cp && !isblank(*cp); cp++)
		*np++ = *cp;
	p->p_uid = pi.pr_uid;
	p->p_gid = pi.pr_gid;
#ifdef	__sun
	p->p_euid = pi.pr_euid;
	p->p_egid = pi.pr_egid;
#endif	/* __sun */
	p->p_lflag = pi.pr_flag;
#if defined (SLOAD)
	if (p->p_lflag & SLOAD)
		p->p_flag |= FL_LOAD;
#elif defined (P_LOAD)
	if (p->p_lflag & P_LOAD)
		p->p_flag |= FL_LOAD;
#endif	/* SLOAD, P_LOAD */
#if defined (SSYS)
	if (p->p_lflag & SSYS)
		p->p_flag |= FL_SYS;
#elif defined (P_SYS)
	if (p->p_lflag & P_SYS)
		p->p_flag |= FL_SYS;
#endif	/* SSYS, P_SYS */
#if defined (SLOCK)
	if (p->p_lflag & SLOCK)
		p->p_flag |= FL_LOCK;
#elif defined (P_NOSWAP)
	if (p->p_lflag & P_NOSWAP)
		p->p_flag |= FL_LOCK;
#endif	/* SLOCK, P_NOSWAP */
#if defined (SPROCTR)
	if (p->p_lflag & SPROCTR)
		p->p_flag |= FL_TRC;
#elif defined (P_PROCTR)
	if (p->p_lflag & P_PROCTR)
		p->p_flag |= FL_TRC;
#endif	/* SPROCTR, P_PROCTR */
	return OKAY;
}

static enum okay
getproc_lwpsinfo(const char *dir, struct proc *p, pid_t lwp)
{
	static char	clname[PRCLSZ+1];
	char	base[100];
	FILE	*fp;
	struct lwpsinfo	li;

	if (p->p_nlwp == 0) {	/* zombie process */
		p->p_lstate[0] = p->p_state[0] = 'Z';
		return OKAY;
	}
	if (lwp != (pid_t)-1) {
		snprintf(base, sizeof base, "lwp/%d/lwpsinfo", (int)lwp);
		fp = fopen(concat(dir, base), "r");
	} else {
		int	i;
		for (i = 1; i <= 255; i++) {
			snprintf(base, sizeof base, "lwp/%d/lwpsinfo", i);
			if ((fp = fopen(concat(dir, base), "r")) != NULL ||
					errno != ENOENT)
				break;
		}
	}
	if (fp == NULL)
		return STOP;
	if (fread(&li, 1, sizeof li, fp) != sizeof li) {
		fclose(fp);
		return STOP;
	}
	fclose(fp);
	p->p_lwp = li.pr_lwpid;
	if (Lflag) {
		p->p_time = tv2sec(li.pr_time, 1);
		if (li.pr_name[0]) {
			strncpy(p->p_fname, li.pr_name, sizeof p->p_fname);
			p->p_fname[sizeof p->p_fname - 1] = '\0';
		}
	}
	p->p_lstate[0] = p->p_state[0] = li.pr_sname;
	p->p_intpri = li.pr_pri;
	p->p_rtpri = li.pr_pri;
	p->p_clname = clname;
	memcpy(clname, li.pr_clname, PRCLSZ);
#ifdef	__sun
	p->p_oldpri = li.pr_oldpri;
#endif	/* __sun */
	p->p_pri = li.pr_pri;
	p->p_nice = li.pr_nice;
	p->p_wchan = (unsigned long)li.pr_wchan;
	p->p_psr = li.pr_onpro;
	return OKAY;
}

#ifdef	__sun
static enum okay
getproc_usage(const char *dir, struct proc *p)
{
	FILE	*fp;
	struct prusage	pu;

	if ((fp = fopen(concat(dir, "usage"), "r")) == NULL)
		return OKAY;
	if (fread(&pu, 1, sizeof pu, fp) != sizeof pu) {
		fclose(fp);
		return STOP;
	}
	fclose(fp);
	p->p_pflts = pu.pr_majf;
	p->p_bufr = pu.pr_inblk;
	p->p_bufw = pu.pr_oublk;
	p->p_mrcv = pu.pr_mrcv;
	p->p_msnd = pu.pr_msnd;
	p->p_utime = tv2sec(pu.pr_utime, 10);
	p->p_ktime = tv2sec(pu.pr_stime, 10);
	return OKAY;
}
#else	/* !__sun */
static enum okay
getproc_cred(const char *dir, struct proc *p)
{
	FILE	*fp;
	struct prcred	pc;

	if ((fp = fopen(concat(dir, "cred"), "r")) == NULL)
		/*
		 * Don't require this, as it may be accessible to root
		 * only and it's better to have no effective uids than
		 * to display no content at all.
		 */
		return OKAY;
	if (fread(&pc, 1, sizeof pc, fp) != sizeof pc) {
		fclose(fp);
		return STOP;
	}
	fclose(fp);
	p->p_euid = pc.pr_euid;
	p->p_egid = pc.pr_egid;
	return OKAY;
}
#endif	/* !__sun */

static enum okay
getproc_status(const char *dir, struct proc *p)
{
	FILE	*fp;
	struct pstatus	ps;

	if ((fp = fopen(concat(dir, "status"), "r")) == NULL)
		/*
		 * Don't require this, as it may be accessible to root
		 * only and the children times are not that important.
		 */
		return OKAY;
	if (fread(&ps, 1, sizeof ps, fp) != sizeof ps) {
		fclose(fp);
		return STOP;
	}
	fclose(fp);
	p->p_utime = tv2sec(ps.pr_utime, 10);
	p->p_ktime = tv2sec(ps.pr_stime, 10);
	p->p_accutime = tv2sec(ps.pr_cutime, 1) + tv2sec(ps.pr_cstime, 1);
	return OKAY;
}

static enum okay
getproc_lwpstatus(const char *dir, struct proc *p, pid_t lwp)
{
	FILE	*fp;
	char	base[100];
	struct lwpstatus	ls;

	if (p->p_nlwp == 0)	/* zombie process */
		return OKAY;
	if (lwp != (pid_t)-1) {
		snprintf(base, sizeof base, "lwp/%d/lwpstatus", (int)lwp);
		fp = fopen(concat(dir, base), "r");
	} else {
		int	i;
		for (i = 1; i <= 20; i++) {
			snprintf(base, sizeof base, "lwp/%d/lwpstatus", i);
			if ((fp = fopen(concat(dir, base), "r")) != NULL ||
					errno != ENOENT)
				break;
		}
	}
	if (fp == NULL)
		/*
		 * Don't require this, as it may be accessible to root
		 * only and the process flags are not that important.
		 */
		return OKAY;
	if (fread(&ls, 1, sizeof ls, fp) != sizeof ls) {
		fclose(fp);
		return STOP;
	}
	fclose(fp);
	if (ls.pr_flags == PR_STOPPED &&
			(ls.pr_why == PR_SYSENTRY || ls.pr_why == PR_SYSEXIT))
		p->p_flag |= FL_LOCK;
	return OKAY;
}

static enum okay
getproc(const char *dir, struct proc *p, pid_t expected_pid, pid_t lwp)
{
	enum okay	result;

	memset(p, 0, sizeof *p);
	if ((result = getproc_psinfo(dir, p, expected_pid)) == OKAY) {
		if ((result = getproc_status(dir, p)) == OKAY)
#ifdef	__sun
			if ((result = getproc_usage(dir, p)) == OKAY)
#else	/* !__sun */
			if ((result = getproc_cred(dir, p)) == OKAY)
#endif	/* !__sun */
				if ((result = getproc_lwpsinfo(dir, p, lwp))
						== OKAY)
					result = getproc_lwpstatus(dir, p, lwp);
	} else
		result = STOP;
	return result;
}

static enum okay
getLWPs(const char *dir, struct proc *p, pid_t expected_pid)
{
	DIR	*Dp;
	struct dirent	*dp;
	unsigned long	val;
	char	*x;

	if ((Dp = opendir(concat(dir, "lwp"))) != NULL) {
		while ((dp = readdir(Dp)) != NULL) {
			if (dp->d_name[0] == '.' && (dp->d_name[1]=='\0' ||
					(dp->d_name[1]=='.' &&
					 dp->d_name[2]=='\0')))
			continue;
			val = strtoul(dp->d_name, &x, 10);
			if (*x != 0)
				continue;
			if (getproc(dir, p, expected_pid, val) == OKAY) {
				postproc(p);
				if (selectproc(p) == OKAY)
					outproc(p);
			}
		}
		closedir(Dp);
		return OKAY;
	} else
		return STOP;
}

#endif	/* !__linux__, !__FreeBSD__, !__DragonFly__ */

static void
postproc(struct proc *p)
{
	cleanline(p);
#ifndef	__sun
	if ((p->p_pctcpu = now - p->p_start) != 0) {
		p->p_pctcpu = (double)p->p_time * 100 / p->p_pctcpu;
		if (p->p_pctcpu < 0)
			p->p_pctcpu = 0;
	}
	if (totalmem)
		p->p_pctmem = (double)p->p_size * 100 / totalmem;
#endif	/* !__sun */
#if !defined (__linux__) && !defined (__sun) && !defined (__FreeBSD__) \
		&& !defined (__DragonFly__)
	p->p_oldpri = 160 - p->p_pri;
#endif	/* !__linux__, !__sun */
#if !defined (__linux__) && !defined (__FreeBSD__) && !defined (__DragonFly__)
	p->p_policy = p->p_clname && strcmp(p->p_clname, "TS") ?
		SCHED_RR : SCHED_OTHER;
#endif	/* !__linux__, !__FreeBSD__, !__DragonFly__ */
}
#endif	/* !__hpux, !_AIX, !__NetBSD__, !__OpenBSD__, !__APPLE__ */

static enum okay
selectproc(struct proc *p)
{
	struct criterion	*ct;

	for (ct = c0; ct; ct = ct->c_nxt) {
		if (ucb_rflag && p->p_lstate[0] != 'R')
			continue;
		switch (ct->c_typ) {
		case CR_ALL:
			return OKAY;
		case CR_ALL_WITH_TTY:
			if (p->p_lstate[0] == 'Z')
				break;
#ifndef	UCB
			if (p->p_pid == p->p_sid)
				break;
#endif	/* !UCB */
			if (p->p_ttydev != (dev_type)PRNODEV)
				return OKAY;
			break;
		case CR_ALL_BUT_SESSION_LEADERS:
			if (p->p_pid != p->p_sid)
				return OKAY;
			break;
		case CR_WITHOUT_TTY:
			if (p->p_ttydev == (dev_type)PRNODEV ||
					p->p_lstate[0] == 'Z')
				return OKAY;
			break;
		case CR_NO_TTY_NO_SESSION_LEADER:
			if (p->p_ttydev == (dev_type)PRNODEV &&
					p->p_pid != p->p_sid &&
					p->p_lstate[0] != 'Z')
				return OKAY;
			break;
		case CR_PROCESS_GROUP:
#if defined (SUS) || defined (UCB)
			if (p->p_sid == ct->c_val)
				return OKAY;
#else	/* !SUS, !UCB */
			if (p->p_pgid == ct->c_val)
				return OKAY;
#endif	/* !SUS, !UCB */
			break;
		case CR_REAL_GID:
			if (p->p_gid == ct->c_val)
				return OKAY;
			break;
		case CR_PROCESS_ID:
			if (p->p_pid == ct->c_val)
				return OKAY;
			break;
		case CR_TTY_DEVICE:
			if (/*p->p_ttydev != (dev_type)PRNODEV &&*/
					p->p_ttydev == ct->c_val &&
					p->p_lstate[0] != 'Z')
				return OKAY;
			break;
		case CR_SESSION_LEADER:
			if (p->p_sid == ct->c_val)
				return OKAY;
			break;
		case CR_EFF_UID:
			if (p->p_euid == ct->c_val)
				return OKAY;
			break;
		case CR_REAL_UID:
			if (p->p_uid == ct->c_val)
				return OKAY;
			break;
		case CR_ADD_UNINTERESTING:
			if (p->p_lstate[0] != 'Z' &&
					p->p_euid == myuid &&
					p->p_ttydev != (dev_type)PRNODEV &&
					p->p_ttydev == myproc.p_ttydev)
				return OKAY;
			break;
		case CR_DEFAULT:
			if (p->p_lstate[0] != 'Z' &&
#if defined (SUS) || defined (UCB)
					p->p_euid == myuid &&
#endif	/* SUS || UCB */
#ifdef	UCB
					p->p_pid != p->p_sid &&
					p->p_ttydev != (dev_type)PRNODEV &&
#endif	/* UCB */
					p->p_ttydev == myproc.p_ttydev)
				return OKAY;
			break;
		}
	}
	return STOP;
}

#if !defined (__hpux) && !defined (_AIX) && !defined (__NetBSD__) && \
	!defined (__OpenBSD__) && !defined (__APPLE__)
static void
do_procs(void)
{
	struct proc	p;
	DIR	*Dp;
	struct dirent	*dp;
	unsigned long	val;
	char	*x;

	if ((Dp = opendir(".")) != NULL) {
		while ((dp = readdir(Dp)) != NULL) {
			if (dp->d_name[0] == '.' && (dp->d_name[1] == '\0' ||
					(dp->d_name[1] == '.' &&
				 	dp->d_name[2] == '\0')))
				continue;
			val = strtoul(dp->d_name, &x, 10);
			if (*x != 0)
				continue;
#if !defined (__FreeBSD__) && !defined (__DragonFly__)
			if (Lflag)
				if (getLWPs(dp->d_name, &p, val) == OKAY)
					continue;
#endif	/* !__FreeBSD__, !__DragonFly__ */
			if (getproc(dp->d_name, &p, val, -1) == OKAY) {
				postproc(&p);
				if (selectproc(&p) == OKAY)
					outproc(&p);
			}
		}
		closedir(Dp);
	}
}
#elif defined (__hpux)

static unsigned long
getmem(void)
{
	return 0;
}

static void
getproc(struct proc *p, struct pst_status *pst)
{
	char	*cp, *np;

	memset(p, 0, sizeof *p);
	p->p_pid = pst->pst_pid;
	strncpy(p->p_fname, pst->pst_ucomm, sizeof p->p_fname);
	p->p_fname[sizeof p->p_fname - 1] = '\0';
	strncpy(p->p_psargs, pst->pst_cmd, sizeof p->p_psargs);
	p->p_psargs[sizeof p->p_psargs - 1] = '\0';
	for (np = p->p_comm, cp = p->p_psargs; *cp && !isblank(*cp); cp++)
		*np++ = *cp;
	p->p_lstate[0] = pst->pst_stat;
	p->p_ppid = pst->pst_ppid;
	p->p_pgid = pst->pst_pgrp;
	p->p_sid = pst->pst_sid;
	/*p->p_lwp = pst->pst_lwpid;*/
	if (pst->pst_term.psd_major != -1 || pst->pst_term.psd_minor != -1)
		p->p_ttydev = makedev(pst->pst_term.psd_major,
				pst->pst_term.psd_minor);
	p->p_lflag = pst->pst_flag;
	p->p_time = pst->pst_utime + pst->pst_stime;
	p->p_accutime = pst->pst_utime + pst->pst_stime +
		pst->pst_child_utime.pst_sec +
		(pst->pst_child_utime.pst_usec > + 500000) +
		pst->pst_child_stime.pst_sec +
		(pst->pst_child_stime.pst_usec > + 500000);
	p->p_utime = pst->pst_utime * 10;
	p->p_ktime = pst->pst_stime * 10;
	p->p_intpri = p->p_rtpri = pst->pst_pri;
	p->p_policy = pst->pst_schedpolicy;
	p->p_c = pst->pst_cpu;
	p->p_nice = pst->pst_nice;
	p->p_nlwp = pst->pst_nlwps;
	p->p_start = pst->pst_start;
	p->p_osz = pst->pst_dsize + pst->pst_tsize + pst->pst_ssize;
	p->p_size = p->p_osz * kbytes_per_page;
	p->p_orss = pst->pst_rssize;
	p->p_rssize = p->p_orss * kbytes_per_page;
	p->p_pflts = pst->pst_majorfaults;
	p->p_mrcv = pst->pst_msgrcv;
	p->p_msnd = pst->pst_msgsnd;
	p->p_addr = pst->pst_addr;
	p->p_wchan = pst->pst_wchan;
	p->p_psr = pst->pst_procnum;
	p->p_pctcpu = pst->pst_pctcpu;
	p->p_uid = pst->pst_uid;
	p->p_euid = pst->pst_euid;
	p->p_gid = pst->pst_gid;
	p->p_egid = pst->pst_egid;
}

static void
getlwp(struct proc *p, struct lwp_status *lwp)
{
	p->p_lwp = lwp->lwp_lwpid;
	p->p_intpri = p->p_rtpri = lwp->lwp_pri;
	p->p_c = lwp->lwp_cpu;
	p->p_wchan = lwp->lwp_wchan;
	p->p_psr = lwp->lwp_cpu;
	p->p_start = lwp->lwp_start;
	p->p_lstate[0] = lwp->lwp_stat;
	p->p_policy = lwp->lwp_schedpolicy;
	p->p_utime = lwp->lwp_utime * 10;
	p->p_ktime = lwp->lwp_stime * 10;
	p->p_pflts = lwp->lwp_majorfaults;
	p->p_mrcv = lwp->lwp_msgrcv;
	p->p_msnd = lwp->lwp_msgsnd;
	/*p->p_pctcpu = lwp->lwp_pctcpu;*/
}

static void
postproc(struct proc *p)
{
	cleanline(p);
	switch (p->p_lstate[0]) {
	case PS_SLEEP:
		p->p_lstate[0] = 'S';
		break;
	case PS_RUN:
		p->p_lstate[0] = 'R';
		break;
	case PS_STOP:
		p->p_lstate[0] = 'T';
		break;
	case PS_ZOMBIE:
		p->p_lstate[0] = 'Z';
		break;
	case PS_IDLE:
	case PS_OTHER:
	default:
		p->p_lstate[0] = 'I';
		break;
	}
	p->p_state[0] = p->p_lstate[0];
	if (p->p_lflag & PS_INCORE)
		p->p_flag |= FL_LOAD;
	if (p->p_lflag & PS_SYS)
		p->p_flag |= FL_SYS;
	if (p->p_lflag & PS_LOCKED)
		p->p_flag |= FL_LOCK;
	if (p->p_lflag & PS_TRACE)
		p->p_flag |= FL_TRC;
	if (p->p_lflag & PS_TRACE2)
		p->p_flag |= FL_WTED;
	p->p_oldpri = p->p_intpri;
	p->p_pri = 220 - p->p_intpri;
	switch (p->p_policy) {
	case PS_TIMESHARE:
		p->p_clname = "TS";
		p->p_policy = SCHED_OTHER;
		p->p_pri /= 2;
		break;
	case PS_RTPRIO:
	case PS_RR:
	case PS_RR2:
#ifdef	S42
		p->p_clname = "FP";
#else
		p->p_clname = "RT";
#endif
		p->p_policy = SCHED_RR;
		p->p_pri += 100;
		break;
	case PS_FIFO:
		p->p_clname = "FF";
		p->p_policy = SCHED_FIFO;
		p->p_pri += 100;
		break;
	case PS_NOAGE:
		p->p_clname = "FC";
		p->p_policy = SCHED_NOAGE;
		p->p_pri /= 2;
		break;
	default:
		p->p_clname = "??";
	}
}

#define	burst	((size_t)10)

static void
getLWPs(struct proc *p)
{
	struct lwp_status	lwp[burst];
	int	i, count;
	int	idx = 0;

	while ((count = pstat_getlwp(lwp, sizeof *lwp, burst, idx, p->p_pid))
			> 0) {
		for (i = 0; i < count; i++) {
			getlwp(p, &lwp[i]);
			postproc(p);
			if (selectproc(p) == OKAY)
				outproc(p);
		}
		idx = lwp[count-1].lwp_idx + 1;
	}
}

static void
do_procs(void)
{
	struct proc	p;
	struct pst_status	pst[burst];
	int	i, count;
	int	idx = 0;

	while ((count = pstat_getproc(pst, sizeof *pst, burst, idx)) > 0) {
		for (i = 0; i < count; i++) {
			getproc(&p, &pst[i]);
			if (Lflag && p.p_nlwp > 1)
				getLWPs(&p);
			else {
				postproc(&p);
				if (selectproc(&p) == OKAY)
					outproc(&p);
			}
		}
		idx = pst[count-1].pst_idx + 1;
	}
}

#elif defined (_AIX)

static unsigned long
getmem(void)
{
	return 0;
}

static time_t
tv2sec(struct timeval64 tv, int mult)
{
	return tv.tv_sec*mult + (tv.tv_usec >= 500000/mult);
}

static void
getproc(struct proc *p, struct procentry64 *pi)
{
	char	args[100], *ap, *cp, *xp;

	memset(p, 0, sizeof *p);
	p->p_pid = pi->pi_pid;
	strncpy(p->p_fname, pi->pi_comm, sizeof p->p_fname);
	p->p_fname[sizeof p->p_fname - 1] = '\0';
	p->p_lstate[0] = pi->pi_state;
	p->p_ppid = pi->pi_ppid;
	p->p_pgid = pi->pi_pgrp;
	p->p_sid = pi->pi_sid;
	p->p_ttydev = pi->pi_ttyp ? pi->pi_ttyd : PRNODEV;
	p->p_lflag = pi->pi_flags;
	p->p_time = pi->pi_utime + pi->pi_stime;
	p->p_accutime = pi->pi_utime + pi->pi_stime;
	p->p_utime = pi->pi_utime * 10;
	p->p_ktime = pi->pi_stime * 10;
	p->p_intpri = pi->pi_pri;
	p->p_c = pi->pi_cpu;
	p->p_nice = pi->pi_nice;
	p->p_nlwp = pi->pi_thcount;
	p->p_start = pi->pi_start;
	p->p_osz = pi->pi_size;
	p->p_orss = pi->pi_drss + pi->pi_trss;
	p->p_pflts = pi->pi_majflt;
	p->p_addr = pi->pi_adspace;
	p->p_uid = pi->pi_uid;
	p->p_euid = pi->pi_cred.crx_uid;
	p->p_gid = pi->pi_cred.crx_rgid;
	p->p_egid = pi->pi_cred.crx_gid;
	if (getargs(pi, sizeof *pi, args, sizeof args) == 0) {
		ap = args;
		cp = p->p_psargs;
		xp = p->p_comm;
		while (cp < &p->p_psargs[sizeof p->p_psargs - 1]) {
			if (ap[0] == '\0') {
				if (ap[1] == '\0')
					break;
				*cp++ = ' ';
				if (xp) {
					*xp = '\0';
					xp = NULL;
				}
			} else {
				*cp++ = *ap;
				if (xp)
					*xp++ = *ap;
			}
			ap++;
		}
		*cp = '\0';
		if (xp)
			*xp = '\0';
	}
}

static void
postproc(struct proc *p)
{
	char	*np, *cp;

	if (p->p_psargs[0] == '\0') {
		strncpy(p->p_psargs, p->p_fname, sizeof p->p_psargs);
		p->p_psargs[sizeof p->p_psargs - 1] = '\0';
	}
	if (p->p_comm[0] == '\0') {
		for (np = p->p_comm, cp = p->p_psargs;
				*cp && !isblank(*cp); cp++)
			*np++ = *cp;
	}
	cleanline(p);
	p->p_clname = "TS";
	switch (p->p_lstate[0]) {
	case SSWAP:
		p->p_flag |= FL_SWAP;
		/*FALLTHRU*/
	default:
	case SIDL:
		p->p_state[0] = 'I';
		break;
	case SZOMB:
		p->p_state[0] = 'Z';
		break;
	case SSTOP:
		p->p_state[0] = 'T';
		break;
	case SACTIVE:
		p->p_state[0] = 'R';
		break;
	}
	p->p_lstate[0] = p->p_state[0];
	if (p->p_lflag & SLOAD)
		p->p_flag |= FL_LOAD;
	if (p->p_lflag & SNOSWAP)
		p->p_flag |= FL_LOCK;
	if (p->p_lflag & SKPROC)
		p->p_flag |= FL_SYS;
	if (p->p_lflag & SFIXPRI)
		p->p_clname = "RT";
	p->p_oldpri = p->p_intpri / 2;
	p->p_pri = 255 - p->p_intpri;
	p->p_size = p->p_osz * kbytes_per_page;
	p->p_rssize = p->p_orss * kbytes_per_page;
}

static void
getlwp(struct proc *p, struct thrdentry64 *ti)
{
	p->p_lwp = ti->ti_tid;
	p->p_psr = ti->ti_cpuid;
	p->p_wchan = ti->ti_wchan;
	if (Lflag) {
		p->p_intpri = ti->ti_pri;
		p->p_c = ti->ti_cpu;
		p->p_policy = ti->ti_policy;
		p->p_utime = tv2sec(ti->ti_ru.ru_utime, 10);
		p->p_ktime = tv2sec(ti->ti_ru.ru_stime, 10);
		p->p_time = (p->p_utime + p->p_ktime) / 10;
	}
	p->p_pflts = ti->ti_ru.ru_majflt;
	p->p_bufr = ti->ti_ru.ru_inblock;
	p->p_bufw = ti->ti_ru.ru_oublock;
	p->p_mrcv = ti->ti_ru.ru_msgrcv;
	p->p_msnd = ti->ti_ru.ru_msgsnd;
}

#define	burst	((size_t)10)

static void
getLWPs(struct proc *p)
{
	struct	thrdentry64	ti[burst];
	tid64_t	idx = 0;
	int	i, count;

	while ((count=getthrds64(p->p_pid, ti, sizeof *ti, &idx, burst)) > 0) {
		for (i = 0; i < count; i++) {
			getlwp(p, &ti[i]);
			postproc(p);
			if (selectproc(p) == OKAY)
				outproc(p);
		}
		if (count < burst)
			break;
	}
}

static void
oneLWP(struct proc *p)
{
	struct thrdentry64	ti;
	tid64_t	idx = 0;

	if (getthrds64(p->p_pid, &ti, sizeof ti, &idx, 1) == 1)
		getlwp(p, &ti);
}

static void
do_procs(void)
{
	struct proc	p;
	struct procentry64	pi[burst];
	pid_t	idx = 0;
	int	i, count;

	while ((count = getprocs64(pi, sizeof *pi, NULL, 0, &idx, burst)) > 0) {
		for (i = 0; i < count; i++) {
			getproc(&p, &pi[i]);
			if (Lflag && p.p_nlwp > 1)
				getLWPs(&p);
			else {
				oneLWP(&p);
				postproc(&p);
				if (selectproc(&p) == OKAY)
					outproc(&p);
			}
		}
		if (count < burst)
			break;
	}
}

#elif defined (__OpenBSD__)

#include <uvm/uvm_extern.h>

static unsigned long
getmem(void)
{
	return 0;
}

static time_t
tv2sec(long sec, long usec, int mult)
{
	return sec*mult + (usec >= 500000/mult);
}

static void
getproc(struct proc *p, struct kinfo_proc *kp)
{
	memset(p, 0, sizeof *p);
	p->p_pid = kp->kp_proc.p_pid;
	strncpy(p->p_fname, kp->kp_proc.p_comm, sizeof p->p_fname);
	p->p_fname[sizeof p->p_fname - 1] = '\0';
	p->p_lstate[0] = kp->kp_proc.p_stat;
	p->p_ppid = kp->kp_eproc.e_ppid;
	p->p_pgid = kp->kp_eproc.e_pgid;
	p->p_sid = kp->kp_eproc.e_tpgid;	/* ? */
	p->p_ttydev = kp->kp_eproc.e_tdev;
	p->p_lflag = kp->kp_proc.p_flag;
	p->p_time = tv2sec(kp->kp_eproc.e_pstats.p_ru.ru_utime.tv_sec +
				kp->kp_eproc.e_pstats.p_ru.ru_stime.tv_sec,
			kp->kp_eproc.e_pstats.p_ru.ru_utime.tv_usec +
				kp->kp_eproc.e_pstats.p_ru.ru_stime.tv_usec, 1);
	p->p_accutime = p->p_time +
		tv2sec(kp->kp_eproc.e_pstats.p_cru.ru_utime.tv_sec +
				kp->kp_eproc.e_pstats.p_cru.ru_stime.tv_sec,
			kp->kp_eproc.e_pstats.p_cru.ru_utime.tv_usec +
				kp->kp_eproc.e_pstats.p_cru.ru_stime.tv_usec,
			1);
	p->p_utime = tv2sec(kp->kp_eproc.e_pstats.p_ru.ru_utime.tv_sec,
			kp->kp_eproc.e_pstats.p_ru.ru_utime.tv_usec, 10);
	p->p_ktime = tv2sec(kp->kp_eproc.e_pstats.p_ru.ru_stime.tv_sec,
			kp->kp_eproc.e_pstats.p_ru.ru_stime.tv_usec, 10);
	p->p_intpri = kp->kp_proc.p_usrpri;
	p->p_rtpri = kp->kp_proc.p_priority;
	p->p_policy = SCHED_OTHER;
	p->p_c = kp->kp_proc.p_cpticks;
	p->p_nice = kp->kp_proc.p_nice;
	p->p_nlwp = 1;
	p->p_start = tv2sec(kp->kp_eproc.e_pstats.p_start.tv_sec,
		kp->kp_eproc.e_pstats.p_start.tv_usec, 1);
	p->p_osz = kp->kp_eproc.e_vm.vm_tsize + kp->kp_eproc.e_vm.vm_dsize +
		kp->kp_eproc.e_vm.vm_ssize;
	p->p_orss = kp->kp_eproc.e_vm.vm_rssize;
	p->p_pflts = kp->kp_eproc.e_pstats.p_ru.ru_majflt;
	p->p_bufr = kp->kp_eproc.e_pstats.p_ru.ru_inblock;
	p->p_bufw = kp->kp_eproc.e_pstats.p_ru.ru_oublock;
	p->p_mrcv = kp->kp_eproc.e_pstats.p_ru.ru_msgrcv;
	p->p_msnd = kp->kp_eproc.e_pstats.p_ru.ru_msgsnd;
	p->p_addr = (unsigned long)kp->kp_proc.p_addr;
	p->p_wchan = (unsigned long)kp->kp_proc.p_wchan;
	p->p_pctcpu = kp->kp_proc.p_pctcpu;
	p->p_clname = "TS";
	p->p_uid = kp->kp_eproc.e_pcred.p_ruid;
	p->p_euid = kp->kp_eproc.e_ucred.cr_uid;
	p->p_gid = kp->kp_eproc.e_pcred.p_rgid;
	p->p_egid = kp->kp_eproc.e_ucred.cr_gid;
}

static void
getargv(struct proc *p, struct kinfo_proc *kp, kvm_t *kt)
{
	char	**args;
	char	*ap, *pp, *xp;

	if ((args = kvm_getargv(kt, kp, sizeof p->p_psargs)) == NULL) {
		strncpy(p->p_psargs, p->p_fname, sizeof p->p_psargs);
		p->p_psargs[sizeof p->p_psargs - 1] = '\0';
		for (ap = p->p_comm, pp = p->p_psargs;
				*pp && !isblank(*pp); pp++)
			*ap++ = *pp;
		return;
	}
	ap = args[0];
	xp = p->p_comm;
	for (pp = p->p_psargs; pp < &p->p_psargs[sizeof p->p_psargs-1]; pp++) {
		if (*ap == '\0') {
			*pp = ' ';
			if (xp) {
				*xp = '\0';
				xp = NULL;
			}
			ap = *++args;
			if (ap == NULL)
				break;
		} else {
			if (xp)
				*xp++ = *ap;
			*pp = *ap++;
		}
	}
}

static void
postproc(struct proc *p)
{
	cleanline(p);
	switch (p->p_lstate[0]) {
	default:
	case SIDL:
		p->p_state[0] = 'I';
		break;
	case SRUN:
		p->p_state[0] = 'R';
		break;
	case SSLEEP:
		p->p_state[0] = 'S';
		break;
	case SSTOP:
		p->p_state[0] = 'T';
		break;
	case SZOMB:
	case SDEAD:
		p->p_state[0] = 'Z';
		break;
	}
	p->p_lstate[0] = p->p_state[0];
	if ((p->p_lflag & P_CONTROLT) == 0)
		p->p_ttydev = PRNODEV;
	if ((p->p_lflag & P_INMEM) == 0)
		p->p_flag |= FL_SWAP;
	else
		p->p_flag |= FL_LOAD;
	if (p->p_lflag & P_SYSTEM)
		p->p_flag |= FL_SYS;
	if ((p->p_lflag & P_SINTR) == 0)
		p->p_flag |= FL_LOCK;
	p->p_pri = p->p_rtpri;
	p->p_oldpri = p->p_intpri + 40;
	p->p_size = p->p_osz * kbytes_per_page;
	p->p_rssize = p->p_orss * kbytes_per_page;
}

static void
do_procs(void)
{
	struct proc	p;
	kvm_t	*kt;
	struct kinfo_proc	*kp;
	int	i, cnt;
	pid_t	mypid = getpid();
	int	gotme = 0;

	if ((kt = kvm_open(NULL, NULL, NULL, KVM_NO_FILES, "kvm_open")) == NULL)
		exit(1);
	kp = kvm_getprocs(kt, KERN_PROC_ALL, 0, &cnt);
	i = cnt;
	while (--i >= 0) {
	one:	if (kp[i].kp_proc.p_pid == mypid)
			gotme = 1;
		getproc(&p, &kp[i]);
		getargv(&p, &kp[i], kt);
		postproc(&p);
		if (selectproc(&p) == OKAY)
			outproc(&p);
	}
	if (gotme == 0) {
		kp = kvm_getprocs(kt, KERN_PROC_PID, mypid, &cnt);
		goto one;
	}
	kvm_close(kt);
}

#elif defined (__NetBSD__)

static unsigned long
getmem(void)
{
	return 0;
}

static time_t
tv2sec(long sec, long usec, int mult)
{
	return sec*mult + (usec >= 500000/mult);
}

static void
getproc(struct proc *p, struct kinfo_proc2 *kp)
{
	memset(p, 0, sizeof *p);
	p->p_pid = kp->p_pid;
	strncpy(p->p_fname, kp->p_comm, sizeof p->p_fname);
	p->p_fname[sizeof p->p_fname - 1] = '\0';
	p->p_lstate[0] = kp->p_stat;
	p->p_ppid = kp->p_ppid;
	p->p_pgid = kp->p__pgid;
	p->p_sid = kp->p_sid;
	p->p_ttydev = kp->p_tdev;
	p->p_lflag = kp->p_flag;
	p->p_time = tv2sec(kp->p_uutime_sec + kp->p_ustime_sec,
			kp->p_uutime_usec + kp->p_ustime_usec, 1);
	p->p_accutime = p->p_time + tv2sec(kp->p_uctime_sec,
			kp->p_uctime_usec, 1);
	p->p_utime = tv2sec(kp->p_uutime_sec, kp->p_uutime_usec, 10);
	p->p_ktime = tv2sec(kp->p_ustime_sec, kp->p_ustime_usec, 10);
	p->p_intpri = kp->p_usrpri;
	p->p_rtpri = kp->p_priority;
	p->p_policy = SCHED_OTHER;
	p->p_c = kp->p_cpticks;
	p->p_nice = kp->p_nice;
	p->p_nlwp = 1;
	p->p_start = tv2sec(kp->p_ustart_sec, kp->p_ustart_usec, 1);
	p->p_osz = kp->p_vm_tsize + kp->p_vm_dsize + kp->p_vm_ssize;
	p->p_orss = kp->p_vm_rssize;
	p->p_pflts = kp->p_uru_majflt;
	p->p_bufr = kp->p_uru_inblock;
	p->p_bufw = kp->p_uru_oublock;
	p->p_mrcv = kp->p_uru_msgrcv;
	p->p_msnd = kp->p_uru_msgsnd;
	p->p_addr = kp->p_addr;
	p->p_wchan = kp->p_wchan;
	p->p_psr = kp->p_cpuid;
	p->p_pctcpu = kp->p_pctcpu;
	p->p_clname = "TS";
	p->p_uid = kp->p_ruid;
	p->p_euid = kp->p_uid;
	p->p_gid = kp->p_rgid;
	p->p_egid = kp->p_gid;
}

static void
getargv(struct proc *p, struct kinfo_proc2 *kp, kvm_t *kt)
{
	char	**args;
	char	*ap, *pp, *xp;

	if ((args = kvm_getargv2(kt, kp, sizeof p->p_psargs)) == NULL) {
		strncpy(p->p_psargs, p->p_fname, sizeof p->p_psargs);
		p->p_psargs[sizeof p->p_psargs - 1] = '\0';
		for (ap = p->p_comm, pp = p->p_psargs;
				*pp && !isblank(*pp); pp++)
			*ap++ = *pp;
		return;
	}
	ap = args[0];
	xp = p->p_comm;
	for (pp = p->p_psargs; pp < &p->p_psargs[sizeof p->p_psargs-1]; pp++) {
		if (*ap == '\0') {
			*pp = ' ';
			if (xp) {
				*xp = '\0';
				xp = NULL;
			}
			ap = *++args;
			if (ap == NULL)
				break;
		} else {
			if (xp)
				*xp++ = *ap;
			*pp = *ap++;
		}
	}
}

static void
postproc(struct proc *p)
{
	cleanline(p);
	switch (p->p_lstate[0]) {
	default:
	case SIDL:
		p->p_state[0] = 'I';
		break;
	case SRUN:
		p->p_state[0] = 'R';
		break;
	case SSLEEP:
		p->p_state[0] = 'S';
		break;
	case SSTOP:
		p->p_state[0] = 'T';
		break;
	case SZOMB:
	case SDEAD:
		p->p_state[0] = 'Z';
		break;
	case SONPROC:
		p->p_state[0] = 'O';
		break;
	}
	p->p_lstate[0] = p->p_state[0];
	if ((p->p_lflag & P_CONTROLT) == 0)
		p->p_ttydev = PRNODEV;
	if ((p->p_lflag & P_INMEM) == 0)
		p->p_flag |= FL_SWAP;
	else
		p->p_flag |= FL_LOAD;
	if (p->p_lflag & P_SYSTEM)
		p->p_flag |= FL_SYS;
	if ((p->p_lflag & P_SINTR) == 0)
		p->p_flag |= FL_LOCK;
	p->p_pri = p->p_rtpri;
	p->p_oldpri = p->p_intpri + 40;
	p->p_size = p->p_osz * kbytes_per_page;
	p->p_rssize = p->p_orss * kbytes_per_page;
}

static void
do_procs(void)
{
	struct proc	p;
	kvm_t	*kt;
	struct kinfo_proc2	*kp;
	int	i, cnt;
	pid_t	mypid = getpid();
	int	gotme = 0;

	if ((kt = kvm_open(NULL, NULL, NULL, KVM_NO_FILES, "kvm_open")) == NULL)
		exit(1);
	kp = kvm_getproc2(kt, KERN_PROC_ALL, 0, sizeof *kp, &cnt);
	i = cnt;
	while (--i >= 0) {
	one:	if (kp[i].p_pid == mypid)
			gotme = 1;
		getproc(&p, &kp[i]);
		getargv(&p, &kp[i], kt);
		postproc(&p);
		if (selectproc(&p) == OKAY)
			outproc(&p);
	}
	if (gotme == 0) {
		kp = kvm_getproc2(kt, KERN_PROC_PID, mypid, sizeof *kp, &cnt);
		goto one;
	}
	kvm_close(kt);
}

#elif defined (__APPLE__)

typedef struct kinfo_proc kinfo_proc;

static int
GetBSDProcessList(pid_t thepid, struct kinfo_proc **procList, size_t *procCount)
    /* derived from http://developer.apple.com/qa/qa2001/qa1123.html */
    /* Returns a list of all BSD processes on the system.  This routine
       allocates the list and puts it in *procList and a count of the
       number of entries in *procCount.  You are responsible for freeing
       this list (use "free" from System framework).
       all classic apps run in one process
       On success, the function returns 0.
       On error, the function returns a BSD errno value.
       Preconditions:
	assert( procList != NULL);
	assert(*procList == NULL);
	assert(procCount != NULL);
       Postconditions:
	assert( (err == 0) == (*procList != NULL) );
    */
{
	int			err;
	struct kinfo_proc	*result;
	int			mib[4];
	size_t			length;

	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	if (thepid == 0) {
		mib[2] = KERN_PROC_ALL;
		mib[3] = 0;
	} else {
		mib[2] = KERN_PROC_PID;
		mib[3] = thepid;
	}
	/* We start by calling sysctl with result == NULL and length == 0.
	   That will succeed, and set length to the appropriate length.
	   We then allocate a buffer of that size and call sysctl again
	   with that buffer.
	*/
	length = 0;
	err = sysctl(mib, 4, NULL, &length, NULL, 0);
	if (err == -1)
		err = errno;
	if (err == 0) {
		result = smalloc(length);
		err = sysctl(mib, 4, result, &length, NULL, 0);
		if (err == -1)
			err = errno;
		if (err == ENOMEM) {
			free(result); /* clean up */
			result = NULL;
		}
	}
	*procList = result;
	*procCount = err == 0 ? length / sizeof **procList : 0;
	return err;
}

static time_t
tv2sec(time_value_t *tv, int mult)
{
	return tv->seconds*mult + (tv->microseconds >= 500000/mult);
}

static unsigned long
getmem(void)
{
	static int	mib[] = {CTL_HW, HW_PHYSMEM, 0};
	size_t		size;
	unsigned long	mem;

	size = sizeof mem;
	if (sysctl(mib, 2, &mem, &size, NULL, 0) == -1) {
		fprintf(stderr, "error in sysctl(): %s\n", strerror(errno));
		exit(3);
	}
	return mem;
}

extern kern_return_t task_for_pid(task_port_t task, pid_t pid, task_port_t *target);

static void
getproc(struct proc *p, struct kinfo_proc *kp)
{
	kern_return_t   error;
	unsigned int	info_count = TASK_BASIC_INFO_COUNT;
	unsigned int 	thread_info_count = THREAD_BASIC_INFO_COUNT;
	task_port_t	task;
	pid_t		pid;
	struct		task_basic_info	task_binfo;
	struct		task_thread_times_info task_times;
	time_value_t	total_time, system_time;
	struct		task_events_info task_events;
	struct		policy_timeshare_info tshare;
	struct		policy_rr_info rr;
	struct		policy_fifo_info fifo;
	struct		thread_basic_info th_binfo;
	thread_port_array_t	thread_list;
	int		thread_count;
	int		j, temp, curpri;

	memset(p, 0, sizeof *p);

	p->p_pid = kp->kp_proc.p_pid;
	strncpy(p->p_fname, kp->kp_proc.p_comm, sizeof p->p_fname);
	p->p_fname[sizeof p->p_fname - 1] = '\0';
	p->p_lstate[0] = kp->kp_proc.p_stat; /* contains at least zombie info */
	p->p_lflag = kp->kp_proc.p_flag;
	p->p_ppid = kp->kp_eproc.e_ppid;
	p->p_pgid = kp->kp_eproc.e_pgid;
	p->p_sid = kp->kp_eproc.e_tpgid;
	p->p_ttydev = kp->kp_eproc.e_tdev == -1 ? PRNODEV : kp->kp_eproc.e_tdev;
	p->p_uid = kp->kp_eproc.e_pcred.p_ruid;
	p->p_euid = kp->kp_eproc.e_ucred.cr_uid;
	p->p_gid = kp->kp_eproc.e_pcred.p_rgid;
	p->p_egid = kp->kp_eproc.e_ucred.cr_gid;
	p->p_start = kp->kp_proc.p_starttime.tv_sec +
		(kp->kp_proc.p_starttime.tv_usec >= 500000);
	p->p_addr = (unsigned long)kp->kp_proc.p_addr;
	p->p_wchan = (unsigned long)kp->kp_proc.p_wchan;

	if (p->p_lstate[0] == SZOMB) {
		p->p_lstate[0] = 7;
		return; /* do not fetch more data for zombies */
	}

	pid = kp->kp_proc.p_pid;
	error = task_for_pid(mach_task_self(), pid, &task);
	if (error != KERN_SUCCESS) {
		/* process already left the system */
		p->p_lstate[0] = 7; /* handle exited process/task like zombie */
		p->p_clname = "??"; /* will be used as nice value */
		return;
	}
	info_count = TASK_BASIC_INFO_COUNT;
	error = task_info(task, TASK_BASIC_INFO, &task_binfo, &info_count);
	if (error != KERN_SUCCESS) {
		fprintf(stderr, "Error calling task_info():%d\n", error);
		exit(3);
	}
	info_count = TASK_THREAD_TIMES_INFO_COUNT;
	error = task_info(task, TASK_THREAD_TIMES_INFO, &task_times, &info_count);
	if (error != KERN_SUCCESS) {
		fprintf(stderr, "Error calling task_info():%d\n", error);
		exit(3);
	}
	info_count = TASK_EVENTS_INFO_COUNT;
	error = task_info(task, TASK_EVENTS_INFO, &task_events, &info_count);
	if (error != KERN_SUCCESS) {
		fprintf(stderr, "Error calling task_info():%d\n", error);
		exit(3);
	}

	total_time = task_times.user_time;
	p->p_utime = tv2sec(&total_time, 1);

	system_time = task_times.system_time;
	p->p_ktime = tv2sec(&system_time, 1);

	time_value_add(&total_time, &system_time);
	p->p_time = tv2sec(&total_time, 1);

	time_value_add(&total_time, &task_binfo.user_time);
	time_value_add(&total_time, &task_binfo.system_time);
	p->p_accutime = tv2sec(&total_time, 1);

	switch(task_binfo.policy) {
		case POLICY_TIMESHARE :
			info_count = POLICY_TIMESHARE_INFO_COUNT;
			error = task_info(task, TASK_SCHED_TIMESHARE_INFO, &tshare, &info_count);
			if (error == KERN_SUCCESS) {
				p->p_intpri = tshare.cur_priority;
				p->p_rtpri = tshare.base_priority;
				p->p_clname = "TS";
				p->p_policy = SCHED_OTHER;
			}
			break;
		case POLICY_RR :
			info_count = POLICY_RR_INFO_COUNT;
			error = task_info(task, TASK_SCHED_RR_INFO, &rr, &info_count);
			if (error == KERN_SUCCESS) {
				p->p_intpri = rr.base_priority;
				p->p_rtpri = rr.base_priority;
				p->p_clname = "RT";
				p->p_policy = SCHED_RR;
			}
			break;
		case POLICY_FIFO :
			info_count = POLICY_FIFO_INFO_COUNT;
			error = task_info(task, TASK_SCHED_FIFO_INFO, &fifo, &info_count);
			if (error == KERN_SUCCESS) {
				p->p_intpri = fifo.base_priority;
				p->p_rtpri = fifo.base_priority;
				p->p_clname = "FF";
				p->p_policy = SCHED_FIFO;
			}
			break;
	}
	p->p_nice = kp->kp_proc.p_nice;

	/* allocates a thread port array */
	error = task_threads(task, &thread_list, &thread_count);
	if (error != KERN_SUCCESS) {
		mach_port_deallocate(mach_task_self(), task);
		fprintf(stderr, "Error calling task_threads():%d\n", error);
		exit(3);
	}
	p->p_nlwp = thread_count;
	/* iterate over all threads for: cpu, state, swapped, prio */
	/* it should also be possible to print all mach threads as LWPs */
	p->p_lflag |= FL_SWAP; /* assume swapped */
	curpri = p->p_intpri;
	for (j = 0; j < thread_count; j++) {
		info_count = THREAD_BASIC_INFO_COUNT;
		error = thread_info(thread_list[j], THREAD_BASIC_INFO, &th_binfo, &info_count);
		if (error != KERN_SUCCESS) {
			fprintf(stderr, "Error calling thread_info():%d\n", error);
			exit(3);
		}
		p->p_c += th_binfo.cpu_usage;
		switch (th_binfo.run_state) {
			case TH_STATE_RUNNING:
				temp=1;
				break;
			case TH_STATE_UNINTERRUPTIBLE:
				temp=2;
				break;
			case TH_STATE_WAITING:
				temp=(th_binfo.sleep_time <= 20) ? 3 : 4;
				break;
			case TH_STATE_STOPPED:
				temp=5;
				break;
			case TH_STATE_HALTED:
				temp=6;
				break;
			default:
				temp=8;
		}
		if (temp < p->p_lstate[0])
			p->p_lstate[0] = temp;
		if ((th_binfo.flags & TH_FLAGS_SWAPPED ) == 0)
			p->p_lflag &= ~FL_SWAP; /* in mem */
		switch(th_binfo.policy) {
			case POLICY_TIMESHARE :
				info_count = POLICY_TIMESHARE_INFO_COUNT;
				error = thread_info(thread_list[j], THREAD_SCHED_TIMESHARE_INFO, &tshare, &info_count);
				if (error == KERN_SUCCESS && curpri < tshare.cur_priority)
					curpri = tshare.cur_priority;
				break;
			case POLICY_RR :
				info_count = POLICY_RR_INFO_COUNT;
				error = thread_info(thread_list[j], THREAD_SCHED_RR_INFO, &rr, &info_count);
				if (error == KERN_SUCCESS && curpri < rr.base_priority)
					curpri = rr.base_priority;
				break;
			case POLICY_FIFO :
				info_count = POLICY_FIFO_INFO_COUNT;
				error = thread_info(thread_list[j], THREAD_SCHED_FIFO_INFO, &fifo, &info_count);
				if (error == KERN_SUCCESS && curpri < fifo.base_priority)
					curpri = fifo.base_priority;
				break;
		}
		mach_port_deallocate(mach_task_self(), thread_list[j]);
	}
	p->p_intpri = curpri;
	/* free the thread port array */
	error = vm_deallocate(mach_task_self(), (vm_address_t)thread_list, thread_count * sizeof(thread_port_array_t));
	p->p_c = p->p_c / (TH_USAGE_SCALE/100);
	p->p_pctcpu = p->p_c;

	p->p_osz = task_binfo.virtual_size / pagesize;
	p->p_orss = task_binfo.resident_size / pagesize;

	p->p_pflts = task_events.pageins;
	p->p_bufr = 0;
	p->p_bufw = 0;
	p->p_mrcv = task_events.messages_sent; /* Mach messages */
	p->p_msnd = task_events.messages_received;
	
	mach_port_deallocate(mach_task_self(), task);
}

static void
getargv(struct proc *p, struct kinfo_proc *kp)
{
	size_t	size, argsz;
	char	*argbuf;
	int	mib[3];
	long	nargs;
	char	*ap, *pp, *xp;

	/* ignore kernel and zombies */
	if (kp->kp_proc.p_pid == 0 || p->p_lstate[0] == 7)
		return;

	/* allocate a procargs space per process */
	mib[0] = CTL_KERN;
	mib[1] = KERN_ARGMAX;
	size = sizeof argsz;
	if (sysctl(mib, 2, &argsz, &size, NULL, 0) == -1) {
		fprintf(stderr, "error in sysctl(): %s\n", strerror(errno));
		exit(3);
	}
	argbuf = smalloc(argsz);

	/* fetch the process arguments */
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROCARGS2;
	mib[2] = kp->kp_proc.p_pid;
	if (sysctl(mib, 3, argbuf, &argsz, NULL, 0) == -1) {
		/* process already left the system */
		return;
	}

	/* the number of args is at offset 0, this works for 32 and 64bit */
	memcpy(&nargs, argbuf, sizeof nargs);
	ap = argbuf + sizeof nargs;

	/* skip the exec_path */
	while (ap < &argbuf[argsz] && *ap != '\0')
		ap++;
	if (ap == &argbuf[argsz])
		goto DONE; /* no args to show */
	/* skip trailing '\0' chars */
	while (ap < &argbuf[argsz] && *ap == '\0')
		ap++;
	if (ap == &argbuf[argsz])
		goto DONE; /* no args to show */

	xp = p->p_comm; /* copy the command name also */
	/* now concat copy the arguments */
	for (pp = p->p_psargs; pp < &p->p_psargs[sizeof p->p_psargs-1]; pp++) {
		if (*ap == '\0') {
			if (xp) {
				*xp = '\0';
				xp = NULL;
			}
			if (--nargs == 0)
				break;
			*pp = ' ';
			++ap;
		} else {
			if (xp)
				*xp++ = *ap;
			*pp = *ap++;
		}
	}
	*pp = '\0';

DONE:	free(argbuf);
	return;
}

static void
postproc(struct proc *p)
{
	cleanline(p);
	if (p->p_lstate[0] < 0 || p->p_lstate[0] > 8) /* play safe */
		p->p_lstate[0] = 8;
	p->p_state[0] = " RSSITHZ?"[p->p_lstate[0]];
	p->p_lstate[0] = p->p_state[0];
	if (p->p_lflag & P_SYSTEM)
		p->p_flag |= FL_SYS;
	p->p_pri = p->p_rtpri;
	p->p_oldpri = p->p_intpri;
	p->p_size = p->p_osz * kbytes_per_page;
	p->p_rssize = p->p_orss * kbytes_per_page;
}

static void
do_procs(void)
{
	struct	proc p;
	struct	kinfo_proc *kp = NULL;
	size_t	i, cnt;
	pid_t	pid0;
	int	err;

	/* get all processes */
	pid0 = 0;
	if ((err = GetBSDProcessList(pid0, &kp, &cnt)) != 0) {
		fprintf(stderr, "error getting proc list: %s\n", strerror(err));
		exit(3);
	}
	i = cnt;
	while (--i >= 0) {
		/* ignore trailing garbage processes with pid 0 */
		if (kp[i].kp_proc.p_pid == 0 && pid0++ > 0)
			break;
		getproc(&p, &kp[i]);
		getargv(&p, &kp[i]);
		postproc(&p);
		if (selectproc(&p) == OKAY)
			outproc(&p);
	}
	/* free the memory allocated by GetBSDProcessList */
	free(kp);	
}

#endif	/* all */

/************************************************************************
 * 			Option scanning					*
 ************************************************************************/

static void
add_device(const char *prefix, size_t prefixlen,
		const char *name, struct stat *sp)
{
	char	*str;
	dev_type	sz;

	if (eq(name, "stdin") || eq(name, "stdout") || eq(name, "stderr"))
		return;
	sz = prefixlen + strlen(name) + 1;
	str = smalloc(sz);
	strcpy(str, prefix);
	strcpy(&str[prefixlen], name);
	dlook(sp->st_rdev, d0, str);
#ifdef	USE_PS_CACHE
	if (devfp != NULL) {
		dev_type	dev = sp->st_rdev;

		fwrite(&dev, sizeof dev, 1, devfp);
		fwrite(&sz, sizeof sz, 1, devfp);
		fwrite(str, 1, sz, devfp);
	}
#endif	/* USE_PS_CACHE */
}

static void
add_devices_from(const char *path, const char *prefix)
{
	DIR	*Dp;
	struct dirent	*dp;
	struct stat	st;
	size_t	prefixlen;

	if (chdir(path) == 0) {
		if ((Dp = opendir(".")) != NULL) {
			prefixlen = strlen(prefix);
			while ((dp = readdir(Dp)) != NULL) {
				if (stat(dp->d_name, &st) == 0 &&
						S_ISCHR(st.st_mode))
					add_device(prefix, prefixlen,
							dp->d_name, &st);
			}
			closedir(Dp);
		}
	}
}

static void
devices(void)
{
#ifdef	USE_PS_CACHE
	struct stat	dst, fst;
#endif	/* USE_PS_CACHE */
	struct output	*o;

	for (o = o0; o; o = o->o_nxt)
		if (o->o_typ == OU_TTY)
			break;
	if (o == NULL)
		return;
	d0 = scalloc(256, sizeof *d0);
	add_devices_from("/dev/pts", "pts/");
	/*
	 * Names in devfs.
	 */
	add_devices_from("/dev/tts", "tts/");
	add_devices_from("/dev/cua", "cua/");
	add_devices_from("/dev/vc", "vc/");
	add_devices_from("/dev/vcc", "vcc/");
	add_devices_from("/dev/pty", "pty/");
#ifdef	USE_PS_CACHE
	if (stat(ps_cache_file, &fst) < 0 ||
		(stat("/dev", &dst) == 0 && dst.st_mtime > fst.st_mtime) ||
		(stat("/dev/usb", &dst) == 0 && dst.st_mtime > fst.st_mtime) ||
		(stat("/dev/term", &dst) == 0 && dst.st_mtime > fst.st_mtime)) {
putcache:
		if (dropprivs && myuid && myeuid && myuid != myeuid)
			setuid(myeuid);
		umask(0022);
		if ((devfp = wopen(ps_cache_file)) != NULL) {
			fchown(fileno(devfp), myeuid, ps_cache_gid);
			fchmod(fileno(devfp), ps_cache_mode);
			fwrite(cacheid, 1, strlen(cacheid) + 1, devfp);
		}
		if (dropprivs && myuid != myeuid)
			setuid(myuid);
muststat:
#endif	/* USE_PS_CACHE */
		add_devices_from("/dev/term", "term/");
		add_devices_from("/dev/usb", "usb/");
		add_devices_from("/dev", "");
#ifdef	USE_PS_CACHE
	} else {
		char	*str;
		dev_type	dev;
		dev_type	sz;
		char	*thisid;

		if ((fst.st_uid != 0 && fst.st_uid != myeuid) ||
				(devfp = fopen(ps_cache_file, "r")) == NULL)
			goto muststat;
		sz = strlen(cacheid) + 1;
		thisid = alloca(sz);
		if (fread(thisid, 1, sz, devfp) != sz ||
				strcmp(cacheid, thisid)) {
			fclose(devfp);
			devfp = NULL;
			goto putcache;
		}
		if (dropprivs && myuid != myeuid)
			setuid(myuid);
		while (fread(&dev, sizeof dev, 1, devfp) == 1 &&
				fread(&sz, sizeof sz, 1, devfp) == 1) {
			str = smalloc(sz);
			if (fread(str, 1, sz, devfp) != sz)
				break;
			dlook(dev, d0, str);
		}
	}
	if (devfp != NULL) {
		fclose(devfp);
		devfp = NULL;
	}
#endif	/* USE_PS_CACHE */
}

#ifdef	UCB
static void
usage(void)
{
	fprintf(stderr, "usage: %s [ -acglnrSuvwx ] [ -t term ] [ num ]\n",
		progname);
	exit(2);
}
#else	/* !UCB */
static void
usage(void)
{
	fprintf(stderr, "\
usage: %s [ -edalfcj ] [ -r sysname ] [ -t termlist ]\n\
        [ -u uidlist ] [ -p proclist ] [ -g grplist ] [ -s sidlist ]\n",
	  progname);
	exit(2);
}
#endif	/* !UCB */

static const char *
element(const char **listp, int override)
{
	static char	*buf;
	static size_t	buflen;
	const char	*cp, *op;
	char	*cq;
	size_t	sz;
	int	stop = ',';

	if (**listp == '\0')
		return NULL;
	op = *listp;
	while (**listp != '\0') {
		if (**listp == override)
			stop = '\0';
		if (stop != '\0' && (**listp == stop || isblank(**listp)))
			break;
		(*listp)++;
	}
	if (**listp == '\0')
		return op;
	if ((sz = *listp - op + 1) > buflen) {
		buflen = sz;
		buf = srealloc(buf, buflen);
	}
	for (cp = op, cq = buf; cp < *listp; cp++, cq++)
		*cq = *cp;
	*cq = '\0';
	if (**listp) {
		while (**listp == stop || isblank(**listp))
			(*listp)++;
	}
	return buf;
}

static void
add_criterion(enum crtype cy, unsigned long val)
{
	struct criterion	*ct;

	ct = scalloc(1, sizeof *ct);
	ct->c_typ = cy;
	ct->c_val = val;
	ct->c_nxt = c0;
	c0 = ct;
}

static enum okay
get_rdev(const char *device, unsigned long *id)
{
	struct stat	st;
	char	*file;

	*id = 0;
	file = alloca(strlen(device) + 9);
	strcpy(file, "/dev/");
	strcpy(&file[5], device);
	if (stat(file, &st) < 0) {
		strcpy(file, "/dev/tty");
		strcpy(&file[8], device);
		if (stat(file, &st) == 0)
			*id = st.st_rdev;
		else if ((device[0] == '?' || device[0] == '-') &&
				device[1] == '\0')
			add_criterion(CR_WITHOUT_TTY, 0);
		else
			return STOP;
	} else
		*id = st.st_rdev;
	return OKAY;
}

static void
nonnumeric(const char *string, enum crtype ct)
{
#ifndef	UCB
	int	c;

	switch (ct) {
	case CR_PROCESS_GROUP:
		c = 'g';
		break;
	case CR_PROCESS_ID:
		c = 'p';
		break;
	case CR_SESSION_LEADER:
		c = 's';
		break;
	default:
		c = '?';
	}
	fprintf(stderr,
		"%s: %s is an invalid non-numeric argument for -%c option\n",
			progname, string, c);
#else	/* UCB */
	fprintf(stderr,
		"%s: %s is an invalid non-numeric argument for a process id\n",
			progname, string);
#endif	/* UCB */
}

static void
add_criterion_string(enum crtype ct, const char *string)
{
	struct passwd	*pwd;
	struct group	*grp;
	char	*x;
	unsigned long	val = 0;

	switch (ct) {
	case CR_ALL:
	case CR_ALL_WITH_TTY:
	case CR_ALL_BUT_SESSION_LEADERS:
	case CR_WITHOUT_TTY:
	case CR_NO_TTY_NO_SESSION_LEADER:
	case CR_ADD_UNINTERESTING:
	case CR_DEFAULT:
		val = 0;
		break;
	case CR_PROCESS_GROUP:
	case CR_PROCESS_ID:
	case CR_SESSION_LEADER:
		val = strtoul(string, &x, 10);
		if (*x != '\0' || *string == '+' || *string == '-' ||
				*string == '\0') {
			nonnumeric(string, ct);
			ct = CR_INVALID_STOP;
		}
		break;
	case CR_REAL_GID:
		if ((grp = getgrnam(string)) != NULL) {
			val = grp->gr_gid;
		} else {
			val = strtoul(string, &x, 10);
			if (*x != '\0' || *string == '+' || *string == '-' ||
					*string == '\0') {
				fprintf(stderr, "%s: unknown group %s\n",
						progname, string);
				ct = CR_INVALID_REAL_GID;
			}
		}
		break;
	case CR_EFF_UID:
	case CR_REAL_UID:
		if ((pwd = getpwnam(string)) != NULL) {
			val = pwd->pw_uid;
		} else {
			val = strtoul(string, &x, 10);
			if (*x != '\0' || *string == '+' || *string == '-' ||
					*string == '\0') {
				fprintf(stderr, "%s: unknown user %s\n",
						progname, string);
				ct = (ct == CR_EFF_UID ? CR_INVALID_EFF_UID :
					CR_INVALID_REAL_UID);
			}
		}
		break;
	case CR_TTY_DEVICE:
		if (get_rdev(string, &val) == STOP) {
			add_criterion(CR_INVALID_TTY_DEVICE, 0);
			return;
		}
		break;
	}
	add_criterion(ct, val);
}

static void
add_criteria_list(enum crtype ct, const char *list)
{
	const char	*cp;

	if (*list)
		while ((cp = element(&list, '\0')) != NULL)
			add_criterion_string(ct, cp);
	else
		add_criterion_string(ct, "");
}

static void
add_format(enum outype ot, const char *name)
{
	struct output	*op, *oq;
	unsigned	i = 0;

	while (outspec[i].os_typ != ot)
		i++;
	op = scalloc(1, sizeof *op);
	op->o_typ = ot;
	if (outspec[i].os_flags & OS_Lflag)
		Lflag |= 1;
	if (name == NULL) {
		op->o_nam = outspec[i].os_def;
		op->o_len = strlen(op->o_nam);
		dohdr++;
	} else if (*name == '\0') {
		op->o_nam = "";
		op->o_len = strlen(outspec[i].os_def);
	} else {
		op->o_nam = smalloc(strlen(name) + 1);
		strcpy(op->o_nam, name);
		op->o_len = strlen(op->o_nam);
		dohdr++;
	}
	if (o0 != NULL) {
		for (oq = o0; oq->o_nxt; oq = oq->o_nxt);
		oq->o_nxt = op;
	} else
		o0 = op;
}

static void
add_format_string(const char *string)
{
	char	*fmt;
	char	*name = NULL;
	unsigned	i;

	fmt = alloca(strlen(string) + 1);
	strcpy(fmt, string);
	if ((name = strchr(fmt, '=')) != NULL) {
		*name++ = '\0';
	}
	for (i = 0; outspec[i].os_fmt != NULL; i++) {
		if (eq(outspec[i].os_fmt, fmt)) {
			add_format(outspec[i].os_typ, name);
			break;
		}
	}
	if (outspec[i].os_fmt == NULL) {
		fprintf(stderr, "%s: unknown output format: -o %s\n",
				progname, string);
		usage();
	}
}

static void
add_format_list(const char *list)
{
	const char	*cp;

	while ((cp = element(&list, '=')) != NULL)
		add_format_string(cp);
}

static void
defaults(void)
{
	FILE	*fp;

	if ((fp = fopen(DEFAULT, "r")) != NULL) {
		char	buf[LINE_MAX];
		char	*cp, *x;

		while (fgets(buf, sizeof buf, fp) != NULL) {
			if (buf[0] == '\0' || buf[0] == '\n' || buf[0] == '#')
				continue;
			if ((cp = strchr(buf, '\n')) != NULL)
				*cp = '\0';
			if (strncmp(buf, "O1_SCHEDULER=", 13) == 0) {
				sched_selection = atoi(&buf[13]) ? 1 : -1;
			} else if (strncmp(buf, "DROPPRIVS=", 10) == 0) {
				dropprivs = strtol(&buf[10], &x, 10);
				if (*x != '\0' || dropprivs != 1)
					dropprivs = 0;
			}
#ifdef	USE_PS_CACHE
			else if (strncmp(buf, "CACHE_FILE=", 11) == 0) {
				if (buf[11] == '/' && buf[12] != '\0') {
					ps_cache_file = smalloc(strlen(
							&buf[11]) + 1);
					strcpy(ps_cache_file, &buf[11]);
				}
			} else if (strncmp(buf, "CACHE_MODE=", 11) == 0) {
				mode_t	m;

				m = strtol(&buf[11], &x, 8);
				if (*x == '\0')
					ps_cache_mode = m;
			} else if (strncmp(buf, "CACHE_GROUP=", 12) == 0) {
				struct group	*grp;
				gid_t	gid;

				if ((grp = getgrnam(&buf[12])) == NULL) {
					gid = strtoul(&buf[12], &x, 10);
					if (*x == '\0')
						ps_cache_gid = gid;
				} else
					ps_cache_gid = grp->gr_gid;
			}
#endif	/* USE_PS_CACHE */
		}
		fclose(fp);
	}
}

#ifndef	UCB
static const char	optstring[] = ":aAcdefg:G:jlLn:o:Pp:r:Rs:t:u:U:Ty";
#else	/* UCB */
static const char	optstring[] = ":acglLnrSuvwxt:R:AG:p:U:o:";
#endif	/* UCB */

/*
 * If -r sysname is given, chroot() needs to be done before any files are
 * opened -> scan options twice, first for evaluating '-r' and syntactic
 * correctness, then for evaluating other options (in options() below).
 */
static void
sysname(int ac, char **av)
{
	extern int	chroot(const char *);
	const char	*dir = NULL;
	int	i, hadflag = 0, illegal = 0;

	while ((i = getopt(ac, av, optstring)) != EOF) {
		switch (i) {
#ifndef	UCB
		case 'r':
			rflag = optarg;
			break;
		case 'e':
		case 's':
		case 'd':
		case 'a':
		case 't':
		case 'p':
		case 'u':
		case 'g':
		case 'U':
		case 'G':
		case 'A':
			hadflag = 1;
			break;
#else	/* UCB */
		case 'R':
			rflag = optarg;
			break;
		case 'a':
		case 'x':
		case 't':
		case 'p':
		case 'U':
		case 'G':
		case 'A':
			hadflag = 1;
			break;
#endif	/* UCB */
		case ':':
			fprintf(stderr,
				"%s: option requires an argument -- %c\n",
				progname, optopt);
			illegal = 1;
			break;
		case '?':
			fprintf(stderr, "%s: illegal option -- %c\n",
				progname, optopt);
			illegal = 1;
			break;
		}
	}
	if (illegal)
		usage();
#ifndef	UCB
	if (av[optind])
		usage();
#else	/* UCB */
	if (av[optind] && av[optind + 1]) {
		fprintf(stderr, "%s: too many arguments\n", progname);
		usage();
	}
#endif	/* UCB */
	if (rflag) {
		if (hadflag == 0) {
			fprintf(stderr,
		"%s: one of -%s must be used with -%c sysname\n",
				progname,
#ifndef	UCB
				"esdatpugUGA", 'r'
#else
				"axtpUGA", 'R'
#endif
				);
			usage();
		}
		if (*rflag != '/') {
#if defined (__linux__) || defined (__hpux) || defined (_AIX)
			FILE	*fp;
			struct mntent	*mp;
#if defined (__linux__) || defined (_AIX)
			const char	mtab[] = "/etc/mtab";
#else
			const char	mtab[] = "/etc/mnttab";
#endif

			if ((fp = setmntent(mtab, "r")) == NULL) {
				fprintf(stderr, "%s: cannot open %s\n",
						progname, mtab);
				exit(1);
			}
			dir = NULL;
			while ((mp = getmntent(fp)) != NULL) {
				if (strcmp(mp->mnt_type, MNTTYPE_IGNORE) == 0)
					continue;
				if (strcmp(rflag, basename(mp->mnt_dir)) == 0) {
					dir = sstrdup(mp->mnt_dir);
					break;
				}
			}
			endmntent(fp);
#elif defined (__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) \
	|| defined (__DragonFly__) || defined (__APPLE__)
			struct statfs	*sp = NULL;
			int	cnt, i;

			if ((cnt = getmntinfo(&sp, MNT_WAIT)) <= 0) {
				fprintf(stderr, "%s: cannot get mounts\n",
						progname);
				exit(1);
			}
			for (i = 0; i < cnt; i++)
				if (!strcmp(rflag,
						basename(sp[i].f_mntonname))) {
					dir = sstrdup(sp[i].f_mntonname);
					break;
				}
#else	/* SVR4 */
			FILE	*fp;
			struct mnttab	mt;
			const char	mtab[] = "/etc/mnttab";

			if ((fp = fopen(mtab, "r")) == NULL) {
				fprintf(stderr, "%s: cannot open %s\n",
						progname, mtab);
				exit(1);
			}
			dir = NULL;
			while (getmntent(fp, &mt) == 0)
				if (!strcmp(rflag, basename(mt.mnt_mountp))) {
					dir = sstrdup(mt.mnt_mountp);
					break;
				}
			fclose(fp);
#endif	/* SVR4 */
			if (dir == NULL) {
				fprintf(stderr,
					"%s: cannot find path to system %s\n",
					progname, rflag);
				exit(1);
			}
		} else
			dir = rflag;
		if (chroot(dir) < 0) {
			fprintf(stderr, "%s: cannot change root to %s\n",
					progname, dir);
			exit(1);
		}
	}
	optind = 1;
}

#ifndef	UCB
extern int		sysv3;		/* emulate SYSV3 behavior */

static void
options(int ac, char **av)
{
	int	cflag = 0;		/* priocntl format */
	int	fflag = 0;		/* full format */
	int	jflag = 0;		/* jobs format */
	int	lflag = 0;		/* long format */
	int	Pflag = 0;		/* print processor information */
	int	Rflag = 0;		/* EP/IX resource format */
	int	Tflag = 0;		/* EP/IX thread format */
	int	yflag = 0;		/* modify format */
	int	i;

	if (getenv("SYSV3") != NULL)
		sysv3 = 1;
#ifdef	S42
	cflag = 1;
#endif	/* S42 */
	while ((i = getopt(ac, av, optstring)) != EOF) {
		switch (i) {
		case 'a':
			add_criterion(CR_ALL_WITH_TTY, 0);
			break;
		case 'c':
			cflag++;
			break;
		case 'd':
			add_criterion(CR_ALL_BUT_SESSION_LEADERS, 0);
			break;
		case 'e':
		case 'A':
			add_criterion(CR_ALL, 0);
			break;
		case 'f':
			fflag = 1;
			break;
		case 'g':
			add_criteria_list(CR_PROCESS_GROUP, optarg);
			break;
		case 'G':
			add_criteria_list(CR_REAL_GID, optarg);
			break;
		case 'j':
			jflag = 1;
			break;
		case 'l':
			lflag = 1;
			break;
		case 'L':
			Lflag = 3;
			break;
		case 'n':
			fprintf(stderr, "%s: warning: -n option ignored\n",
					progname);
			break;
		case 'o':
			oflag = 1;
			add_format_list(optarg);
			break;
		case 'P':
			Pflag = 1;
			break;
		case 'p':
			add_criteria_list(CR_PROCESS_ID, optarg);
			break;
		case 'r':
			rflag = optarg;
			break;
		case 'R':
			Rflag = 1;
			break;
		case 's':
			add_criteria_list(CR_SESSION_LEADER, optarg);
			break;
		case 't':
			add_criteria_list(CR_TTY_DEVICE, optarg);
			break;
		case 'T':
			Tflag = 1;
			break;
		case 'u':
#ifdef	SUS
			add_criteria_list(CR_EFF_UID, optarg);
			break;
#else	/* !SUS */
			/*FALLTHRU*/
#endif	/* !SUS */
		case 'U':
			add_criteria_list(CR_REAL_UID, optarg);
			break;
		case 'y':
			yflag = 1;
			break;
		}
	}
	if (Rflag)
		lflag = fflag = 0;
	if (o0 == NULL) {
#ifdef	SUS
		const char	*cmd_str = "CMD";
#else
		const char	*cmd_str = "COMD";
		if (sysv3 && !lflag)
			cmd_str = "COMMAND";
#endif
		if (fflag || jflag || lflag) {
			if (jflag || (lflag && yflag))
				add_format(OU_SPACE, NULL);
			if (lflag && !yflag)
				add_format(OU_F, NULL);
			if (lflag)
				add_format(OU_S, NULL);
#ifdef	SUS
			if (fflag)
				add_format(OU_USER, "     UID");
			else if (lflag)
				add_format(OU_UID, NULL);
#else	/* !SUS */
			if (fflag)
				add_format(OU_RUSER, "     UID");
			else if (lflag)
				add_format(OU_RUID, "  UID");
#endif	/* !SUS */
			add_format(OU_PID, NULL);
			if (Tflag && !fflag)
				add_format(OU_STID, NULL);
			if (fflag || lflag)
				add_format(OU_PPID, NULL);
			if (jflag) {
				add_format(OU_PGID, NULL);
				add_format(OU_SID, NULL);
			}
			if (Lflag & 2) {
				add_format(OU_LWP, NULL);
				if (fflag)
					add_format(OU_NLWP, NULL);
			}
			if (Tflag)
				add_format(OU_TID, NULL);
			if (Pflag || Tflag)
				add_format(OU_PSR, NULL);
			if (Tflag && !fflag)
				add_format(OU_NTP, NULL);
#ifndef	S42
			if (cflag) {
				add_format(OU_CLASS, NULL);
				add_format(OU_PRI, NULL);
			} else {
				if (fflag || lflag)
					add_format(OU_C, NULL);
				if (lflag) {
					add_format(OU_OPRI, NULL);
					add_format(OU_NICE, NULL);
				}
			}
#else	/* S42 */
			add_format(OU_CLASS, NULL);
			add_format(OU_PRI, NULL);
			if (fflag || lflag)
				add_format(OU_C, NULL);
#endif	/* S42 */
			if (lflag) {
				if (yflag) {
					add_format(OU_RSS, NULL);
					add_format(OU_VSZ, "    SZ");
				} else {
					add_format(OU_ADDR, NULL);
					add_format(OU_OSZ, NULL);
				}
				add_format(OU_WCHAN, NULL);
			}
			if (fflag)
				add_format(OU_STIME, NULL);
			add_format(OU_TTY, "TTY    ");
			if (Lflag & 2)
				add_format(OU_LTIME, NULL);
			else
				add_format(OU_OTIME, NULL);
			if (fflag)
				add_format(OU_ARGS, cmd_str);
			else
				add_format(OU_FNAME, cmd_str);
		} else {
			add_format(OU_SPACE, NULL);
			add_format(OU_PID, NULL);
			if (Lflag & 2)
				add_format(OU_LWP, NULL);
			if (Tflag) {
				add_format(OU_STID, NULL);
				add_format(OU_TID, NULL);
			}
			if (Pflag || Tflag)
				add_format(OU_PSR, NULL);
			if (Tflag)
				add_format(OU_NTP, NULL);
			if (cflag) {
				add_format(OU_CLASS, NULL);
				add_format(OU_PRI, NULL);
			}
			if (Rflag) {
				add_format(OU_OSZ, "   SZ");
				add_format(OU_MRSZ, NULL);
				add_format(OU_PFLTS, NULL);
				add_format(OU_BUFR, NULL);
				add_format(OU_BUFW, NULL);
				add_format(OU_MRCV, NULL);
				add_format(OU_MSND, NULL);
				add_format(OU_UTIME, NULL);
				add_format(OU_KTIME, NULL);
			} else {
				add_format(OU_TTY, "TTY    ");
				if (Lflag & 2)
					add_format(OU_LTIME, NULL);
				else
					add_format(OU_OTIME, NULL);
			}
			add_format(OU_FNAME, cmd_str);
		}
	}
}
#else	/* UCB */
/*
 * Note that the 'UCB' version is not actually oriented at historical
 * BSD usage, but at /usr/ucb/ps of SVR4 (with POSIX.2 extensions).
 */
static void
options(int ac, char **av)
{
	char	*cp;
	int	i, format = 0, agxsel = 0, illegal = 0;
	int	cflag = 0;	/* display command name instead of args */
	int	nflag = 0;	/* print numerical IDs */
	int	Sflag = 0;	/* display accumulated time */
	int	wflag = 0;	/* screen width */

	while ((i = getopt(ac, av, optstring)) != EOF) {
		switch (i) {
		case 'a':
			agxsel |= 01;
			break;
		case 'A':
			add_criterion(CR_ALL, 0);
			break;
		case 'c':
			cflag = 1;
			break;
		case 'g':
			agxsel |= 02;
			break;
		case 'G':
			add_criteria_list(CR_REAL_GID, optarg);
			break;
		case 'l':
			format = 'l';
			break;
		case 'L':
			Lflag = 1;
			break;
		case 'n':
			nflag = 1;
			break;
		case 'o':
			oflag = 1;
			wflag = 2;	/* do not limit width */
			add_format_list(optarg);
			break;
		case 'p':
			add_criteria_list(CR_PROCESS_ID, optarg);
			break;
		case 'r':
			ucb_rflag = 1;
			break;
		case 'S':
			Sflag = 1;
			break;
		case 't':
			add_criteria_list(CR_TTY_DEVICE, optarg);
			agxsel &= ~04;
			break;
		case 'u':
			format = 'u';
			break;
		case 'U':
			/*
			 * 'U' without argument is 'update ps database' in
			 * historical /usr/ucb/ps. We implement the POSIX.2
			 * option instead.
			 */
			add_criteria_list(CR_REAL_UID, optarg);
			break;
		case 'v':
			format = 'v';
			break;
		case 'w':
			wflag++;
			break;
		case 'x':
			agxsel |= 04;
			break;
		}
	}
	if (illegal)
		usage();
	if (av[optind]) {
		add_criteria_list(CR_PROCESS_ID, av[optind]);
		agxsel = 0;
		ucb_rflag = 0;
	}
	switch (agxsel) {
	case 01|04:
	case 01|02|04:
		add_criterion(CR_ALL, 0);
		break;
	case 02|04:
		add_criterion(CR_WITHOUT_TTY, 0);
		add_criterion(CR_ADD_UNINTERESTING, 0);
		break;
	case 01:
	case 01|02:
		add_criterion(CR_ALL_WITH_TTY, 0);
		break;
	case 02:
		add_criterion(CR_ADD_UNINTERESTING, 0);
		break;
	case 04:
		add_criterion(CR_NO_TTY_NO_SESSION_LEADER, 0);
		break;
	}
	if (o0 == NULL) {
		if (format == 'l') {
			add_format(OU_F, NULL);
			add_format(OU_RUID, "  UID");
		} else if (format == 'u') {
			if (nflag)
				add_format(OU_RUID, "   UID");
			else
				add_format(OU_RUSER, "USER    ");
		}
		if (format == 'l') {
			add_format(OU_PID, NULL);
			add_format(OU_PPID, NULL);
		} else if (format == 'u')
			add_format(OU_PID, "  PID");
		else
			add_format(OU_PID, "   PID");
		if (format == 'l' || format == 'u')
			add_format(OU_C, "CP");
		if (format == 'l') {
			add_format(OU_OPRI, NULL);
			add_format(OU_NICE, NULL);
		}
		if (format == 'l' || format == 'u') {
			add_format(OU_OSZ, "   SZ");
			add_format(OU_ORSS, "  RSS");
		}
		if (format == 'l') {
			add_format(OU_WCHAN, NULL);
			add_format(OU_S, NULL);
			add_format(OU_TTY, "TT      ");
		} else
			add_format(OU_TTY, "TT     ");
		if (format == 'u' || format == 'v' || format == 0)
			add_format(OU_S, " S");
		if (format == 'u')
			add_format(OU_STIME, "   START");
		else if (format == 'v') {
			add_format(OU_OSZ, " SIZE");
			add_format(OU_ORSS, "  RSS");
		}
		add_format(Sflag ? OU_ACCUTIME : OU_OTIME, NULL);
		add_format(cflag ? OU_FNAME : OU_ARGS, "COMMAND");
	}
	if ((cp = getenv("COLUMNS")) != NULL)
		maxcolumn = strtol(cp, NULL, 10);
	if (maxcolumn <= 0) {
#ifdef	TIOCGWINSZ
		struct winsize	winsz;

		if (ioctl(1, TIOCGWINSZ, &winsz) == 0 && winsz.ws_col > 0)
			maxcolumn = winsz.ws_col;
		else
#endif	/* TIOCGWINSZ */
			maxcolumn = 80;
	}
	if (wflag == 1)
		maxcolumn += 52;
	else if (wflag > 1)
		maxcolumn = 0;
}
#endif	/* UCB */

int
main(int argc, char **argv)
{
#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	progname = basename(argv[0]);
	sysname(argc, argv);
	defaults();
	myuid = getuid();
	myeuid = geteuid();
	if (dropprivs && myuid && myeuid && myuid != myeuid)
		setuid(myuid);
	setlocale(LC_CTYPE, "");
	setlocale(LC_TIME, "");
	mb_cur_max = MB_CUR_MAX;
	ontty = isatty(1);
	options(argc, argv);
	devices();
#if !defined (__hpux) && !defined (_AIX) && !defined (__NetBSD__) && \
		!defined (__OpenBSD__) && !defined (__APPLE__)
	chdir_to_proc();
#endif
#ifdef	__linux__
	get_linux_version();
	if (linux_version_lt(2, 5, 0) && has_o1_sched() == 0)
		compute_priority = compute_priority_old;
	else
		compute_priority = compute_priority_new;
#endif	/* __linux__ */
	hz = sysconf(_SC_CLK_TCK);
	time(&now);
#ifdef	__linux__
	uptime = sysup();
#endif	/* __linux__ */
#ifdef __APPLE__
	{
		static int mib[] = {CTL_HW, HW_PAGESIZE, 0};
		size_t size;
		size = sizeof pagesize;
		if (sysctl(mib, 2, &pagesize, &size, NULL, 0) == -1) {
			fprintf(stderr, "error in sysctl(): %s\n", strerror(errno));
			exit(3);
		}
	}
#else
	pagesize = sysconf(_SC_PAGESIZE);
#endif
	kbytes_per_page = (pagesize >> 10);
#ifndef	__sun
	totalmem = getmem();
#endif	/* !__sun */
#if defined (__linux__) || defined (__sun)
	getproc("self", &myproc, getpid(), -1);
#elif defined (__FreeBSD__) || defined (__DragonFly__)
	getproc("curproc", &myproc, getpid(), -1);
#elif defined (__hpux)
	{
		struct pst_status	pst;
		pid_t	mypid = getpid();
		pstat_getproc(&pst, sizeof pst, (size_t)0, mypid);
		getproc(&myproc, &pst);
	}
#elif defined (_AIX)
	{
		struct stat	st;
		int	fd;

		if ((fd = open("/dev/tty", O_RDONLY)) >= 0) {
			if (stat(ttyname(fd), &st) == 0)
				myproc.p_ttydev = st.st_rdev;
			close(fd);
		}
	}
#elif defined (__OpenBSD__)
	{
		kvm_t	*kt;
		struct kinfo_proc	*kp;
		int	mypid = getpid();
		int	cnt;

		if ((kt = kvm_open(NULL, NULL, NULL, KVM_NO_FILES,
						"kvm_open")) == NULL)
			exit(1);
		kp = kvm_getprocs(kt, KERN_PROC_PID, mypid, &cnt);
		if (kp != NULL)
			getproc(&myproc, &kp[0]);
		kvm_close(kt);
	}
#elif defined (__NetBSD__)
	{
		kvm_t	*kt;
		struct kinfo_proc2	*kp;
		int	mypid = getpid();
		int	cnt;

		if ((kt = kvm_open(NULL, NULL, NULL, KVM_NO_FILES,
						"kvm_open")) == NULL)
			exit(1);
		kp = kvm_getproc2(kt, KERN_PROC_PID, mypid,
				sizeof *kp, &cnt);
		if (kp != NULL)
			getproc(&myproc, &kp[0]);
		kvm_close(kt);
	}
#elif defined (__APPLE__)
	{
		struct kinfo_proc	*kp;
		pid_t	mypid = getpid();
		size_t	cnt;
		int	err;

		kp = NULL;
		if ((err = GetBSDProcessList(mypid, &kp, &cnt)) != 0) {
			fprintf(stderr, "error getting proc list: %s\n", strerror(err));
			exit(3);
		}
		if (kp != NULL) {
			getproc(&myproc, kp);
			free(kp);
		}
	}
#else	/* SVR4 */
	{
		/*
		 * /proc/self has no useful pr_ttydev value on Open UNIX 8.
		 */
		 char	num[20];
		 pid_t	mypid = getpid();
		 snprintf(num, sizeof num, "%d", mypid);
		 getproc(num, &myproc, mypid, -1);
	}
#endif	/* SVR4 */
	if (c0 == NULL) {
#ifndef	UCB
		if (myproc.p_ttydev == (dev_type)PRNODEV) {
			fprintf(stderr,
				"%s: can't find controlling terminal\n",
				progname);
			exit(1);
		}
#endif	/* !UCB */
		add_criterion(CR_DEFAULT, 0);
	} else {
		struct criterion	*ct;
		int	valid = 0, invalid = 0;

		for (ct = c0; ct; ct = ct->c_nxt) {
			if (ct->c_typ == CR_INVALID_STOP)
				usage();
			else if (ct->c_typ == CR_EFF_UID)
				valid |= 01;
			else if (ct->c_typ == CR_INVALID_EFF_UID)
				invalid |= 01;
			else if (ct->c_typ == CR_REAL_UID)
				valid |= 02;
			else if (ct->c_typ == CR_INVALID_REAL_UID)
				invalid |= 02;
			else if (ct->c_typ == CR_REAL_GID)
				valid |= 04;
			else if (ct->c_typ == CR_INVALID_REAL_GID)
				invalid |= 04;
		}
		if ((invalid & valid) != invalid)
			return 1;
	}
	if (dohdr)
		putheader();
	do_procs();
	return errcnt;
}
