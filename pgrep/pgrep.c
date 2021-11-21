/*
 * pgrep, pkill - find or signal processes by name and other attributes
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
static const char sccsid[] USED = "@(#)pgrep.sl	1.24 (gritter) 1/12/07";

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>
#include	<alloca.h>
#include	<dirent.h>
#include	<limits.h>
#include	<pwd.h>
#include	<signal.h>
#include	<grp.h>
#include	<locale.h>
#include	<ctype.h>
#include	<regex.h>

#if !defined (__linux__) && !defined (__NetBSD__) && !defined (__OpenBSD__) \
	&& !defined (__APPLE__)
#if defined (__hpux)
#include	<sys/param.h>
#include	<sys/pstat.h>
#elif defined (_AIX)
#include	<procinfo.h>
#define	proc	process
#else	/* !__hpux, !_AIX */
#ifdef	sun
#define	_STRUCTURED_PROC	1
#endif	/* sun */
#include	<sys/procfs.h>
#endif	/* !__hpux, !_AIX */
#endif	/* !__linux__, !__NetBSD__, !__OpenBSD__ */

#if defined (__NetBSD__) || defined (__OpenBSD__) || defined (__APPLE__)
#include <kvm.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#if defined (__APPLE__)
#include <mach/mach_types.h>
#include <mach/task_info.h>
#endif /* __APPLE__ */
#define	proc	process
#undef	p_pgid
#define	p_pgid	p__pgid
#endif /* __NetBSD__, __OpenBSD__, __APPLE__ */

#ifndef	PRNODEV
#define	PRNODEV		0
#endif

#include	<blank.h>

#define	PROCDIR		"/proc"
#define	eq(a, b)	(strcmp(a, b) == 0)

enum	okay {
	OKAY,
	STOP
};

enum	valtype {
	VT_CHAR,
	VT_INT,
	VT_UINT,
	VT_LONG,
	VT_ULONG
};

union	value {
	char	v_char;
	int	v_int;
	unsigned int	v_uint;
	long	v_long;
	unsigned long	v_ulong;
};

struct	proc {
	struct proc	*p_nxt;		/* next proc structure */
	pid_t		p_pid;		/* process id */
	char		p_fname[19];	/* executable name */
	pid_t		p_ppid;		/* parent process id */
	pid_t		p_pgid;		/* process group id */
	pid_t		p_sid;		/* session id */
	int		p_ttydev;	/* controlling terminal */
	char		p_psargs[80];	/* process arguments */
	uid_t		p_uid;		/* real user id */
	uid_t		p_euid;		/* effective user id */
	gid_t		p_gid;		/* real group id */
	gid_t		p_egid;		/* effective group id */
	unsigned long	p_start;	/* start time (in jiffies except BSD) */
	unsigned long	p_size;		/* size in kilobytes */
	int		p_match;	/* matched this process */
};

enum	attype {
	ATT_PPID,			/* parent process id */
	ATT_PGRP,			/* process group id */
	ATT_SID,			/* sessiond id */
	ATT_EUID,			/* effective user id */
	ATT_UID,			/* real user id */
	ATT_GID,			/* real group id */
	ATT_TTY,			/* controlling terminal */
	ATT_ALL
};

struct	attrib {
	struct attrib	*a_nxt;		/* next element of list */
	enum	attype	a_type;		/* type of attribute */
	long		a_val;		/* value of attribute */
};

struct	attlist {
	struct attlist	*al_nxt;	/* next element of list */
	struct attrib	*al_att;	/* this attribute */
};

static const char	*progname;
static pid_t		mypid;		/* this instance's pid */
static unsigned		errcnt;		/* error count */
static int		matched;	/* a process matched */
static int		pkill;		/* this is the pkill command */
static int		fflag;		/* match against full command line */
static int		lflag;		/* long output format */
static int		nflag;		/* match newest process only */
static int		oflag;		/* match oldest process only */
static int		vflag;		/* reverse matching */
static int		xflag;		/* match exact string */
static int		signo = SIGTERM;	/* signal to send */
static int		need_euid_egid;	/* need euid or egid */
static struct attlist	*attributes;	/* required attributes */
static struct proc	*processes;	/* collected processes */
static regex_t		*expression;	/* regular expression to match */
static const char	*delimiter;	/* delimiter string */
static int		prdelim;	/* print a delimiter (not first proc) */

static int	str_2_sig(const char *, int *);

static void *
srealloc(void *vp, size_t nbytes)
{
	void	*p;

	if ((p = realloc(vp, nbytes)) == NULL) {
		write(2, "no memory\n", 10);
		exit(3);
	}
	return p;
}

static void *
smalloc(size_t nbytes)
{
	return srealloc(NULL, nbytes);
}

static void *
scalloc(size_t nmemb, size_t size)
{
	void	*p;

	if ((p = (void *)calloc(nmemb, size)) == NULL) {
		write(2, "no memory\n", 10);
		exit(3);
	}
	return p;
}

#if !defined (__hpux) && !defined (_AIX) && !defined (__NetBSD__) \
	&& !defined (__OpenBSD__) && !defined (__APPLE__)
static void
chdir_to_proc(void)
{
	static int	fd = -1;

	if (fd == -1 && (fd = open(PROCDIR, O_RDONLY)) < 0) {
		fprintf(stderr, "%s: cannot open %s\n", progname, PROCDIR);
		exit(3);
	}
	if (fchdir(fd) < 0) {
		fprintf(stderr, "%s: cannot chdir to %s\n", progname, PROCDIR);
		exit(3);
	}
}

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
		if ((separator==' ' ? isspace(**listp) : **listp == separator)
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
#endif	/* !__hpux, !_AIX, !__NetBSD__, !__OpenBSD__, !__APPLE__ */

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
static enum okay
getproc_stat(struct proc *p, pid_t expected_pid)
{
	static char	*buf;
	static size_t	buflen;
	union value	*v;
	FILE	*fp;
	char	*cp, *cq, *ce;
	size_t	sz, sc;

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
	GETVAL_REQ(VT_CHAR);
	if (v->v_char == 'Z')
		return STOP;
	GETVAL_REQ(VT_INT);
	p->p_ppid = v->v_int;
	GETVAL_REQ(VT_INT);
	p->p_pgid = v->v_int;
	GETVAL_REQ(VT_INT);
	p->p_sid = v->v_int;
	GETVAL_REQ(VT_INT);
	p->p_ttydev = v->v_int;
	GETVAL_REQ(VT_INT);
	/* tty_pgrp not used */
	GETVAL_REQ(VT_ULONG);
	/* flag not used */
	GETVAL_REQ(VT_ULONG);
	/* min_flt */
	GETVAL_REQ(VT_ULONG);
	/* cmin_flt */
	GETVAL_REQ(VT_ULONG);
	/* maj_flt */
	GETVAL_REQ(VT_ULONG);
	/* cmaj_flt */
	GETVAL_REQ(VT_ULONG);
	/* time */
	GETVAL_REQ(VT_ULONG);
	/* stime */
	GETVAL_REQ(VT_LONG);
	/* ctime */
	GETVAL_REQ(VT_LONG);
	/* cstime */
	GETVAL_REQ(VT_LONG);
	/* priority */
	GETVAL_REQ(VT_LONG);
	/* nice */
	GETVAL_REQ(VT_LONG);
	/* timeout not used */
	GETVAL_REQ(VT_LONG);
	/* it_real_value not used */
	GETVAL_REQ(VT_ULONG);
	p->p_start = v->v_ulong;
	GETVAL_REQ(VT_ULONG);
	p->p_size = (v->v_ulong >> 10);
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
		}
	}
	fclose(fp);
	if (scanr != 2)
		return STOP;
	return OKAY;
}

static struct proc *
getproc(const char *dir, pid_t expected_pid)
{
	struct proc	*p;
	enum okay	result;

	p = scalloc(1, sizeof *p);
	if (chdir(dir) == 0) {
		if ((result = getproc_stat(p, expected_pid)) == OKAY)
			if ((result = getproc_cmdline(p)) == OKAY)
				result = getproc_status(p);
		chdir_to_proc();
	} else
		result = STOP;
	if (result == STOP) {
		free(p);
		return NULL;
	}
	return p;
}

#elif defined (__FreeBSD__) || defined (__DragonFly__)

static enum okay
getproc_status(struct proc *p, pid_t expected_pid)
{
	static char	*buf;
	static size_t	buflen;
	union value	*v;
	FILE	*fp;
	char	*cp, *ce, *cq;
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
	while (*cp != ' ') {
		if (cp - buf < sizeof p->p_fname - 2)
			p->p_fname[cp-buf] = *cp;
		cp++;
	}
	if (cp - buf < sizeof p->p_fname - 1)
		p->p_fname[cp-buf] = '\0';
	else
		p->p_fname[sizeof p->p_fname - 1] = '\0';
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
	GETVAL_REQ(VT_LONG);
	/* skip microseconds */
	while (*cp != ' ') cp++; while (*cp == ' ') cp++;
	/* skip user time */
	while (*cp != ' ') cp++; while (*cp == ' ') cp++;
	/* skip system time */
	while (*cp != ' ') cp++; while (*cp == ' ') cp++;
	/* skip wchan message */
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

static struct proc *
getproc(const char *dir, pid_t expected_pid)
{
	struct proc	*p;
	enum okay	result;

	p = scalloc(1, sizeof *p);
	if (chdir(dir) == 0) {
		if ((result = getproc_status(p, expected_pid)) == OKAY)
				result = getproc_cmdline(p);
		chdir_to_proc();
	} else
		result = STOP;
	if (result == STOP) {
		free(p);
		return NULL;
	}
	return p;
}

#else	/* !__linux__, !__FreeBSD__, !__DragonFly__ */

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

static enum okay
getproc_psinfo(const char *dir, struct proc *p, pid_t expected_pid)
{
	FILE	*fp;
	struct psinfo	pi;

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
	p->p_ttydev = pi.pr_ttydev;
	strncpy(p->p_psargs, pi.pr_psargs, sizeof p->p_psargs);
	p->p_psargs[sizeof p->p_psargs - 1] = '\0';
	p->p_uid = pi.pr_uid;
	p->p_gid = pi.pr_gid;
#ifdef	__sun
	p->p_euid = pi.pr_euid;
	p->p_egid = pi.pr_egid;
#endif	/* __sun */
	p->p_start = pi.pr_start.tv_sec;
	p->p_size = pi.pr_size;
	return OKAY;
}

#ifndef	__sun
static enum okay
getproc_cred(const char *dir, struct proc *p)
{
	FILE	*fp;
	struct prcred	pc;

	if ((fp = fopen(concat(dir, "cred"), "r")) == NULL)
		return need_euid_egid ? STOP : OKAY;
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

static struct proc *
getproc(const char *dir, pid_t expected_pid)
{
	struct proc	*p;
	enum okay	result;

	p = scalloc(1, sizeof *p);
	result = getproc_psinfo(dir, p, expected_pid);
#ifndef	__sun
	if (result == OKAY)
		result = getproc_cred(dir, p);
#endif	/* !__sun */
	if (result == STOP) {
		free(p);
		return NULL;
	}
	return p;
}
#endif	/* !__linux__ */

static void
collectprocs(void)
{
	struct proc	*p, *pq = NULL;
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
			if ((p = getproc(dp->d_name, val)) != NULL) {
				if (pq)
					pq->p_nxt = p;
				else
					processes = p;
				pq = p;
			}
		}
		closedir(Dp);
	}
}

#elif defined (__hpux)
static void
collectprocs(void)
{
#define	burst	((size_t)10)
	struct proc	*p, *pq = NULL;
	struct pst_status	pst[burst];
	int	i, count;
	int	idx = 0;

	while ((count = pstat_getproc(pst, sizeof *pst, burst, idx)) > 0) {
		for (i = 0; i < count; i++) {
			p = scalloc(sizeof *p, 1);
			if (pq)
				pq->p_nxt = p;
			else
				processes = p;
			pq = p;
			p->p_pid = pst[i].pst_pid;
			strncpy(p->p_fname, pst[i].pst_ucomm,
					sizeof p->p_fname);
			p->p_fname[sizeof p->p_fname - 1] = '\0';
			p->p_ppid = pst[i].pst_ppid;
			p->p_pgid = pst[i].pst_pgrp;
			p->p_sid = pst[i].pst_sid;
			if (pst[i].pst_term.psd_major != -1 ||
					pst[i].pst_term.psd_minor != -1)
				p->p_ttydev = makedev(pst[i].pst_term.psd_major,
					pst[i].pst_term.psd_minor);
			strncpy(p->p_psargs, pst[i].pst_cmd,
					sizeof p->p_psargs);
			p->p_psargs[sizeof p->p_psargs - 1] = '\0';
			p->p_uid = pst[i].pst_uid;
			p->p_euid = pst[i].pst_euid;
			p->p_gid = pst[i].pst_gid;
			p->p_egid = pst[i].pst_egid;
			p->p_start = pst[i].pst_start;
			p->p_size = pst[i].pst_dsize + pst[i].pst_tsize +
				pst[i].pst_ssize;
		}
		idx = pst[count-1].pst_idx + 1;
	}
}
#elif defined (_AIX)
static void
oneproc(struct proc *p, struct procentry64 *pi)
{
	char	args[100], *ap, *cp;

	p->p_pid = pi->pi_pid;
	strncpy(p->p_fname, pi->pi_comm, sizeof p->p_fname);
	p->p_fname[sizeof p->p_fname - 1] = '\0';
	p->p_ppid = pi->pi_ppid;
	p->p_pgid = pi->pi_pgrp;
	p->p_sid = pi->pi_sid;
	p->p_ttydev = pi->pi_ttyp ? pi->pi_ttyd : PRNODEV;
	p->p_uid = pi->pi_uid;
	p->p_euid = pi->pi_cred.crx_uid;
	p->p_gid = pi->pi_cred.crx_rgid;
	p->p_egid = pi->pi_cred.crx_gid;
	p->p_start = pi->pi_start;
	p->p_size = pi->pi_size;
	if (getargs(pi, sizeof *pi, args, sizeof args) == 0) {
		ap = args;
		cp = p->p_psargs;
		while (cp < &p->p_psargs[sizeof p->p_psargs - 1]) {
			if (ap[0] == '\0') {
				if (ap[1] == '\0')
					break;
				*cp++ = ' ';
			} else
				*cp++ = *ap;
			ap++;
		}
		*cp = '\0';
	}
}

static void
collectprocs(void)
{
#define	burst	((size_t)10)
	struct proc	*p, *pq = NULL;
	struct procentry64	pi[burst];
	pid_t	idx = 0;
	int	i, count;

	while ((count = getprocs64(pi, sizeof *pi, NULL, 0, &idx, burst)) > 0) {
		for (i = 0; i < count; i++) {
			p = scalloc(sizeof *p, 1);
			if (pq)
				pq->p_nxt = p;
			else
				processes = p;
			pq = p;
			oneproc(p, &pi[i]);
		}
		if (count < burst)
			break;
	}
}
#elif defined (__OpenBSD__)
#include <uvm/uvm_extern.h>
static void
oneproc(struct proc *p, struct kinfo_proc *kp)
{
	p->p_pid = kp->kp_proc.p_pid;
	strncpy(p->p_fname, kp->kp_proc.p_comm, sizeof p->p_fname);
	p->p_fname[sizeof p->p_fname - 1] = '\0';
	p->p_ppid = kp->kp_eproc.e_ppid;
	p->p_pgid = kp->kp_eproc.e_pgid;
	p->p_sid = kp->kp_eproc.e_tpgid;	/* ? */
	p->p_ttydev = kp->kp_eproc.e_tdev;
	p->p_uid = kp->kp_eproc.e_pcred.p_ruid;
	p->p_euid = kp->kp_eproc.e_ucred.cr_uid;
	p->p_gid = kp->kp_eproc.e_pcred.p_rgid;
	p->p_egid = kp->kp_eproc.e_ucred.cr_gid;
	p->p_start = kp->kp_eproc.e_pstats.p_start.tv_sec;
	p->p_size = kp->kp_eproc.e_vm.vm_tsize +
		kp->kp_eproc.e_vm.vm_dsize +
		kp->kp_eproc.e_vm.vm_ssize;
}
static void
argproc(struct proc *p, struct kinfo_proc *kp, kvm_t *kt)
{
	char	**args;
	char	*ap, *pp;

	if ((args = kvm_getargv(kt, kp, sizeof p->p_psargs)) == NULL)
		return;
	ap = args[0];
	for (pp = p->p_psargs; pp < &p->p_psargs[sizeof p->p_psargs-1]; pp++) {
		if (*ap == '\0') {
			*pp = ' ';
			ap = *++args;
			if (ap == NULL)
				break;
		} else
			*pp = *ap++;
	}
}

static void
collectprocs(void)
{
	struct proc	*p, *pq = NULL;
	kvm_t	*kt;
	struct	kinfo_proc *kp;
	int	i, cnt;

	if ((kt = kvm_open(NULL, NULL, NULL, KVM_NO_FILES, "kvm_open")) == NULL)
		exit(1);
	kp = kvm_getprocs(kt, KERN_PROC_ALL, 0, &cnt);
	for (i = 0; i < cnt; i++) {
		p = scalloc(sizeof *p, 1);
		if (pq)
			pq->p_nxt = p;
		else
			processes = p;
		pq = p;
		oneproc(p, &kp[i]);
		argproc(p, &kp[i], kt);
	}
	kvm_close(kt);
}
#elif defined (__NetBSD__)
static void
oneproc(struct proc *p, struct kinfo_proc2 *kp)
{
	p->p_pid = kp->p_pid;
	strncpy(p->p_fname, kp->p_comm, sizeof p->p_fname);
	p->p_fname[sizeof p->p_fname - 1] = '\0';
	p->p_ppid = kp->p_ppid;
	p->p_pgid = kp->p__pgid;
	p->p_sid = kp->p_sid;
	p->p_ttydev = kp->p_tdev;
	p->p_uid = kp->p_ruid;
	p->p_euid = kp->p_uid;
	p->p_gid = kp->p_rgid;
	p->p_egid = kp->p_gid;
	p->p_start = kp->p_ustart_sec;
	p->p_size = kp->p_vm_tsize + kp->p_vm_dsize + kp->p_vm_ssize;
}

static void
argproc(struct proc *p, struct kinfo_proc2 *kp, kvm_t *kt)
{
	char	**args;
	char	*ap, *pp;

	if ((args = kvm_getargv2(kt, kp, sizeof p->p_psargs)) == NULL)
		return;
	ap = args[0];
	for (pp = p->p_psargs; pp < &p->p_psargs[sizeof p->p_psargs-1]; pp++) {
		if (*ap == '\0') {
			*pp = ' ';
			ap = *++args;
			if (ap == NULL)
				break;
		} else
			*pp = *ap++;
	}
}

static void
collectprocs(void)
{
	struct proc	*p, *pq = NULL;
	kvm_t	*kt;
	struct	kinfo_proc2 *kp;
	int	i, cnt;

	if ((kt = kvm_open(NULL, NULL, NULL, KVM_NO_FILES, "kvm_open")) == NULL)
		exit(1);
	kp = kvm_getproc2(kt, KERN_PROC_ALL, 0, sizeof *kp, &cnt);
	for (i = 0; i < cnt; i++) {
		p = scalloc(sizeof *p, 1);
		if (pq)
			pq->p_nxt = p;
		else
			processes = p;
		pq = p;
		oneproc(p, &kp[i]);
		argproc(p, &kp[i], kt);
	}
	kvm_close(kt);
}
#elif defined (__APPLE__)

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

extern	kern_return_t task_for_pid(task_port_t task, pid_t pid, task_port_t *target);

static void
oneproc(struct proc *p, struct kinfo_proc *kp)
{
	task_port_t	task;
	kern_return_t   error;
	struct		task_basic_info	task_binfo;
	unsigned int	info_count = TASK_BASIC_INFO_COUNT;

	p->p_pid = kp->kp_proc.p_pid;
	strncpy(p->p_fname, kp->kp_proc.p_comm, sizeof p->p_fname);
	p->p_fname[sizeof p->p_fname - 1] = '\0';
	p->p_ppid = kp->kp_eproc.e_ppid;
	p->p_pgid = kp->kp_eproc.e_pgid;
	p->p_sid = kp->kp_eproc.e_tpgid;
	p->p_ttydev = kp->kp_eproc.e_tdev == -1 ? PRNODEV : kp->kp_eproc.e_tdev;;
	p->p_uid = kp->kp_eproc.e_pcred.p_ruid;
	p->p_euid = kp->kp_eproc.e_ucred.cr_uid;
	p->p_gid = kp->kp_eproc.e_pcred.p_rgid;
	p->p_egid = kp->kp_eproc.e_ucred.cr_gid;
	p->p_start = kp->kp_proc.p_starttime.tv_sec +
		(kp->kp_proc.p_starttime.tv_usec >= 500000);

	error = task_for_pid(mach_task_self(), p->p_pid, &task);
	if (error != KERN_SUCCESS) {
		return; /* no process, nothing to show/kill */
	}

	info_count = TASK_BASIC_INFO_COUNT;
	error = task_info(task, TASK_BASIC_INFO, &task_binfo, &info_count);
	if (error != KERN_SUCCESS) {
		fprintf(stderr, "Error calling task_info():%d\n", error);
		exit(3);
	}

	p->p_size = task_binfo.virtual_size / 1024; /* in kilobytes */
}

static void
argproc(struct proc *p, struct kinfo_proc *kp)
{
	size_t	size, argsz;
	char	*argbuf;
	int	mib[3];
	long	nargs;
	char	*ap, *pp;

	/* allocate a procargs space per process */
	mib[0] = CTL_KERN;
	mib[1] = KERN_ARGMAX;
	size = sizeof argsz;
	if (sysctl(mib, 2, &argsz, &size, NULL, 0) == -1) {
		fprintf(stderr, "error in sysctl(): %s\n", strerror(errno));
		exit(3);
	}
	argbuf = (char *)smalloc(argsz);

	/* fetch the process arguments */
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROCARGS2;
	mib[2] = kp->kp_proc.p_pid;
	if (sysctl(mib, 3, argbuf, &argsz, NULL, 0) == -1)
		goto DONE; /* process has no args or already left the system */

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

	/* now concat copy the arguments */
	for (pp = p->p_psargs; pp < &p->p_psargs[sizeof p->p_psargs-1]; pp++) {
		if (*ap == '\0') {
			if (--nargs == 0)
				break;
			*pp = ' ';
			++ap;
		} else
			*pp = *ap++;
	}
	*pp = '\0';

DONE:	free(argbuf);
	return;
}

static void
collectprocs(void)
{
	int	mib[2];
	struct	proc *p, *pq = NULL;
	struct	kinfo_proc *kp = NULL;
	size_t	i, cnt;
	int	err;

	if ((err = GetBSDProcessList(0, &kp, &cnt)) != 0) {
		fprintf(stderr, "error getting proc list: %s\n", strerror(err));
		exit(3);
	}
	for (i = 0; i < cnt; i++) {
		p = smalloc(sizeof *p);
		if (pq)
			pq->p_nxt = p;
		else
			processes = p;
		pq = p;
		oneproc(p, &kp[i]);
		argproc(p, &kp[i]);
	}
	/* free the memory allocated by GetBSDProcessList */
	free(kp);	
}
#endif	/* all */

static enum okay
hasattr(struct proc *p, struct attrib *a)
{
	long	val = 0;

	switch (a->a_type) {
	case ATT_ALL:
		return OKAY;
	case ATT_PPID:
		val = p->p_ppid;
		break;
	case ATT_PGRP:
		val = p->p_pgid;
		break;
	case ATT_SID:
		val = p->p_sid;
		break;
	case ATT_EUID:
		val = p->p_euid;
		break;
	case ATT_UID:
		val = p->p_uid;
		break;
	case ATT_GID:
		val = p->p_gid;
		break;
	case ATT_TTY:
		/*
		 * Never matches processes without controlling tty.
		 */
		if (p->p_ttydev == PRNODEV)
			return STOP;
		val = p->p_ttydev;
		break;
	}
	return val == a->a_val ? OKAY : STOP;
}

static void
tryproc(struct proc *p)
{
	struct attlist	*alp;
	struct attrib	*ap;
	const char	*line;
	regmatch_t	where;

	for (alp = attributes; alp; alp = alp->al_nxt) {
		for (ap = alp->al_att; ap; ap = ap->a_nxt)
			if (hasattr(p, ap) == OKAY)
				break;
		if (ap == NULL)
			return;
	}
	if (expression) {
		line = fflag ? p->p_psargs : p->p_fname;
		if (regexec(expression, line, 1, &where, 0) != 0)
			return;
		if (xflag && (where.rm_so != 0 || where.rm_eo == -1 ||
					line[where.rm_eo] != '\0'))
			return;
	}
	p->p_match = 1;
}

static void
selectprocs(void)
{
	struct proc	*p;

	for (p = processes; p; p = p->p_nxt)
		tryproc(p);
}

static void
outproc(struct proc *p)
{
	if (pkill) {
		if (kill(p->p_pid, signo) < 0)
			fprintf(stderr,
				"%s: Failed to signal pid %ld: %s\n",
				progname, (long)p->p_pid, strerror(errno));
	} else {
		if (delimiter && prdelim++)
			printf("%s", delimiter);
		if (lflag)
			printf("%5ld %s", (long)p->p_pid,
					fflag ? p->p_psargs : p->p_fname);
		else
			printf("%ld", (long)p->p_pid);
		if (delimiter == NULL)
			printf("\n");
	}
}

static void
handleprocs(void)
{
	struct proc	*p, *selected = NULL;

	for (p = processes; p; p = p->p_nxt) {
		if (p->p_pid != mypid && p->p_match ^ vflag) {
			matched = 1;
			if (nflag) {
				if (selected == NULL ||
						p->p_start >= selected->p_start)
					selected = p;
			} else if (oflag) {
				if (selected == NULL ||
						p->p_start < selected->p_start)
					selected = p;
			} else
				outproc(p);
		}
	}
	if ((nflag || oflag) && selected)
		outproc(selected);
	if (prdelim && delimiter)
		printf("\n");
}

static long
getrdev(const char *device)
{
	struct stat	st;
	long	id = 0;
	char	*file;

	file = alloca(strlen(device) + 9);
	strcpy(file, "/dev/");
	strcpy(&file[5], device);
	if (stat(file, &st) < 0) {
		strcpy(file, "/dev/tty/");
		strcpy(&file[8], device);
		if (stat(file, &st) == 0)
			id = st.st_rdev;
		else {
			fprintf(stderr, "%s: unknown terminal name -- %s\n",
					progname, device);
			exit(2);
		}
	} else
		id = st.st_rdev;
	return id;
}

static struct attrib *
makatt(enum attype at, const char *string, int optc, struct attrib *aq)
{
	struct attrib	*ap;
	struct passwd	*pwd;
	struct group	*grp;
	char	*x;
	long	val = 0;

	if (*string == '\0')
		at = ATT_ALL;
	else switch (at) {
	case ATT_PPID:
	case ATT_PGRP:
	case ATT_SID:
		val = strtol(string, &x, 10);
		if (*x != '\0' || *string == '+' || *string == '-') {
			fprintf(stderr,
				"%s: invalid argument for option '%c' -- %s\n",
					progname, optc, string);
			exit(2);
		}
		if (val == 0) switch (at) {
		case ATT_PGRP:
			val = getpgid(0);
			break;
		case ATT_SID:
			val = getsid(0);
			break;
		}
		break;
	case ATT_EUID:
		need_euid_egid = 1;
		/*FALLTHRU*/
	case ATT_UID:
		if ((pwd = getpwnam(string)) != NULL)
			val = pwd->pw_uid;
		else {
			val = strtol(string, &x, 10);
			if (*x != '\0' || *string == '+' || *string == '-') {
				fprintf(stderr,
					"%s: invalid user name -- %s\n",
						progname, string);
				exit(2);
			}
		}
		break;
	case ATT_GID:
		if ((grp = getgrnam(string)) != NULL)
			val = grp->gr_gid;
		else {
			val = strtol(string, &x, 10);
			if (*x != '\0' || *string == '+' || *string == '-') {
				fprintf(stderr,
					"%s: invalid group name -- %s\n",
						progname, string);
				exit(2);
			}
		}
		break;
	case ATT_TTY:
		val = getrdev(string);
		break;
	}
	ap = scalloc(1, sizeof *ap);
	ap->a_type = at;
	ap->a_val = val;
	ap->a_nxt = aq;
	return ap;
}

static void
addattribs(enum attype at, const char *list, int optc)
{
	struct attlist	*al = NULL;
	const char	*cp;

	for (al = attributes; al; al = al->al_nxt)
		if (al->al_att && al->al_att->a_type == at)
			break;
	if (al == NULL) {
		al = scalloc(1, sizeof *al);
		al->al_nxt = attributes;
		attributes = al;
	}
	while (*list == ',' || isblank(*list&0377))
		list++;
	if (*list)
		while ((cp = element(&list, '\0')) != NULL)
			al->al_att = makatt(at, cp, optc, al->al_att);
	else
		al->al_att = makatt(at, "", optc, al->al_att);
}

static enum okay
getsig(const char *str)
{
	char	*x;
	int	val;

	if ((val = strtol(str, &x, 10)) >= 0 && *x == '\0' &&
			*str != '-' && *str != '+') {
		signo = val;
		return OKAY;
	}
	if (str_2_sig(str, &val) == OKAY) {
		signo = val;
		return OKAY;
	}
	return STOP;
}

static void
usage(void)
{
	if (pkill)
		fprintf(stderr, "\
Usage: %s [-signal] [-fnovx] [-P ppidlist] [-g pgrplist] [-s sidlist]\n\
\t[-u euidlist] [-U uidlist] [-G gidlist] [-t termlist] [pattern]\n",
			progname);
	else
		fprintf(stderr, "\
Usage: %s [-flnovx] [-d delim] [-P ppidlist] [-g pgrplist] [-s sidlist]\n\
\t[-u euidlist] [-U uidlist] [-G gidlist] [-t termlist] [pattern]\n",
			progname);
	exit(2);
}

int
main(int argc, char **argv)
{
	int	i, flags;

	progname = basename(argv[0]);
	if (strncmp(progname, "pkill", 5) == 0)
		pkill = 1;
	setlocale(LC_COLLATE, "");
	setlocale(LC_CTYPE, "");
	if (pkill && argc > 1 && argv[1][0] == '-' &&
			getsig(&argv[1][1]) == OKAY)
		optind = 2;
	while ((i = getopt(argc, argv, pkill ? "fnovxP:g:s:u:U:G:t:" :
					"flnovxd:P:g:s:u:U:G:t:")) != EOF) {
		switch (i) {
		case 'f':
			fflag = 1;
			break;
		case 'l':
			lflag = 1;
			break;
		case 'n':
			nflag = 1;
			break;
		case 'o':
			oflag = 1;
			break;
		case 'v':
			vflag = 1;
			break;
		case 'x':
			xflag = 1;
			break;
		case 'd':
			delimiter = optarg;
			break;
		case 'P':
			addattribs(ATT_PPID, optarg, i);
			break;
		case 'g':
			addattribs(ATT_PGRP, optarg, i);
			break;
		case 's':
			addattribs(ATT_SID, optarg, i);
			break;
		case 'u':
			addattribs(ATT_EUID, optarg, i);
			break;
		case 'U':
			addattribs(ATT_UID, optarg, i);
			break;
		case 'G':
			addattribs(ATT_GID, optarg, i);
			break;
		case 't':
			addattribs(ATT_TTY, optarg, i);
			break;
		default:
			usage();
		}
	}
	if (nflag && oflag) {
		fprintf(stderr, "%s: -n and -o are mutually exclusive\n",
				progname);
		usage();
	}
	if (argv[optind]) {
		if (argv[optind+1]) {
			fprintf(stderr, "%s: illegal argument -- %s\n",
					progname, argv[optind + 1]);
			usage();
		}
		flags = REG_EXTENDED;
#ifdef	REG_MTPARENBAD
		flags |= REG_MTPARENBAD;
#endif
		if (!xflag)
			flags |= REG_NOSUB;
#ifdef	REG_ONESUB
		else
			flags |= REG_ONESUB;
#endif
		expression = scalloc(1, sizeof *expression);
		if ((i = regcomp(expression, argv[optind], flags)) != 0) {
			char	*errst;
			size_t	errsz;

			errsz = regerror(i, expression, NULL, 0) + 1;
			errst = smalloc(errsz);
			regerror(i, expression, errst, errsz);
			fprintf(stderr, "%s: %s\n", progname, errst);
			exit(2);
		}
	} else if (attributes == NULL) {
		fprintf(stderr, "%s: No matching criteria specified\n",
				progname);
		usage();
	}
	mypid = getpid();
#if !defined (__hpux) && !defined (_AIX) && !defined (__NetBSD__) && \
		!defined (__OpenBSD__) && !defined (__APPLE__)
	chdir_to_proc();
#endif	/* !__hpux, !_AIX, !__NetBSD__, !__OpenBSD__, !__APPLE__ */
	collectprocs();
	selectprocs();
	handleprocs();
	return errcnt ? errcnt : matched == 0;
}

struct sig_strlist
{
	const int	sig_num;
	const char	*sig_str;
};

static const struct sig_strlist sig_strs[] = {
	{ 0,		"EXIT"	},
	{ SIGHUP,	"HUP"	},
	{ SIGINT,	"INT"	},
	{ SIGQUIT,	"QUIT"	},
	{ SIGILL,	"ILL"	},
	{ SIGTRAP,	"TRAP"	},
	{ SIGABRT,	"ABRT"	},
#ifdef	SIGIOT
	{ SIGIOT,	"IOT"	},
#endif
#ifdef	SIGEMT
	{ SIGEMT,	"EMT"	},
#endif
#ifdef	SIGFPE
	{ SIGFPE,	"FPE"	},
#endif
#ifdef	SIGKILL
	{ SIGKILL,	"KILL"	},
#endif
#ifdef	SIGBUS
	{ SIGBUS,	"BUS"	},
#endif
#ifdef	SIGSEGV
	{ SIGSEGV,	"SEGV"	},
#endif
#ifdef	SIGSYS
	{ SIGSYS,	"SYS"	},
#endif
#ifdef	SIGPIPE
	{ SIGPIPE,	"PIPE"	},
#endif
#ifdef	SIGALRM
	{ SIGALRM,	"ALRM"	},
#endif
#ifdef	SIGTERM
	{ SIGTERM,	"TERM"	},
#endif
#ifdef	SIGUSR1
	{ SIGUSR1,	"USR1"	},
#endif
#ifdef	SIGUSR2
	{ SIGUSR2,	"USR2"	},
#endif
#ifdef	SIGCLD
	{ SIGCLD,	"CLD"	},
#endif
#ifdef	SIGCHLD
	{ SIGCHLD,	"CHLD"	},
#endif
#ifdef	SIGPWR
	{ SIGPWR,	"PWR"	},
#endif
#ifdef	SIGWINCH
	{ SIGWINCH,	"WINCH"	},
#endif
#ifdef	SIGURG
	{ SIGURG,	"URG"	},
#endif
#ifdef	SIGPOLL
	{ SIGPOLL,	"POLL"	},
#endif
#ifdef	SIGIO
	{ SIGIO,	"IO"	},
#endif
#ifdef	SIGSTOP
	{ SIGSTOP,	"STOP"	},
#endif
#ifdef	SIGTSTP
	{ SIGTSTP,	"TSTP"	},
#endif
#ifdef	SIGCONT
	{ SIGCONT,	"CONT"	},
#endif
#ifdef	SIGTTIN
	{ SIGTTIN,	"TTIN"	},
#endif
#ifdef	SIGTTOU
	{ SIGTTOU,	"TTOU"	},
#endif
#ifdef	SIGVTALRM
	{ SIGVTALRM,	"VTALRM"	},
#endif
#ifdef	SIGPROF
	{ SIGPROF,	"PROF"	},
#endif
#ifdef	SIGXCPU
	{ SIGXCPU,	"XCPU"	},
#endif
#ifdef	SIGXFSZ
	{ SIGXFSZ,	"XFSZ"	},
#endif
#ifdef	SIGWAITING
	{ SIGWAITING,	"WAITING"	},
#endif
#ifdef	SIGLWP
	{ SIGLWP,	"LWP"	},
#endif
#ifdef	SIGFREEZE
	{ SIGFREEZE,	"FREEZE"	},
#endif
#ifdef	SIGTHAW
	{ SIGTHAW,	"THAW"	},
#endif
#ifdef	SIGCANCEL
	{ SIGCANCEL,	"CANCEL"	},
#endif
#ifdef	SIGLOST
	{ SIGLOST,	"LOST"	},
#endif
#ifdef	SIGSTKFLT
	{ SIGSTKFLT,	"STKFLT"	},
#endif
#ifdef	SIGINFO
	{ SIGINFO,	"INFO"	},
#endif
#ifdef	SIG_2_STR_WITH_RT_SIGNALS
	{ SIGRTMIN,	"RTMIN"	},
	{ SIGRTMIN+1,	"RTMIN+1"	},
	{ SIGRTMIN+2,	"RTMIN+2"	},
	{ SIGRTMIN+3,	"RTMIN+3"	},
	{ SIGRTMAX-3,	"RTMAX-3"	},
	{ SIGRTMAX-2,	"RTMAX-2"	},
	{ SIGRTMAX-1,	"RTMAX-1"	},
	{ SIGRTMAX,	"RTMAX"	},
#endif	/* SIG_2_STR_WITH_RT_SIGNALS */
	{ -1,		NULL	}
};

static int
str_2_sig(const char *str, int *signum)
{
	int	i;

	for (i = 0; sig_strs[i].sig_str; i++)
		if (eq(str, sig_strs[i].sig_str))
			break;
	if (sig_strs[i].sig_str == NULL)
		return STOP;
	*signum = sig_strs[i].sig_num;
	return OKAY;
}
