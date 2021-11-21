/*
 * priocntl - process scheduler control
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
#if defined (__linux__) || defined (__FreeBSD__)

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
#if defined (S42)
static const char sccsid[] USED = "@(#)priocntl_s42.sl	1.19 (gritter) 7/23/06";
#else
static const char sccsid[] USED = "@(#)priocntl.sl	1.19 (gritter) 7/23/06";
#endif

#include	<sys/time.h>
#include	<sys/resource.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<sched.h>
#include	<libgen.h>
#include	<dirent.h>
#include	<ctype.h>
#include	<alloca.h>

#ifndef	SCHED_BATCH
#define	SCHED_BATCH	3
#endif
#ifndef	SCHED_ISO
#define	SCHED_ISO	4
#endif

#define	eq(p, q)	(strcmp(p, q) == 0)

struct	idnode {
	struct idnode	*id_nxt;
	int	id_val;
	int	id_upri;
	int	id_class;
	unsigned long	id_tqntm;
};

struct	proc {
	int	p_pid;
	int	p_ppid;
	int	p_pgid;
	int	p_sid;
	int	p_class;
	int	p_uid;
	int	p_gid;
	int	p_upri;
	unsigned long	p_tqntm;
};

static enum {
	ID_PID,
	ID_PPID,
	ID_PGID,
	ID_SID,
	ID_CLASS,
	ID_UID,
	ID_GID,
	ID_ALL
} idtype = ID_PID;

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

static unsigned		errcnt;		/* count of errors */
static int		action;		/* one of l, d, s, e */
static int		cflag = -1;	/* selected class */
static int		iflag;		/* an id was specified */
static int		pflag = -1;	/* numeric argument to -p option */
static char		*progname;	/* argv[0] to main() */
static struct idnode	*idlist;	/* list of id criteria */
static struct idnode	*proclist;	/* list of processes */

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
		if ((separator == ' ' ? isspace(**listp) :
					**listp == separator) ||
				**listp == sep2)
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

static void *
srealloc(char *op, size_t size)
{
	char	*np;

	if ((np = realloc(op, size)) == NULL) {
		write(2, "Out of memory\n", 14);
		exit(077);
	}
	return np;
}

static void *
scalloc(size_t nmemb, size_t size)
{
	void	*p;

	if ((p = calloc(nmemb, size)) == NULL) {
		write(2, "Out of memory\n", 14);
		exit(077);
	}
	return p;
}

static void
usage(void)
{
	fprintf(stderr, "usage:\t%s -l\n\
\t%s -d [-i idtype] [idlist]\n\
\t%s -s [-c class] [c.s.o.] [-i idtype] [idlist]\n\
\t%s -e [-c class] [c.s.o.] command [argument(s)]\n",
	progname, progname, progname, progname);
	exit(2);
}

static void
addproc(struct proc *sp)
{
	struct idnode	*id, *iq;

	id = calloc(1, sizeof *id);
	id->id_val = sp->p_pid;
	id->id_upri = sp->p_upri;
	id->id_class = sp->p_class;
	id->id_tqntm = sp->p_tqntm;
	if (proclist) {
		for (iq = proclist; iq->id_nxt; iq = iq->id_nxt);
		iq->id_nxt = id;
	} else
		proclist = id;
}

static void
tryproc(int val, struct proc *sp)
{
	struct idnode	*id;

	if (sp->p_pid == 1 && action == 's' && idtype != ID_PID)
		return;
	for (id = idlist; id; id = id->id_nxt) {
		if (id->id_val == val) {
			addproc(sp);
			return;
		}
	}
}

#define	GETVAL(a)	if ((v = getval(&cp, (a), ' ', 0)) == NULL) { \
				free(sp); \
				return NULL; \
			}

#define	GETVAL_COMMA(a)	if ((v = getval(&cp, (a), ' ', ',')) == NULL) { \
				free(sp); \
				return NULL; \
			}

#if defined (__linux__)
static struct proc *
getproc(int pid)
{
	struct proc	*sp;
	static char	*buf;
	static size_t	buflen;
	char	fn[256];
	char	line[2049];
	FILE	*fp;
	union value	*v;
	char	*cp, *cq, *ce;
	size_t	sz, sc;
	int	c;

	sp = scalloc(1, sizeof *sp);
	snprintf(fn, sizeof fn, "/proc/%d/stat", pid);
	if ((fp = fopen(fn, "r")) == NULL) {
		free(sp);
		return NULL;
	}
	for (cp = buf; ; ) {
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
	if (*ce != '\n') {
		free(sp);
		return NULL;
	}
	*ce-- = '\0';
	cp = buf;
	GETVAL(VT_INT);
	sp->p_pid = v->v_int;
	if (*cp++ != '(') {
		free(sp);
		return NULL;
	}
	for (cq = ce; cq >= cp && *cq != ')'; cq--);
	if (cq < cp) {
		free(sp);
		return NULL;
	}
	*cq = '\0';
	cp = &cq[1];
	while (isspace(*cp))
		cp++;
	GETVAL(VT_CHAR);	/* state */
	if (v->v_char == 'Z') {
		free(sp);
		return NULL;
	}
	GETVAL(VT_INT);
	sp->p_ppid = v->v_int;
	GETVAL(VT_INT);
	sp->p_pgid = v->v_int;
	GETVAL(VT_INT);
	sp->p_sid = v->v_int;
	sp->p_class = sched_getscheduler(pid);
	if (sp->p_class == SCHED_OTHER || sp->p_class == SCHED_BATCH ||
			sp->p_class == SCHED_ISO) {
		GETVAL(VT_INT);		/* device */
		GETVAL(VT_INT);		/* tty_pgrp */
		GETVAL(VT_ULONG);	/* flags */
		GETVAL(VT_ULONG);	/* min_flt */
		GETVAL(VT_ULONG);	/* cmin_flt */
		GETVAL(VT_ULONG);	/* maj_flt */
		GETVAL(VT_ULONG);	/* cmaj_flt */
		GETVAL(VT_ULONG);	/* utime */
		GETVAL(VT_ULONG);	/* stime */
		GETVAL(VT_ULONG);	/* cutime */
		GETVAL(VT_ULONG);	/* cstime */
		GETVAL(VT_LONG);	/* internal priority */
		GETVAL(VT_LONG);	/* nice */
		sp->p_upri = 0 - v->v_long;
	} else {
		struct sched_param	scp;
		sched_getparam(pid, &scp);
		sp->p_upri = scp.sched_priority;
		if (sp->p_class == SCHED_RR) {
			struct timespec	ts;
			if (sched_rr_get_interval(pid, &ts) == 0) {
				sp->p_tqntm = ts.tv_sec * 1000;
				sp->p_tqntm += ts.tv_nsec / 1000000;
			} else
				sp->p_tqntm = 150;
		}
	}
	snprintf(fn, sizeof fn, "/proc/%d/status", pid);
	if ((fp = fopen(fn, "r")) == NULL) {
		free(sp);
		return NULL;
	}
	c = 0;
	while (fgets(line, sizeof line, fp) != NULL) {
		if (strncmp(line, "Uid:", 4) == 0) {
			cp = &line[4];
			while (isspace(*cp))
				cp++;
			if ((v = getval(&cp, VT_INT, ' ', 0)) == NULL) {
				fclose(fp);
				free(sp);
				return NULL;
			}
			if ((v = getval(&cp, VT_INT, ' ', 0)) == NULL) {
				fclose(fp);
				free(sp);
				return NULL;
			}
			sp->p_uid = v->v_int;
			c++;
		}
		if (strncmp(line, "Gid:", 4) == 0) {
			cp = &line[4];
			while (isspace(*cp))
				cp++;
			if ((v = getval(&cp, VT_INT, ' ', 0)) == NULL) {
				fclose(fp);
				free(sp);
				return NULL;
			}
			if ((v = getval(&cp, VT_INT, ' ', 0)) == NULL) {
				fclose(fp);
				free(sp);
				return NULL;
			}
			sp->p_gid = v->v_int;
			c++;
		}
	}
	fclose(fp);
	if (c != 2) {
		free(sp);
		return NULL;
	}
	return sp;
}

#elif defined (__FreeBSD__)

#define	SKIPFIELD	while (*cp != ' ') cp++; while (*cp == ' ') cp++;

static struct proc *
getproc(int pid)
{
	struct proc	*sp;
	static char	*buf;
	static size_t	buflen;
	struct sched_param	s;
	struct timespec	ts;
	char	fn[256];
	FILE	*fp;
	union value	*v;
	char	*cp, *ce;
	size_t	sz, sc;

	sp = scalloc(1, sizeof *sp);
	snprintf(fn, sizeof fn, "/proc/%d/status", pid);
	if ((fp = fopen(fn, "r")) == NULL) {
		free(sp);
		return NULL;
	}
	for (cp = buf; ; ) {
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
	if (*ce != '\n') {
		free(sp);
		return NULL;
	}
	*ce-- = '\0';
	cp = buf;
	SKIPFIELD;	/* name */
	GETVAL(VT_INT);
	sp->p_pid = v->v_int;
	GETVAL(VT_INT);
	sp->p_ppid = v->v_int;
	GETVAL(VT_INT);
	sp->p_pgid = v->v_int;
	GETVAL(VT_INT);
	sp->p_sid = v->v_int;
	SKIPFIELD;	/* flags */
	SKIPFIELD;	/* start */
	SKIPFIELD;	/* user time */
	SKIPFIELD;	/* system time */
	SKIPFIELD;	/* wchan */
	SKIPFIELD;	/* euid */
	GETVAL(VT_INT);
	sp->p_uid = v->v_int;
	GETVAL_COMMA(VT_INT);
	sp->p_gid = v->v_int;
	switch (sp->p_class = sched_getscheduler(sp->p_pid)) {
	case SCHED_OTHER:
		sp->p_upri = 0 - getpriority(PRIO_PROCESS, sp->p_pid);
		break;
	case SCHED_RR:
		if (sched_rr_get_interval(pid, &ts) == 0) {
			sp->p_tqntm = ts.tv_sec * 1000;
			sp->p_tqntm += ts.tv_nsec / 1000000;
		} else
			sp->p_tqntm = 150;
		/*FALLTHRU*/
	case SCHED_FIFO:
		if (sched_getparam(sp->p_pid, &s) == 0) {
			sp->p_upri = s.sched_priority;
			break;
		}
		/*FALLTHRU*/
	default:
		free(sp);
		return NULL;
	}
	return sp;
}
#endif	/* __FreeBSD__ */

static void
evalproc(int pid)
{
	struct proc	*sp;

	if ((sp = getproc(pid)) == NULL)
		return;
	switch (idtype) {
	case ID_PID:
		tryproc(sp->p_pid, sp);
		break;
	case ID_PPID:
		tryproc(sp->p_ppid, sp);
		break;
	case ID_PGID:
		tryproc(sp->p_pgid, sp);
		break;
	case ID_SID:
		tryproc(sp->p_sid, sp);
		break;
	case ID_CLASS:
		tryproc(sp->p_class, sp);
		break;
	case ID_UID:
		tryproc(sp->p_uid, sp);
		break;
	case ID_GID:
		tryproc(sp->p_gid, sp);
		break;
	case ID_ALL:
		addproc(sp);
	}
	free(sp);
}

static void
getprocs(void)
{
	DIR	*Dp;
	struct dirent	*dp;

	if ((Dp = opendir("/proc")) == NULL) {
		fprintf(stderr, "%s: cannot open /proc, errno %d\n",
				progname, errno);
		exit(1);
	}
	while ((dp = readdir(Dp)) != NULL) {
		char	*x;
		int	pid;

		if ((pid = strtol(dp->d_name, &x, 10)) >= 0 && *x == '\0')
			evalproc(pid);
	}
	closedir(Dp);
}

static void
getidlist(char **args)
{
	if (idtype == ID_ALL)
		return;
	if (args == NULL || *args == NULL) {
		if (iflag) {
			idlist = scalloc(1, sizeof *idlist);
			switch (idtype) {
			case ID_PID:
				idlist->id_val = getpid();
				break;
			case ID_PPID:
				idlist->id_val = getppid();
				break;
			case ID_PGID:
				idlist->id_val = getpgid(0);
				break;
			case ID_SID:
				idlist->id_val = getsid(0);
				break;
			case ID_CLASS:
				idlist->id_val = sched_getscheduler(0);
				break;
			case ID_UID:
				idlist->id_val = geteuid();
				break;
			case ID_GID:
				idlist->id_val = getegid();
				break;
			}
			return;
		} else
			usage();
	}
	do {
		struct idnode	*id;

		id = scalloc(1, sizeof *id);
		if (idtype == ID_CLASS) {
			if (eq(args[0], "TS"))
				id->id_val = SCHED_OTHER;
			else if (eq(args[0], "B"))
				id->id_val = SCHED_BATCH;
			else if (eq(args[0], "BA"))	/* old */
				id->id_val = SCHED_BATCH;
			else if (eq(args[0], "ISO"))
				id->id_val = SCHED_ISO;
			else if (eq(args[0], "IS"))	/* old */
				id->id_val = SCHED_ISO;
			else if (eq(args[0], "FF"))
				id->id_val = SCHED_FIFO;
			else if (eq(args[0], "FI"))	/* old */
				id->id_val = SCHED_FIFO;
			else if (eq(args[0], "RT") || eq(args[0], "FP"))
				id->id_val = SCHED_RR;
			else {
				fprintf(stderr, "%s: Invalid or unconfigured "
						"class %s in idlist - "
						"ignored\n",
						progname, args[0]);
				free(id);
				continue;
			}
		} else
			id->id_val = atoi(args[0]);
		id->id_nxt = idlist;
		idlist = id;
	} while (*++args != NULL);
}

static void
noprocs(void)
{
	fprintf(stderr, "%s: Process(es) not found.\n", progname);
	exit(1);
}

static void
getproclist(char **args)
{
	getidlist(args);
	getprocs();
	if (proclist == NULL)
		noprocs();
}

static void
listclasses(void)
{
	const char	*batchclass;
	const char	*isoclass;

	batchclass = sched_get_priority_max(SCHED_BATCH) < 0 ? "" : "\
B (Batch)\n\
\tConfigured BA User Priority Range: -19 through 20\n\
\n";
	isoclass = sched_get_priority_max(SCHED_ISO) < 0 ? "" : "\
ISO (Isochronous)\n\
\tConfigured IS User Priority Range: -19 through 20\n\
\n";
	fprintf(stderr, "\
CONFIGURED CLASSES\n\
==================\n\
\n\
TS (Time Sharing)\n\
\tConfigured TS User Priority Range: -19 through 20\n\
\n"
"%s%s"
#ifndef	S42
"\
RT (Real Time Round Robin)\n\
\tMaximum Configured RT Priority: 99\n\
\n"
#else	/* S42 */
"\
FP (Real Time Round Robin)\n\
\tMaximum Configured FP Priority: 99\n\
\n"
#endif	/* S42 */
"FF (Real Time First In-First Out)\n\
\tMaximum Configured FI Priority: 99\n\
", batchclass, isoclass);
}

static void
disp_this(int class, const char *title, const char *head)
{
	struct idnode	*id;

	for (id = proclist; id; id = id->id_nxt)
		if (id->id_class == class)
			break;
	if (id == NULL)
		return;
	printf("%s\n    PID    %s", title, head);
	if (class == SCHED_RR)
		printf("       TQNTM");
	putchar('\n');
	for (id = proclist; id; id = id->id_nxt) {
		if (id->id_class == class) {
			printf("%7d %7d", id->id_val, id->id_upri);
			if (class == SCHED_RR)
				printf(" %11lu", id->id_tqntm);
			putchar('\n');
		}
	}
}

static void
display(void)
{
	disp_this(SCHED_OTHER, "TIME SHARING PROCESSES", "TSUPRI");
	if (sched_get_priority_max(SCHED_BATCH) >= 0)
		disp_this(SCHED_BATCH, "BATCH PROCESSES", "BAUPRI");
	if (sched_get_priority_max(SCHED_ISO) >= 0)
		disp_this(SCHED_ISO, "ISOCHRONOUS PROCESSES", "ISUPRI");
#ifndef	S42
	disp_this(SCHED_RR, "REAL TIME PROCESSES", "RTPRI");
#else	/* S42 */
	disp_this(SCHED_RR, "FIXED PRIORITY PROCESSES", "FPPRI");
#endif	/* S42 */
	disp_this(SCHED_FIFO, "FIFO CLASS PROCESSES", "FIPRI");
}

static int
set_this(int pid, int defclass)
{
	struct sched_param	scp;
	int	class, upri;

	memset(&scp, 0, sizeof scp);
	if (cflag != -1)
		class = cflag;
	else
		class = defclass;
	if (pflag != -1) {
		if (class != SCHED_BATCH)
			scp.sched_priority = pflag;
		else
			scp.sched_priority = 0;
		upri = pflag;
	} else {
		sched_getparam(pid, &scp);
		if (scp.sched_priority == 0 && class != SCHED_BATCH &&
				class != SCHED_ISO)
			scp.sched_priority = 1;
		upri = 0 - getpriority(PRIO_PROCESS, pid);
	}
	errno = 0;
	if ((class == SCHED_RR || class == SCHED_FIFO ||
				class == SCHED_ISO ||
				class == SCHED_BATCH) &&
			sched_setscheduler(pid, class, &scp) < 0)
		return 1;
	if ((class == SCHED_OTHER || class == SCHED_BATCH ||
				class == SCHED_ISO) &&
			setpriority(PRIO_PROCESS, pid, 0 - upri) < 0)
		return 2;
	return 0;
}

static void
set(void)
{
	struct idnode	*id;
	int	cnt = 0;

	if (cflag == -1) {
		for (id = proclist; id; id = id->id_nxt) {
			if (id->id_class != proclist->id_class) {
				fprintf(stderr, "%s: Specified processes from "
						"different classes.\n",
					progname);
				exit(1);
			}
		}
	}
	for (id = proclist; id; id = id->id_nxt) {
		if (set_this(id->id_val, id->id_class) != 0) {
			switch (errno) {
			case EPERM:
				errcnt |= 1;
				cnt++;
				fprintf(stderr, "Permissions error encountered "
						"on pid %d.\n", id->id_val);
				break;
			case ESRCH:
				/*
				 * Do nothing as this just means that the
				 * process has terminated after we read its
				 * data before.
				 */
				break;
			case EINVAL:
				errcnt |= 1;
				cnt++;
				fprintf(stderr, "%s: Invalid argument on "
						"pid %d.\n",
					progname, id->id_val);
				break;
			default:
				errcnt |= 1;
				cnt++;
				fprintf(stderr, "%s: pid %d: errno %d\n",
					progname, id->id_val, errno);
			}
		} else
			cnt++;
	}
	if (cnt == 0)
		noprocs();
}

static void
execute(char **args)
{
	struct proc	*sp;
	int	defclass;
	const char	*fcall = NULL;

	if (args == NULL || *args == NULL)
		usage();
	if ((sp = getproc(getpid())) != NULL)
		defclass = sp->p_class;
	else
		defclass = SCHED_OTHER;
	switch (set_this(0, defclass)) {
	case 0:
		break;
	case 1:
		if (fcall == NULL)
			fcall = "sched_setparam";
		/*FALLTHRU*/
	case 2:
		if (fcall == NULL)
			fcall = "setpriority";
		/*FALLTHRU*/
	default:
		if (fcall == NULL)
			fcall = "unknown";
		fprintf(stderr, "%s: Can't set scheduling parameters\n"
				"%s system call failed with errno %d\n",
			progname, fcall, errno);
		exit(1);
	}
	execvp(args[0], args);
	fprintf(stderr, "%s: Can't execute %s, exec failed with errno %d\n",
			progname, args[0], errno);
	exit(1);
}

static void
selclass(const char *s)
{
	if (eq(s, "TS"))
		cflag = SCHED_OTHER;
	else if (eq(s, "B"))
		cflag = SCHED_BATCH;
	else if (eq(s, "BA"))	/* old */
		cflag = SCHED_BATCH;
	else if (eq(s, "ISO"))
		cflag = SCHED_ISO;
	else if (eq(s, "IS"))	/* old */
		cflag = SCHED_ISO;
	else if (eq(s, "FF"))
		cflag = SCHED_FIFO;
	else if (eq(s, "FI"))	/* old */
		cflag = SCHED_FIFO;
	else if (eq(s, "RT") || eq(s, "FP"))
		cflag = SCHED_RR;
	else {
		fprintf(stderr, "%s: Invalid or unconfigured class %s\n",
				progname, s);
		exit(2);
	}
}

static void
selid(const char *s)
{
	if (eq(s, "pid"))
		idtype = ID_PID;
	else if (eq(s, "ppid"))
		idtype = ID_PPID;
	else if (eq(s, "pgid"))
		idtype = ID_PGID;
	else if (eq(s, "sid"))
		idtype = ID_SID;
	else if (eq(s, "class"))
		idtype = ID_CLASS;
	else if (eq(s, "uid"))
		idtype = ID_UID;
	else if (eq(s, "gid"))
		idtype = ID_GID;
	else if (eq(s, "all"))
		idtype = ID_ALL;
	else {
		fprintf(stderr, "%s: bad idtype %s\n", progname, s);
		exit(2);
	}
}

int
main(int argc, char **argv)
{
	const char	optstring[] = "ldi:sc:ep:";
	int	i;

#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	progname = basename(argv[0]);
	while ((i = getopt(argc, argv, optstring)) != EOF) {
		switch (i) {
		case 'l':
		case 'd':
		case 's':
		case 'e':
			if (action)
				usage();
			action = i;
			break;
		case 'c':
			selclass(optarg);
			break;
		case 'p':
			pflag = atoi(optarg);
			break;
		case 'i':
			iflag = 1;
			selid(optarg);
			break;
		default:
			usage();
		}
	}
	switch (action) {
	case 'l':
		if (iflag || cflag != -1 || pflag != -1)
			usage();
		listclasses();
		break;
	case 'd':
		if (cflag != -1)
			usage();
		getproclist(&argv[optind]);
		display();
		break;
	case 's':
		getproclist(&argv[optind]);
		set();
		break;
	case 'e':
		if (iflag)
			usage();
		execute(&argv[optind]);
		break;
	default:
		usage();
	}
	return errcnt;
}

#else	/* !__linux__, !__FreeBSD__ */

#include	<unistd.h>

int
main(int argc, char **argv)
{
	execv("/usr/bin/priocntl", argv);
	write(2, "priocntl not available\n", 23);
	_exit(0177);
}

#endif	/* !__linux__, !__FreeBSD__ */
