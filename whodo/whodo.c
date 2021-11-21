/*
 * whodo - who is doing what
 *
 * also source for "w" and "uptime" commands
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
static const char sccsid[] USED = "@(#)whodo.sl	1.42 (gritter) 1/12/07";

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/utsname.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<dirent.h>
#include	<limits.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<time.h>
#include	<utmpx.h>
#include	<libgen.h>
#include	<alloca.h>
#include	<ctype.h>
#include	<locale.h>
#include	<wchar.h>
#include	<wctype.h>

#if defined (__FreeBSD__) || defined (__DragonFly__)
#include	<sys/sysctl.h>
#endif

#if defined (__NetBSD__) || defined (__OpenBSD__) || defined (__APPLE__)
#if defined (__APPLE__)
#include	<mach/mach_types.h>
#include	<mach/task_info.h>
#else	/* !__APPLE__ */
#include	<kvm.h>
#endif /* !__APPLE__ */
#include	<sys/param.h>
#include	<sys/sysctl.h>
#endif /* __NetBSD__, __NetBSD__, __APPLE__ */

#ifdef	__hpux
#include	<sys/param.h>
#include	<sys/pstat.h>
#endif

#ifdef	_AIX
#include	<procinfo.h>
#endif

#if !defined (__linux__) && !defined (__FreeBSD__) && !defined (__hpux) && \
	!defined (_AIX) && !defined (__NetBSD__) && !defined (__OpenBSD__) && \
	!defined (__DragonFly__) && !defined (__APPLE__)
#ifdef	sun
#include	<sys/loadavg.h>
#define	_STRUCTURED_PROC	1
#endif
#include	<sys/procfs.h>
#endif	/* !__linux__, !__FreeBSD__, !__hpux, !_AIX, !__NetBSD__, !__OpenBSD__,
	!__DragonFly__, !__APPLE__ */

#ifndef	PRNODEV
#define	PRNODEV		0
#endif

#define	next(wc, s, n)	(mb_cur_max > 1 && *(s) & 0200 ? \
		((n) = mbtowc(&(wc), (s), mb_cur_max), \
		 (n) = ((n) > 0 ? (n) : (n) < 0 ? (wc=WEOF, 1) : 1)) :\
		((wc) = *(s) & 0377, (n) = 1))

enum {
	WHODO,
	W,
	UPTIME
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

static int		cmd = WHODO;		/* command personality */
static int		errcnt;			/* count of errors */
static int		hflag;			/* omit header */
static int		lflag;			/* w-like display format */
static int		sflag;			/* short form of output */
static int		uflag;			/* "uptime" format */
#undef	hz
static int		hz;			/* clock ticks per second */
static char		*user;			/* look for one user only */
#if defined (__linux__) || defined (__sun) || defined (__FreeBSD__) || \
	defined (__hpux) || defined (__NetBSD__) || defined (__OpenBSD__) || \
	defined (__DragonFly__) || defined (__APPLE__)
static char		loadavg[64];		/* load average */
#endif	/* __linux__ || __sun || __FreeBSD__ || __hpux || __NetBSD__ ||
		__OpenBSD__ || __DragonFly__ || __APPLE__ */
static char		*progname;		/* argv0 to main */
static const char	*unknown = " unknown";	/* unknown */
static time_t		now;			/* time at program start */
static int		mb_cur_max;		/* MB_CUR_MAX acceleration */

#undef	UGLY_XDM_HACK

/*
 * Structure to represent a single process.
 */
struct pslot {
	struct pslot	*p_next;	/* next slot */
	long		p_time;		/* cpu time of process */
	long		p_ctime;	/* cpu time of children */
	char		p_name[16];	/* name of the executable file */
	pid_t		p_pid;		/* process id */
	dev_t		p_termid;	/* device id of the controlling tty */
	char		p_cmdline[30];	/* command line */
};

/*
 * Structure to represent a user login.
 */
struct tslot {
	struct pslot	*t_pslot;	/* start of process table */
	struct tslot	*t_next;	/* next slot */
	time_t		t_time;		/* login timestamp */
	char	t_line[sizeof (((struct utmpx *)0)->ut_line)]; /*tty line name*/
	char	t_user[sizeof (((struct utmpx *)0)->ut_user)]; /* user name */
	dev_t		t_termid;	/* device id of the tty */
};

/*
 * perror()-alike.
 */
static void
pnerror(int eno, const char *string)
{
	fprintf(stderr, "%s: %s: %s\n", progname, string, strerror(eno));
	errcnt |= 1;
}

/*
 * Memory allocation with check.
 */
static void *
srealloc(void *vp, size_t sz)
{
	void	*p;

	if ((p = realloc(vp, sz)) == NULL) {
		write(2, "no memory\n", 10);
		exit(077);
	}
	return p;
}

static void *
smalloc(size_t sz)
{
	return srealloc(NULL, sz);
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
		if ((separator == ' ' ?
				isspace(**listp) : **listp == separator) ||
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

/*
 * Print time in am/pm format.
 */
static char *
time12(time_t t)
{
	static char buf[8];
	struct tm *tp;
	int hour12;

	tp = localtime(&t);
	if (tp->tm_hour == 0)
		hour12 = 0;
	else if (tp->tm_hour < 13)
		hour12 = tp->tm_hour;
	else
		hour12 = tp->tm_hour - 12;
	snprintf(buf, sizeof buf, "%2u:%02u%s", hour12, tp->tm_min,
			tp->tm_hour < 12 ? "am" : "pm");
	return buf;
}

/*
 * Print time in 24 hour format.
 */
static char *
time24(time_t t)
{
	static char buf[8];
	struct tm *tp;

	tp = localtime(&t);
	snprintf(buf, sizeof buf, "%u:%02u", tp->tm_hour, tp->tm_min);
	return buf;
}

/*
 * Return the system's uptime.
 */
static const char *
uptime(void)
{
	static char buf[80];
	long upsec = -1;
	unsigned upday = 0, uphr = 0, upmin = 0;
	char *cp;
#if defined (__linux__)
	FILE *fp;
	union value	*v;
	const char upfile[] = "/proc/uptime";
	
	if ((fp = fopen(upfile, "r")) == NULL) {
		pnerror(errno, upfile);
		return unknown;
	}
	errno = 0;
	if (fread(buf, 1, sizeof buf, fp) > 0) {
		cp = buf;
		if ((v = getval(&cp, VT_ULONG, '.', 0)) != NULL)
			upsec = v->v_ulong;
	}
	if (upsec == -1) {
		if (errno)
			pnerror(errno, upfile);
		else {
			fprintf(stderr, "%s: invalid uptime file\n", upfile);
			errcnt |= 1;
		}
		fclose(fp);
		return unknown;
	}
	fclose(fp);
#elif defined (__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) \
		|| defined (__DragonFly__) || defined (__APPLE__)
	int	name[2] = { CTL_KERN, KERN_BOOTTIME };
	struct timeval	old;
	size_t	oldlen = sizeof old;

	if (sysctl(name, 2, &old, &oldlen, NULL, 0) < 0)
		return unknown;
	upsec = now - old.tv_sec;
#elif defined (__hpux)
	struct pst_static	pst;

	pstat_getstatic(&pst, sizeof pst, 1, 0);
	upsec = now - pst.boot_time;
#elif defined (_AIX)
	struct procentry64	pi;
	pid_t	idx = 0;

	getprocs64(&pi, sizeof pi, NULL, 0, &idx, 1);
	upsec = now - pi.pi_start;
#else	/* !__linux__, !__FreeBSD__, !__hpux, !_AIX, !__NetBSD__, !__OpenBSD__,
	!__DragonFly__, !__APPLE__ */
	FILE *fp;
	struct psinfo	pi;

	if ((fp = fopen("/proc/0/psinfo", "r")) == NULL)
		return unknown;
	if (fread(&pi, 1, sizeof pi, fp) != sizeof pi) {
		fclose(fp);
		return unknown;
	}
	fclose(fp);
	upsec = now - pi.pr_start.tv_sec;
#endif	/* !__linux__, !__FreeBSD__, !__hpux, !_AIX, !__NetBSD__,
		!__OpenBSD__, !__DragonFly__, !__APPLE__ */
	if (upsec > 59) {
		upmin = upsec / 60;
		if (upmin > 59) {
			uphr = upmin / 60;
			upmin %= 60;
			if (uphr > 23) {
				upday = uphr / 24;
				uphr %= 24;
			}
		}
	}
	if (cmd == WHODO)
		snprintf(buf, sizeof buf, " %u day(s), %u hr(s), %u min(s)",
				upday, uphr, upmin);
	else {
		cp = buf;
		if (upday) {
			sprintf(cp, " %u day(s),", upday);
			cp += strlen(cp);
		}
		if (uphr && upmin)
			sprintf(cp, " %2u:%02u,", uphr, upmin);
		else {
			if (uphr) {
				sprintf(cp, " %u hr(s),", uphr);
				cp += strlen(cp);
			}
			if (upmin)
				sprintf(cp, " %u min(s),", upmin);
		}
	}
	return buf;
}

/*
 * Return the number of users currently logged on.
 */
static unsigned
userno(struct tslot *t)
{
	unsigned uno;

	for (uno = 0; t; t = t->t_next, uno++);
	return uno;
}

/*
 * Print the heading.
 */
static void
printhead(struct tslot *t0)
{
	time_t t;
	unsigned users;
	struct utsname un;

	time(&t);
	if (lflag) {
		users = userno(t0);
		printf(" %s  up%s  ", time12(t), uptime());
		if (cmd != WHODO) {
			printf("%u %s", users, users > 1 ? "users" : "user");
#if defined (__linux__) || defined (__sun) || defined (__FreeBSD__) || \
	defined (__hpux) || defined (__NetBSD__) || defined (__OpenBSD__) || \
	defined (__DragonFly__) || defined (__APPLE__)
			printf(",  load average: %s", loadavg);
#endif	/* __linux__ || __sun || __FreeBSD__ || __hpux || __NetBSD__ ||
	__OpenBSD__ || __DragonFly__ || __APPLE__ */
			printf("\n");
		} else
			printf("%u user(s)\n", users);
		if (!uflag) {
			if (sflag)
				printf("User     tty           idle   what\n");
			else
				printf(
		"User     tty           login@  idle   JCPU   PCPU  what\n");
		}
	} else {
		if (uname(&un) < 0)
			strcpy(un.nodename, unknown);
		printf("%s%s\n", ctime(&t), un.nodename);
	}
}

/*
 * Convert a login stamp to text.
 */
static char *
logtime(time_t t, int hform)
{
	static char b[64];
	struct tm tl, tn;
	size_t sz;
	char *form;

	memcpy(&tl, localtime(&t), sizeof tl);
	memcpy(&tn, localtime(&now), sizeof tn);
	t = mktime(&tl);
	if (hform == 12) {
		snprintf(b, sizeof b, "%s", time12(t));
	} else {
		if (tn.tm_year != tl.tm_year)
			form = "%a %b %d %Y ";
		else if (tn.tm_mday != tl.tm_mday)
			form = "%a %b %d ";
		else
			form = NULL;
		if (form)
			sz = strftime(b, 63, form, &tl);
		else {
			*b = '\0';
			sz = 0;
		}
		snprintf(b + sz, sizeof b - sz, "%s", time24(t));
	}
	return b;
}

/*
 * Convert a time value given in jiffies (on Linux) to text.
 */
static char *
jifftime(long jiff, char *buf, size_t buflen, int colon, int width)
{
	static char _b[63];
	char *b = buf ? buf : _b;
	long t;

#ifdef	__linux__
	t = jiff / hz;
#else
	t = jiff;
#endif
	if (t >= 60 || colon)
		snprintf(b, 63, "%*lu:%02lu", width - 3, t / 60, t % 60);
	else if (t > 0)
		snprintf(b, 63, "%*lu", width, t);
	else
		snprintf(b, 64, "%*s", width, "");
	return b;
}

/*
 * Print a tty's idle time.
 */
static char *
idletime(char *line)
{
	struct stat st;
	char fn[_POSIX_PATH_MAX];
	static char buf[8];
	time_t t;

	snprintf(fn, sizeof fn, "/dev/%s", line);
	if (stat(fn, &st) < 0)
		return "";
	time(&t);
	t -= st.st_atime;
	t /= 60;
	if (t >= 60)
		snprintf(buf, sizeof buf,
				"%2lu:%02lu", (long)t / 60, (long)t % 60);
	else if (t > 0)
		snprintf(buf, sizeof buf, " %4lu", (long)t);
	else
		strcpy(buf, "     ");
	return buf;
}

/*
 * Print a process table's accumulated time (+children).
 */
static char *
jcpu(struct pslot *p0, char *buf, size_t buflen)
{
	struct pslot *p;
	long acc = 0;

	for (p = p0; p; p = p->p_next)
		acc += p->p_time + p->p_ctime;
	return jifftime(acc, buf, buflen, 0, 6);
}

/*
 * Print a process table's accumulated time.
 */
static char *
pcpu(struct pslot *p0, char *buf, size_t buflen)
{
	struct pslot *p;
	long acc = 0;

	for (p = p0; p; p = p->p_next) 
		acc += p->p_time;
	return jifftime(acc, buf, buflen, 0, 6);
}

/*
 * Output the user/process table.
 */
static void
printout(struct tslot *t0)
{
	struct tslot *t;
	struct pslot *p;
	char jbuf[8], pbuf[8];

	if (hflag == 0)
		printhead(t0);
	if (uflag)
		return;
	if (lflag) {
		for (t = t0; t != NULL; t = t->t_next) {
#ifndef	UGLY_XDM_HACK
			if (t->t_termid == PRNODEV)
				continue;
#endif
			if (t->t_pslot == NULL)
				continue;
			for (p = t->t_pslot; p->p_next != NULL; p = p->p_next);
			if (sflag)
				printf("%-8s %-12s %s   %s\n",
					t->t_user, t->t_line,
					idletime(t->t_line), p->p_name);
			else
				printf("%-8s %-12s %s %s %s %s  %s\n",
					t->t_user, t->t_line,
					logtime(t->t_time, 12),
					idletime(t->t_line),
					jcpu(t->t_pslot, jbuf, sizeof jbuf),
					pcpu(t->t_pslot, pbuf, sizeof pbuf),
					p->p_cmdline);
		}
	} else {
		for (t = t0; t != NULL; t = t->t_next) {
#ifndef	UGLY_XDM_HACK
			if (t->t_termid == PRNODEV)
				continue;
#endif
			printf("\n%-10s %-9s %s\n", t->t_line, t->t_user,
					logtime(t->t_time, 24));
			if (t->t_pslot == NULL)
				continue;
			for (p = t->t_pslot; p != NULL; p = p->p_next)
				printf("    %-10s %-6lu %s %s\n",
					p->p_termid != PRNODEV ?
						t->t_line : "fd/0",
					(long)p->p_pid,
					jifftime(p->p_time, NULL, 0, 1, 6),
					p->p_name);
		}
	}
}

/*
 * Insert a process slot into the terminal line table.
 */
static void
queueproc(struct tslot *t0, struct pslot *ps)
{
	struct tslot *t;
	struct pslot *p;

	for (t = t0; t != NULL; t = t->t_next) {
#ifdef	UGLY_XDM_HACK
		if (t->t_termid == PRNODEV && ps->p_termid == PRNODEV) {
			/*
			 * Hack for XFree86 xdm: The process uses -$DISPLAY on
			 * its command line.
			 */
			if (strcmp(ps->p_name, "xdm") == 0) {
				size_t sz = strlen(t->t_line);

				if (ps->p_cmdline[0] == '-'
					&& strncmp(&ps->p_cmdline[1],
						t->t_line, sz) == 0
					&& (ps->p_cmdline[sz+1] == ' '
					|| ps->p_cmdline[sz+1] == '\0')) {
					strcpy(ps->p_cmdline, t->t_line);
					strcpy(t->t_line, "X,");
					strcat(t->t_line, ps->p_cmdline);
					ps->p_name[0] = '\0';
					strcpy(ps->p_cmdline, "/usr/X/bin/xdm");
				} else
					continue;
			} else
				continue;
		} else
#endif	/* UGLY_XDM_HACK */
		if (t->t_termid != ps->p_termid)
			continue;
		if (t->t_pslot != NULL) {
			for (p = t->t_pslot; p->p_next != NULL; p = p->p_next);
			p->p_next = ps;
		} else
			t->t_pslot = ps;
		break;
	}
}

/*
 * Read an entry from /proc into a process slot.
 */

#if !defined (__hpux) && !defined (_AIX) && !defined (__NetBSD__) && \
	!defined (__OpenBSD__) && !defined (__APPLE__)

#if defined (__linux__) || defined (__FreeBSD__) || defined (__DragonFly__)

#define	GETVAL(a)		if ((v = getval(&cp, (a), ' ', 0)) == NULL) \
					goto error

#define	GETVAL_COMMA(a)		if ((v = getval(&cp, (a), ' ', ',')) == NULL) \
					goto error
#endif	/* __linux__ || __FreeBSD__ || __DragonFly__ */

#if defined (__linux__)
static struct pslot *
readproc(char *pdir)
{
	static char	*buf;
	static size_t	buflen;
	char fn[_POSIX_PATH_MAX];
	FILE *fp;
	union value	*v;
	struct pslot p, *pp;
	char *cp, *cq, *ce;
	int	c;
	size_t sz, sc;

	memset(&p, 0, sizeof p);
	snprintf(fn, sizeof fn, "%s/stat", pdir);
	if ((fp = fopen(fn, "r")) == NULL) {
		pnerror(errno, fn);
		return NULL;
	}
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
		goto error;
	*ce-- = '\0';
	cp = buf;
	GETVAL(VT_INT);
	p.p_pid = v->v_int;
	if (*cp++ != '(')
		goto error;
	for (cq = ce; cq >= cp && *cq != ')'; cq--);
	if (cq < cp)
		goto error;
	*cq = '\0';
	strncpy(p.p_name, cp, sizeof p.p_name);
	p.p_name[sizeof p.p_name - 1] = '\0';
	cp = &cq[1];
	while (isspace(*cp))
		cp++;
	GETVAL(VT_CHAR);
	if (v->v_char == 'Z')
		return NULL;
	GETVAL(VT_INT);
	/* ppid unused */
	GETVAL(VT_INT);
	/* pgrp unused */
	GETVAL(VT_INT);
	/* session unused */
	GETVAL(VT_INT);
	p.p_termid = v->v_int;
	if (p.p_termid == PRNODEV
#ifdef	UGLY_XDM_HACK
			&& strcmp(p.p_name, "(xdm)")
#endif
			)
		return NULL;
	GETVAL(VT_INT);
	/* tty_pgrp unused */
	GETVAL(VT_ULONG);
	/* flags unused */
	GETVAL(VT_ULONG);
	/* min_flt unused */
	GETVAL(VT_ULONG);
	/* cmin_flt unused */
	GETVAL(VT_ULONG);
	/* maj_flt unused */
	GETVAL(VT_ULONG);
	/* cmaj_flt unused */
	GETVAL(VT_ULONG);
	p.p_time = v->v_ulong;
	GETVAL(VT_ULONG);
	p.p_time += v->v_ulong;
	GETVAL(VT_ULONG);
	p.p_ctime = v->v_ulong;
	GETVAL(VT_ULONG);
	p.p_ctime += v->v_ulong;
	snprintf(fn, sizeof fn, "%s/cmdline", pdir);
	if ((fp = fopen(fn, "r")) != NULL) {
		int	hadzero = 0;

		cp = p.p_cmdline;
		ce = cp + sizeof p.p_cmdline - 1;
		while (cp < ce && (c = getc(fp)) != EOF) {
			if (c != '\0') {
				if (hadzero) {
					*cp++ = ' ';
					if (cp == ce)
						break;
					hadzero = 0;
				}
				*cp++ = c;
			} else
				hadzero = 1;
		}
		*cp = '\0';
		fclose(fp);
	}
	if (*p.p_cmdline == '\0')
		snprintf(p.p_cmdline, sizeof p.p_cmdline, "[ %s ]", p.p_name);
	pp = smalloc(sizeof *pp);
	memcpy(pp, &p, sizeof *pp);
	return pp;
error:
	fprintf(stderr, "%s: invalid entry\n", pdir);
	errcnt |= 1;
	return NULL;
}

#elif defined (__FreeBSD__) || defined (__DragonFly__)

enum okay {
	OKAY,
	STOP
};

static enum okay
getproc_status(char *pdir, struct pslot *p)
{
	static char	*buf;
	static size_t	buflen;
	char	fn[_POSIX_PATH_MAX];
	union value	*v;
	FILE	*fp;
	char	*cp, *ce, *cq;
	size_t	sz, sc;
	int	mj, mi;

	snprintf(fn, sizeof fn, "%s/status", pdir);
	if ((fp = fopen(fn, "r")) == NULL)
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
		if (cp - buf < sizeof p->p_name - 2)
			p->p_name[cp-buf] = *cp;
		cp++;
	}
	if (cp - buf < sizeof p->p_name - 1)
		p->p_name[cp-buf] = '\0';
	else
		p->p_name[sizeof p->p_name - 1] = '\0';
	while (*cp == ' ')
		cp++;
	GETVAL(VT_INT);
	p->p_pid = v->v_int;
	GETVAL(VT_INT);
	/* ppid unused */
	GETVAL(VT_INT);
	/* pgid unused */
	GETVAL(VT_INT);
	/* sid unused */
	if (isdigit(*cp)) {
		GETVAL_COMMA(VT_INT);
		mj = v->v_int;
		GETVAL(VT_INT);
		mi = v->v_int;
		if (mj != -1 || mi != -1)
			p->p_termid = makedev(mj, mi);
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
			p->p_termid = PRNODEV;
		else
			p->p_termid = st.st_rdev;
		free(dev);
		*cp = ' ';
		while (*cp == ' ') cp++;
	}
	while (*cp != ' ') cp++; while (*cp == ' ') cp++;
	/* skip flags */
	GETVAL_COMMA(VT_LONG);
	/* start unused */
	GETVAL(VT_LONG);
	/* skip microseconds */
	GETVAL_COMMA(VT_LONG);
	p->p_time = v->v_long;
	GETVAL(VT_LONG);
	/* skip microseconds */
	GETVAL_COMMA(VT_LONG);
	p->p_time += v->v_long;
	return OKAY;
error:
	fprintf(stderr, "%s: invalid entry\n", pdir);
	errcnt |= 1;
	return STOP;
}

static enum okay
getproc_cmdline(char *pdir, struct pslot *p)
{
	char	fn[_POSIX_PATH_MAX];
	FILE	*fp;
	char	*cp, *ce;
	int	hadzero = 0, c;

	snprintf(fn, sizeof fn, "%s/cmdline", pdir);
	if ((fp = fopen(fn, "r")) != NULL) {
		cp = p->p_cmdline;
		ce = cp + sizeof p->p_cmdline - 1;
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
	if (*p->p_cmdline == '\0')
		snprintf(p->p_cmdline,sizeof p->p_cmdline, "[ %s ]", p->p_name);
	return OKAY;
}

static struct pslot *
readproc(char *pdir)
{
	struct pslot	*p;
	enum okay	result;

	p = calloc(1, sizeof *p);
	if ((result = getproc_status(pdir, p)) == OKAY)
		result = getproc_cmdline(pdir, p);
	if (result == STOP) {
		free(p);
		return NULL;
	}
	return p;
}

#else	/* !__linux__, !__FreeBSD__, !__DragonFly__ */
static struct pslot *
readproc(char *pdir)
{
	char	fn[_POSIX_PATH_MAX];
	FILE	*fp;
	struct psinfo	pi;
	struct pslot	*p;

	snprintf(fn, sizeof fn, "%s/psinfo", pdir);
	if ((fp = fopen(fn, "r")) == NULL)
		return NULL;
	if (fread(&pi, 1, sizeof pi, fp) != sizeof pi) {
		fclose(fp);
		return NULL;
	}
	fclose(fp);
	p = smalloc(sizeof *p);
	p->p_next = NULL;
	p->p_time = pi.pr_time.tv_sec;
#ifdef	__sun
	p->p_ctime = pi.pr_ctime.tv_sec;
#endif	/* __sun */
	strncpy(p->p_name, pi.pr_fname, sizeof p->p_name);
	p->p_name[sizeof p->p_name - 1] = '\0';
	p->p_pid = pi.pr_pid;
	p->p_termid = pi.pr_ttydev;
	strncpy(p->p_cmdline, pi.pr_psargs, sizeof p->p_cmdline);
	p->p_cmdline[sizeof p->p_cmdline - 1] = '\0';
#ifndef	__sun
	{
		struct pstatus	ps;
		snprintf(fn, sizeof fn, "%s/status", pdir);
		if ((fp = fopen(fn, "r")) != NULL) {
			if (fread(&ps, 1, sizeof ps, fp) == sizeof ps)
				p->p_ctime = ps.pr_cutime.tv_sec +
					ps.pr_cstime.tv_sec;
			fclose(fp);
		}
	}
#endif	/* !__sun */
	return p;
}

#endif	/* !__linux__, !__FreeBSD__, !__DragonFly__ */

/*
 * Convert an entry in /proc to a slot entry, if advisable.
 */
static struct pslot *
getproc(char *pname)
{
	struct stat st;
	char *ep;
	char fn[_POSIX_PATH_MAX];
	struct pslot	*p;
	wchar_t	wc;
	int	n;
	
	strcpy(fn, "/proc/");
	strcat(fn, pname);
	if (lstat(fn, &st) < 0) {
		pnerror(errno, fn);
		return NULL;
	}
	if (!S_ISDIR(st.st_mode))
		return NULL;
	strtol(pname, &ep, 10);
	if (*ep != '\0')
		return NULL;
	if ((p = readproc(fn)) != NULL) {
		ep = p->p_cmdline;
		while (next(wc, ep, n), wc != '\0') {
			ep += n;
			if (mb_cur_max > 1 ? !iswprint(wc) : !isprint(wc))
				do
					ep[-n] = '?';
				while (--n);
		}
	}
	return p;
}

/*
 * Find all relevant processes.
 */
static void
findprocs(struct tslot *t0)
{
	DIR *dir;
	struct dirent *dent;
	struct pslot *p;

	if ((dir = opendir("/proc")) == NULL) {
		pnerror(errno, "/proc");
		return;
	}
	while ((dent = readdir(dir)) != NULL)
		if ((p = getproc(dent->d_name)) != NULL)
			queueproc(t0, p);
	closedir(dir);
}

#elif defined __hpux
static struct pslot *
readproc(struct pst_status *pst)
{
	struct pslot	*p;

	p = smalloc(sizeof *p);
	p->p_next = NULL;
	p->p_time = pst->pst_utime + pst->pst_stime;
	p->p_ctime = pst->pst_child_utime.pst_sec +
		(pst->pst_child_utime.pst_usec > + 500000) +
		pst->pst_child_stime.pst_sec +
		(pst->pst_child_stime.pst_usec > + 500000);
	strncpy(p->p_name, pst->pst_ucomm, sizeof p->p_name);
	p->p_name[sizeof p->p_name - 1] = '\0';
	p->p_pid = pst->pst_pid;
	if (pst->pst_term.psd_major != -1 || pst->pst_term.psd_minor != -1)
		p->p_termid = makedev(pst->pst_term.psd_major,
			pst->pst_term.psd_minor);
	else
		p->p_termid = PRNODEV;
	strncpy(p->p_cmdline, pst->pst_cmd, sizeof p->p_cmdline);
	p->p_cmdline[sizeof p->p_cmdline - 1] = '\0';
	return p;
}

static void
findprocs(struct tslot *t0) {
#define	burst	((size_t)10)
	struct pslot	*p;
	struct pst_status	pst[burst];
	int	i, count;
	int	idx = 0;

	while ((count = pstat_getproc(pst, sizeof *pst, burst, idx)) > 0) {
		for (i = 0; i < count; i++) {
			p = readproc(&pst[i]);
			queueproc(t0, p);
		}
		idx = pst[count-1].pst_idx + 1;
	}
}
#elif defined (_AIX)
static struct pslot *
readproc(struct procentry64 *pi)
{
	struct pslot	*p;
	char	args[100], *ap, *cp;

	p = smalloc(sizeof *p);
	p->p_next = NULL;
	p->p_time = pi->pi_utime + pi->pi_stime;
	p->p_ctime = p->p_time;
	strncpy(p->p_name, pi->pi_comm, sizeof p->p_name);
	p->p_name[sizeof p->p_name - 1] = '\0';
	p->p_pid = pi->pi_pid;
	p->p_termid = pi->pi_ttyp ? pi->pi_ttyd : PRNODEV;
	if (getargs(pi, sizeof *pi, args, sizeof args) == 0) {
		ap = args;
		cp = p->p_cmdline;
		while (cp < &p->p_cmdline[sizeof p->p_cmdline - 1]) {
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
	return p;
}

static void
findprocs(struct tslot *t0) {
#define	burst	((size_t)10)
	struct pslot	*p;
	struct procentry64	pi[burst];
	int	i, count;
	pid_t	idx = 0;

	while ((count = getprocs64(pi, sizeof *pi, NULL, 0, &idx, burst)) > 0) {
		for (i = 0; i < count; i++) {
			p = readproc(&pi[i]);
			queueproc(t0, p);
		}
		if (count < burst)
			break;
	}
}
#elif defined (__OpenBSD__)
static struct pslot *
readproc(struct kinfo_proc *kp, kvm_t *kt)
{
	struct pslot	*p;
	char	**args;
	char	*ap, *pp;

	p = smalloc(sizeof *p);
	p->p_next = 0;
	p->p_time = kp->kp_eproc.e_pstats.p_ru.ru_utime.tv_sec +
		kp->kp_eproc.e_pstats.p_ru.ru_stime.tv_sec;
	p->p_ctime = kp->kp_eproc.e_pstats.p_cru.ru_utime.tv_sec +
		kp->kp_eproc.e_pstats.p_cru.ru_stime.tv_sec;
	strncpy(p->p_name, kp->kp_proc.p_comm, sizeof p->p_name);
	p->p_name[sizeof p->p_name - 1] = '\0';
	p->p_pid = kp->kp_proc.p_pid;
	if (kp->kp_proc.p_flag & P_CONTROLT)
		p->p_termid = kp->kp_eproc.e_tdev;
	else
		p->p_termid = PRNODEV;
	if ((args = kvm_getargv(kt, kp, sizeof p->p_cmdline)) == NULL) {
		strncpy(p->p_cmdline, p->p_name, sizeof p->p_name);
		p->p_cmdline[sizeof p->p_cmdline - 1] = '\0';
	} else {
		ap = args[0];
		for (pp = p->p_cmdline; pp <
				&p->p_cmdline[sizeof p->p_cmdline-1]; pp++) {
			if (*ap == '\0') {
				*pp = ' ';
				ap = *++args;
				if (ap == NULL)
					break;
			} else
				*pp = *ap++;
		}
	}
	return p;
}

static void
findprocs(struct tslot *t0) {
	struct pslot	*p;
	kvm_t	*kt;
	struct kinfo_proc	*kp;
	int	i, cnt;

	if ((kt = kvm_open(NULL, NULL, NULL, KVM_NO_FILES, "kvm_open")) == NULL)
		exit(1);
	kp = kvm_getprocs(kt, KERN_PROC_ALL, 0, &cnt);
	i = cnt;
	while (--i > 0) {
		p = readproc(&kp[i], kt);
		queueproc(t0, p);
	}
	kvm_close(kt);
}
#elif defined (__NetBSD__)
static struct pslot *
readproc(struct kinfo_proc2 *kp, kvm_t *kt)
{
	struct pslot	*p;
	char	**args;
	char	*ap, *pp;

	p = smalloc(sizeof *p);
	p->p_next = 0;
	p->p_time = kp->p_uutime_sec + kp->p_ustime_sec;
	p->p_ctime = kp->p_uctime_sec;
	strncpy(p->p_name, kp->p_comm, sizeof p->p_name);
	p->p_name[sizeof p->p_name - 1] = '\0';
	p->p_pid = kp->p_pid;
	if (kp->p_flag & P_CONTROLT)
		p->p_termid = kp->p_tdev;
	else
		p->p_termid = PRNODEV;
	if ((args = kvm_getargv2(kt, kp, sizeof p->p_cmdline)) == NULL) {
		strncpy(p->p_cmdline, p->p_name, sizeof p->p_name);
		p->p_cmdline[sizeof p->p_cmdline - 1] = '\0';
	} else {
		ap = args[0];
		for (pp = p->p_cmdline; pp <
				&p->p_cmdline[sizeof p->p_cmdline-1]; pp++) {
			if (*ap == '\0') {
				*pp = ' ';
				ap = *++args;
				if (ap == NULL)
					break;
			} else
				*pp = *ap++;
		}
	}
	return p;
}

static void
findprocs(struct tslot *t0) {
	struct pslot	*p;
	kvm_t	*kt;
	struct kinfo_proc2	*kp;
	int	i, cnt;

	if ((kt = kvm_open(NULL, NULL, NULL, KVM_NO_FILES, "kvm_open")) == NULL)
		exit(1);
	kp = kvm_getproc2(kt, KERN_PROC_ALL, 0, sizeof *kp, &cnt);
	i = cnt;
	while (--i > 0) {
		p = readproc(&kp[i], kt);
		queueproc(t0, p);
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

static int
getargv(struct pslot *p, struct kinfo_proc *kp)
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
	if (sysctl(mib, 2, &argsz, &size, NULL, 0) == -1)
		return 0;
	argbuf = smalloc(argsz);

	/* fetch the process arguments */
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROCARGS2;
	mib[2] = kp->kp_proc.p_pid;
	if (sysctl(mib, 3, argbuf, &argsz, NULL, 0) == -1)
		goto DONE; /* process already left the system */

	/* the number of args is at offset 0, this works for 32 and 64bit */
	memcpy(&nargs, argbuf, sizeof nargs);
	ap = argbuf + sizeof nargs;
	pp = NULL;

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
	for (pp = p->p_cmdline; pp < &p->p_cmdline[sizeof p->p_cmdline-1]; pp++) {
		if (*ap == '\0') {
			if (--nargs == 0)
				break;
			*pp = ' ';
			++ap;
		} else {
			*pp = *ap++;
		}
	}
	*pp = '\0';

DONE:	free(argbuf);
	return pp != NULL;
}

static time_t
tv2sec(time_value_t *tv, int mult)
{
	return tv->seconds*mult + (tv->microseconds >= 500000/mult);
}

extern	kern_return_t task_for_pid(task_port_t task, pid_t pid, task_port_t *target);

static struct pslot *
readproc(struct kinfo_proc *kp)
{
	kern_return_t   error;
	unsigned int	info_count = TASK_BASIC_INFO_COUNT;
	unsigned int 	thread_info_count = THREAD_BASIC_INFO_COUNT;
	pid_t		pid;
	task_port_t	task;
	struct		task_basic_info	task_binfo;
	struct		task_thread_times_info task_times;
	time_value_t	total_time;
	struct		pslot	*p;
	char		**args;
	char		*ap, *pp;

	p = smalloc(sizeof *p);
	p->p_next = NULL;
	strncpy(p->p_name, kp->kp_proc.p_comm, sizeof p->p_name);
	p->p_name[sizeof p->p_name - 1] = '\0';
	p->p_pid = kp->kp_proc.p_pid;
	p->p_time = 0;
	p->p_ctime = 0;
	if (kp->kp_proc.p_flag & P_CONTROLT)
		p->p_termid = kp->kp_eproc.e_tdev;
	else
		p->p_termid = PRNODEV;

	if (kp->kp_proc.p_stat == SZOMB || !getargv(p, kp)) {
		/* fallback to p_comm */
		strncpy(p->p_cmdline, p->p_name, sizeof p->p_name);
		p->p_cmdline[sizeof p->p_cmdline - 1] = '\0';
	}
	
	/* now try to fetch the times out of mach structures */
	pid = kp->kp_proc.p_pid;
	error = task_for_pid(mach_task_self(), pid, &task);
	if (error != KERN_SUCCESS)
		goto DONE; /* process already left the system */
	info_count = TASK_BASIC_INFO_COUNT;
	error = task_info(task, TASK_BASIC_INFO, &task_binfo, &info_count);
	if (error != KERN_SUCCESS)
		goto DONE;
	info_count = TASK_THREAD_TIMES_INFO_COUNT;
	error = task_info(task, TASK_THREAD_TIMES_INFO, &task_times, &info_count);
	if (error != KERN_SUCCESS)
		goto DONE;

	total_time = task_times.user_time;
	time_value_add(&total_time, &task_times.system_time);
	p->p_time = tv2sec(&total_time, 1);

	time_value_add(&total_time, &task_binfo.user_time);
	time_value_add(&total_time, &task_binfo.system_time);
	p->p_ctime = tv2sec(&total_time, 1);

DONE:	mach_port_deallocate(mach_task_self(), task);	
	return p;
}

static void
findprocs(struct tslot *t0) {
	struct	pslot	*p;
	struct	kinfo_proc *kp = NULL;
	size_t	cnt;
	int	err;

	if ((err = GetBSDProcessList(0, &kp, &cnt)) != 0) {
		fprintf(stderr, "error getting proc list: %s\n", strerror(err));
		exit(3);
	}
	while (--cnt > 0) {
		p = readproc(&kp[cnt]);
		queueproc(t0, p);
	}
	/* free the memory allocated by GetBSDProcessList */
	free(kp);	
}

#endif	/* all */

/*
 * Return the device id that correspondends to the given file name.
 */
static dev_t
lineno(char *line)
{
	struct stat st;
	char fn[_POSIX_PATH_MAX];

	strcpy(fn, "/dev/");
	strcat(fn, line);
	if (stat(fn, &st) < 0)
		return 0;
	return st.st_rdev;
}

/*
 * Get load average.
 */
#if defined (__linux__)
static void
getload(void)
{
	FILE *fp;
	char *sp1, *sp2, *sp3;
	char tmp[64];

	if ((fp = fopen("/proc/loadavg", "r")) == NULL) {
		strcpy(loadavg, unknown);
		errcnt |= 1;
		return;
	}
	fread(tmp, sizeof(char), sizeof tmp, fp);
	fclose(fp);
	if ((sp1 = strchr(tmp, ' ')) != NULL &&
			(sp2 = strchr(&sp1[1], ' ')) != NULL &&
			(sp3 = strchr(&sp2[1], ' ')) != NULL) {
		sp1[0] = sp2[0] = sp3[0] = '\0';
		snprintf(loadavg, sizeof loadavg, "%s, %s, %s",
				tmp, &sp1[1], &sp2[1]);
	} else {
		strcpy(loadavg, unknown);
		errcnt |= 1;
	}
}
#elif defined (__sun) || defined (__FreeBSD__) || defined (__NetBSD__) || \
	defined (__OpenBSD__) || defined (__DragonFly__) || defined (__APPLE__)

#ifndef	LOADAVG_NSTATS
#define	LOADAVG_NSTATS	3
#endif
#ifndef	LOADAVG_1MIN
#define	LOADAVG_1MIN	0
#endif
#ifndef	LOADAVG_5MIN
#define	LOADAVG_5MIN	1
#endif
#ifndef	LOADAVG_15MIN
#define	LOADAVG_15MIN	2
#endif

static void
getload(void)
{
	double	val[LOADAVG_NSTATS];

	if (getloadavg(val, LOADAVG_NSTATS) == LOADAVG_NSTATS)
		snprintf(loadavg, sizeof loadavg, "%.2f, %.2f, %.2f",
				val[LOADAVG_1MIN],
				val[LOADAVG_5MIN],
				val[LOADAVG_15MIN]);
	else
		strcpy(loadavg, unknown);
}
#elif defined (__hpux)
static void
getload(void)
{
	struct pst_dynamic	pst;

	pstat_getdynamic(&pst, sizeof pst, 1, 0);
	snprintf(loadavg, sizeof loadavg, "%.2f, %.2f, %.2f",
			pst.psd_avg_1_min,
			pst.psd_avg_5_min,
			pst.psd_avg_15_min);
}
#endif	/* __hpux */

/*
 * Get the list of user logins.
 */
static void
getlogins(void)
{
	struct utmpx *ut;
	struct tslot *t, *t0 = NULL, *tprev = NULL;

#if defined (__linux__) || defined (__sun) || defined (__FreeBSD__) || \
	defined (__hpux) || defined (__NetBSD__) || defined (__OpenBSD__) || \
	defined (__DragonFly__) || defined (__APPLE__)
	if (cmd != WHODO)
		getload();
#endif	/* __linux__ || __sun || __FreeBSD__ || __hpux || __NetBSD__ ||
		__OpenBSD__ ||  __DragonFly__ || __APPLE__ */
	setutxent();
	while ((ut = getutxent()) != NULL) {
		if (ut->ut_type == USER_PROCESS) {
			if (user)
				if (ut->ut_user == NULL
						|| strcmp(user, ut->ut_user))
					continue;
			t = (struct tslot *)smalloc(sizeof *t);
			if (t0 == NULL)
				t0 = t;
			else
				tprev->t_next = t;
			tprev = t;
			if (ut->ut_line) {
				strcpy(t->t_line, ut->ut_line);
				t->t_termid = lineno(ut->ut_line);
			} else {
				strcpy(t->t_line, unknown);
				t->t_termid = PRNODEV;
			}
			if (ut->ut_user)
				strcpy(t->t_user, ut->ut_user);
			else
				strcpy(t->t_user, unknown);
			t->t_time = ut->ut_tv.tv_sec;
			if (ut->ut_tv.tv_usec >= 500000)
				t->t_time++;
			t->t_pslot = NULL;
			t->t_next = NULL;
		}
	}
	findprocs(t0);
	printout(t0);
	endutxent();
}

static void
usage(void)
{
	switch (cmd) {
	case W:
	case UPTIME:
		break;
	default:
		fprintf(stderr, "usage: %s [ -hl ] [ user ]\n", progname);
	}
	exit(2);
}

int
main(int argc, char **argv)
{
	int i;
	const char *opts = ":hl";

	time(&now);
#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	progname = basename(argv[0]);
	if (progname[0] == 'w' && progname[1] == '\0') {
		cmd = W;
		lflag = 1;
		opts = ":hlsuw";
	} else if (strcmp(progname, "uptime") == 0) {
		cmd = UPTIME;
		lflag = 1;
		uflag = 1;
		opts = ":";
	}
	hz = sysconf(_SC_CLK_TCK);
	while ((i = getopt(argc, argv, opts)) != EOF) {
		switch (i) {
		case 'h':
			hflag = 1;
			break;
		case 'l':
		case 'w':
			lflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 'u':
			uflag = 1;
			break;
		default:
			if (cmd == W || cmd == UPTIME)
				fprintf(stderr, "%s: bad flag -%c\n",
					progname, optopt);
			usage();
		}
	}
	if (argv[optind]) {
		if (cmd == UPTIME)
			usage();
		user = argv[optind];
		if (argv[++optind])
			usage();
	}
	getlogins();
	return errcnt;
}
