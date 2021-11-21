/*
 * time - time a command
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
#ifdef	PTIME
static const char sccsid[] USED = "@(#)ptime.sl	1.28 (gritter) 5/29/05";
#else
static const char sccsid[] USED = "@(#)time.sl	1.28 (gritter) 5/29/05";
#endif

#include	<sys/types.h>
#include	<sys/wait.h>
#include	<sys/times.h>
#include	<unistd.h>
#include	<signal.h>
#include	"sigset.h"
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<errno.h>
#include	<libgen.h>
#include	<ctype.h>
#ifdef	PTIME
#include	<limits.h>
#endif
#include	<locale.h>

static int		pflag;		/* portable format */
#undef	hz
static long		hz;		/* clock ticks per second */
static char		*progname;	/* this command's name */
static struct lconv	*localec;	/* locale information */

/*
 * perror()-alike.
 */
#ifdef	PTIME
static void
pnerror(int eno, const char *string)
{
	fprintf(stderr, "%s: %s: %s\n", progname, string, strerror(eno));
}
#endif	/* PTIME */

/*
 * Write a single output line for type msg. Val is given in clock ticks.
 */
static void
out(const char *msg, clock_t val)
{
#ifndef	PTIME
	long divider = pflag ? 100 : 10;
#else
	long divider = pflag ? 100 : 1000;
#endif
	long hours, mins, secs, fracs;

	secs = val * divider / hz;
	fracs = secs % divider;
	secs = (secs - fracs) / divider;
	if (pflag)
		fprintf(stderr, "%s %lu%s%02lu\n", msg, secs,
				localec->decimal_point, fracs);
	else {
		fprintf(stderr,"%-4s ", msg);
		if (secs > 59) {
			mins = secs / 60;
			secs %= 60;
			if (mins > 59) {
				hours = mins / 60;
				mins %= 60;
				fprintf(stderr, "%2lu:%02lu:%02lu.",
						hours, mins, secs);
			} else
				fprintf(stderr, "   %2lu:%02lu.",
						mins, secs);
		} else
			fprintf(stderr, "      %2lu.", secs);
#ifdef	PTIME
		fprintf(stderr, "%03lu\n", fracs);
#else
		fprintf(stderr, "%lu\n", fracs);
#endif
	}
}

#ifdef	PTIME
/*
 * Memory allocation with check.
 */
static void *
srealloc(void *vp, size_t nbytes)
{
	void *p;

	if ((p = (void *)realloc(vp, nbytes)) == NULL) {
		write(2, "Out of memory\n", 14);
		exit(077);
	}
	return p;
}

static void *
smalloc(size_t sz)
{
	return srealloc(NULL, sz);
}

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

static union value *
getval(char **listp, enum valtype type, int separator)
{
	char	*buf;
	static union value	v;
	const char	*cp, *op;
	char	*cq, *x;

	if (**listp == '\0')
		return NULL;
	op = *listp;
	while (**listp != '\0') {
		if (separator == ' ' ? isspace(**listp) : **listp == separator)
			break;
		(*listp)++;
	}
	buf = smalloc(*listp - op + 1);
	for (cp = op, cq = buf; cp < *listp; cp++, cq++)
		*cq = *cp;
	*cq = '\0';
	if (**listp) {
		while (separator == ' ' ?
				isspace(**listp) : **listp == separator)
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

static int
failed(void)
{
	fprintf(stderr, "/proc read failed\n");
	return -1;
}

#define	GETVAL(a)	if ((v = getval(&cp, (a), ' ')) == NULL) \
				return failed()

/*
 * Get process times from /proc.
 */
static int
ptimes(struct tms *tp, FILE *fp, char *stfn)
{
	static char	*buf;
	static size_t	buflen;
	union value	*v;
	char *cp, *cq, *ce;
	size_t	sz, sc;

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
		failed();
	*ce-- = '\0';
	cp = buf;
	GETVAL(VT_INT);
	/* pid not used */
	if (*cp++ != '(')
		failed();
	for (cq = ce; cq >= cp && *cq != ')'; cq--);
	if (cq < cp)
		failed();
	*cq = '\0';
	cp = &cq[1];
	while (isspace(*cp))
		cp++;
	GETVAL(VT_CHAR);
	/* state not used */
	GETVAL(VT_INT);
	/* ppid not used */
	GETVAL(VT_INT);
	/* pgrp unused */
	GETVAL(VT_INT);
	/* session unused */
	GETVAL(VT_INT);
	/* nr not used */
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
	/* cmaj_flut unused */
	GETVAL(VT_ULONG);
	tp->tms_cutime = v->v_ulong;
	GETVAL(VT_ULONG);
	tp->tms_cstime = v->v_ulong;
	return 0;
}
#endif	/* PTIME */

/*
 * In child process: Execute the command or set error status.
 */
static void
child(char **av)
{
	int err;

	execvp(av[0], av);
	err = errno;
	fprintf(stderr, "%s: %s\n", strerror(err), av[0]);
	_exit(err == ENOENT ? 0177 : 0176);
}

/*
 * Time a command.
 */
static int
timecmd(char **av)
{
	void (*oldint)(int), (*oldquit)(int);
	struct tms tp;
#ifdef	PTIME
	char pdir[_POSIX_PATH_MAX];
	FILE *fp;
	char *stfn = "stat";
#endif
	clock_t t1, t2;
	pid_t pid;
	int status;

	oldint = sigset(SIGINT, SIG_IGN);
	oldquit = sigset(SIGQUIT, SIG_IGN);
	t1 = times(&tp);
	switch (pid = fork()) {
	case 0:
		sigset(SIGINT, oldint);
		sigset(SIGQUIT, oldquit);
		child(av);
		/*NOTREACHED*/
	case -1:
		fprintf(stderr, "%s: cannot fork -- try again.\n", progname);
		return 1;
	}
#ifdef	PTIME
	/*
	 * Changing to the child's /proc entry will keep it in zombie status
	 * even after it has been waited for.
	 */
	snprintf(pdir, sizeof pdir, "/proc/%lu", (long)pid);
	if (chdir(pdir) < 0) {
		pnerror(errno, pdir);
		return 1;
	}
	/*
	 * Starting with Linux 2.4.5, the stat file has to be opened to
	 * make it persistent.
	 */
	if ((fp = fopen(stfn, "r")) == NULL) {
		pnerror(errno, stfn);
		return 1;
	}
#endif
	while (wait(&status) != pid);
	t2 = times(&tp);
	sigset(SIGINT, oldint);
	sigset(SIGQUIT, oldquit);
	if (WIFSIGNALED(status))
		fprintf(stderr, "%s: command terminated abnormally.\n\n",
				progname);
	else
		fprintf(stderr, "\n");
#ifdef	PTIME
	if (ptimes(&tp, fp, stfn) < 0)
		return 1;
#endif
	out("real", t2 - t1);
	out("user", tp.tms_cutime);
	out("sys", tp.tms_cstime);
	return status ? (WIFEXITED(status)
			? WEXITSTATUS(status) : WTERMSIG(status) | 0200)
		: 0;
}

int
main(int argc, char **argv)
{
	int i;

	progname = basename(argv[0]);
	setlocale(LC_NUMERIC, "");
	localec = localeconv();
	i = 1;
	while (i < argc && argv[i][0] == '-' && argv[i][1]) {
	nxt:	switch (argv[i][1]) {
		case 'p':
			pflag = 1;
			if (argv[i][2] == 'p') {
				(argv[i])++;
				goto nxt;
			} else
				i++;
			break;
		case '-':
			if (argv[i][2] == '\0')
				i++;
			/*FALLTHRU*/
		default:
			goto opnd;
		}
	}
opnd:	if (i >= argc)
		return 0;
	hz = sysconf(_SC_CLK_TCK);
	return timecmd(&argv[i]);
}
