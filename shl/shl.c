/*
 * shl - shell layer manager
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, April 2001.
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
static const char sccsid[] USED = "@(#)shl.sl	1.29 (gritter) 12/25/06";

#if !defined (__FreeBSD__) && !defined (__hpux) && !defined (_AIX) && \
	!defined (__NetBSD__) && !defined (__OpenBSD__) && \
	!defined (__DragonFly__) && !defined (__APPLE__)

/*
 * UnixWare 2.1 needs _KMEMUSER to access some flags for STREAMS. Maybe other
 * SVR4 do too, so define it on non-Sun systems for now.
 */
#ifndef	sun
#define	_KMEMUSER
#endif

#include	<sys/types.h>
#include	<termios.h>

/*
 * Structure for a single shell layer.
 */
struct layer {
	struct layer	*l_prev;	/* previous node in layer list */
	struct layer	*l_next;	/* next node in layer list */
	struct termios	l_tio;		/* termios struct of layer */
	char		l_name[9];	/* name of layer */
	char		l_line[64];	/* device name for tty */
	pid_t		l_pid;		/* pid/pgid of layer */
	int		l_pty;		/* pty master */
	int		l_blk;		/* output is blocked */
};

extern int	sysv3;

#ifdef	__dietlibc__
#define	_XOPEN_SOURCE	600L
#endif

/*
 * This implements the interface of the SVID3 shl command using regular
 * SVR4-type pseudo terminal mechanisms. Each layer gets its own pty,
 * and shl poll()s them and writes their output to its own tty. Input
 * is collected by the same mechanism in reverse direction.
 */

#include	<sys/wait.h>
#include	<sys/stat.h>
#include	<sys/ioctl.h>
#include	<fcntl.h>
#include	<signal.h>
#include	<unistd.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>
#include	<limits.h>
#include	<stdarg.h>
#if !defined (__dietlibc__) && !defined (__UCLIBC__)
#include	<stropts.h>
#endif
#include	<poll.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<time.h>
#include	<utmpx.h>
#include	<sigset.h>
#include	<pathconf.h>
#if !defined (__dietlibc__) && !defined (__UCLIBC__)
#define	WORDEXP
#endif
#ifdef	WORDEXP
#include	<wordexp.h>
#endif	/* WORDEXP */
#include	<pwd.h>
#include	<locale.h>
#ifndef	__linux__
#include	<termio.h>
#include	<sys/stream.h>
#include	<sys/ptms.h>
#endif	/* !__linux__ */

/*
 * Linux systems call VSWTC what the SVID calls VSWTCH.
 */
#ifndef	VSWTCH
#ifdef	VSWTC
#define	VSWTCH	VSWTC
#else
#error	"Cannot find a VSWTCH or VSWTC key definition"
#endif	/* VSWTC */
#endif	/* !VSWTCH */

/*
 * The SVID specifies ^Z to be used as switch character if it is unset
 * when shl starts. As this implementation can co-exist with job control
 * within its layers, it might be a good idea to change either the susp
 * or the switch key before starting shl.
 */
#define	DEFSWTCH	'\32'		/* default switch character (^Z) */

/*
 * Only used if $SHELL is not set.
 */
#define	DEFSHELL	"/bin/sh"	/* default shell */

/*
 * This implementation of shl can support an arbitrary number of layers. As
 * pseudo-terminals are a sparse resource on most systems, the old default
 * of seven layers as a maximum is retained here. It can be raised without
 * problems if suitable.
 */
#define	MAXLAYER	7		/* maximum layers */

/*
 * Locale-independent whitespace recognition.
 */
#define	blankchar(c)	((c) == ' ' || (c) == '\t')

/*
 * Constants for utmp handling.
 */
enum {
	UTMP_ADD, UTMP_DEL
};

/*
 * This structure describes a shl command. A table of all commands resides
 * at the bottom end of this file.
 */
struct command {
	char	*c_string;		/* command's name */
	int	(*c_func)(char *);	/* function to do it */
	char	*c_arg;			/* argument to command */
	char	*c_desc;		/* command's description */
	int	c_dnum;			/* description's number in msg cat. */
};

unsigned	errstat;		/* error status of last command */
char		*progname;		/* argv[0] to main() */
char		*shell;			/* $SHELL */
char		*dashell;		/* -`basename $SHELL` */
pid_t		mypid;			/* shl's own pid */
uid_t		myuid;			/* shl's own uid */
uid_t		myeuid;			/* shl's own euid/saved set-user-ID */
gid_t		mygid;			/* shl's own gid */
gid_t		myegid;			/* shl's own egid/saved set-group-ID */
unsigned	lcount;			/* count of layers */
char		cmdbuf[LINE_MAX];	/* buffer for commands */
char		*cmdptr;		/* current pointer for cmdbuf */
int		hadquit;		/* quit cmd executed but layers run */
struct layer	*l0;			/* begin of layer chain */
struct layer	*lcur;			/* current layer */
struct termios	otio;			/* old terminal attributes */
struct termios	dtio;			/* default terminal attributes */
struct termios	rtio;			/* raw terminal attributes */
struct winsize	winsz;			/* window sizes */
void		(*oldhup)(int);		/* old SIGHUP handler */
void		(*oldint)(int);		/* old SIGINT handler */
void		(*oldquit)(int);	/* old SIGQUIT handler */
void		(*oldttou)(int);	/* old SIGTTOU handler */
void		(*oldtstp)(int);	/* old SIGTSTP handler */
void		(*oldttin)(int);	/* old SIGTTIN handler */
void		(*oldwinch)(int);	/* old SIGWINCH handler */

/*
 * Common english error messages.
 */
const char	*syntax = "syntax error";
const char	*lnotfound = "layer %s not found";
const char	*nocurl = "no current layer";

/*
 * The table of all shl commands residing at the bottom end of this file.
 */
extern struct command commands[];

/*
 * Messages to stdout.
 */
void
msg(const char *fmt, ...)
{
	char buf[256];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf - 1, fmt, ap);
	va_end(ap);
	strcat(buf, "\n");
	write(2, buf, strlen(buf));
}

/*
 * Memory allocation with check.
 */
void *
srealloc(void *vp, size_t nbytes)
{
	void	*p;

	if ((p = (void *)realloc(vp, nbytes)) == NULL) {
		write(2, "no memory\n", 10);
		exit(077);
	}
	return p;
}

void *
smalloc(size_t nbytes)
{
	return srealloc(NULL, nbytes);
}

/*
 * Allocate pty master (APUE, p. 638).
 */
int
ptym_open(char *pts_name, size_t namsz)
{
	char *ptr;
	int fdm;

	strcpy(pts_name, "/dev/ptmx");
	if ((fdm = open(pts_name, O_RDWR)) < 0)
		return -1;
	if (grantpt(fdm) < 0) {
		close(fdm);
		return -2;
	}
	if (unlockpt(fdm) < 0) {
		close(fdm);
		return -3;
	}
	if ((ptr = ptsname(fdm)) == NULL) {
		close(fdm);
		return -4;
	}
	strncpy(pts_name, ptr, namsz);
#ifndef	__linux__
	if (ioctl(fdm, I_PUSH, "pckt") < 0) {
		close(fdm);
		return -5;
	}
#endif	/* !__linux__ */
	return fdm;
}

/*
 * Allocate a pty slave (APUE, p. 639).
 */
int
ptys_open(int fdm, char *pts_name)
{
	int fds;

	if ((fds = open(pts_name, O_RDWR)) < 0) {
		close(fdm);
		return -5;
	}
#ifndef	__linux__
	if (ioctl(fds, I_PUSH, "ptem") < 0) {
		close(fdm);
		close(fds);
		return -6;
	}
	if (ioctl(fds, I_PUSH, "ldterm") < 0) {
		close(fdm);
		close(fds);
		return -7;
	}
	if (ioctl(fds, I_PUSH, "ttcompat") < 0) {
		close(fdm);
		close(fds);
		return -8;
	}
#endif	/* !__linux__ */
	return fds;
}

/*
 * Add or delete an entry in the system's utmp file.
 */
void
doutmp(int action, struct layer *l)
{
	/*
	 * Note that pututxline() may need privileges to work; but at least
	 * on Solaris, it does not as the libc arranges this for us. If shl
	 * has suid or sgid privileges, these are reset on startup and
	 * restored for utmp handling here.
	 */
	struct passwd *pwd = getpwuid(myuid);
	struct utmpx utx;
	char *id;

	memset(&utx, 0, sizeof utx);
	strncpy(utx.ut_line, l->l_line + 5, sizeof utx.ut_line);
	strncpy(utx.ut_user, pwd->pw_name, sizeof utx.ut_user);
	utx.ut_pid = l->l_pid;
	gettimeofday(&utx.ut_tv, NULL);
	if ((id = strrchr(l->l_line, '/')) != NULL)
		strncpy(utx.ut_id, id, sizeof utx.ut_id);
	switch (action) {
	case UTMP_ADD:
		utx.ut_type = USER_PROCESS;
		break;
	case UTMP_DEL:
		utx.ut_type = DEAD_PROCESS;
		break;
	}
	if (myuid != myeuid)
		setuid(myeuid);
	if (mygid != myegid)
		setgid(myegid);
#ifndef	__linux__
	if (action == UTMP_DEL) {
		/*
		 * On (at least) Solaris 8, /usr/lib/utmp_update will hang at
		 * ioctl(tty, TCGETA, ...) respective isatty() when the pty is
		 * in packet mode, but removing the pckt module we once pushed
		 * fails with EPERM in some circumstances (exact conditions
		 * unknown). If it does, close the pty master; pututxline()
		 * refuses to work then, but utmpd(1M) will remove the stale
		 * entry a few seconds later. This is not the ultimate
		 * solution, but better than hanging, of course. If shl needs
		 * privilegues to call pututxline(), these calls will not cause
		 * any harm since there is no dependency on the actual terminal
		 * device then.
		 */
		if (ioctl(l->l_pty, I_POP, 0) < 0) {
			close(l->l_pty);
			l->l_pty = -1;
		}
	}
#endif	/* !__linux__ */
	setutxent();
	getutxline(&utx);
	pututxline(&utx);
	endutxent();
	if (myuid != myeuid)
		setuid(myuid);
	if (mygid != myegid)
		setgid(mygid);
}

/*
 * Quit shl, killing all active layers.
 */
void
quit(int status)
{
	struct layer *l;

	for (l = l0; l; l = l->l_next) {
		kill(0 - l->l_pid, SIGHUP);
		doutmp(UTMP_DEL, l);
	}
	tcsetattr(0, TCSADRAIN, &otio);
	exit(status);
}

/*
 * Check for valid layer name.
 */
int
invlname(char *name)
{
	int err = 0;

	if (strpbrk(name, " \t"))
		err++;
	if (name[0] == '(' && isdigit(name[1] & 0377) && name[2] == ')'
			&& name[3] == '\0')
		err++;
	if (err)
		msg(syntax);
	return err;
}

/*
 * Find a layer by its pid.
 */
struct layer *
lbypid(pid_t pid)
{
	struct layer *l;

	for (l = l0; l; l = l->l_next)
		if (l->l_pid == pid)
			return l;
	return NULL;
}

/*
 * Find a layer by its pty.
 */
struct layer *
lbypty(int pty)
{
	struct layer *l;

	for (l = l0; l; l = l->l_next)
		if (l->l_pty == pty)
			return l;
	return NULL;
}

/*
 * Find a layer by its name.
 */
struct layer *
lbyname(char *name)
{
	struct layer *l;
	char bname[11];

	for (l = l0; l; l = l->l_next)
		if (strncmp(l->l_name, name, 8) == 0)
			return l;
	snprintf(bname, sizeof bname, "(%s)", name);
	for (l = l0; l; l = l->l_next)
		if (strncmp(l->l_name, bname, 8) == 0)
			return l;
	return NULL;
}

/*
 * Guess a layer by a possibly incomplete name part.
 */
struct layer *
lguess(char *name)
{
	struct layer *l, *lm = NULL;
	size_t nmlen = strlen(name);
	char bname[11];

	if (nmlen > 8)
		nmlen = 8;
	for (l = l0; l; l = l->l_next) {
		if (strncmp(l->l_name, name, nmlen) == 0) {
			if (lm) {
				msg("layer %s not unique", name);
				return NULL;
			}
			lm = l;
		}
	}
	if (lm == NULL) {
		snprintf(bname, sizeof bname, "(%s)", name);
		nmlen += 2;
		for (l = l0; l; l = l->l_next) {
			if (strncmp(l->l_name, bname, nmlen) == 0) {
				if (lm) {
					msg("layer %s not unique", name);
					return NULL;
				}
				lm = l;
			}
		}
	}
	if (lm == NULL)
		msg(lnotfound, name);
	return lm;
}

/*
 * Allocate a layer structure.
 */
struct layer *
lalloc(char *name, int accept)
{
	struct layer *l, *lp;

	if (lcount >= MAXLAYER) {
		msg("only %u layers allowed", MAXLAYER);
		return NULL;
	}
	if (accept == 0 && invlname(name))
		return NULL;
	if (lbyname(name)) {
		msg("layer %s already exists", name);
		return NULL;
	}
	l = (struct layer *)smalloc(sizeof *l);
	if (l0 == NULL) {
		l0 = l;
		l->l_prev = NULL;
	} else {
		for (lp = l0; lp->l_next != NULL; lp = lp->l_next);
		lp->l_next = l;
		l->l_prev = lp;
	}
	l->l_next = NULL;
	strncpy(l->l_name, name, 8);
	l->l_name[8] = '\0';
	l->l_pty = -1;
	l->l_blk = 0;
	lcount++;
	return l;
}

/*
 * Deallocate a layer structure.
 */
void
lfree(struct layer *l)
{
	if (l->l_prev)
		l->l_prev->l_next = l->l_next;
	if (l->l_next)
		l->l_next->l_prev = l->l_prev;
	if (l == l0)
		l0 = l->l_next;
	close(l->l_pty);
	free(l);
	lcount--;
}

/*
 * Block a layer.
 */
int
lblock(struct layer *l)
{
	l->l_blk = 1;
	return 0;
}

/*
 * Unblock a layer.
 */
int
lunblock(struct layer *l)
{
	l->l_blk = 0;
	return 0;
}

/*
 * Set a layer as current layer, that is, assign it to lcur and put it at
 * the end of the layer list.
 */
void
setcur(struct layer *l)
{
	struct layer *lp;

	if (l->l_next) {
		if (l->l_prev)
			l->l_prev->l_next = l->l_next;
		else
			l0 = l->l_next;
		l->l_next->l_prev = l->l_prev;
		for (lp = l; lp->l_next; lp = lp->l_next);
		l->l_prev = lp;
		lp->l_next = l;
		l->l_next = NULL;
	}
	lcur = l;
}

/*
 * Set the previous layer to the current.
 */
void
setprev(int nullok)
{
	struct layer *l = lcur;

	if ((lcur = l->l_prev) == NULL) {
		if (nullok)
			l0 = NULL;
		else
			lcur = l0;
	}
}

/*
 * Pass input from terminal to pty, or read shl commands.
 */
int
doinput(void)
{
	int seen = 0;
	char c;

#ifdef	__linux__
	if (cmdptr == NULL)
		tcgetattr(lcur->l_pty, &lcur->l_tio);
#endif	/* __linux__ */
	while (read(0, &c, 1) == 1) {
		if (cmdptr) {
			/*
			 * Do control character processing for shl commands.
			 */
			if ((c == '\n' && (otio.c_iflag & INLCR) == 0)
				|| (c == '\r' && (otio.c_iflag & ICRNL))) {
				write(1, "\r\n", 2);
				*cmdptr = '\0';
				seen++;
				break;
			} else if (cmdptr == cmdbuf &&
					(c&0377) == (dtio.c_cc[VEOF]&0377)) {
				/*EMPTY*/;
			} else if ((c&0377) == (dtio.c_cc[VERASE]&0377)) {
				if (cmdptr > cmdbuf)
					*cmdptr-- = '\0';
				write(1, "\b \b", 3);
			} else if ((c&0377) == (dtio.c_cc[VKILL]&0377)) {
				*(cmdptr = cmdbuf) = '\0';
				write(1, "\r\n", 2);
			} else {
				write(1, &c, 1);
				*cmdptr++ = c & 0377;
			}
			if (cmdptr == cmdbuf + sizeof cmdbuf) {
				msg("\r\nInput string too long, limit %u",
						sizeof cmdbuf - 1);
				*--cmdptr = '\0';
				write(1, "\r", 1);
				write(1, cmdbuf, strlen(cmdbuf));
			}
		} else {
			/*
			 * Pass input to the currently active layer and
			 * look for SWTCH character. If a LNEXT character
			 * is found, keep it so it can escape a following
			 * SWTCH.
			 */
			static char hadlnext;

#ifdef	VLNEXT
			if (c && (c&0377) == (lcur->l_tio.c_cc[VLNEXT]&0377) &&
					hadlnext == 0) {
				hadlnext = c;
			} else
#endif	/* VLNEXT */
			{
				if (c && (c&0377) ==
				    (lcur->l_tio.c_cc[VSWTCH]&0377)) {
					if (hadlnext) {
						hadlnext = 0;
					} else {
						seen++;
						break;
					}
				}
				if (hadlnext) {
					write(lcur->l_pty, &hadlnext, 1);
					hadlnext = 0;
				}
				write(lcur->l_pty, &c, 1);
			}
		}
	}
	return seen;
}

/*
 * Write to a file descriptor with possible restart.
 */
ssize_t
swrite(int fd, const char *data, size_t sz)
{
	ssize_t wo, wt = 0;

	do {
		if ((wo = write(fd, data + wt, sz - wt)) < 0) {
			if (errno == EINTR)
				continue;
			else
				return -1;
		}
		wt += wo;
	} while (wt < sz);
	return sz;
}

/*
 * Pass output from pty to terminal. On Linux systems, this is a simple
 * task as we can issue tcgetattr() on the pty master to get the slave's
 * settings. On SVR4 however, we have to use getmsg() for this and must
 * update our copy of the slave's settings whenever a M_IOCTL message
 * is received.
 */
int
dooutput(int pty, struct layer *l)
#ifdef	__linux__
{
	char buf[4096];
	ssize_t sz;

	if ((sz = read(pty, buf, sizeof buf)) > 0)
		swrite(1, buf, sz);
	return 0;
}
#else	/* !__linux__ */
{
	char cbuf[64], dbuf[1024];
	struct strbuf cs, ds;
	struct iocblk *ib;
	struct termio *to;
	int more, flag, maybehup = 0;

	if (l == NULL)
		l = lbypty(pty);
	for (;;) {
		cs.maxlen = sizeof cbuf;
		cs.buf = cbuf;
		ds.maxlen = sizeof dbuf;
		ds.buf = dbuf;
		flag = 0;
		if ((more = getmsg(l->l_pty, &cs, &ds, &flag)) < 0)
			break;
		switch (cbuf[0] & 0377) {
		case M_IOCTL:
			/*LINTED*/
			ib = (struct iocblk *)dbuf;
			switch (ib->ioc_cmd) {
			case TCSETS:
			case TCSETSW:
			case TCSETSF:
				memcpy(&l->l_tio, dbuf + sizeof *ib,
						sizeof l->l_tio);
				break;
			case TCSETA:
			case TCSETAW:
			case TCSETAF:
				/*LINTED*/
				to = (struct termio *)(dbuf + sizeof *ib);
				l->l_tio.c_iflag = to->c_iflag;
				l->l_tio.c_oflag = to->c_oflag;
				l->l_tio.c_cflag = to->c_cflag;
				l->l_tio.c_lflag = to->c_lflag;
				memcpy(l->l_tio.c_cc, to->c_cc, NCC);
				break;
			}
			break;
		case M_DATA:
			if (ds.len == 0)
				maybehup++;
			else
				swrite(1, ds.buf, ds.len);
			break;
		}
		if (more == 0)
			break;
	}
	return maybehup;
}
#endif	/* !__linux__ */

/*
 * Wait for children.
 */
struct layer *
await(void)
{
	struct layer *l;
	int	status;
	pid_t	pid;

	for (;;) {
		if ((pid = waitpid(-1, &status, WNOHANG)) < 0)
			return NULL;
		if ((l = lbypid(pid)) == NULL) /* not our job */
			continue;
		errstat = status;
		break;
	}
	return l;
}

/*
 * Input/output loop. Poll for input or status change on stdin and all pty
 * descriptors and invoke the appropriate handler.
 */
void
doio(void)
{
	struct pollfd pfd[MAXLAYER + 1];
	struct layer *l;
	int nfds, nevents, i;
	int endloop = 0;

	tcsetattr(0, TCSADRAIN, &rtio);
	pfd[0].fd = 0;
	pfd[0].events = POLLIN;
	pfd[0].revents = 0;
	for (nfds = 0, l = l0; nfds <= MAXLAYER && l; l = l->l_next) {
		nfds++;
		pfd[nfds].fd = l->l_pty;
		pfd[nfds].events = POLLIN;
		pfd[nfds].revents = 0;
	}
	while (endloop == 0) {
		nevents = poll(pfd, nfds + 1, -1);
		if (nevents == -1 && errno == EINTR)
			continue;
		for (i = 0; i <= nfds && nevents; i++) {
			if (pfd[i].revents == 0)
				continue;
			nevents--;
			if (pfd[i].revents & POLLIN) {
				if (i == 0)
					endloop += doinput();
				else {
					/*
					 * Check whether output is blocked.
					 * i != nfds does the same as
					 * l != lcur but is faster here.
					 */
					if (i != nfds || cmdptr != NULL) {
						l = lbypty(pfd[i].fd);
						if (l->l_blk) {
							pfd[i].events = 0;
							continue;
						}
					} else
						l = NULL;
					if (dooutput(pfd[i].fd, l))
						goto maybehup;
				}
				/*
				 * Do not check for other conditions as
				 * dooutput() possibly did not fetch all
				 * waiting data.
				 */
				continue;
			}
			if (pfd[i].revents & (POLLERR | POLLHUP)) {
				if (i == 0) {
					/*
					 * This is an error on input. What
					 * can we do to recover from here?
					 */
					msg("input error; retrying");
					sleep(1);
					continue;
				}
maybehup:
				l = await();
				if (l == lcur)
					endloop++;
				if (l != NULL) {
					if (l == lcur)
						setprev(1);
					doutmp(UTMP_DEL, l);
					lfree(l);
				}

			}
		}
	}
	tcsetattr(0, TCSADRAIN, &dtio);
}

/*
 * Delete a layer, kill its residents.
 */
int
ldelete(struct layer *l)
{
	kill(0 - l->l_pid, SIGHUP);
	/*
	 * Do not remove the utmp entry now; this will be done once the
	 * process has been waited for.
	 */
	if (l == lcur)
		setprev(0);
	return 0;
}

/*
 * Determine blocking status of a layer.
 */
char *
blockstat(struct layer *l)
{
	/*
	 * There is an additional message "blocked on input", but its
	 * conditions are unclear.
	 */
	if (l->l_blk) {
		struct pollfd pfd[1];

		pfd[0].fd = l->l_pty;
		pfd[0].events = POLLIN;
		pfd[0].revents = 0;
		if (poll(pfd, 1, 0) > 0 && (pfd[0].revents & POLLIN))
			return "blocked on output";
	}
	return "executing or awaiting input";
}

/*
 * List a layer, simple format.
 */
int
llist(struct layer *l)
{
	msg("%s (%ld) %s", l->l_name, (long)l->l_pid, blockstat(l));
	return 0;
}

/*
 * List a layer, extended ("ps"-like) format.
 */
int
lpslist(struct layer *l)
{
	pid_t	pid;
	int	status;

	llist(l);
	switch (pid = fork()) {
	case -1:
		return 1;
	default:
		while (waitpid(pid, &status, 0) != pid);
		msg("");
		break;
	case 0:
		putenv("PATH=" SV3BIN ":" DEFBIN ":/bin:/usr/bin");
		if (sysv3 && getenv("SYSV3") == NULL)
			putenv("SYSV3=set");
		dup2(2, 1);
		execlp("ps", "ps", "-f", "-t", &l->l_line[5], NULL);
		msg("no ps command found");
		_exit(0177);
	}
	return status;
}

/*
 * Resume a layer.
 */
int
lresume(struct layer *l)
{
	msg("resuming %s", l->l_name);
	setcur(l);
	doio();
	return 0;
}

/*
 * Execute a command on a layer list.
 */
int
foreach(char *list, int (*func)(struct layer *))
{
	struct layer *l;
	char *cp = list;
	int ecnt = 0;

	do {
		if ((cp = strpbrk(cp, " \t")) != NULL)
			while (blankchar(*cp))
				*cp++ = '\0';
		if ((l = lbyname(list)) == NULL) {
			msg(lnotfound, list);
			ecnt++;
		} else {
			ecnt += func(l);
		}
	} while ((list = cp) != NULL);
	return ecnt;
}

/*
 * Child command for new layer.
 */
void
child(char *arg0, char *arg, char **args)
{
	int estat;

	sigset(SIGHUP, oldhup);
	sigset(SIGINT, oldint);
	sigset(SIGQUIT, oldquit);
	sigset(SIGTTOU, oldttou);
	sigset(SIGTSTP, oldtstp);
	sigset(SIGTTIN, oldttin);
	sigset(SIGWINCH, oldwinch);
	if (args)
		execvp(args[0], args);
	else
		execlp(arg0, arg, NULL);
	switch (errno) {
	case ELOOP:
	case ENAMETOOLONG:
	case ENOENT:
		estat = 0176;
		break;
	default:
		estat = 0177;
	}
	msg("cannot exec %s", args ? args[0] : arg0);
	_exit(estat);
}

/*
 * Execute a new layer. If args is not NULL, use it for execution, else,
 * use arg0 and arg.
 */
pid_t
lspawn(struct layer *l, char *arg0, char *arg, char **args)
{
	char *ps1;
	size_t sz;
	pid_t pid;
	int fds;

	if ((l->l_pty = ptym_open(l->l_line, sizeof l->l_line)) < 0) {
		msg("control channel unavailable");
		return -1;
	}
	fcntl(l->l_pty, F_SETFD, FD_CLOEXEC);
	switch (pid = fork()) {
	case -1:
		msg("cannot fork");
		break;
	case 0:
		setsid();
		if ((fds = ptys_open(l->l_pty, l->l_line)) < 0) {
			msg("virtual tty %s open failed", l->l_line);
			_exit(0177);
		}
		close(l->l_pty);
		if (tcsetattr(fds, TCSADRAIN, &dtio) < 0) {
			msg("virtual tty %s stty failed", l->l_line);
			_exit(0177);
		}
		if (winsz.ws_row)
			ioctl(fds, TIOCSWINSZ, &winsz);
		dup2(fds, 0);
		dup2(fds, 1);
		dup2(fds, 2);
		close(fds);
		ps1 = smalloc(sz = strlen(l->l_name) + 7);
		snprintf(ps1, sz, "PS1=%s%s ", l->l_name, myuid ? "" : "#");
		putenv(ps1);
		child(arg0, arg, args);
		/*NOTREACHED*/
	default:
		memcpy(&l->l_tio, &dtio, sizeof l->l_tio);
	}
	return pid;
}

/*
 * Create a default layer name formed (digit).
 */
char *
defnumlnam(void)
{
	static char str[6];
	unsigned int count;

	for (count = 1; count <= MAXLAYER; count++) {
		snprintf(str, sizeof str, "(%u)", count);
		if (lbyname(str) == NULL)
			break;
	}
	return str;
}

/*
 * Create a new layer.
 */
int
c_create(char *arg)
{
#ifdef	WORDEXP
	wordexp_t we;
	int usewe = 0;
#endif	/* WORDEXP */
	char cmd0[_POSIX_PATH_MAX];
	char *cmd;
	const char *cp;
	struct layer *l;
	int accept = 0;

	if (arg == NULL) {
		arg = defnumlnam();
		accept++;
	}
	if (*arg == '-') {
		arg++;
		cmd = dashell;
		strncpy(cmd0, shell, sizeof cmd0);
	} else if ((cmd = strpbrk(arg, " \t")) == NULL) {
		cmd = dashell + 1;
		strncpy(cmd0, shell, sizeof cmd0);
	} else {
		while (blankchar(*cmd))
			*cmd++ = '\0';
#ifdef	WORDEXP
		usewe++;
		switch (wordexp(cmd, &we, 0)) {
		case 0:	/* success */
			cp = NULL;
			break;
		case WRDE_NOSPACE:
			cp = "no memory";
			break;
		case WRDE_BADCHAR:
		case WRDE_SYNTAX:
		default:
			cp = syntax;
		}
		if (cp != NULL) {
			msg(cp);
			return 1;
		}
#else	/* !WORDEXP */
		cp = cmd + strlen(cmd) - 1;
		while (blankchar(*cmd))
			*cmd-- = '\0';
		strncpy(cmd0, cmd, sizeof cmd0);
#endif	/* !WORDEXP */
	}
	cmd0[sizeof cmd0 - 1] = '\0';
	if ((l = lalloc(arg, accept)) == NULL)
		return 1;
	setcur(l);
	if ((l->l_pid = lspawn(l, cmd0, cmd,
#ifdef	WORDEXP
					usewe ? we.we_wordv :
#endif	/* WORDEXP */
					NULL))
			== -1) {
		setprev(1);
		lfree(l);
#ifdef	WORDEXP
		if (usewe)
			wordfree(&we);
#endif	/* WORDEXP */
		return 1;
	}
	doutmp(UTMP_ADD, l);
#ifdef	WORDEXP
	if (usewe)
		wordfree(&we);
#endif	/* WORDEXP */
	doio();
	return 0;
}

/*
 * Rename a layer.
 */
int
c_name(char *arg)
{
	struct layer *l;
	char *newname;

	if (arg == NULL) {
		msg(syntax);
		return 1;
	}
	if ((newname = strpbrk(arg, " \t")) != NULL) {
		while (blankchar(*newname))
			*newname++ = '\0';
		if ((l = lbyname(arg)) == NULL) {
			msg(lnotfound, arg);
			return 1;
		}
	} else {
		newname = arg;
		if ((l = lcur) == NULL) {
			msg(nocurl);
			return 1;
		}
	}
	if (invlname(newname))
		return 1;
	strcpy(l->l_name, newname);
	return 0;
}

/*
 * Invoke a subshell on the same tty as shl.
 */
int
c_bang(char *arg)
{
	if (arg == NULL)
		arg = shell;
	errstat = system(arg);
	return 0;
}

/*
 * Block layer output.
 */
int
c_block(char *arg)
{
	if (arg == NULL) {
		msg(syntax);
		return 1;
	}
	return foreach(arg, lblock);
}

/*
 * Delete a layer.
 */
int
c_delete(char *arg)
{
	if (arg == NULL) {
		msg(syntax);
		return 1;
	}
	return foreach(arg, ldelete);
}

/*
 * Print summary for all commands.
 */
/*ARGSUSED*/
int
c_help(char *arg)
{
	int i;

	/*msg("\nshl %s %s by Gunnar Ritter\n", shl_version, shl_date);*/
	for (i = 0; commands[i].c_string; i++)
		if (commands[i].c_desc)
			msg("%s", commands[i].c_desc);
	/*msg("");*/
	return 0;
}

/*
 * List current layers.
 */
int
c_layers(char *arg)
{
	int (*lfunc)(struct layer *);
	struct layer *l;

	if (arg && arg[0] == '-' && arg[1] == 'l'
			&& (arg[2] == '\0' || blankchar(arg[2]))) {
		lfunc = lpslist;
		arg += 2;
		while (blankchar(*arg))
			arg++;
		if (*arg == '\0')
			arg = NULL;
	} else
		lfunc = llist;
	if (arg)
		return foreach(arg, lfunc);
	for (l = l0; l; l = l->l_next)
		lfunc(l);
	return 0;
}

/*
 * Resume a background layer.
 */
int
c_resume(char *arg)
{
	struct layer *l;

	if (arg != NULL) {
		if ((l = lbyname(arg)) == NULL) {
			msg(lnotfound, arg);
			return 1;
		}
	} else {
		if ((l = lcur) == NULL) {
			msg(nocurl);
			return 1;
		}
	}
	return lresume(l);
}

/*
 * Resume the previously backgrounded layer.
 */
/*ARGSUSED*/
int
c_toggle(char *arg)
{
	if (lcur != NULL) {
		if (lcur->l_prev != NULL)
			setcur(lcur->l_prev);
	} else {
		msg(nocurl);
		return 1;
	}
	return lresume(lcur);
}

/*
 * Unblock a layer.
 */
int
c_unblock(char *arg)
{
	if (arg == NULL) {
		msg(syntax);
		return 1;
	}
	return foreach(arg, lunblock);
}

/*
 * Quit shl.
 */
int
c_quit(char *arg)
{
	if (arg) {
		msg(syntax);
		return 1;
	}
	if (l0 != NULL && hadquit == 0) {
		msg("warning: there may still be shl layers running");
		hadquit++;
		return 0;
	}
	quit(errstat);
	/*NOTREACHED*/
	return 0;
}

/*
 * Do nothing.
 */
/*ARGSUSED*/
int
c_null(char *arg)
{
	return 0;
}

/*
 * Read a line from standard input, without terminating newline.
 */
char *
getaline(void)
{
	cmdbuf[0] = '\n';
	cmdptr = cmdbuf;
	while (cmdbuf[0] == '\n')
		doio();
	if (cmdptr == NULL)
		return NULL;
	cmdptr = NULL;
	return cmdbuf;
}

/*
 * Prompt for command and do initial splitting.
 */
struct command *
prompt(void)
{
	static struct command pc;
	char *cmdstr = NULL, *argument;
	size_t cmdlen;
	int i;

	write(1, ">>> ", 4);
	if ((cmdstr = getaline()) == NULL) {
		pc.c_func = c_quit;
		pc.c_arg = NULL;
		return &pc;
	}
	while (blankchar(*cmdstr))
		cmdstr++;
	argument = cmdstr + strlen(cmdstr) - 1;
	while (blankchar(*argument))
		*argument-- = '\0';
	if (*cmdstr == '!' && cmdstr[1] != '\0') {
		cmdlen = 1;
		pc.c_arg = argument = &cmdstr[1];
	} else {
		if ((argument = strpbrk(cmdstr, " \t")) != NULL)
			while (blankchar(*argument))
				*argument++ = '\0';
		pc.c_arg = argument;
		cmdlen = strlen(cmdstr);
	}
	pc.c_func = c_null;
	if (cmdlen == 0)
		return &pc;
	for (i = 0; commands[i].c_string; i++) {
		if (strncmp(commands[i].c_string, cmdstr, cmdlen) == 0) {
			if (pc.c_func != c_null) {
				msg(syntax);
				pc.c_func = c_null;
				return &pc;
			}
			pc.c_func = commands[i].c_func;
		}
	}
	if (pc.c_func == c_null && *cmdstr) {
		struct layer *l;

		if ((l = lguess(cmdstr)) != NULL)
			lresume(l);
	}
	if (pc.c_func != c_quit)
		hadquit = 0;
	return &pc;
}

/*
 * SIGHUP handler.
 */
void
onhup(int signum)
{
	quit(signum | 0200);
}

/*
 * SIGWINCH handler.
 */
/*ARGSUSED*/
void
onwinch(int signum)
{
	struct layer *l;

	if (ioctl(0, TIOCGWINSZ, &winsz) != -1)
		for (l = l0; l; l = l->l_next)
			ioctl(l->l_pty, TIOCSWINSZ, &winsz);
}

/*ARGSUSED*/
int
main(int argc, char **argv)
{
	struct command *p;
	char *cp;
	char vdis;

	progname = basename(argv[0]);
	mypid = getpid();
	myuid = getuid();
	myeuid = geteuid();
	mygid = getgid();
	myegid = getegid();
	if (myuid != myeuid)
		setuid(myuid);
	if (mygid != myegid)
		setgid(mygid);
	if (getenv("SYSV3") != 0)
		sysv3 = 1;
	if ((cp = getenv("SHELL")) == NULL)
		cp = DEFSHELL;
	shell = smalloc(strlen(cp) + 1);
	strcpy(shell, cp);
	cp = basename(cp);
	dashell = smalloc(strlen(cp) + 2);
	*dashell = '-';
	strcpy(dashell + 1, cp);
	if ((oldhup = sigset(SIGHUP, SIG_IGN)) != SIG_IGN)
		sigset(SIGHUP, onhup);
	oldint = sigset(SIGINT, SIG_IGN);
	oldquit = sigset(SIGQUIT, SIG_IGN);
	oldttou = sigset(SIGTTOU, SIG_IGN);
	oldtstp = sigset(SIGTSTP, SIG_IGN);
	oldttin = sigset(SIGTTIN, SIG_IGN);
	oldwinch = sigset(SIGWINCH, onwinch);
	if (tcgetattr(0, &otio) < 0) {
		msg("not using a tty device");
		quit(1);
	}
	memcpy(&dtio, &otio, sizeof dtio);
	vdis = fpathconf(0, _PC_VDISABLE);
	if (dtio.c_cc[VSWTCH] == vdis) {
		if (dtio.c_cc[VSUSP] == DEFSWTCH) {
		msg("warning: default swtch same as susp, disabling susp");
			dtio.c_cc[VSUSP] = vdis;
		}
		dtio.c_cc[VSWTCH] = DEFSWTCH;
	}
	memcpy(&rtio, &dtio, sizeof rtio);
	rtio.c_lflag &= ~(ISIG | ICANON | ECHO | IEXTEN);
	rtio.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	rtio.c_cflag &= ~(CSIZE | PARENB);
	rtio.c_cflag |= CS8;
	rtio.c_oflag &= ~(OPOST);
	rtio.c_cc[VMIN] = 0;
	rtio.c_cc[VTIME] = 0;
	if (ioctl(0, TIOCGWINSZ, &winsz) == -1)
		winsz.ws_row = 0;
	while ((p = prompt()) != NULL)
		p->c_func(p->c_arg);
	quit(errstat);
	/*NOTREACHED*/
	return 0;
}

/*
 * Table of shl commands.
 */
struct command commands[] = {
	{ "block",	c_block,	NULL,
		"block name [name ...]", 23 },
	{ "create",	c_create,	NULL,
		"create [-[name] | name [command]]", 20 },
	{ "delete",	c_delete,	NULL,
		"delete name [name ...]", 24 },
	{ "help",	c_help,		NULL,
		"help or ?", 25 },
	{ "?",		c_help,		NULL,
		NULL, 25 },
	{ "layers",	c_layers,	NULL,
		"layers [-l] [name ...]", 26 },
	{ "name",	c_name,		NULL,
		"name [oldname] newname", 21 },
	{ "quit",	c_quit,		NULL,
		"quit", 30 },
	{ "toggle",	c_toggle,	NULL,
		"toggle", 28 },
	{ "resume",	c_resume,	NULL,
		"resume [name]", 27 },
	{ "unblock",	c_unblock,	NULL,
		"unblock name [name ...]", 29 },
	{ "!",		c_bang,		NULL,
		"! [command]", 22 },
	{ NULL,		NULL,		NULL,
		NULL, 0 }
};

#else	/* __FreeBSD__, __hpux, _AIX, __NetBSD__, __OpenBSD__, __DragonFly__,
	 __APPLE__ */

#include <stdio.h>

int
main(void)
{
	fprintf(stderr, "command not supported\n");
	return 1;
}

#endif	/* __FreeBSD__, __hpux, _AIX, __NetBSD__, __OpenBSD__, __DragonFly__,
	 __APPLE__ */
