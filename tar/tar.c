/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, March 2003.
 */
/*	from Unix 32V /usr/src/cmd/tar.c	*/
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
static const char sccsid[] USED = "@(#)tar.sl	1.177 (gritter) 4/14/07";

#include <sys/types.h>
#include <sys/stat.h>
#ifdef	__linux__
#include <linux/fd.h>
#if !defined (__UCLIBC__) && !defined (__dietlibc__)
#include <linux/fs.h>
#endif	/* !__UCLIBC__, !__dietlibc__ */
#undef	WNOHANG
#undef	WUNTRACED
#undef	P_ALL
#undef	P_PID
#undef	P_PGID
#ifdef	__dietlibc__
#undef	NR_OPEN
#undef	PATH_MAX
#endif	/* __dietlibc__ */
#endif	/* __linux__ */
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <utime.h>
#include <stdio.h>
#include <dirent.h>
#include <signal.h>
#include "sigset.h"
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <libgen.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <inttypes.h>
#include <iblok.h>
#include <locale.h>
#include <alloca.h>

#include <sys/ioctl.h>

#if defined (__linux__) || defined (__sun) || defined (__FreeBSD__) || \
	defined (__hpux) || defined (_AIX) || defined (__NetBSD__) || \
	defined (__OpenBSD__) || defined (__DragonFly__) || defined (__APPLE__)
#include <sys/mtio.h>
#else	/* SVR4.2MP */
#include <sys/scsi.h>
#include <sys/st01.h>
#endif	/* SVR4.2MP */

#ifdef	_AIX
#include <sys/sysmacros.h>
#endif

#ifndef	major
#include <sys/sysmacros.h>
#endif	/* !major */

#include <getdir.h>
#include <asciitype.h>
#include <atoll.h>
#include <memalign.h>

#ifdef	__GLIBC__
#ifdef	_IO_getc_unlocked
#undef	getc
#define	getc(f)		_IO_getc_unlocked(f)
#endif
#ifdef	_IO_putc_unlocked
#undef	putc
#define	putc(c, f)	_IO_putc_unlocked(c, f)
#endif
#endif

#if defined (__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) || \
	defined (__DragonFly__) || defined (__APPLE__)
/*
 * For whatever reason, FreeBSD casts the return values of major() and
 * minor() to signed values so that normal limit comparisons will fail.
 */
static unsigned long
mymajor(long dev)
{
	return major(dev) & 0xFFFFFFFFUL;
}
#undef	major
#define	major(a)	mymajor(a)
static unsigned long
myminor(long dev)
{
	return minor(dev) & 0xFFFFFFFFUL;
}
#undef	minor
#define	minor(a)	myminor(a)
#endif	/* __FreeBSD__, __NetBSD__, __OpenBSD__, __DragonFly__, __APPLE__ */

#define TBLOCK	512
#define	MAXBLF	(SSIZE_MAX/TBLOCK)
static int	NBLOCK = 20;
#define NAMSIZ	100
#define	PFXSIZ	155
#define	MAGSIZ	6
#define	TIMSIZ	12
static union hblock {
	char dummy[TBLOCK];
	struct header {
		char name[NAMSIZ];
		char mode[8];
		char uid[8];
		char gid[8];
		char size[12];
		char mtime[TIMSIZ];
		char chksum[8];
		char linkflag;
		char linkname[NAMSIZ];
		char magic[MAGSIZ];
		char version[2];
		char uname[32];
		char gname[32];
		char devmajor[8];
		char devminor[8];
		char prefix[PFXSIZ];
	} dbuf;
} dblock, *tbuf;

static struct internal_header {
	char	*name;
	long	namesize;
	char	*rname;
	char	*linkname;
	char	*rlinkname;
	long	linksize;
} hbuf;

static struct islot {
	struct islot	*left;	/* left link */
	struct islot 	*right;	/* right link */
	struct islot	*nextp;	/* ordered list link */
	ino_t		inum;	/* inode number */
	int		count;	/* count of links found */
	char		*pathname;/* name of first link encountered */
} *ihead;			/* ordered list for report with 'l' */
static struct islot	*inull;	/* splay tree null element */

struct dslot {
	struct dslot	*nextp;	/* next device */
	struct islot	*itree;	/* inode slots */
	dev_t		devnum;	/* real device id */
};
static struct dslot	*devices;/* devices list */

static enum paxrec {
	PR_NONE		= 0000,
	PR_ATIME	= 0001,
	PR_GID		= 0002,
	PR_LINKPATH	= 0004,
	PR_MTIME	= 0010,
	PR_PATH		= 0020,
	PR_SIZE		= 0040,
	PR_UID		= 0100,
	PR_SUN_DEVMAJOR	= 0200,
	PR_SUN_DEVMINOR	= 0400
} paxrec, globrec;

static struct stat	globst;

static int	rflag, xflag, vflag, tflag, mt, cflag, mflag, nflag, kflag;
static int	oflag, hflag, pflag, iflag, eflag, Aflag, Bflag, Eflag;
enum {
	B_UNSPEC = 0,
	B_AUTO   = 1,
	B_DEFN   = 2,
	B_USER   = 3
} bflag;
static int	gnuflag = -1;
static int	oldflag = -1;
static long long	volsize;
static char	*Fflag, *Xflag;
static int	term, chksum, wflag, recno, first, linkerrok;
static int	freemem = 1;
static int	nblock = 1;
static struct stat	mtstat;
static int	tapeblock = -1;
static int	writerror;

static int	workdir;

static off_t	low;
static off_t	high;

static FILE	*tfile;

static const char	*usefile;

struct	magtape {
	const char	*device;
	long	block;
	long long	size;
	int	nflag;
};

static struct magtape	magtapes[] = {
	{ NULL,	1,	0,	0 },
	{ NULL,	1,	0,	0 },
	{ NULL,	1,	0,	0 },
	{ NULL,	1,	0,	0 },
	{ NULL,	1,	0,	0 },
	{ NULL,	1,	0,	0 },
	{ NULL,	1,	0,	0 },
	{ NULL,	1,	0,	0 },
	{ NULL,	1,	0,	0 },
	{ NULL,	1,	0,	0 },
	{ NULL,	1,	0,	0 },
	{ NULL,	0,	0,	0 }
};

static int	hadtape;

static int	maybezip;

static enum {
	POSTORDER = 0,
	PREORDER  = 1
} order;

static char	*progname;
extern int	sysv3;

static dev_t	*vis_dev;
static ino_t	*vis_ino;
static int	vis_max;

static long	files;
static long	gotcha;

static int	midfile;

static uid_t	myuid;
static gid_t	mygid;

static long long	rdtotal;
static long long	wrtotal;

static int	N = 300;

static void	usage(void);
static void	ckusefile(void);
static void	cktxru(int);
static void	dorep(char *[]);
static void	doarg(char *);
static int	endtape(int);
static void	tgetdir(struct stat *);
static void	tgetnam(void);
static void	tgetgnu(char **, long *);
static void	tgetpax(struct stat *, long long *, long long *);
static enum paxrec	tgetrec(char **, char **, char **);
static long long	tgetval(const char *, int);
static void	passtape(struct stat *);
static void	putfile(const char *, const char *, int, int);
static void	putreg(const char *, const char *, int, struct stat *);
static int	putsym(const char *, const char *, char **, size_t);
static void	wrhdr(const char *, const char *, struct stat *);
static void	wrpax(const char *, const char *, struct stat *);
static void	addrec(char **, long *, long *,
			const char *, const char *, long long);
static void	paxnam(struct header *, const char *);
static void	doxtract(char *[]);
static int	xtrreg(const char *, struct stat *);
static int	xtrlink(const char *, struct stat *, int);
static int	xtrdev(const char *, struct stat *, mode_t);
static int	xtrdir(const char *, struct stat *);
static void	dotable(char *[]);
static void	putempty(void);
static void	longt(register struct stat *, int);
static void	pmode(register struct stat *, int);
static void	tselect(int *, struct stat *);
static int	checkdir(register char *, struct stat *);
static void	onsig(int);
static void	tomodes(const char *, register struct stat *);
static int	checksum(int);
static int	checkw(int, const char *, struct stat *, int);
static int	response(void);
static int	checkupdate(const char *, struct stat *);
static void	done(int);
static int	prefix(register const char *, register const char *);
static off_t	lookup(const char *);
static off_t	bsrch(const char *, int, off_t, off_t);
static int	cmp(const char *, const char *, size_t);
static int	readtape(char *);
static int	writetape(const char *);
static void	backtape(int);
static void	flushtape(void);
static void	*srealloc(void *, size_t);
static void	*smalloc(size_t);
static void	*scalloc(size_t, size_t);
static void	*bfalloc(size_t);
static char	*nameof(struct header *, char *);
static int	mkname(struct header *, const char *);
static char	*linkof(struct header *, char *);
static int	mklink(struct header *, const char *, const char *);
static void	blocksof(const char *, struct stat *, long long *, long long *);
static void	tchown(int (*)(const char *, uid_t, gid_t),
			const char *, uid_t, gid_t);
static void	edone(int);
static ssize_t	mtread(void *, size_t);
static ssize_t	mtwrite(const void *, size_t);
static void	newvolume(void);
static void	goback(int);
static void	getpath(const char *, char **, char **, size_t *, size_t *);
static void	setpath(const char *, char **, char **,
			size_t, size_t *, size_t *);
static void	defaults(void);
static void	settape(int);
static void	suprmsg(void);
static void	odirect(void);
static void	domtstat(void);
static int	checkzip(const char *, int);
static int	redirect(const char *, const char *, int);
static const char	*getuser(uid_t);
static const char	*getgroup(gid_t);
static char	*sstrdup(const char *);
static void	fromfile(const char *);
static void	readexcl(const char *);
static void	creatfile(void);
static mode_t	cmask(struct stat *, int);
static struct islot	*isplay(ino_t, struct islot *);
static struct islot	*ifind(ino_t, struct islot **);
static void	iput(struct islot *, struct islot **);
static struct dslot	*dfind(struct dslot **, dev_t);
static char	*sequence(void);
static void	docomp(const char *);
static int	jflag, zflag, Zflag;
static int	utf8(const char *);

int
main(int argc, char *argv[])
{
	char *cp;

	progname = basename(argv[0]);
	setlocale(LC_TIME, "");
	if (getenv("SYSV3") != NULL)
		sysv3 = 2;
	if (argc < 2)
		usage();

	if ((myuid = getuid()) != 0)
		oflag = 1;
	mygid = getgid();
	tfile = NULL;
	inull = scalloc(1, sizeof *inull);
	inull->left = inull->right = inull;
	defaults();
	argv[argc] = 0;
	argv++;
	for (cp = *argv++; *cp; cp++) 
		switch(*cp) {
		case 'f':
			usefile = *argv++;
			hadtape++;
			break;
		case 'c':
			if (sysv3 > 1 && Eflag == 0)
				oldflag = 1;
			cflag++;
			rflag++;
			cktxru('r');
			break;
		case 'X':
			Xflag = *argv++;
			if (Xflag == NULL) {
				fprintf(stderr, "%s: exclude file must be "
						"specified with 'X' option\n",
						progname);
				done(1);
			}
			creatfile();
			readexcl(Xflag);
			break;
		case 'u':
			creatfile();
			/*FALLTHRU*/
		case 'r':
			cktxru(*cp);
			rflag++;
			break;
		case 'v':
			vflag++;
			break;
		case 'w':
			wflag++;
			break;
		case 'x':
			cktxru(*cp);
			xflag++;
			break;
		case 't':
			cktxru(*cp);
			tflag++;
			break;
		case 'm':
			mflag++;
			break;
		case '-':
			break;
		case '0':
		case '1': case '2': case '3':
		case '4': case '5': case '6':
		case '7': case '8': case '9':
			settape(*cp);
			hadtape++;
			break;
		case 'b':
			bflag = B_USER;
			if (*argv == NULL)
				goto invblk;
			nblock = atoi(*argv++);
			if (nblock <= 0 || (long)nblock > MAXBLF) {
			invblk:	fprintf(stderr,
					"%s: invalid blocksize. (Max %ld)\n",
					progname, (long)MAXBLF);
				done(1);
			}
			break;
		case 'l':
			linkerrok++;
			break;
		case 'o':
			oflag++;
			break;
		case 'h': case 'L':
			hflag++;
			break;
		case 'p':
			pflag++;
			break;
		case 'i':
			iflag++;
			break;
		case 'e':
			eflag++;
			break;
		case 'A':
			Aflag++;
			break;
		case 'E':
			Eflag++;
			oldflag = -1;
			break;
		case 'F':
			Fflag = *argv++;
			if (Fflag == NULL) {
				fprintf(stderr,
					"%s: F requires a file name.\n",
					progname);
				done(1);
			}
			break;
		case 'k':
			if (*argv == NULL || (volsize = atoll(*argv)) < 250) {
				fprintf(stderr, "%s: sizes below %dk "
						"not supported\n", progname,
						250);
				done(1);
			}
			volsize *= 1024;
			kflag = 1;
			argv++;
			/*FALLTHRU*/
		case 'n':
			nflag = 2;
			break;
		case 'B':
			Bflag = 1;
			break;
		case 'z':
			zflag = 1;
			jflag = 0;
			Zflag = 0;
			break;
		case 'j':
			jflag = 1;
			zflag = 0;
			Zflag = 0;
			break;
		case 'Z':
			Zflag = 1;
			jflag = 0;
			zflag = 0;
			break;
		default:
			fprintf(stderr, "%s: %c: unknown option\n",
					progname, *cp & 0377);
			usage();
		}

	if (hadtape == 0) {
		if ((cp = getenv("TAPE")) != NULL)
			usefile = sstrdup(cp);
		else
			settape(0);
	}
	/*if (Fflag && Xflag) {
		fprintf(stderr, "%s: specify only one of X or F.\n", progname);
		usage();
	} our implementation doesn't need to enforce this */
	if ((rflag || volsize) && (workdir = open(".", O_RDONLY)) < 0) {
		fprintf(stderr, "%s: cannot open working directory\n",
				progname);
		done(1);
	}
	fcntl(workdir, F_SETFD, FD_CLOEXEC);
	for (files = 0; argv[files]; files++);
	if (rflag) {
		if (cflag && tfile != NULL && Xflag == 0) {
			usage();
			done(1);
		}
		if (argv[0] == NULL && Fflag == NULL) {
			fprintf(stderr, "%s: Missing filenames\n",
					progname);
			done(1);
		}
		if (cflag == 0 && (jflag || zflag || Zflag)) {
			fprintf(stderr, "%s: can only create "
					"compressed archives\n",
					progname);
			done(1);
		}
		ckusefile();
		if (strcmp(usefile, "-") == 0) {
			if (cflag == 0) {
				fprintf(stderr, "%s: can only create "
						"standard output archives\n",
						progname);
				done(1);
			}
			mt = dup(1);
		}
		else if ((mt = open(usefile, O_RDWR)) < 0) {
			if (cflag == 0 || (mt =  creat(usefile, 0666)) < 0) {
				fprintf(stderr, "%s: cannot open %s.\n",
						progname, usefile);
				done(1);
			}
		}
		domtstat();
		if (jflag || zflag || Zflag)
			docomp(jflag ? "bzip2" : Zflag ? "compress" : "gzip");
		dorep(argv);
	}
	else if (xflag)  {
		ckusefile();
		if (strcmp(usefile, "-") == 0) {
			mt = dup(0);
			Bflag = 1;
		} else if ((mt = open(usefile, O_RDONLY)) < 0) {
			fprintf(stderr, "%s: cannot open %s.\n",
					progname, usefile);
			done(1);
		}
		maybezip = 1;
		domtstat();
		doxtract(argv);
	}
	else if (tflag) {
		ckusefile();
		if (strcmp(usefile, "-") == 0) {
			mt = dup(0);
			Bflag = 1;
		} else if ((mt = open(usefile, O_RDONLY)) < 0) {
			fprintf(stderr, "%s: cannot open %s.\n",
					progname, usefile);
			done(1);
		}
		maybezip = 1;
		domtstat();
		dotable(argv);
	}
	else
		usage();
	done(0);
	/*NOTREACHED*/
	return 0;
}

static void
usage(void)
{
	fprintf(stderr, "Usage: %s -{txruc}[0-9vfbk[FX]hLiBelmnopwA] "
			"[tapefile] [blocksize] [tapesize] [argfile] "
			"[exclude-file] [-I include-file] files ...\n",
		progname);
	done(1);
}

static void
ckusefile(void)
{
	if (usefile == NULL) {
		fprintf(stderr, "%s: device argument required\n", progname);
		done(1);
	}
}

static void
cktxru(int c)
{
	static int	txruflag;

	if (c == 't' || c == 'x' || c == 'r' || c == 'u') {
		if (txruflag) {
			fprintf(stderr, "%s: specify only one of [txru].\n",
					progname);
			usage();
		}
		txruflag = c;
	}
}

static void
dorep(char *argv[])
{
	struct stat	stbuf;

	if (!cflag || Xflag) {
		if (!cflag) {
			tgetdir(&stbuf);
			do {
				passtape(&stbuf);
				tgetdir(&stbuf);
			} while (!endtape(1));
		}
		if (tfile != NULL) {
			char tname[] = "/tmp/tarXXXXXX";
			int tfd;
			pid_t pid;
			fflush(tfile);
			rewind(tfile);
			if ((tfd = mkstemp(tname)) < 0) {
				fprintf(stderr, "%s: cannot create temporary "
						"file (%s)\n", progname, tname);
				done(1);
			}
			unlink(tname);
			fcntl(tfd, F_SETFD, FD_CLOEXEC);
			switch (pid = fork()) {
			case -1:
				fprintf(stderr, "%s: cannot fork\n", progname);
				done(1);
				/*NOTREACHED*/
			case 0:
				dup2(fileno(tfile), 0);
				dup2(tfd, 1);
				execl(SHELL, "sh", "-c",
					"PATH=" SV3BIN ":" DEFBIN ":$PATH; "
					"LC_ALL=C export LC_ALL; "
					/*
					 * +2 sorts by file name first, for
					 * binary search.
					 * +0 sorts by key (X overrides u).
					 * +1nr sorts by modtime (newer files
					 * first).
					 */
					"sort +2 +0 -1 +1nr -2 | uniq -2",
					NULL);
				fprintf(stderr, "%s: cannot execute %s\n",
						progname, SHELL);
				_exit(0177);
			}
			while (waitpid(pid, NULL, 0) != pid);
			fclose(tfile);
			lseek(tfd, 0, SEEK_SET);
			tfile = fdopen(tfd, "r");
			fstat(fileno(tfile), &stbuf);
			high = stbuf.st_size;
		}
	}

	suprmsg();
	if (sigset(SIGHUP, SIG_IGN) != SIG_IGN)
		sigset(SIGINT, onsig);
	if (sigset(SIGHUP, SIG_IGN) != SIG_IGN)
		sigset(SIGHUP, onsig);
	if (sigset(SIGQUIT, SIG_IGN) != SIG_IGN)
		sigset(SIGQUIT, onsig);
/*
	if (sigset(SIGTERM, SIG_IGN) != SIG_IGN)
		sigset(SIGTERM, onsig);
*/
	odirect();
	if (Fflag)
		fromfile(Fflag);
	while (*argv && !term) {
		if (argv[0][0] == '-' && argv[0][1] == 'C' &&
				argv[0][2] == '\0' && argv[1]) {
			if (chdir(argv[1]) < 0)
				fprintf(stderr, "%s: can't change directories "
						"to %s: %s\n",
					progname, argv[0],
					strerror(errno));
			argv += 2;
			if (argv[0] == NULL)
				break;
		}
		if (argv[0][0] == '-' && argv[0][1] == 'I' &&
				argv[0][2] == '\0') {
			if (argv[1]) {
				fromfile(argv[1]);
				argv += 2;
			} else {
				fprintf(stderr, "%s: missing file name "
						"for -I flag.\n",
					progname);
				done(1);
			}
		} else {
			doarg(*argv++);
			goback(workdir);
		}
	}
	putempty();
	putempty();
	flushtape();
	if (linkerrok == 1)
		for (; ihead != NULL; ihead = ihead->nextp)
			if (ihead->count != 0)
				fprintf(stderr, "Missing links to %s\n",
						ihead->pathname);
}

static void
doarg(char *arg)
{
	register char *cp, *cp2;

	cp2 = arg;
	for (cp = arg; *cp; cp++)
		if (*cp == '/')
			cp2 = cp;
	if (cp2 != arg) {
		*cp2 = '\0';
		chdir(arg);
		*cp2 = '/';
		cp2++;
	}
	putfile(arg, cp2, workdir, 0);
}

static int
endtape(int rew)
{
	if (hbuf.name[0] == '\0') {
		if (rew)
			backtape(rew);
		return(1);
	}
	else
		return(0);
}

static void
tgetdir(register struct stat *sp)
{
	long long	lval1, lval2;

	readtape( (char *) &dblock);
	if (dblock.dbuf.name[0] && gnuflag < 0)
		if ((gnuflag=memcmp(dblock.dbuf.magic, "ustar  \0", 8)==0)!=0)
			Eflag = 0;
	if (dblock.dbuf.name[0] && oldflag < 0)
		if ((oldflag=memcmp(dblock.dbuf.magic, "ustar", 5)!=0)!=0)
			Eflag = 0;
	if (hbuf.name)
		hbuf.name[0] = '\0';
	if (hbuf.linkname)
		hbuf.linkname[0] = '\0';
	paxrec = globrec;
	*sp = globst;
	while (gnuflag==0 && oldflag==0 && (dblock.dbuf.linkflag == 'x' ||
				dblock.dbuf.linkflag == 'g' ||
				dblock.dbuf.linkflag == 'X' /* sun */))
		tgetpax(sp, &lval1, &lval2);
	tgetnam();
	if (hbuf.name[0] == '\0')
		return;
	sp->st_mode = tgetval(dblock.dbuf.mode, sizeof dblock.dbuf.mode)&07777;
	if ((paxrec & PR_UID) == 0)
		sp->st_uid = tgetval(dblock.dbuf.uid, sizeof dblock.dbuf.uid);
	if ((paxrec & PR_GID) == 0)
		sp->st_gid = tgetval(dblock.dbuf.gid, sizeof dblock.dbuf.gid);
	if ((paxrec & PR_SIZE) == 0)
		sp->st_size =
			tgetval(dblock.dbuf.size, sizeof dblock.dbuf.size);
	if ((paxrec & PR_MTIME) == 0)
		sp->st_mtime =
			tgetval(dblock.dbuf.mtime, sizeof dblock.dbuf.mtime);
	sscanf(dblock.dbuf.chksum, "%o", &chksum);
	if (chksum != checksum(0) && chksum != checksum(1)) {
		fprintf(stderr, "%s: directory checksum error\n", progname);
		if (iflag == 0)
			done(2);
	}
	if ((paxrec & PR_SUN_DEVMAJOR) == 0)
		sscanf(dblock.dbuf.devmajor, "%llo", &lval1);
	if ((paxrec & PR_SUN_DEVMINOR) == 0)
		sscanf(dblock.dbuf.devminor, "%llo", &lval2);
	sp->st_rdev = makedev(lval1, lval2);
	if (tfile != NULL) {
		if (strchr(hbuf.name, '\n') == NULL) {
			int	s;
			s = 3 * fprintf(tfile, "u %0*lo %s\n", TIMSIZ,
					(long)sp->st_mtime, hbuf.name);
			if (s > N)
				N = s;
		} else
			fprintf(stderr, "%s: warning: file name '%s' in "
					"archive contains a newline character "
					"and will always be added to archive\n",
				progname, hbuf.name);
	}
}

static void
tgetnam(void)
{
again:	if (dblock.dbuf.linkflag == 'L' && (gnuflag>0 ||
			strcmp(dblock.dbuf.name, "././@LongLink") == 0)) {
		tgetgnu(&hbuf.name, &hbuf.namesize);
		goto again;
	}
	if (dblock.dbuf.linkflag == 'K' && (gnuflag>0 ||
			strcmp(dblock.dbuf.name, "././@LongLink") == 0)) {
		tgetgnu(&hbuf.linkname, &hbuf.linksize);
		goto again;
	}
	if ((hbuf.name == NULL || hbuf.namesize < NAMSIZ+PFXSIZ+2) &&
			(paxrec & PR_PATH) == 0) {
		hbuf.namesize = NAMSIZ+PFXSIZ+2;
		hbuf.name = srealloc(hbuf.name, hbuf.namesize);
		hbuf.name[0] = '\0';
	}
	if ((hbuf.linkname == NULL || hbuf.linksize < NAMSIZ+1) &&
			(paxrec & PR_LINKPATH) == 0) {
		hbuf.linksize = NAMSIZ+1;
		hbuf.linkname = srealloc(hbuf.linkname, hbuf.linksize);
		hbuf.linkname[0] = '\0';
	}
	if (hbuf.name[0] == '\0' && (paxrec & PR_PATH) == 0)
		nameof(&dblock.dbuf, hbuf.name);
	if (hbuf.linkname[0] == '\0' && (paxrec & PR_LINKPATH) == 0)
		linkof(&dblock.dbuf, hbuf.linkname);
	hbuf.rname = hbuf.name;
	if (Aflag) {
		while (hbuf.rname[0] == '/')
			hbuf.rname++;
		if (hbuf.name[0] && hbuf.rname[0] == '\0')
			hbuf.rname = ".";
	}
	hbuf.rlinkname = hbuf.linkname;
	if (Aflag) {
		while (hbuf.rlinkname[0] == '/')
			hbuf.rlinkname++;
		if (hbuf.linkname[0] && hbuf.rlinkname[0] == '\0')
			hbuf.rlinkname = ".";
	}
}

static void
tgetgnu(char **np, long *sp)
{
	char	buf[TBLOCK];
	long long	blocks;
	long	n, bytes;

	n = tgetval(dblock.dbuf.size, sizeof dblock.dbuf.size);
	if (*sp <= n)
		*np = srealloc(*np, *sp = n+1);
	blocks = n;
	blocks += TBLOCK-1;
	blocks /= TBLOCK;
	bytes = n;
	while (blocks-- > 0) {
		readtape(buf);
		memcpy(&(*np)[n-bytes], buf, bytes>TBLOCK?TBLOCK:bytes);
		bytes -= TBLOCK;
	}
	(*np)[n] = '\0';
	readtape((char *)&dblock);
}

static void
tgetpax(struct stat *sp, long long *devmajor, long long *devminor)
{
	char	*keyword, *value;
	char	buf[TBLOCK];
	char	*block, *bp;
	long long	n, blocks, bytes;
	enum paxrec	pr;

	n = tgetval(dblock.dbuf.size, sizeof dblock.dbuf.size);
	bp = block = smalloc(n+1);
	blocks = n;
	blocks += TBLOCK-1;
	blocks /= TBLOCK;
	bytes = n;
	while (blocks-- > 0) {
		readtape(buf);
		memcpy(&block[n-bytes], buf, bytes>TBLOCK?TBLOCK:bytes);
		bytes -= TBLOCK;
	}
	block[n] = '\0';
	while (bp < &block[n]) {
		int	c;
		pr = tgetrec(&bp, &keyword, &value);
		switch (pr) {
		case PR_ATIME:
			sp->st_atime = strtoll(value, NULL, 10);
			break;
		case PR_GID:
			sp->st_gid = strtoll(value, NULL, 10);
			break;
		case PR_LINKPATH:
			c = strlen(value);
			if (hbuf.linkname == NULL || hbuf.linksize < c+1) {
				hbuf.linksize = c+1;
				hbuf.linkname = srealloc(hbuf.linkname, c+1);
			}
			strcpy(hbuf.linkname, value);
			break;
		case PR_MTIME:
			sp->st_mtime = strtoll(value, NULL, 10);
			break;
		case PR_PATH:
			c = strlen(value);
			if (hbuf.name == NULL || hbuf.namesize < c+1) {
				hbuf.namesize = c+1;
				hbuf.name = srealloc(hbuf.name, c+1);
			}
			strcpy(hbuf.name, value);
			break;
		case PR_SIZE:
			sp->st_size = strtoll(value, NULL, 10);
			break;
		case PR_UID:
			sp->st_uid = strtoll(value, NULL, 10);
			break;
		case PR_SUN_DEVMAJOR:
			*devmajor = strtoll(value, NULL, 10);
			break;
		case PR_SUN_DEVMINOR:
			*devminor = strtoll(value, NULL, 10);
			break;
		}
		paxrec |= pr;
	}
	if (dblock.dbuf.linkflag == 'g') {
		globrec = paxrec & ~(PR_LINKPATH|PR_PATH|PR_SIZE);
		globst = *sp;
	} else if (dblock.dbuf.linkflag == 'X')
		Eflag = 1;
	readtape((char *)&dblock);
	free(block);
}

static enum paxrec
tgetrec(char **bp, char **keyword, char **value)
{
	char	*x;
	long	n = 0;
	enum paxrec	pr;

	*keyword = "";
	*value = "";
	while (**bp && (n = strtol(*bp, &x, 10)) <= 0 && (*x!=' ' || *x!='\t'))
		do
			(*bp)++;
		while (**bp && **bp != '\n');
	if (*x == '\0' || **bp == '\0') {
		(*bp)++;
		return PR_NONE;
	}
	while (x < &(*bp)[n] && (*x == ' ' || *x == '\t'))
		x++;
	if (x == &(*bp)[n] || *x == '=')
		goto out;
	*keyword = x;
	while (x < &(*bp)[n] && *x != '=')
		x++;
	if (x == &(*bp)[n])
		goto out;
	*x = '\0';
	if (&x[1] < &(*bp)[n])
		*value = &x[1];
	(*bp)[n-1] = '\0';
out:	*bp = &(*bp)[n];
	if (strcmp(*keyword, "atime") == 0)
		pr = PR_ATIME;
	else if (strcmp(*keyword, "gid") == 0)
		pr = PR_GID;
	else if (strcmp(*keyword, "linkpath") == 0)
		pr = PR_LINKPATH;
	else if (strcmp(*keyword, "mtime") == 0)
		pr = PR_MTIME;
	else if (strcmp(*keyword, "path") == 0)
		pr = PR_PATH;
	else if (strcmp(*keyword, "size") == 0)
		pr = PR_SIZE;
	else if (strcmp(*keyword, "uid") == 0)
		pr = PR_UID;
	else if (strcmp(*keyword, "SUN.devmajor") == 0)
		pr = PR_SUN_DEVMAJOR;
	else if (strcmp(*keyword, "SUN.devminor") == 0)
		pr = PR_SUN_DEVMINOR;
	else
		pr = PR_NONE;
	return pr;
}

static long long
tgetval(const char *s, int k)
{
	long long	n = 0;
	int	i, h = 0;

	if (gnuflag>0 && s[0] & 0200) {
		for (i = k-1; i > 0; i--) {
			n += (s[i]&0377) << h;
			h += 8;
		}
		n += (s[0]&0177) << h;
	} else {
		i = 0;
		while (spacechar(s[i]&0377))
			i++;
		while (i < k && s[i] && !spacechar(s[i]&0377)) {
			n *= 8;
			n += s[i++]-'0';
		}
	}
	return n;
}

static void
passtape(register struct stat *sp)
{
	long long blocks;
	char buf[TBLOCK];

	switch (dblock.dbuf.linkflag) {
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
		break;
	case '1':
		if (oldflag > 0 || gnuflag > 0)
			break;
		/*FALLTHRU*/
	default:
		blocks = sp->st_size;
		blocks += TBLOCK-1;
		blocks /= TBLOCK;

		while (blocks-- > 0) {
			if (((mtstat.st_mode&S_IFMT) == S_IFBLK ||
					(mtstat.st_mode&S_IFMT) == S_IFREG) &&
					recno >= nblock && blocks >= nblock) {
				long long	lpos;
				lpos = (blocks/nblock)*nblock*TBLOCK;
				if ((volsize == 0 || rdtotal+lpos < volsize) &&
						lseek(mt, lpos, SEEK_CUR)
						!= (off_t)-1) {
					blocks %= nblock;
					rdtotal += lpos;
				}
			}
			readtape(buf);
		}
	}
}

static void
putfile(const char *longname, const char *shortname, int olddir, int vis_cnt)
{
	struct stat	stbuf;
	int infile = -1;
	char *copy = NULL, *cend;
	size_t	sz, slen, ss;
	struct direc	*dp;
	struct getdb	*db;
	int i, j;
	int skip = 0;
	char	*symblink = 0;

	paxrec = globrec;
	if ((hflag ? stat : lstat)(shortname, &stbuf) < 0) {
		fprintf(stderr, "%s: could not stat %s\n", progname, longname);
		edone(1);
		return;
	}
	if ((stbuf.st_mode&S_IFMT) == S_IFREG &&
			stbuf.st_dev == mtstat.st_dev &&
			stbuf.st_ino == mtstat.st_ino) {
		fprintf(stderr, "%s: %s same as archive file\n",
				progname, longname);
		return;
	}
	if ((stbuf.st_mode&S_IFMT) == S_IFREG &&
			stbuf.st_size > 077777777777LL) {
		if (gnuflag > 0 || oldflag > 0) {
			fprintf(stderr, "%s: %s too large (limit 8 GB)\n",
					progname, longname);
			return;
		}
		paxrec |= PR_SIZE;
	}
	if (sysv3 && eflag && (stbuf.st_mode&S_IFMT) == S_IFREG &&
			stbuf.st_size > volsize - wrtotal - 512) {
		fprintf(stderr, "%s: Single file cannot fit on volume\n",
				progname);
		done(3);
	}
	if ((stbuf.st_mode&S_IFMT) == S_IFREG ||
			(stbuf.st_mode&S_IFMT) == S_IFDIR) {
		infile = open(shortname, O_RDONLY);
		if (infile < 0) {
			fprintf(stderr, "%s: %s: cannot open file\n",
					progname, longname);
			edone(1);
			return;
		}
	}

	if (tfile != NULL && (i = checkupdate(longname, &stbuf)) != '\0') {
		if (infile >= 0 && (stbuf.st_mode & S_IFMT) == S_IFDIR &&
				i != 'X')
			skip = 1;
		else {
			if (i == 'X' && vflag)
				fprintf(stderr, "a %s%s excluded\n", longname,
					(stbuf.st_mode&S_IFMT) == S_IFDIR ?
						"/" : "");
			goto ret;
		}
	}
	if (!skip && checkw('r', shortname, &stbuf, -1) == 0)
		goto ret;

	if ((stbuf.st_mode & S_IFMT) == S_IFDIR && infile >= 0) {
		if (order == PREORDER)
			goto out;
	cont:	if (hflag) {
			for (i = 0; i < vis_cnt; i++)
				if (stbuf.st_dev == vis_dev[i] &&
						stbuf.st_ino == vis_ino[i])
					goto fini;
			if (vis_cnt >= vis_max) {
				vis_max += 20;
				vis_dev = srealloc(vis_dev, sizeof *vis_dev *
						vis_max);
				vis_ino = srealloc(vis_ino, sizeof *vis_ino *
						vis_max);
			}
			vis_dev[vis_cnt] = stbuf.st_dev;
			vis_ino[vis_cnt] = stbuf.st_ino;
			vis_cnt++;
		}
		getpath(longname, &copy, &cend, &sz, &slen);
		if (fchdir(infile) < 0) {
			fprintf(stderr, "can't change directories to %s\n",
					shortname);
			goto fini;
		}
		db = getdb_alloc(shortname, infile);
		while ((dp = getdir(db, &j)) != NULL && !term) {
			if (dp->d_ino == 0)
				continue;
			if (strcmp(".", dp->d_name) == 0 ||
					strcmp("..", dp->d_name) == 0)
				continue;
			setpath(dp->d_name, &copy, &cend, slen, &sz, &ss);
			putfile(copy, dp->d_name, infile, vis_cnt);
			i++;
		}
		getdb_free(db);
		if (fchdir(olddir) < 0) {
			fprintf(stderr, "cannot change back?: %s\n",
					strerror(errno));
			done(1);
		}
	fini:	free(copy);
		if (order == PREORDER)
			goto ret;
	}
out:
	tomodes(longname, &stbuf);

	if (skip || mkname(&dblock.dbuf, longname) < 0)
		goto ret;

	if (stbuf.st_nlink > 1 && (stbuf.st_mode&S_IFMT) != S_IFDIR) {
		struct dslot *dp;
		struct islot *ip;

		dp = dfind(&devices, stbuf.st_dev);
		if ((ip = ifind(stbuf.st_ino, &dp->itree)) != NULL) {
			ip->count--;
			sprintf(dblock.dbuf.size, "%11.11o", 0);
			if (mklink(&dblock.dbuf, ip->pathname, longname) < 0)
				goto ret;
			dblock.dbuf.linkflag = '1';
			sprintf(dblock.dbuf.chksum, "%7.7o", checksum(0));
			if (paxrec != PR_NONE && oldflag <= 0 && gnuflag <= 0)
				wrpax(longname, ip->pathname, &stbuf);
			writetape( (char *) &dblock);
			if (vflag)
				fprintf(stderr, "a %s link to %s\n",
						longname, ip->pathname);
			goto ret;
		}
		else {
			int	namelen = strlen(longname);
			ip = calloc(1, sizeof *ip);
			if (ip == 0 || (ip->pathname=malloc(namelen+1)) == 0) {
				if (freemem) {
					write(2, "Out of memory. "
						"Link information lost\n", 37);
					freemem = 0;
					edone(1);
				}
			} else {
				ip->nextp = ihead;
				ihead = ip;
				ip->inum = stbuf.st_ino;
				ip->count = stbuf.st_nlink - 1;
				strcpy(ip->pathname, longname);
				iput(ip, &dp->itree);
			}
		}
	}

	switch (stbuf.st_mode & S_IFMT) {
	default:
	def:	fprintf(stderr, "%s: %s is not a file. Not dumped\n",
				progname, longname);
		edone(1);
		goto ret;
	case S_IFREG:
		dblock.dbuf.linkflag = '0';
		wrhdr(longname, NULL, &stbuf);
		putreg(longname, shortname, infile, &stbuf);
		goto ret;
	case S_IFLNK:
		if (putsym(longname, shortname, &symblink, stbuf.st_size) < 0)
			goto ret;
		dblock.dbuf.linkflag = '2';
		break;
	case S_IFCHR:
		if (oldflag > 0)
			goto def;
		dblock.dbuf.linkflag = '3';
		break;
	case S_IFBLK:
		if (oldflag > 0)
			goto def;
		dblock.dbuf.linkflag = '4';
		break;
	case S_IFDIR:
		if (oldflag > 0)
			goto nop;
		dblock.dbuf.linkflag = '5';
		break;
	case S_IFIFO:
		if (oldflag > 0)
			goto def;
		dblock.dbuf.linkflag = '6';
		break;
	}
	wrhdr(longname, symblink, &stbuf);
	free(symblink);
nop:	if (order == PREORDER && infile >= 0 && (stbuf.st_mode&S_IFMT)==S_IFDIR)
		goto cont;
ret:	if (infile >= 0)
		close(infile);
}

static void
putreg(const char *longname, const char *shortname, int infile, struct stat *sp)
{
	long long blocks;
	char buf[TBLOCK];
	int i;

	blocks = (sp->st_size + (TBLOCK-1)) / TBLOCK;
	midfile = 1;
	while ((i = read(infile, buf, TBLOCK)) > 0 && blocks > 0) {
		if (i < TBLOCK)
			memset(&buf[i], 0, TBLOCK - i);
		writetape(buf);
		blocks--;
	}
	close(infile);
	midfile = 0;
	if (blocks != 0 || i != 0) {
		fprintf(stderr, "%s: file changed size\n", longname);
		edone(1);
	}
	while (blocks-- >  0)
		putempty();
}

static int
putsym(const char *longname, const char *shortname,
		char **symblink, size_t size)
{
	size_t	n;
	ssize_t	len;

	n = size ? size : PATH_MAX;
	*symblink = smalloc(n+1);
	if ((len = readlink(shortname, *symblink, n)) < 0) {
		fprintf(stderr, "can't read symbolic link %s\n", longname);
		edone(1);
		return -1;
	}
	(*symblink)[len] = '\0';
	if (len >= 100) {
		if (oldflag <= 0 && gnuflag <= 0 && utf8(*symblink)) {
			paxrec |= PR_LINKPATH;
			strcpy(dblock.dbuf.linkname, sequence());
			return 0;
		}
		fprintf(stderr, "%s: %s: symbolic link too long\n",
				progname, longname);
		edone(1);
		return -1;
	}
	memcpy(dblock.dbuf.linkname, *symblink, len);
	if (len < NAMSIZ)
		dblock.dbuf.linkname[len] = '\0';
	return 0;
}

static void
wrhdr(const char *longname, const char *symblink, struct stat *sp)
{
	long long blocks;

	blocks = (sp->st_mode&S_IFMT) == S_IFREG ?
			(sp->st_size + (TBLOCK-1)) / TBLOCK : 0;
	if (vflag) {
		if (nflag)
			fprintf(stderr, "seek = %lldK\t",
					(wrtotal+recno*2+1023)/1024);
		fprintf(stderr, "a %s%s ", longname,
				(sp->st_mode&S_IFMT) == S_IFDIR ? "/" : "");
		if (symblink)
			fprintf(stderr, "symbolic link to %s\n", symblink);
		else if (nflag)
			fprintf(stderr, "%lldK\n",
					blocks&01?blocks|02:blocks>>1);
		else
			fprintf(stderr, "%lld tape blocks\n", blocks);
	}
	sprintf(dblock.dbuf.chksum, "%7.7o", checksum(0));
	if (paxrec != PR_NONE && oldflag <= 0 && gnuflag <= 0)
		wrpax(longname, symblink, sp);
	writetape( (char *) &dblock);
}

static void
wrpax(const char *longname, const char *linkname, struct stat *sp)
{
	char	oblock[TBLOCK];
	char	*pdata = NULL;
	long	psize = 0, pcur = 0;
	long long	blocks, i;

	memcpy(oblock, (char *)&dblock, TBLOCK);
	memset((char *)&dblock, 0, TBLOCK);
	if (paxrec & PR_ATIME)
		addrec(&pdata, &psize, &pcur, "atime", NULL, sp->st_atime);
	if (paxrec & PR_GID)
		addrec(&pdata, &psize, &pcur, "gid", NULL, sp->st_gid);
	if (paxrec & PR_LINKPATH)
		addrec(&pdata, &psize, &pcur, "linkpath", linkname, 0);
	if (paxrec & PR_MTIME)
		addrec(&pdata, &psize, &pcur, "mtime", NULL, sp->st_mtime);
	if (paxrec & PR_PATH)
		addrec(&pdata, &psize, &pcur, "path", longname, 0);
	if (paxrec & PR_SIZE)
		addrec(&pdata, &psize, &pcur, "size", NULL, sp->st_size);
	if (paxrec & PR_UID)
		addrec(&pdata, &psize, &pcur, "uid", NULL, sp->st_uid);
	if (paxrec & PR_SUN_DEVMAJOR)
		addrec(&pdata, &psize, &pcur, "SUN.devmajor", NULL,
				major(sp->st_rdev));
	if (paxrec & PR_SUN_DEVMINOR)
		addrec(&pdata, &psize, &pcur, "SUN.devminor", NULL,
				minor(sp->st_rdev));
	paxnam(&dblock.dbuf, longname);
	sprintf(dblock.dbuf.mode, "%7.7o", 0444);
	sprintf(dblock.dbuf.uid, "%7.7o", 0);
	sprintf(dblock.dbuf.gid, "%7.7o", 0);
	sprintf(dblock.dbuf.size, "%11.11lo", pcur);
	sprintf(dblock.dbuf.mtime, "%11.11o", 0);
	strcpy(dblock.dbuf.magic, "ustar");
	dblock.dbuf.version[0] = dblock.dbuf.version[1] = '0';
	strcpy(dblock.dbuf.uname, "root");
	strcpy(dblock.dbuf.gname, "root");
	dblock.dbuf.linkflag = Eflag ? 'X' : 'x';
	sprintf(dblock.dbuf.chksum, "%7.7o", checksum(0));
	writetape( (char *) &dblock);
	memset(&pdata[pcur], 0, psize - pcur);
	blocks = (pcur + (TBLOCK-1)) / TBLOCK;
	for (i = 0; i < blocks; i++)
		writetape(&pdata[i*TBLOCK]);
	memcpy((char *)&dblock, oblock, TBLOCK);
	free(pdata);
}

static void
addrec(char **pdata, long *psize, long *pcur,
		const char *keyword, const char *sval, long long lval)
{
	char	dval[25], xval[25];
	long	od, d, r;

	if (sval == 0) {
		sprintf(xval, "%lld", lval);
		sval = xval;
	}
	r = strlen(keyword) + strlen(sval) + 3;
	d = 0;
	do {
		od = d;
		sprintf(dval, "%ld", od + r);
		d = strlen(dval);
	} while (d != od);
	*psize += d + r + 1 + 512;
	*pdata = srealloc(*pdata, *psize);
	sprintf(&(*pdata)[*pcur], "%s %s=%s\n", dval, keyword, sval);
	*pcur += d + r;
}

static void
paxnam(struct header *hp, const char *name)
{
	char	buf[257], *bp;
	const char	*cp, *np;
	int	bl = 0;
	static int	pid;

	if (pid == 0)
		pid = getpid();
	for (np = name; *np; np++);
	while (np > name && *np != '/') {
		np--;
		bl++;
	}
	if ((np > name || *name == '/') && np-name <= 120)
		for (bp = buf, cp = name; cp < np; bp++, cp++)
			*bp = *cp;
	else {
		*buf = '.';
		bp = &buf[1];
	}
	snprintf(bp, sizeof buf - (bp - buf), "/PaxHeaders.%d/%s",
			pid, bl < 100 ? np>name?&np[1]:name : sequence());
	mkname(hp, buf);
}

static void
doxtract(char *argv[])
{
	struct stat	stbuf;
	char *name;
	char **cp;
	int	try;

	suprmsg();
	for (;;) {
		try = 0;
		tgetdir(&stbuf);
		if (endtape(0))
			break;

		name = hbuf.rname;
		if (*argv == 0)
			goto gotit;

		for (cp = argv; *cp; cp++)
			if (prefix(*cp, name)) {
				try = 1;
				goto gotit;
			}
		passtape(&stbuf);
		continue;

gotit:
		if (checkw('x', name, &stbuf, dblock.dbuf.linkflag) == 0) {
			passtape(&stbuf);
			continue;
		}

		if (checkdir(name, &stbuf) == '/')
			goto dir;

		switch (dblock.dbuf.linkflag) {
		default:
		case '0':
		case '\0':
			if (xtrreg(name, &stbuf) < 0)
				continue;
			break;
		case '1':
			if (xtrlink(name, &stbuf, 0) == 0)
				passtape(&stbuf);
			else if (stbuf.st_size > 0)
				xtrreg(name, &stbuf);
			continue;
		case '2':
			xtrlink(name, &stbuf, 1);
			continue;
		case '3':
			if (xtrdev(name, &stbuf, S_IFCHR) < 0)
				continue;
			break;
		case '4':
			if (xtrdev(name, &stbuf, S_IFBLK) < 0)
				continue;
			break;
		case '5':
		dir:	if (xtrdir(name, &stbuf) < 0)
				continue;
			break;
		case '6':
			if (xtrdev(name, &stbuf, S_IFIFO) < 0)
				continue;
			break;
		}
		if (pflag)
			chmod(name, stbuf.st_mode & cmask(&stbuf, 0));
		if (mflag == 0) {
			struct utimbuf timep;

			if (paxrec & PR_ATIME)
				timep.actime = stbuf.st_atime;
			else
				timep.actime = time(NULL);
			timep.modtime = stbuf.st_mtime;
			if (utime(name, &timep) < 0)
				fprintf(stderr, "can't set time on %s\n", name);
		}
		gotcha += try;
	}
	if (gotcha < files)
		fprintf(stderr, "%s: %ld file(s) not extracted\n",
				progname, files - gotcha);
}

static int
xtrreg(const char *name, struct stat *sp)
{
	long long blocks, bytes;
	char buf[TBLOCK];
	int ofile;

	remove(name);
	if ((ofile = creat(name, sp->st_mode & cmask(sp, 1))) < 0) {
		fprintf(stderr, "%s: %s - cannot create\n", progname, name);
		edone(1);
		passtape(sp);
		return -1;
	}
	tchown(chown, name, sp->st_uid, sp->st_gid);
	blocksof(name, sp, &blocks, &bytes);
	while (blocks-- > 0) {
		readtape(buf);
		if (bytes > TBLOCK) {
			if (write(ofile, buf, TBLOCK) < 0) {
				fprintf(stderr,
					"%s: %s: HELP - extract write error\n",
					progname, name);
				done(2);
			}
		} else
			if (write(ofile, buf, (int) bytes) < 0) {
				fprintf(stderr,
					"%s: %s: HELP - extract write error\n",
					progname, name);
				done(2);
			}
		bytes -= TBLOCK;
	}
	close(ofile);
	return 0;
}

static int
xtrlink(const char *name, struct stat *sp, int symbolic)
{
	remove(name);
	if ((symbolic?symlink:link)(symbolic?hbuf.linkname:hbuf.rlinkname,
				name) < 0) {
		if (symbolic)
			fprintf(stderr, "%s: symbolic link failed\n", name);
		else
			fprintf(stderr, "%s: %s: cannot link\n",
					progname, name);
		edone(1);
		return -1;
	}
	if (vflag)
		fprintf(stderr, "%s %s %s\n", name,
				symbolic ? "symbolic link to" : "linked to",
				hbuf.linkname);
	if (symbolic)
		tchown(lchown, name, sp->st_uid, sp->st_gid);
	return 0;
}

static int
xtrdev(const char *name, struct stat *sp, mode_t type)
{
	remove(name);
	if (mknod(name, sp->st_mode&cmask(sp, 1) | type, sp->st_rdev) < 0) {
		fprintf(stderr, "Can't create special %s\n", name);
		edone(1);
		return -1;
	}
	tchown(chown, name, sp->st_uid, sp->st_gid);
	return 0;
}

static int
xtrdir(const char *name, struct stat *sp)
{
	remove(name);
	if (mkdir(name, sp->st_mode&cmask(sp, 1)|0700) < 0 && errno != EEXIST) {
		fprintf(stderr, "%s: %s: %s\n", progname, name,
				strerror(errno));
		edone(1);
		return -1;
	}
	tchown(chown, name, sp->st_uid, sp->st_gid);
	return 0;
}

static void
blocksof(const char *name, struct stat *sp, long long *blocks, long long *bytes)
{
	*blocks = ((*bytes = sp->st_size) + TBLOCK-1)/TBLOCK;
	if (vflag)
		fprintf(stderr, "x %s, %lld bytes, ""%lld%s\n",
			name, *bytes,
			nflag ? (*blocks&01?*blocks|02:*blocks)>>1 : *blocks,
			nflag ? "K" : " tape blocks");
}

static void
tchown(int (*chfn)(const char *, uid_t, gid_t),
		const char *name, uid_t uid, gid_t gid)
{
	if (oflag == 0) {
		if (chfn(name, uid, -1) < 0)
			fprintf(stderr, "%s: %s: owner not changed\n",
					progname, name);
		if (chfn(name, -1, gid) < 0)
			fprintf(stderr, "%s: %s: group not changed\n",
					progname, name);
	}
}

static void
dotable(char *argv[])
{
	struct stat	stbuf;
	char	**cp;
	char	*name;

	for (;;) {
		tgetdir(&stbuf);
		if (endtape(0))
			break;
		name = hbuf.name;
		if (*argv == 0)
			goto yes;
		for (cp = argv; *cp; cp++)
			if (prefix(*cp, name)) {
				gotcha++;
				goto yes;
			}
		goto no;
	yes:	if (vflag)
			longt(&stbuf, dblock.dbuf.linkflag);
		printf("%s", name);
		if (dblock.dbuf.linkflag == '1')
			printf(" linked to %s", hbuf.linkname);
		else if (dblock.dbuf.linkflag == '2')
			printf(" symbolic link to %s", hbuf.linkname);
		printf("\n");
	no:	passtape(&stbuf);
	}
	if (gotcha < files)
		fprintf(stderr, "%s: %ld file(s) not found\n",
				progname, files - gotcha);
}

static void
putempty(void)
{
	char buf[TBLOCK];

	memset(buf, 0, sizeof buf);
	writetape(buf);
}

static void
longt(register struct stat *st, int linkflag)
{
	struct tm	*tp;
	char	buf[20];

	pmode(st, linkflag);
	printf("%3ld/%-3ld", (long)st->st_uid, (long)st->st_gid);
	printf(" %6lld", (long long)st->st_size);
	tp = localtime(&st->st_mtime);
	strftime(buf, sizeof buf, "%b %e %H:%M %Y", tp);
	printf(" %17.17s ", buf);
}

#define	SUID	04000
#define	SGID	02010
#define	NFMT	02000
#define	ROWN	0400
#define	WOWN	0200
#define	XOWN	0100
#define	RGRP	040
#define	WGRP	020
#define	XGRP	010
#define	ROTH	04
#define	WOTH	02
#define	XOTH	01
#define	STXT	01000
static int	m1[] = { 1, ROWN, 'r', '-' };
static int	m2[] = { 1, WOWN, 'w', '-' };
static int	m3[] = { 2, SUID, 's', XOWN, 'x', '-' };
static int	m4[] = { 1, RGRP, 'r', '-' };
static int	m5[] = { 1, WGRP, 'w', '-' };
static int	m6[] = { 3, SGID, 's', NFMT, 'l', XGRP, 'x', '-' };
static int	m7[] = { 1, ROTH, 'r', '-' };
static int	m8[] = { 1, WOTH, 'w', '-' };
static int	m9[] = { 2, STXT, 't', XOTH, 'x', '-' };

static int	*m[] = { m1, m2, m3, m4, m5, m6, m7, m8, m9};

static void
pmode(register struct stat *st, int linkflag)
{
	register int **mp;
	int	c;

	switch (linkflag) {
	case -1:
		switch (st->st_mode & S_IFMT) {
		case S_IFLNK:	c = 'l'; break;
		case S_IFCHR:	c = 'c'; break;
		case S_IFBLK:	c = 'b'; break;
		case S_IFDIR:	c = 'd'; break;
		case S_IFIFO:	c = 'p'; break;
		default:	c = '-';
		}		 break;
	case '2':	c = 'l'; break;
	case '3':	c = 'c'; break;
	case '4':	c = 'b'; break;
	case '5':	c = 'd'; break;
	case '6':	c = 'p'; break;
	default:	c = '-';
	}
	printf("%c", c);
	for (mp = &m[0]; mp < &m[9];)
		tselect(*mp++, st);
}

static void
tselect(int *pairp, struct stat *st)
{
	register int n, *ap;

	ap = pairp;
	n = *ap++;
	while (--n>=0 && (ap++, (st->st_mode&ap[-1])!=ap[-1]))
		ap++;
	printf("%c", *ap);
}

static int
checkdir(register char *name, struct stat *sp)
{
	register char *cp;

	for (cp = name; *cp; cp++) {
		if (*cp == '/' && cp > name) {
			*cp = '\0';
			if (access(name, X_OK) < 0) {
				if (mkdir(name, 0777) < 0 && errno != EEXIST) {
					fprintf(stderr, "%s: %s: %s\n",
							progname, name,
							strerror(errno));
					edone(1);
				}
				tchown(chown, name, sp->st_uid, sp->st_gid);
			}
			*cp = '/';
		}
	}
	return cp > name ? cp[-1] : cp[0];
}

/*ARGUSED*/
static void
onsig(int signo)
{
	sigset(signo, SIG_IGN);
	if (midfile) {
		fprintf(stderr, "%s: Interrupted in the middle of a file\n",
				progname);
		done(signo | 0200);
	}
	term = 1;
}

static void
tomodes(const char *name, register struct stat *sp)
{
	const char	*cp;
	int	mode;

	memset(&dblock, 0, sizeof dblock);
	mode = gnuflag<=0&&!Eflag?sp->st_mode&07777:sp->st_mode&(07777|S_IFMT);
	sprintf(dblock.dbuf.mode, "%7.7o", mode);
	sprintf(dblock.dbuf.uid, "%7.7lo", (long)(sp->st_uid <= 07777777 ?
				sp->st_uid : (paxrec |= PR_UID, 60001)));
	sprintf(dblock.dbuf.gid, "%7.7lo", (long)(sp->st_gid <= 07777777 ?
				sp->st_gid : (paxrec |= PR_GID, 60001)));
	sprintf(dblock.dbuf.size, "%11.11llo", (sp->st_mode&S_IFMT)==S_IFREG ?
			(long long)sp->st_size&077777777777LL : 0LL);
	sprintf(dblock.dbuf.mtime, "%11.11lo", (long)sp->st_mtime);
	if (oldflag <= 0) {
		strcpy(dblock.dbuf.magic, gnuflag>0 ? "ustar  " : "ustar");
		if (gnuflag <= 0)
			dblock.dbuf.version[0] = dblock.dbuf.version[1] = '0';
		if ((cp = getuser(sp->st_uid)) != NULL)
			sprintf(dblock.dbuf.uname, "%.31s", cp);
		else
			fprintf(stderr,
				"%s: could not get passwd information for %s\n",
				progname, name);
		if ((cp = getgroup(sp->st_gid)) != NULL)
			sprintf(dblock.dbuf.gname, "%.31s", cp);
		else
			fprintf(stderr,
				"%s: could not get group information for %s\n",
				progname, name);
		if (Eflag && major(sp->st_rdev) > 07777777 &&
				((sp->st_mode&S_IFMT) == S_IFBLK ||
				 (sp->st_mode&S_IFMT) == S_IFCHR))
			paxrec |= PR_SUN_DEVMAJOR;
		sprintf(dblock.dbuf.devmajor, "%7.7o",
				(int)major(sp->st_rdev)&07777777);
		if (Eflag && minor(sp->st_rdev) > 07777777 &&
				((sp->st_mode&S_IFMT) == S_IFBLK ||
				 (sp->st_mode&S_IFMT) == S_IFCHR))
			paxrec |= PR_SUN_DEVMINOR;
		sprintf(dblock.dbuf.devminor, "%7.7o",
				(int)minor(sp->st_rdev)&07777777);
	}
}

static int
checksum(int invert)
{
	register uint32_t i;
	register char *cp;

	for (cp = dblock.dbuf.chksum;
			cp < &dblock.dbuf.chksum[sizeof(dblock.dbuf.chksum)];
			cp++)
		*cp = ' ';
	i = 0;
	for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
		i += oldflag>0^invert ? *(signed char *)cp : *cp & 0377;
	return(i);
}

static int
checkw(int c, const char *name, struct stat *sp, int linkflag)
{
	if (wflag) {
		printf("%c ", c);
		if (vflag)
			longt(sp, linkflag);
		printf("%s: ", name);
		fflush(stdout);
		if (response() == 'y'){
			return(1);
		}
		return(0);
	}
	return(1);
}

static int
response(void)
{
	char c, c2;

	if (read(0, &c, 1) == 1 && c != '\n')
		while (read(0, &c2, 1) == 1 && c2 != '\n');
	else c = 'n';
	return(c);
}

static int
checkupdate(const char *arg, struct stat *sp)
{
	long	mtime;	/* cf. LONG_MAX in readexcl() */
	off_t seekp;
	char	c;

	rewind(tfile);
	for (;;) {
		if ((seekp = lookup(arg)) < 0)
			return(0);
		fseeko(tfile, seekp+1, SEEK_SET);
		fscanf(tfile, "%c %lo", &c, &mtime);
		if (c == 'u' && sp->st_mtime > mtime)
			return(0);
		else
			return(c);
	}
}

static void
done(int n)
{
	if (rflag && mt >= 0 && close(mt) < 0 && writerror == 0) {
		fprintf(stderr, "%s: tape write error\n", progname);
		if (n == 0)
			n = 2;
	}
	exit(n);
}

static int
prefix(register const char *s1, register const char *s2)
{
	while (*s1)
		if (*s1++ != *s2++)
			return(0);
	if (*s2)
		return(*s2 == '/');
	return(1);
}

static int	njab;
static off_t
lookup(const char *s)
{
	return bsrch(s, strlen(s), low, high);
}

static off_t
bsrch(const char *s, int n, off_t l, off_t h)
{
	register int i, j;
	char *b;
	off_t m, m1;

	b = alloca(N);
	njab = 0;

loop:
	if(l >= h)
		return(-1L);
	m = l + (h-l)/2 - N/2;
	if(m < l)
		m = l;
	fseeko(tfile, m, SEEK_SET);
	fread(b, 1, N, tfile);
	njab++;
	for(i=0; i<N; i++) {
		if(b[i] == '\n')
			break;
		m++;
	}
	if(m >= h)
		return(-1L);
	m1 = m;
	j = i;
	for(i++; i<N; i++) {
		m1++;
		if(b[i] == '\n')
			break;
	}
	i = cmp(b+j, s, n);
	if(i < 0) {
		h = m;
		goto loop;
	}
	if(i > 0) {
		l = m1;
		goto loop;
	}
	return(m);
}

static int
cmp(const char *b, const char *s, size_t n)
{
	register int i;

	if(b[0] != '\n')
		abort();
	for(i=0; i<n; i++) {
		if((b[i+1+TIMSIZ+1+2]&0377) > (s[i]&0377))
			return(-1);
		if((b[i+1+TIMSIZ+1+2]&0377) < (s[i]&0377))
			return(1);
	}
	return(b[i+1+TIMSIZ+1+2] == '\n'? 0 : -1);
}

static int
readtape(char *buffer)
{
	static int rd;
	int i = -1, j;

again:	if (recno >= nblock || first == 0) {
		if (first == 0 && nblock == 1 && bflag == 0)
			j = NBLOCK;
		else
			j = nblock;
		if (volsize % TBLOCK*j) {
			fprintf(stderr, "%s: Volume size not a multiple "
					"of block size.\n", progname);
			done(1);
		}
		if ((rd = i = mtread(tbuf, TBLOCK*j)) < 0) {
			fprintf(stderr, "%s: tape read error\n", progname);
			done(3);
		}
		if (maybezip && checkzip(tbuf[0].dummy, i) == 1)
			goto again;
		if (first == 0 || i == 0) {
			if ((i % TBLOCK) != 0 || i == 0) {
			tbe:	fprintf(stderr, "%s: tape blocksize error\n",
						progname);
				done(3);
			}
			i /= TBLOCK;
			if (i != nblock) {
				if ((mtstat.st_mode&S_IFMT) == S_IFCHR &&
						(i != 1 || bflag >= B_DEFN))
					fprintf(stderr, "%s: blocksize = %d\n",
				 		progname, i);
				nblock = i;
			}
			if (tapeblock == 0)
				tapeblock = i;
		}
		recno = 0;
	}
	first = 1;
	if ((rd -= TBLOCK) < 0)
		goto tbe;
	memcpy(buffer, &tbuf[recno++], TBLOCK);
	return(TBLOCK);
}

static int
writetape(const char *buffer)
{
	first = 1;
	if (recno >= nblock) {
		if (mtwrite(tbuf, TBLOCK*nblock) < 0) {
			fprintf(stderr, "%s: tape write error\n", progname);
			writerror++;
			done(2);
		}
		recno = 0;
	}
	memcpy(&tbuf[recno++], buffer, TBLOCK);
	if (recno >= nblock) {
		if (mtwrite(tbuf, TBLOCK*nblock) < 0) {
			fprintf(stderr, "%s: tape write error\n", progname);
			writerror++;
			done(2);
		}
		recno = 0;
	}
	return(TBLOCK);
}

static void
tseek(int n, int rew)
{
	int	fault;
	if (tapeblock > 0 && rew) {
#if defined (__linux__) || defined (__sun) || defined (__FreeBSD__) || \
	defined (__hpux) || defined (_AIX) || defined (__NetBSD__) || \
	defined (__OpenBSD__) || defined (__DragonFly__) || defined (__APPLE__)
		struct mtop	mo;
		mo.mt_op = n > 0 ? MTFSR : MTBSR;
		mo.mt_count = (n > 0 ? n : -n) / tapeblock;
		fault = ioctl(mt, MTIOCTOP, &mo) < 0;
#else	/* SVR4.2MP */
		int	t, a;
		t = n > 0 ? T_SBF : T_SBB;
		a = (n > 0 ? n : -n) / tapeblock;
		fault = ioctl(mt, t, a) < 0;
#endif	/* SVR4.2MP */
	} else
		fault = lseek(mt, TBLOCK*n, SEEK_CUR) == (off_t)-1;
	if (fault && rew) {
		fprintf(stderr, "%s: device seek error\n", progname);
		done(4);
	}
}

static void
backtape(int rew)
{
	tseek(-nblock, rew);
	if (recno >= nblock) {
		recno = nblock - 1;
		if (mtread(tbuf, TBLOCK*nblock) < 0) {
			fprintf(stderr, "%s: tape read error after seek\n",
					progname);
			done(4);
		}
		tseek(-nblock, rew);
	} else if (rew)
		recno--;
}

static void
flushtape(void)
{
	if (mtwrite(tbuf, TBLOCK*nblock) < 0) {
		fprintf(stderr, "%s: tape write error\n", progname);
		writerror++;
		done(2);
	}
}

static void *
srealloc(void *op, size_t size)
{
	void	*np;

	if ((np = realloc(op, size)) == NULL) {
		write(2, "no memory\n", 10);
		_exit(077);
	}
	return np;
}

static void *
smalloc(size_t size)
{
	return srealloc(NULL, size);
}

static void *
scalloc(size_t count, size_t nelem)
{
	void	*np;

	if ((np = calloc(count, nelem)) == NULL) {
		write(2, "no memory\n", 10);
		_exit(077);
	}
	return np;
}

static void *
bfalloc(size_t n)
{
	static long	pagesize;
	void	*vp;

	if (pagesize == 0)
		if ((pagesize = sysconf(_SC_PAGESIZE)) < 0)
			pagesize = 4096;
	if ((vp = memalign(pagesize, n)) == NULL) {
		fprintf(stderr, "%s: cannot allocate physio buffer\n",
				progname);
		done(1);
	}
	return vp;
}

static char *
nameof(struct header *hp, char *buf)
{
	const char	*cp;
	register char	*bp = buf;

	if (gnuflag <= 0 && hp->prefix[0] != '\0') {
		cp = hp->prefix;
		while (cp < &hp->prefix[PFXSIZ] && *cp)
			*bp++ = *cp++;
		if (bp > buf)
			*bp++ = '/';
	}
	cp = hp->name;
	while (cp < &hp->name[NAMSIZ] && *cp)
		*bp++ = *cp++;
	*bp = '\0';
	return buf;
}

static int
mkname(struct header *hp, const char *fn)
{
	const char	*cp, *cs = NULL;

	if (Aflag)
		while (*fn == '/')
			fn++;
	for (cp = fn; *cp; cp++) {
		if (*cp == '/' && cp[1] != '\0' && cp > fn &&
				cp - fn <= PFXSIZ &&
				gnuflag <= 0 && oldflag <= 0)
			cs = cp;
	}
	if (cp - (cs ? &cs[1] : fn) > NAMSIZ) {
		if (oldflag <= 0 && gnuflag <= 0 && utf8(fn)) {
			paxrec |= PR_PATH;
			strcpy(hp->name, sequence());
			return 0;
		}
		fprintf(stderr, "%s: file name too long\n", fn);
		edone(1);
		return -1;
	}
	if (cs && cp - fn > NAMSIZ) {
		memcpy(hp->prefix, fn, cs - fn);
		if (cs - fn < PFXSIZ)
			hp->prefix[cs - fn] = '\0';
		memcpy(hp->name, &cs[1], cp - &cs[1]);
		if (cp - &cs[1] < NAMSIZ)
			hp->name[cp - &cs[1]] = '\0';
	} else {
		memcpy(hp->name, fn, cp - fn);
		if (cp - fn < NAMSIZ)
			hp->name[cp - fn] = '\0';
	}
	return 0;
}

static char *
linkof(struct header *hp, char *buf)
{
	const char	*cp;
	register char	*bp = buf;


	cp = hp->linkname;
	while (cp < &hp->linkname[NAMSIZ] && *cp)
		*bp++ = *cp++;
	*bp = '\0';
	return buf;
}

static int
mklink(struct header *hp, const char *fn, const char *refname)
{
	const char	*cp;

	if (Aflag)
		while (*fn == '/')
			fn++;
	for (cp = fn; *cp; cp++);
	if (cp - fn > NAMSIZ) {
		if (oldflag <= 0 && gnuflag <= 0 && utf8(fn)) {
			paxrec |= PR_LINKPATH;
			strcpy(hp->linkname, sequence());
			return 0;
		}
		fprintf(stderr, "%s: %s: linked to %s\n",
				progname, refname, fn);
		fprintf(stderr, "%s: %s: linked name too long\n",
				progname, fn);
		edone(1);
		return -1;
	}
	memcpy(hp->linkname, fn, cp - fn);
	if (cp - fn < NAMSIZ)
		hp->linkname[cp - fn] = '\0';
	return 0;
}

static void
edone(int i)
{
	if (eflag && sysv3 == 0)
		done(i);
}

static ssize_t
mtwrite(const void *vdata, size_t sz)
{
	register ssize_t	wo, wt = 0;
	const char	*data = vdata;

	if (volsize && wrtotal >= volsize) {
		newvolume();
		wrtotal = 0;
	}
	do {
		if ((wo = write(mt, data + wt, sz - wt)) < 0) {
			if (errno == EINTR)
				continue;
			else if (wt > 0) {
				wt += wo;
				break;
			} else
				return wo;
		}
		wt += wo;
	} while (wt < sz);
	wrtotal += sz;
	return wt;
}

static ssize_t
mtread(void *vdata, size_t sz)
{
	register ssize_t	ro, rt = 0;
	char	*data = vdata;

	if (volsize && rdtotal >= volsize) {
		newvolume();
		rdtotal = 0;
	}
	do {
		if ((ro = read(mt, data + rt, sz - rt)) <= 0) {
			if (ro < 0) {
				if (errno == EINTR)
					continue;
			}
			if (rt > 0) {
				rt += ro;
				break;
			}
			return ro;
		}
		rt += ro;
	} while (Bflag != 0 && rt < sz);
	rdtotal += sz;
	return rt;
}

static void
newvolume(void)
{
	static int	ttyfd = -1;
	int	curfd;
	char	c;

	if (close(mt) < 0) {
		fprintf(stderr, "%s: close error on archive: %s\n", progname,
				strerror(errno));
		done(1);
	}
	if ((curfd = open(".", O_RDONLY)) < 0) {
		fprintf(stderr, "cannot open current directory: %s\n",
				strerror(errno));
		done(1);
	}
	goback(workdir);
	fprintf(stderr, "%s: please insert new volume, then press RETURN.\a",
			progname);
	if (ttyfd < 0 && isatty(0) || ttyfd == 0)
		ttyfd = 0;
	else
		ttyfd = open("/dev/tty", O_RDONLY);
	do
		if (read(ttyfd, &c, 1) != 1)
			done(0);
	while (c != '\n');
	if (ttyfd > 0)
		close(ttyfd);
	if ((mt = open(usefile, rflag ? O_RDWR : O_RDONLY)) < 0) {
		fprintf(stderr, "%s: cannot open %s\n", progname, usefile);
		done(1);
	}
	domtstat();
	if (rflag)
		odirect();
	goback(curfd);
	close(curfd);
}

static void
goback(int fd)
{
	if (fchdir(fd) < 0) {
		fprintf(stderr, "cannot change back?: %s\n", strerror(errno));
		done(1);
	}
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
defaults(void)
{
	struct iblok	*ip;
	char	*line = NULL, *x, *y, *cp;
	size_t	size = 0;
	struct magtape	*mp;

	if ((ip = ib_open(TARDFL, 0)) == NULL)
		return;
	while (ib_getlin(ip, &line, &size, srealloc) != 0) {
		if (strncmp(line, "archive", 7) == 0) {
			if (line[8] == '=' && line[7] >= '0' && line[7] <= '9'){
				mp = &magtapes[line[7] - '0'];
				x = &line[9];
			} else if (line[7] == '=') {
				x = &line[8];
				mp = &magtapes[10];
			} else
				continue;
			for (y = x; *y && *y != ' ' && *y != '\t'; y++);
			mp->device = cp = smalloc(y - x + 1);
			while (x < y)
				*cp++ = *x++;
			*cp = '\0';
			mp->block = 1;
			mp->size = 0;
			mp->nflag = 0;
			if (*x) {
				mp->block = strtol(x, &y, 10);
				if (y > x && *(x=y)) {
					mp->size = strtoll(x, &y, 10) * 1024;
					if (y > x && *(x=y)) {
						while (*x && (*x == ' ' ||
								*x == '\t' ||
								*x == '\n'))
							x++;
						if (*x == 'n' || *x == 'N')
							mp->nflag = 1;
						else
							mp->nflag = 0;
					}
				}
			}
		} else if (strncmp(line, "order=", 6) == 0) {
			if (strcmp(&line[6], "post\n") == 0)
				order = POSTORDER;
			else if (strcmp(&line[6], "pre\n") == 0)
				order = PREORDER;
		}
	}
	ib_close(ip);
	if (line)
		free(line);
}

static void
settape(int c)
{
	struct magtape	*mp;

	if (c >= '0' && c <= '9')
		mp = &magtapes[c - '0'];
	else {
		if (magtapes[10].device)
			mp = &magtapes[10];
		else
			mp = &magtapes[0];
		c = '0';
	}
	if (mp->device == NULL) {
		fprintf(stderr, "%s: missing or invalid 'archive%c=' entry "
				"in %s.\n", progname, c, TARDFL);
		return;
	}
	usefile = mp->device;
	if (bflag == 0 && mp->block > 0) {
		nblock = mp->block;
		bflag = B_DEFN;
	}
	if (kflag == 0)
		volsize = mp->size;
	if (nflag < 2)
		nflag = mp->nflag;
}

static void
suprmsg(void)
{
	if (Aflag && vflag)
		fprintf(stderr, "Suppressing absolute pathnames\n");
}

static void
odirect(void)
{
#if defined (__linux__) && defined (O_DIRECT)
	/*
	 * If we are operating on a floppy disk block device and know
	 * its track size, use direct i/o. This has the advantage that
	 * signals can be delivered after each write(); otherwise, the
	 * kernel will buffer the entire data, close() will put us in
	 * a non-interruptible blocking state and the user has to wait
	 * ~40 seconds for return after he presses the interrupt key.
	 *
	 * This has no negative speed impact as long as the blocking
	 * factor is set to a multiple of the track size of the floppy.
	 * The only values still useful today (2003) seem to be 18 for
	 * 3.5 inch high densitiy disks at 1440 kB and 15 for 5.25 inch
	 * high density disks at 1200 kB, so we specify these in the
	 * default file; consult fd(4) for other values, or give no
	 * values at all and autodetect them in the code above.
	 *
	 * Addendum: Use direct i/o for all block devices if a block
	 * size was specified or detected since the symptoms are
	 * generally the same as for floppy disks (e. g. with USB
	 * memory sticks). But don't use it when reading since it
	 * just slows down operation then.
	 */
	if ((mtstat.st_mode&S_IFMT) == S_IFBLK && bflag) {
		int	flags;
		if ((flags = fcntl(mt, F_GETFL)) != -1)
			fcntl(mt, F_SETFL, flags | O_DIRECT);
	}
#endif	/* __linux__ && O_DIRECT */
}

static void
domtstat(void)
{
	static int	twice;

	if (fstat(mt, &mtstat) < 0) {
		fprintf(stderr, "%s: cannot stat archive\n", progname);
		done(1);
	}
	if ((mtstat.st_mode&S_IFMT) == S_IFIFO ||
			(mtstat.st_mode&S_IFMT) == S_IFSOCK)
		Bflag = 1;
#if defined (__linux__)
	if ((mtstat.st_mode&S_IFMT) == S_IFBLK) {
		struct floppy_struct	fs;
		int	blkbsz;
		if (ioctl(mt, FDGETPRM, &fs) == 0) {
			if (kflag == 0 && volsize == 0)
				volsize = fs.size * FD_SECTSIZE(&fs);
			if (bflag == 0 && nblock == 1 && twice == 0) {
				if ((nblock=fs.sect*FD_SECTSIZE(&fs)/512)==0 ||
						nblock > NBLOCK)
					nblock = 1;
				else
					bflag = B_AUTO;
			}
#ifdef	O_DIRECT
			if (bflag && (tflag || xflag)) {
				int flags;
				if ((flags = fcntl(mt, F_GETFL)) != -1)
					fcntl(mt, F_SETFL, flags | O_DIRECT);
			}
#endif	/* O_DIRECT */
#ifdef	BLKBSZGET
		} else if (ioctl(mt, BLKBSZGET, &blkbsz) == 0) {
			if (bflag == 0 && nblock == 1 && twice == 0 &&
					(blkbsz&0777) == 0) {
				nblock = blkbsz >> 9;
				bflag = B_AUTO;
			}
#endif	/* BLKBSZGET */
		}
	} else if ((mtstat.st_mode&S_IFMT) == S_IFCHR) {
		struct mtget	mg;
		if (ioctl(mt, MTIOCGET, &mg) == 0)
			tapeblock = ((mg.mt_dsreg&MT_ST_BLKSIZE_MASK)
					>> MT_ST_BLKSIZE_SHIFT);
	}
#elif defined (__sun)
	if ((mtstat.st_mode&S_IFMT) == S_IFCHR) {
		struct mtdrivetype_request	mr;
		static struct mtdrivetype	md;
		mr.size = sizeof md;
		mr.mtdtp = &md;
		if (ioctl(mt,  MTIOCGETDRIVETYPE, &mr) == 0)
			tapeblock = md.bsize;
	}
#elif defined (__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) \
		|| defined (__DragonFly__) || defined (__APPLE__)
	if ((mtstat.st_mode&S_IFMT) == S_IFCHR) {
		struct mtget	mg;
		if (ioctl(mt, MTIOCGET, &mg) == 0)
			tapeblock = mg.mt_blksiz;
	}
#elif defined (__hpux) || defined (_AIX)
#else	/* SVR4.2MP */
	if ((mtstat.st_mode&S_IFMT) == S_IFCHR) {
		struct blklen	bl;
		if (ioctl(mt, T_RDBLKLEN, &bl) == 0)
			/*
			 * This ioctl is (apparently) not useful. It just
			 * returns 1 as minimum and 16M-1 as maximum size
			 * on DAT/DDS tape drives.
			 */
			tapeblock = 0;
	}
#endif	/* SVR4.2MP */
	if (tapeblock > 0) {
		if (tapeblock % TBLOCK != 0) {
			fprintf(stderr, "%s: tape blocksize error\n",
				progname);
			done(3);
		}
		tapeblock /= TBLOCK;
		if (tapeblock > NBLOCK)
			NBLOCK = tapeblock;
#if defined (__linux__) || defined (__sun) || defined (__FreeBSD__) || \
	defined (__NetBSD__) || defined (__OpenBSD__) || \
	defined (__DragonFly__) || defined (__APPLE__)
		if (bflag == 0 && cflag && twice == 0) {
			if (nblock == 1) {
				if ((nblock = tapeblock) > NBLOCK)
					nblock = 1;
				else
					bflag = B_AUTO;
			}
		}
#endif	/* __linux__ || __sun || __FreeBSD__ || __NetBSD__ || __OpenBSD__ ||
	__DragonFly__ || __APPLE__ */
	}
	if (twice == 0 && bflag == 0 && tapeblock < 0) {
		if ((nblock = mtstat.st_blksize >> 9) > NBLOCK)
			nblock = NBLOCK;
		else if (nblock <= 0)
			nblock = 1;
		else if ((mtstat.st_mode&S_IFMT) != S_IFCHR)
			bflag = B_AUTO;
	}
	twice = 1;
	if (nblock > NBLOCK)
		NBLOCK = nblock;
	tbuf = bfalloc(sizeof *tbuf * NBLOCK);
}

static int
checkzip(const char *bp, int rd)
{
	if (rd <= TBLOCK || (memcmp(&bp[257], "ustar", 5) &&
			memcmp(&bp[257], "\0\0\0\0\0", 5))) {
		if (bp[0] == 'B' && bp[1] == 'Z' && bp[2] == 'h')
			return redirect("bzip2", "-cd", rd);
		else if (bp[0] == '\37' && bp[1] == '\235')
			return redirect("zcat", NULL, rd);
		else if (bp[0] == '\37' && bp[1] == '\213')
			return redirect("gzip", "-cd", rd);
	}
	maybezip = 0;
	return -1;
}

static int
redirect(const char *arg0, const char *arg1, int rd)
{
	int	pd[2];

	if (pipe(pd) < 0)
		return -1;
	switch (fork()) {
	case 0:
		if (tapeblock >= 0 || lseek(mt, -rd, SEEK_CUR) == (off_t)-1) {
			int	xpd[2];
			if (pipe(xpd) == 0 && fork() == 0) {
				int	wo, wt;
				close(xpd[0]);
				do {
					wo = wt = 0;
					do {
						if ((wo = write(xpd[1],
								tbuf + wt,
								rd - wt))
									<= 0) {
							if (errno == EINTR)
								continue;
							_exit(0);
						}
						wt += wo;
					} while (wt < rd);
				} while ((rd=mtread(tbuf, TBLOCK*nblock)) >= 0);
				if (rd < 0)
					fprintf(stderr, "%s: tape read error\n",
							progname);
				_exit(0);
			} else {
				close(xpd[1]);
				dup2(xpd[0], 0);
				close(xpd[0]);
			}
		} else
			dup2(mt, 0);
		close(mt);
		dup2(pd[1], 1);
		close(pd[0]);
		close(pd[1]);
		execlp(arg0, arg0, arg1, NULL);
		fprintf(stderr, "%s: could not exec %s: %s\n",
				progname, arg0, strerror(errno));
		_exit(0177);
		/*NOTREACHED*/
	default:
		Bflag = 1;
		tapeblock = -1;
		dup2(pd[0], mt);
		close(pd[0]);
		close(pd[1]);
		domtstat();
		break;
	case -1:
		return -1;
	}
	return 1;
}

#define	CACHESIZE	16

static const char *
getuser(uid_t uid)
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
	cache[i].name = sstrdup(name);
	cache[i].uid = uid;
found:	return cache[i].name[0] ? cache[i].name : NULL;
}

static const char *
getgroup(gid_t gid)
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
	cache[i].name = sstrdup(name);
	cache[i].gid = gid;
found:	return cache[i].name[0] ? cache[i].name : NULL;
}

static char *
sstrdup(const char *op)
{
	char	*np;

	np = smalloc(strlen(op) + 1);
	strcpy(np, op);
	return np;
}

static void
fromfile(const char *fn)
{
	struct iblok	*ip;
	char	*line = NULL;
	size_t	size = 0, len;

	if ((ip = ib_open(fn, 0)) == NULL) {
		fprintf(stderr, "%s: %s: %s\n", progname, fn, strerror(errno));
		goback(workdir);
	} else {
		while ((len = ib_getlin(ip, &line, &size, srealloc)) != 0) {
			if (line[len-1] == '\n')
				line[--len] = '\0';
			doarg(line);
			goback(workdir);
		}
		ib_close(ip);
		if (line)
			free(line);
	}
}

static void
readexcl(const char *fn)
{
	FILE	*fp;
	int	c, slash, s;

	if ((fp = fopen(fn, "r")) == NULL) {
		fprintf(stderr, "%s: could not open %s: %s\n", progname, fn,
				strerror(errno));
		done(1);
	}
	do {
		if ((c = getc(fp)) != EOF && c != '\n') {
			slash = 0;
			s = fprintf(tfile, "X %0*lo %c", TIMSIZ, LONG_MAX, c);
			while ((c = getc(fp)) != EOF && c != '\n') {
				if (c == '/') {
					slash = 1;
					continue;
				} else if (slash == 1) {
					putc('/', tfile);
					s++;
					slash = 0;
				}
				putc(c, tfile);
				s++;
			}
			putc('\n', tfile);
			s++;
			s *= 3;
			if (s > N)
				N = s;
		}
	} while (c != EOF);
	fclose(fp);
}

static void
creatfile(void)
{
	char	tname[] = "/tmp/tarXXXXXX";

	if (tfile != NULL)
		return;
	if ((tfile = fdopen(mkstemp(tname), "w")) == NULL) {
		fprintf(stderr, "%s: cannot create temporary file (%s)\n",
			progname, tname);
		done(1);
	}
	unlink(tname);
	fcntl(fileno(tfile), F_SETFD, FD_CLOEXEC);
	fprintf(tfile, "\177 %0*lo !!!!!/!/!/!/!/!/!/!\n", TIMSIZ, 0L);
}

static mode_t
cmask(struct stat *sp, int creation)
{
	mode_t	mask = 07777;

	if (myuid != 0 || oflag || creation) {
		if (sp->st_uid != myuid || sp->st_gid != mygid) {
			mask &= ~(mode_t)S_ISUID;
			if ((sp->st_mode&S_IFMT)!=S_IFDIR && sp->st_mode&0010)
				mask &= ~(mode_t)S_ISGID;
			if ((sp->st_mode&S_IFMT)==S_IFDIR && sp->st_gid!=mygid)
				mask &= ~(mode_t)S_ISGID;
		}
	}
	return mask;
}

/*
 * Top-down splay function for inode tree.
 */
static struct islot *
isplay(ino_t ino, struct islot *x)
{
	struct islot	hdr;
	struct islot	*leftmax, *rightmin;
	struct islot	*y;

	hdr.left = hdr.right = inull;
	leftmax = rightmin = &hdr;
	inull->inum = ino;
	while (ino != x->inum) {
		if (ino < x->inum) {
			if (ino < x->left->inum) {
				y = x->left;
				x->left = y->right;
				y->right = x;
				x = y;
			}
			if (x->left == inull)
				break;
			rightmin->left = x;
			rightmin = x;
			x = x->left;
		} else {
			if (ino > x->right->inum) {
				y = x->right;
				x->right = y->left;
				y->left = x;
				x = y;
			}
			if (x->right == inull)
				break;
			leftmax->right = x;
			leftmax = x;
			x = x->right;
		}
	}
	leftmax->right = x->left;
	rightmin->left = x->right;
	x->left = hdr.right;
	x->right = hdr.left;
	inull->inum = !ino;
	return x;
}

/*
 * Find the inode number ino.
 */
static struct islot *
ifind(ino_t ino, struct islot **it)
{
	if (*it == NULL)
		return NULL;
	*it = isplay(ino, *it);
	return (*it)->inum == ino ? *it : NULL;
}

/*
 * Put ik into the tree.
 */
static void
iput(struct islot *ik, struct islot **it)
{
	if ((*it) == NULL) {
		ik->left = ik->right = inull;
		(*it) = ik;
	} else {
		/* ifind() is always called before */
		/*(*it) = isplay(ik->inum, (*it));*/
		if (ik->inum < (*it)->inum) {
			ik->left = (*it)->left;
			ik->right = (*it);
			(*it)->left = inull;
			(*it) = ik;
		} else if ((*it)->inum < ik->inum) {
			ik->right = (*it)->right;
			ik->left = (*it);
			(*it)->right = inull;
			(*it) = ik;
		}
	}
}

/*
 * Find the device dev or add it to the device/inode forest if not
 * already present.
 */
static struct dslot *
dfind(struct dslot **root, dev_t dev)
{
	struct dslot	*ds, *dp;

	for (ds = *root, dp = NULL; ds; dp = ds, ds = ds->nextp)
		if (ds->devnum == dev)
			break;
	if (ds == NULL) {
		ds = scalloc(1, sizeof *ds);
		ds->devnum = dev;
		if (*root == NULL)
			*root = ds;
		else
			dp->nextp = ds;
	}
	return ds;
}

static char *
sequence(void)
{
	static char	buf[25];
	static long long	d;

	sprintf(buf, "%10.10lld", ++d);
	return buf;
}

static void
docomp(const char *name)
{
	int	pd[2];
	struct stat	ost;

	if (tapeblock >= 0) {
		fprintf(stderr, "%s: Refusing to write compressed data "
				"to tapes.\n", progname);
		done(1);
	}
	if (pipe(pd) < 0) {
		fprintf(stderr, "%s: pipe() failed\n", progname);
		done(1);
	}
	switch (fork()) {
	case 0:
		dup2(mt, 1);
		close(mt);
		ftruncate(1, 0);
		dup2(pd[0], 0);
		close(pd[0]);
		close(pd[1]);
		execlp(name, name, "-c", NULL);
		fprintf(stderr, "%s: could not exec %s\n", progname, name);
		_exit(0177);
		/*NOTREACHED*/
	case -1:
		fprintf(stderr, "%s: could not fork(), try again later\n",
				progname);
		done(1);
		/*NOTREACHED*/
	default:
		dup2(pd[1], mt);
		close(pd[0]);
		close(pd[1]);
	}
	ost = mtstat;
	domtstat();
	mtstat.st_dev = ost.st_dev;
	mtstat.st_ino = ost.st_ino;
}

static int
utf8(const char *cp)
{
	int	c, n;

	while (*cp) if ((c = *cp++ & 0377) & 0200) {
		if (c == (c & 037 | 0300))
			n = 1;
		else if (c == (c & 017 | 0340))
			n = 2;
		else if (c == (c & 07 | 0360))
			n = 3;
		else if (c == (c & 03 | 0370))
			n = 4;
		else if (c == (c & 01 | 0374))
			n = 5;
		else
			return 0;
		while (n--) {
			c = *cp++ & 0377;
			if (c != (c & 077 | 0200))
				return 0;
		}
	}
	return 1;
}
