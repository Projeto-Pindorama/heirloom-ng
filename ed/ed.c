/*
 * Editor
 */

/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, July 2003.
 */
/*	from Unix 32V /usr/src/cmd/ed.c	*/
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
static const char sccsid[] USED = "@(#)ed_su3.sl	1.99 (gritter) 7/27/06";
#elif defined (SUS)
static const char sccsid[] USED = "@(#)ed_sus.sl	1.99 (gritter) 7/27/06";
#elif defined (S42)
static const char sccsid[] USED = "@(#)ed_s42.sl	1.99 (gritter) 7/27/06";
#else	/* !SU3, !SUS, !S42 */
static const char sccsid[] USED = "@(#)ed.sl	1.99 (gritter) 7/27/06";
#endif	/* !SU3, !SUS, !S42 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "sigset.h"
#include <termios.h>
#include <setjmp.h>
#include <libgen.h>
#include <inttypes.h>
#include <locale.h>
#include <wchar.h>
#include <ctype.h>
#include <wctype.h>
#include <limits.h>
#include <termios.h>
static int	FNSIZE;
static int	LBSIZE;
static int	RHSIZE;
#define	ESIZE	2048
static int	GBSIZE;
#undef	EOF
#define	EOF	-1
#define	puts(s)		xxputs(s)
#define	getline(t, n)	xxgetline(t, n)

#if (LONG_MAX > 017777777777L)
#define	MAXCNT	0777777777777777777777L		/* 2^63-1 */
#else
#define	MAXCNT	017777777777L			/* 2^31-1 */
#endif
#define	BLKMSK	(MAXCNT>>8)			/* was 0377 */

#define	READ	0
#define	WRITE	1
#define	EXIST	2

struct	tabulator {
	struct tabulator	*t_nxt;	/* next list element */
	const char	*t_str;		/* tabulator string */
	int	t_tab;			/* tab stop position */
	int	t_rep;			/* repetitive tab count */
};

static int	peekc;
static int	lastc;
static char	*savedfile;
static char	*file;
static struct stat	fstbuf;
static char	*linebuf;
static char	*rhsbuf;
static char	expbuf[ESIZE + 4];
static long	*zero;
static long	*undzero;
static long	*dot;
static long	*unddot;
static long	*dol;
static long	*unddol;
static long	*addr1;
static long	*addr2;
static char	*genbuf;
static long	count;
static char	*linebp;
static int	ninbuf;
static int	io;
static int	ioeof;
static int	pflag;
static char	*wrtemp;
static uid_t	myuid;
static void	(*oldhup)(int);
static void	(*oldquit)(int);
static void	(*oldpipe)(int);
static int	vflag	= 1;
static int	listf;
static int	numbf;
static char	*globp;
static int	tfile	= -1;
static long	tline;
static char	tfname[64];
static char	ibuff[512];
static int	iblock	= -1;
static char	obuff[512];
static int	oblock	= -1;
static int	ichanged;
static int	nleft;
static long	*names;
static long	*undnames;
static int	anymarks;
static int	subnewa;
static int	fchange;
static int	wrapp;
static unsigned nlall = 128;
static const char	*progname;
static const char	*prompt = "*";
static int	Pflag;
static int	prhelp;
static const char	*prvmsg;
static int	lastsig;
static int	pipid = -1;
static int	readop;
static int	status;
static int	mb_cur_max;
static int	needsub;
static int	insub;
static struct tabulator	*tabstops;
static int	maxlength;
static int	rspec;
static int	Nflag;
static int	bcount = 22;
static int	ocount = 11;

static jmp_buf	savej;

static void	usage(char, int);
static void	commands(void);
static long	*address(void);
static void	setdot(void);
static void	setall(void);
static void	setnoaddr(void);
static void	nonzero(void);
static void	newline(void);
static void	filename(int);
static void	exfile(void);
static void	onintr(int);
static void	onhup(int);
static void	onpipe(int);
static void	error(const char *);
static void	error2(const char *, const char *);
static void	errput(const char *, const char *);
static int	getchr(void);
static int	gettty(void);
static long	getnum(void);
static int	getfile(void);
static void	putfile(void);
static int	append(int (*)(void), long *);
static void	callunix(void);
static char	*readcmd(void);
static void	quit(int);
static void	delete(void);
static void	rdelete(long *, long *);
static void	gdelete(void);
static char	*getline(long, int);
static int	putline(void);
static char	*getblock(long, long);
static void	blkio(long, char *, int);
static void	init(void);
static void	global(int, int);
static void	globrd(char **, int);
static void	join(void);
static void	substitute(int);
static int	compsub(void);
static int	getsub(void);
static int	dosub(int);
static int	place(int, const char *, const char *);
static void	move(int);
static void	reverse(long *, long *);
static int	getcopy(void);
static int	execute(int, long *, int);
static void	cmplerr(int);
static void	doprnt(long *, long *);
static void	putd(long);
static void	puts(const char *);
static void	nlputs(const char *);
static void	list(const char *);
static int	lstchr(int);
static void	putstr(const char *);
static void	putchr(int);
static void	checkpoint(void);
static void	undo(void);
static int	maketf(int);
static int	creatf(const char *);
static int	sopen(const char *, int);
static void	sclose(int);
static void	fspec(const char *);
static const char	*ftok(const char **);
static struct tabulator	*tabstring(const char *);
static void	freetabs(void);
static void	expand(const char *);
static void	growlb(const char *);
static void	growrhs(const char *);
static void	growfn(const char *);
static void	help(void);

#define	INIT
#define	GETC()		getchr()
#define	UNGETC(c)	(peekc = c)
#define	PEEKC()		(peekc = getchr())
#define	RETURN(c)	return c
#define	ERROR(c)	cmplerr(c)
static wint_t	GETWC(char *);

#if defined (SUS) || defined (S42) || defined (SU3)

#include <regex.h>

#define	NBRA	9

static char	*braslist[NBRA];
static char	*braelist[NBRA];
static char	*loc1, *loc2, *locs;
static int	nbra;
static int	circf;
static int	nodelim;

static char	*compile(char *, char *, const char *, int);
static int	step(const char *, const char *);

#else	/* !SUS, !S42, !SU3 */

#include <regexp.h>

#endif	/* !SUS, !S42, !SU3 */

int
main(int argc, char **argv)
{
	register int i;
	void (*oldintr)(int);

	progname = basename(argv[0]);
#if defined (SUS) || defined (S42) || defined (SU3)
	setlocale(LC_COLLATE, "");
#endif
	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	myuid = getuid();
	oldquit = sigset(SIGQUIT, SIG_IGN);
	oldhup = sigset(SIGHUP, SIG_IGN);
	oldintr = sigset(SIGINT, SIG_IGN);
	if (sigset(SIGTERM, SIG_IGN) != SIG_IGN)
		sigset(SIGTERM, quit);
	oldpipe = sigset(SIGPIPE, onpipe);
	argv++;
	while (argc > 1 && **argv=='-') {
		if ((*argv)[1] == '\0') {
			vflag = 0;
			goto next;
		} else if ((*argv)[1] == '-' && (*argv)[2] == '\0') {
			argv++;
			argc--;
			break;
		}
	letter:	switch((*argv)[1]) {

		case 's':
			vflag = 0;
			break;

		case 'q':
			sigset(SIGQUIT, SIG_DFL);
			vflag = 1;
			break;

		case 'p':
			if ((*argv)[2])
				prompt = &(*argv)[2];
			else if (argv[1]) {
				prompt = argv[1];
				argv++;
				argc--;
			} else
				usage((*argv)[1], 1);
			Pflag = 1;
			goto next;

		default:
			usage((*argv)[1], 0);
		}
		if ((*argv)[2]) {
			(*argv)++;
			goto letter;
		}
	next:	argv++;
		argc--;
	}

	growfn("no space");
	if (argc>1) {
		i = -1;
		do
			if (++i >= FNSIZE)
				growfn("maximum of characters in "
						"file names reached");
		while (savedfile[i] = (*argv)[i]);
		globp = "e";
	}
	names = malloc(26*sizeof *names);
	undnames = malloc(26*sizeof *undnames);
	zero = malloc(nlall*sizeof *zero);
	if ((undzero = malloc(nlall*sizeof *undzero)) == NULL)
		puts("no memory for undo");
	growlb("no space");
	growrhs("no space");
	init();
	if (oldintr != SIG_IGN)
		sigset(SIGINT, onintr);
	if (oldhup != SIG_IGN)
		sigset(SIGHUP, onhup);
	setjmp(savej);
	if (lastsig) {
		sigrelse(lastsig);
		lastsig = 0;
	}
	commands();
	quit(0);
	/*NOTREACHED*/
	return 0;
}

static void
usage(char c, int misarg)
{
	if (c) {
		write(2, progname, strlen(progname));
		if (misarg)
			write(2, ": option requires an argument -- ", 33);
		else
			write(2, ": illegal option -- ", 20);
		write(2, &c, 1);
		write(2, "\n", 1);
	}
	write(2, "usage: ", 7);
	write(2, progname, strlen(progname));
	write(2, " [- | -s] [-p string] [file]\n", 29);
	exit(2);
}

static void
commands(void)
{
	register long *a1;
	register int c;
	int	n;

	for (;;) {
	if (pflag) {
		pflag = 0;
		addr1 = addr2 = dot;
		goto print;
	}
	if (Pflag && globp == NULL)
		write(1, prompt, strlen(prompt));
	addr1 = 0;
	addr2 = 0;
	switch (c = getchr()) {
	case ',':
	case ';':
		addr2 = c == ',' ? zero+1 : dot;
		if (((peekc = getchr()) < '0' || peekc > '9') &&
				peekc != ' ' && peekc != '\t' &&
				peekc != '+' && peekc != '-' &&
				peekc != '^' && peekc != '?' &&
				peekc != '/' && peekc != '$' &&
				peekc != '.' && peekc != '\'') {
			addr1 = addr2;
			a1 = dol;
			goto loop;
		}
		break;
	default:
		peekc = c;
	}
	do {
		addr1 = addr2;
		if ((a1 = address())==0) {
			c = getchr();
			break;
		}
	loop:	addr2 = a1;
		if ((c=getchr()) == ';') {
			c = ',';
			dot = a1;
		}
	} while (c==',');
	if (addr1==0)
		addr1 = addr2;
	switch(c) {

	case 'a':
		setdot();
		newline();
		checkpoint();
		append(gettty, addr2);
		continue;

	case 'c':
#if defined (SU3)
		if (addr1 == zero && addr1+1 <= dol) {
			if (addr1 == addr2)
				addr2++;
			addr1++;
		}
#endif	/* SU3 */
		delete();
		append(gettty, addr1-1);
#if defined (SUS) || defined (SU3)
		if (dot == addr1-1 && addr1 <= dol)
			dot = addr1;
#endif	/* SUS || SU3 */
		continue;

	case 'd':
		delete();
		continue;

	case 'E':
		fchange = 0;
		c = 'e';
	case 'e':
		setnoaddr();
		if (vflag && fchange) {
			fchange = 0;
			error("warning: expecting `w'");
		}
		filename(c);
		init();
		addr2 = zero;
		goto caseread;

	case 'f':
		setnoaddr();
		filename(c);
		puts(savedfile);
		continue;

	case 'g':
		global(1, 0);
		continue;

	case 'G':
		global(1, 1);
		continue;

	case 'H':
		prhelp = !prhelp;
		/*FALLTHRU*/

	case 'h':
		if ((peekc = getchr()) == 'e') {
			peekc = 0;
			if (getchr() != 'l' || getchr() != 'p' ||
					getchr() != '\n')
				error("illegal suffix");
			setnoaddr();
			help();
			continue;
		}
		newline();
		setnoaddr();
		if (prvmsg)
			puts(prvmsg);
		continue;

	case 'i':
		setdot();
#if defined (SU3)
		if (addr1 == zero) {
			if (addr1 == addr2)
				addr2++;
			addr1++;
			if (dol != zero)
				nonzero();
		} else
#endif	/* SU3 */
			nonzero();
		newline();
		checkpoint();
		append(gettty, addr2-1);
		if (dot == addr2-1)
			dot++;
		continue;


	case 'j':
		if (addr2==0) {
			addr1 = dot;
			addr2 = dot+1;
		}
		setdot();
		newline();
		nonzero();
		checkpoint();
		if (addr1 != addr2)
			join();
		continue;

	case 'k':
		if ((c = getchr()) < 'a' || c > 'z')
			error("mark not lower case");
		newline();
		setdot();
		nonzero();
		names[c-'a'] = *addr2 & ~01;
		anymarks |= 01;
		continue;

	case 'm':
		move(0);
		continue;

	case '\n':
		if (addr2==0)
			addr2 = dot+1;
		addr1 = addr2;
		goto print;

	case 'n':
		numbf = 1;
		newline();
		goto print;

	case 'N':
		newline();
		setnoaddr();
		Nflag = !Nflag;
		continue;

	case 'b':
	case 'o':
		n = getnum();
		newline();
		setdot();
		nonzero();
		if (n >= 0) {
			if (c == 'b')
				bcount = n;
			else
				ocount = n;
		}
		if (c == 'b') {
			a1 = addr2+bcount > dol ? dol : addr2 + bcount;
			doprnt(addr1, a1);
			dot = a1;
		} else {
			a1 = addr2+ocount > dol ? dol : addr2 + ocount;
			doprnt(addr2-ocount<zero+1?zero+1:addr2-ocount, a1);
			dot = addr2;
		}
		continue;

	case 'l':
		listf++;
	case 'p':
		newline();
	print:
		setdot();
		nonzero();
		doprnt(addr1, addr2);
		dot = addr2;
		continue;

	case 'P':
		setnoaddr();
		newline();
		Pflag = !Pflag;
		continue;

	case 'Q':
		fchange = 0;
	case 'q':
		setnoaddr();
		newline();
		quit(0);

	case 'r':
		filename(c);
	caseread:
		if ((io = sopen(file, READ)) < 0) {
			lastc = '\n';
			error2("cannot open input file", file);
		}
		ioeof = 0;
		setall();
		ninbuf = 0;
		if (c == 'r')
			checkpoint();
		n = zero != dol;
		rspec = (c == 'e' || !n) && file[0] != '!';
		append(getfile, addr2);
		rspec = 0;
		exfile();
		fchange = n;
		continue;

	case 's':
		setdot();
		nonzero();
		substitute(globp!=0);
		continue;

	case 't':
		move(1);
		continue;

	case 'u':
		setdot();
		newline();
		if (unddot == NULL)
			error("nothing to undo");
		undo();
		continue;

	case 'v':
		global(0, 0);
		continue;

	case 'V':
		global(0, 1);
		continue;

	case 'W':
		wrapp++;
	case 'w':
	write:
		setall();
		if (zero != dol)
			nonzero();
		filename(c);
		if(!wrapp ||
		  ((io = open(file,O_WRONLY|O_APPEND)) == -1) ||
		  ((lseek(io, 0, SEEK_END)) == -1)) {
			struct stat	st;
			if (lstat(file, &st) == 0 &&
					(st.st_mode&S_IFMT) == S_IFREG &&
					st.st_nlink == 1 &&
					(myuid==0 || myuid==st.st_uid)) {
				char	*cp, *tp;
				int	nio;
				if ((io = sopen(file, EXIST)) < 0)
					error("cannot create output file");
				if ((wrtemp = malloc(strlen(file)+8)) == NULL)
					error("out of memory");
				for (cp = file, tp = wrtemp; *cp; cp++)
					*tp++ = *cp;
				while (tp > wrtemp && tp[-1] != '/')
					tp--;
				for (cp = "\7XXXXXX"; *cp; cp++)
					*tp++ = *cp;
				*tp = '\0';
				if ((nio = mkstemp(wrtemp)) < 0) {
					free(wrtemp);
					wrtemp = NULL;
					ftruncate(io, 0);
				} else {
					close(io);
					io = nio;
				}
			} else {
				if ((io = sopen(file, WRITE)) < 0)
					error("cannot create output file");
			}
		}
		if (zero != dol) {
			ioeof = 0;
			wrapp = 0;
			putfile();
		}
		exfile();
		if (addr1==zero+1 && addr2==dol || addr1==addr2 && dol==zero)
			fchange = 0;
		if (c == 'z')
			quit(0);
		continue;

	case 'z':
		if ((peekc=getchr()) != '\n')
			error("illegal suffix");
		setnoaddr();
		goto write;

	case '=':
		setall();
		newline();
		putd((addr2-zero)&MAXCNT);
		putchr('\n');
		continue;

	case '!':
		callunix();
		continue;

	case EOF:
		return;

	}
	error("unknown command");
	}
}

static long *
address(void)
{
	register long *a1;
	register int minus, c;
	int n, relerr;

	minus = 0;
	a1 = 0;
	for (;;) {
		c = getchr();
		if ('0'<=c && c<='9') {
			n = 0;
			do {
				n *= 10;
				n += c - '0';
			} while ((c = getchr())>='0' && c<='9');
			peekc = c;
			if (a1==0)
				a1 = zero;
			if (minus<0)
				n = -n;
			a1 += n;
			minus = 0;
			continue;
		}
		relerr = 0;
		if (a1 || minus)
			relerr++;
		switch(c) {
		case ' ':
		case '\t':
			continue;
	
		case '+':
			minus++;
			if (a1==0)
				a1 = dot;
			continue;

		case '-':
		case '^':
			minus--;
			if (a1==0)
				a1 = dot;
			continue;
	
		case '?':
		case '/':
			compile(NULL, expbuf, &expbuf[ESIZE], c);
			a1 = dot;
			for (;;) {
				if (c=='/') {
					a1++;
					if (a1 > dol)
						a1 = zero;
				} else {
					a1--;
					if (a1 < zero)
						a1 = dol;
				}
				if (execute(0, a1, 0))
					break;
				if (a1==dot)
					error("search string not found");
			}
			break;
	
		case '$':
			a1 = dol;
			break;
	
		case '.':
			a1 = dot;
			break;

		case '\'':
			if ((c = getchr()) < 'a' || c > 'z')
				error("mark not lower case");
			for (a1=zero; a1<=dol; a1++)
				if (names[c-'a'] == (*a1 & ~01))
					break;
			break;
	
		default:
			peekc = c;
			if (a1==0)
				return(0);
			a1 += minus;
			if (a1<zero || a1>dol)
				error("line out of range");
			return(a1);
		}
		if (relerr)
			error("bad number");
	}
}

static void
setdot(void)
{
	if (addr2 == 0)
		addr1 = addr2 = dot;
	if (addr1 > addr2)
		error("bad range");
}

static void
setall(void)
{
	if (addr2==0) {
		addr1 = zero+1;
		addr2 = dol;
		if (dol==zero)
			addr1 = zero;
	}
	setdot();
}

static void
setnoaddr(void)
{
	if (addr2)
		error("Illegal address count");
}

static void
nonzero(void)
{
	if (addr1<=zero || addr2>dol)
		error("line out of range");
}

static void
newline(void)
{
	register int c;

	if ((c = getchr()) == '\n')
		return;
	if (c=='p' || c=='l' || c=='n') {
		pflag++;
		if (c=='l')
			listf++;
		else if (c=='n')
			numbf = 1;
		if (getchr() == '\n')
			return;
	}
	error("illegal suffix");
}

static void
filename(int comm)
{
	register char *p1, *p2;
	register int c, i;

	count = 0;
	c = getchr();
	if (c=='\n' || c==EOF) {
		p1 = savedfile;
		if (*p1==0 && comm!='f')
			error("illegal or missing filename");
		p2 = file;
		while (*p2++ = *p1++)
			;
		return;
	}
	if (c!=' ')
		error("no space after command");
	while ((c = getchr()) == ' ')
		;
	if (c=='\n')
		error("illegal or missing filename");
	i = 0;
	do {
		if (i >= FNSIZE)
			growfn("maximum of characters in file names reached");
		file[i++] = c;
		if (c==' ' && file[0] != '!' || c==EOF)
			error("illegal or missing filename");
	} while ((c = getchr()) != '\n');
	file[i++] = 0;
	if ((savedfile[0]==0 || comm=='e' || comm=='f') && file[0] != '!') {
		p1 = savedfile;
		p2 = file;
		while (*p1++ = *p2++)
			;
	}
}

static void
exfile(void)
{
	sclose(io);
	io = -1;
	if (wrtemp) {
		extern int	rename(const char *, const char *);
		if (rename(wrtemp, file) < 0)
			error("cannot create output file");
		if (myuid == 0)
			chown(file, fstbuf.st_uid, fstbuf.st_gid);
		chmod(file, fstbuf.st_mode & 07777);
		free(wrtemp);
		wrtemp = NULL;
	}
	if (vflag) {
		putd(count);
		putchr('\n');
	}
}

static void
onintr(int signo)
{
	lastsig = signo;
	putchr('\n');
	lastc = '\n';
	if (readop) {
		puts("\007read may be incomplete - beware!\007");
		fchange = 0;
	}
	error("interrupt");
}

static void
onhup(int signo)
{
	if (dol > zero && fchange) {
		addr1 = zero+1;
		addr2 = dol;
		io = creat("ed.hup", 0666);
		if (io < 0) {
			char	*home = getenv("HOME");
			if (home) {
				char	*fn = malloc(strlen(home) + 10);
				if (fn) {
					strcpy(fn, home);
					strcat(fn, "/ed.hup");
					io = creat(fn, 0666);
				}
			}
		}
		if (io >= 0)
			putfile();
	}
	fchange = 0;
	status = 0200 | signo;
	quit(0);
}

static void
onpipe(int signo)
{
	lastsig = signo;
	error("write or open on pipe failed");
}

static void
error(const char *s)
{
	error2(s, NULL);
}

static void
error2(const char *s, const char *fn)
{
	register int c;

	wrapp = 0;
	listf = 0;
	numbf = 0;
	errput(s, fn);
	count = 0;
	if (lseek(0, 0, SEEK_END) > 0)
		status = 2;
	pflag = 0;
	if (globp)
		lastc = '\n';
	globp = 0;
	peekc = lastc;
	if(lastc)
		while ((c = getchr()) != '\n' && c != EOF)
			;
	if (io > 0) {
		sclose(io);
		io = -1;
	}
	if (wrtemp) {
		unlink(wrtemp);
		free(wrtemp);
		wrtemp = NULL;
	}
	longjmp(savej, 1);
}

static void
errput(const char *s, const char *fn)
{
	prvmsg = s;
	if (fn) {
		putchr('?');
		puts(fn);
	} else
		puts("?");
	if (prhelp)
		puts(s);
}

static int
getchr(void)
{
	char c;
	if (lastc=peekc) {
		peekc = 0;
		return(lastc);
	}
	if (globp) {
		if ((lastc = *globp++) != 0)
			return(lastc);
		globp = 0;
		return(EOF);
	}
	if (read(0, &c, 1) <= 0)
		return(lastc = EOF);
	lastc = c;
	return(lastc);
}

static int
gettty(void)
{
	register int c, i;
	register char *gf;

	i = 0;
	gf = globp;
	while ((c = getchr()) != '\n') {
		if (c==EOF) {
			if (gf)
				peekc = c;
			return(c);
		}
		if (c == 0)
			continue;
		if (i >= LBSIZE)
			growlb("line too long");
		linebuf[i++] = c;
	}
	if (i >= LBSIZE-2)
		growlb("line too long");
	linebuf[i++] = 0;
	if (linebuf[0]=='.' && linebuf[1]==0)
		return(EOF);
#if !defined (SUS) && !defined (SU3)
	if (linebuf[0]=='\\' && linebuf[1]=='.' && linebuf[2]==0)
		linebuf[0]='.', linebuf[1]=0;
#endif
	return(0);
}

static long
getnum(void)
{
	char	scount[20];
	int	i;

	i = 0;
	while ((peekc=getchr()) >= '0' && peekc <= '9' && i < sizeof scount) {
		scount[i++] = peekc;
		peekc = 0;
	}
	scount[i] = '\0';
	return i ? atol(scount) : -1;
}

static int
getfile(void)
{
	register int c, i, j;
	static int	nextj;

	i = 0;
	j = nextj;
	do {
		if (--ninbuf < 0) {
			if (ioeof || (ninbuf=read(io, genbuf, LBSIZE)-1) < 0) {
				if (ioeof == 0 && ninbuf < -1) {
					puts("input error");
					status = 1;
				}
				if (i > 0) {
					puts("'\\n' appended");
					c = '\n';
					ioeof = 1;
					goto wrc;
				}
				return(EOF);
			}
			j = 0;
		}
		c = genbuf[j++]&0377;
	wrc:	if (i >= LBSIZE) {
			lastc = '\n';
			growlb("line too long");
		}
		linebuf[i++] = c ? c : '\n';
		count++;
	} while (c != '\n');
	linebuf[--i] = 0;
	nextj = j;
	if (rspec && dot == zero)
		fspec(linebuf);
	if (maxlength && i > maxlength) {
		putstr("line too long: lno = ");
		putd((dot - zero+1)&MAXCNT);
		putchr('\n');
	}
	return(0);
}

static void
putfile(void)
{
	long *a1;
	int n;
	register char *fp, *lp;
	register int nib;

	nib = 512;
	fp = genbuf;
	a1 = addr1;
	do {
		lp = getline(*a1++, 0);
		if (maxlength) {
			for (n = 0; lp[n]; n++);
			if (n > maxlength) {
				putstr("line too long: lno = ");
				putd((a1-1 - zero)&MAXCNT);
				putchr('\n');
			}
		}
		for (;;) {
			if (--nib < 0) {
				n = fp-genbuf;
				if(write(io, genbuf, n) != n)
					error("write error");
				nib = 511;
				fp = genbuf;
			}
			count++;
			if ((*fp++ = *lp++) == 0) {
				fp[-1] = '\n';
				break;
			} else if (fp[-1] == '\n')
				fp[-1] = '\0';
		}
	} while (a1 <= addr2);
	n = fp-genbuf;
	if(write(io, genbuf, n) != n)
		error("write error");
}

static int
append(int (*f)(void), long *a)
{
	register long *a1, *a2, *rdot;
	int nline, tl;

	nline = 0;
	dot = a;
	while ((*f)() == 0) {
		if ((dol-zero)+1 >= nlall) {
			long *ozero = zero;
			nlall += 512;
			if ((zero = realloc(zero, nlall*sizeof *zero))==NULL) {
				lastc = '\n';
				zero = ozero;
				error("out of memory for append");
			}
			dot += zero - ozero;
			dol += zero - ozero;
			addr1 += zero - ozero;
			addr2 += zero - ozero;
			if (unddot) {
				unddot += zero - ozero;
				unddol += zero - ozero;
			}
			if (undzero) {
				ozero = undzero;
				if ((undzero = realloc(undzero,
						nlall*sizeof *undzero)) == 0) {
					puts("no memory for undo");
					free(ozero);
				}
			}
		}
		tl = putline();
		nline++;
		a1 = ++dol;
		a2 = a1+1;
		rdot = ++dot;
		while (a1 > rdot)
			*--a2 = *--a1;
		*rdot = tl;
	}
	return(nline);
}

static void
callunix(void)
{
	char	*line;
	void (*savint)(int);
	pid_t pid, rpid;
	int retcode;

	setnoaddr();
	line = readcmd();
	if ((pid = fork()) == 0) {
		sigset(SIGHUP, oldhup);
		sigset(SIGQUIT, oldquit);
		sigset(SIGPIPE, oldpipe);
		execl(SHELL, "sh", "-c", line, NULL);
		_exit(0100);
	} else if (pid < 0)
		error("fork failed - try again");
	savint = sigset(SIGINT, SIG_IGN);
	while ((rpid = wait(&retcode)) != pid && rpid != -1)
		;
	sigset(SIGINT, savint);
	if (vflag)
		puts("!");
}

#define	cmadd(c)	((i>=cmsize ? \
		((line=realloc(line,cmsize+=128)) == 0 ? \
			(error("line too long"),0) : 0, 0) \
		: 0), line[i++]=(c))

static char *
readcmd(void)
{
	static char	*line, *prev;
	static int	cmsize, pvsize;
	char	*pp;
	int	c, mod = 0, i;

	i = 0;
	if ((c = getchr()) == '!') {
		for (pp = prev; *pp; pp++)
			line[i++] = *pp;
		mod = 1;
		c = getchr();
	}
	while (c != '\n' && c != EOF) {
		if (c == '\\') {
			c = getchr();
			if (c != '%')
				cmadd('\\');
			cmadd(c);
		} else if (c == '%') {
			for (pp = savedfile; *pp; pp++)
				cmadd(*pp);
			mod = 1;
		} else
			cmadd(c);
		c = getchr();
	}
	cmadd('\0');
	if (pvsize < cmsize && (prev = realloc(prev, pvsize=cmsize)) == 0)
		error("line too long");
	strcpy(prev, line);
	if (mod)
		nlputs(line);
	return line;
}

static void
quit(int signo)
{
	lastsig = signo;
	if (vflag && fchange) {
		fchange = 0;
		error("warning: expecting `w'");
	}
	if (wrtemp)
		unlink(wrtemp);
	unlink(tfname);
	exit(status);
}

static void
delete(void)
{
	setdot();
	newline();
	nonzero();
	checkpoint();
	rdelete(addr1, addr2);
}

static void
rdelete(long *ad1, long *ad2)
{
	register long *a1, *a2, *a3;

	a1 = ad1;
	a2 = ad2+1;
	a3 = dol;
	dol -= a2 - a1;
	do {
		*a1++ = *a2++;
	} while (a2 <= a3);
	a1 = ad1;
	if (a1 > dol)
		a1 = dol;
	dot = a1;
	fchange = 1;
}

static void
gdelete(void)
{
	register long *a1, *a2, *a3;

	a3 = dol;
	for (a1=zero+1; (*a1&01)==0; a1++)
		if (a1>=a3)
			return;
	for (a2=a1+1; a2<=a3;) {
		if (*a2&01) {
			a2++;
			dot = a1;
		} else
			*a1++ = *a2++;
	}
	dol = a1-1;
	if (dot>dol)
		dot = dol;
	fchange = 1;
}

static char *
getline(long tl, int nulterm)
{
	register char *bp, *lp;
	register long nl;

	lp = linebuf;
	bp = getblock(tl, READ);
	nl = nleft;
	tl &= ~0377;
	while (*lp++ = *bp++) {
		if (lp[-1] == '\n' && nulterm) {
			lp[-1] = '\0';
			break;
		}
		if (--nl == 0) {
			bp = getblock(tl+=0400, READ);
			nl = nleft;
		}
	}
	return(linebuf);
}

static int
putline(void)
{
	register char *bp, *lp;
	register long nl;
	long tl;

	fchange = 1;
	lp = linebuf;
	tl = tline;
	bp = getblock(tl, WRITE);
	nl = nleft;
	tl &= ~0377;
	while (*bp = *lp++) {
		if (*bp++ == '\n' && insub) {
			*--bp = 0;
			linebp = lp;
			break;
		}
		if (--nl == 0) {
			bp = getblock(tl+=0400, WRITE);
			nl = nleft;
		}
	}
	nl = tline;
	tline += (((lp-linebuf)+03)>>1)&(MAXCNT-1);
	return(nl);
}

static char *
getblock(long atl, long iof)
{
	register long bno, off;
	
	bno = (atl>>8)&BLKMSK;
	off = (atl<<1)&0774;
	if (bno >= BLKMSK) {
		lastc = '\n';
		error("temp file too big");
	}
	nleft = 512 - off;
	if (bno==iblock) {
		ichanged |= iof;
		return(ibuff+off);
	}
	if (bno==oblock)
		return(obuff+off);
	if (iof==READ) {
		if (ichanged)
			blkio(iblock, ibuff, 1);
		ichanged = 0;
		iblock = bno;
		blkio(bno, ibuff, 0);
		return(ibuff+off);
	}
	if (oblock>=0)
		blkio(oblock, obuff, 1);
	oblock = bno;
	return(obuff+off);
}

static void
blkio(long b, char *buf, int wr)
{
	lseek(tfile, b<<9, SEEK_SET);
	if ((wr ? write(tfile, buf, 512) : read (tfile, buf, 512)) != 512) {
		status = 1;
		error("I/O error on temp file");
	}
}

static void
init(void)
{
	register long *markp;

	tline = 2;
	for (markp = names; markp < &names[26]; markp++)
		*markp = 0;
	for (markp = undnames; markp < &undnames[26]; markp++)
		*markp = 0;
	subnewa = 0;
	anymarks = 0;
	iblock = -1;
	oblock = -1;
	ichanged = 0;
	tfile = maketf(tfile);
	dot = dol = zero;
	unddot = NULL;
}

static void
global(int k, int ia)
{
	register int c;
	register long *a1;
	static char *globuf;
	char	mb[MB_LEN_MAX+1];
	int	spflag = 0;

	if (globp)
		error("multiple globals not allowed");
	setall();
	nonzero();
	if ((c=GETWC(mb))=='\n')
		error("incomplete global expression");
	compile(NULL, expbuf, &expbuf[ESIZE], c);
	if (!ia) {
		globrd(&globuf, EOF);
		if (globuf[0] == '\n')
			globuf[0] = 'p', globuf[1] = '\n', globuf[2] = '\0';
	} else {
		newline();
		spflag = pflag;
		pflag = 0;
	}
	checkpoint();
	for (a1=zero; a1<=dol; a1++) {
		*a1 &= ~01;
		if (a1>=addr1 && a1<=addr2 && execute(0, a1, 0)==k)
			*a1 |= 01;
	}
	/*
	 * Special case: g/.../d (avoid n^2 algorithm)
	 */
	if (!ia && globuf[0]=='d' && globuf[1]=='\n' && globuf[2]=='\0') {
		gdelete();
		return;
	}
	for (a1=zero; a1<=dol; a1++) {
		if (*a1 & 01) {
			*a1 &= ~01;
			dot = a1;
			if (ia) {
				puts(getline(*a1, 0));
				if ((c = getchr()) == EOF)
					error("command expected");
				if (c == 'a' || c == 'c' || c == 'i')
					error("a, i, or c not allowed in G");
				else if (c == '&') {
					if ((c = getchr()) != '\n')
						error("end of line expected");
					if (globuf == 0 || *globuf == 0)
						error("no remembered command");
				} else if (c == '\n') {
					a1 = zero;
					continue;
				} else
					globrd(&globuf, c);
			}
			globp = globuf;
			commands();
			globp = NULL;
			a1 = zero;
		}
	}
	if (ia)
		pflag = spflag;
}

static void
globrd(char **globuf, register int c)
{
	register int i;

	if (*globuf == 0 && (*globuf = malloc(GBSIZE=256)) == 0)
		error("global too long");
	i = 0;
	if (c != EOF)
		(*globuf)[i++] = c;
	while ((c = getchr()) != '\n') {
		if (c==EOF)
			error("incomplete global expression");
		if (c=='\\') {
			c = getchr();
			if (c!='\n')
				(*globuf)[i++] = '\\';
		}
		(*globuf)[i++] = c;
		if (i>=GBSIZE-4 && (*globuf=realloc(*globuf,GBSIZE+=256)) == 0)
			error("global too long");
	}
	(*globuf)[i++] = '\n';
	(*globuf)[i++] = 0;
}

static void
join(void)
{
	register int i, j;
	register long *a1;

	j = 0;
	for (a1=addr1; a1<=addr2; a1++) {
		i = getline(*a1, 0) - linebuf;
		while (genbuf[j] = linebuf[i++])
			if (j++ >= LBSIZE-2)
				growlb("line too long");
	}
	i = 0;
	j = 0;
	while (linebuf[i++] = genbuf[j++])
		;
	*addr1 = putline();
	if (addr1<addr2)
		rdelete(addr1+1, addr2);
	dot = addr1;
}

static void
substitute(int inglob)
{
	register long *markp;
	register long *a1;
	intptr_t nl;
	int gsubf;

	checkpoint();
	gsubf = compsub();
	insub = 1;
	for (a1 = addr1; a1 <= addr2; a1++) {
		long *ozero;
		if (execute(0, a1, 1)==0)
			continue;
		inglob |= dosub(gsubf < 2);
		if (gsubf) {
			int	i = 1;

			while (*loc2) {
				if (execute(1, NULL, 1)==0)
					break;
				inglob |= dosub(gsubf == -1 || ++i == gsubf);
			}
		}
		subnewa = putline();
		*a1 &= ~01;
		if (anymarks) {
			for (markp = names; markp < &names[26]; markp++)
				if (*markp == *a1)
					*markp = subnewa;
		}
		*a1 = subnewa;
		ozero = zero;
		nl = append(getsub, a1);
		nl += zero-ozero;
		a1 += nl;
		addr2 += nl;
	}
	insub = 0;
	if (inglob==0)
		error("no match");
}

static int
compsub(void)
{
	register int seof, c, i;
	static char	*oldrhs;
	static int	orhssz;
	char	mb[MB_LEN_MAX+1];

	if ((seof = GETWC(mb)) == '\n' || seof == ' ')
		error("illegal or missing delimiter");
	nodelim = 0;
	compile(NULL, expbuf, &expbuf[ESIZE], seof);
	i = 0;
	for (;;) {
		c = GETWC(mb);
		if (c=='\\') {
			if (i >= RHSIZE-2)
				growrhs("replacement string too long");
			rhsbuf[i++] = c;
			c = GETWC(mb);
		} else if (c=='\n') {
			if (globp && *globp) {
				if (i >= RHSIZE-2)
					growrhs("replacement string too long");
				rhsbuf[i++] = '\\';
			}
			else if (nodelim)
				error("illegal or missing delimiter");
			else {
				peekc = c;
				pflag++;
				break;
			}
		} else if (c==seof)
			break;
		for (c = 0; c==0 || mb[c]; c++) {
			if (i >= RHSIZE-2)
				growrhs("replacement string too long");
			rhsbuf[i++] = mb[c];
		}
	}
	rhsbuf[i++] = 0;
	if (rhsbuf[0] == '%' && rhsbuf[1] == 0) {
		if (orhssz == 0)
			error("no remembered replacement string");
		strcpy(rhsbuf, oldrhs);
	} else {
		if (orhssz < RHSIZE &&
				(oldrhs = realloc(oldrhs, orhssz=RHSIZE)) == 0)
			error("replacement string too long");
		strcpy(oldrhs, rhsbuf);
	}
	if ((peekc = getchr()) == 'g') {
		peekc = 0;
		newline();
		return(-1);
	} else if (peekc >= '0' && peekc <= '9') {
		c = getnum();
		if (c < 1 || c > LBSIZE)
			error("invalid count");
		newline();
		return c;
	}
	newline();
	return(0);
}

static int
getsub(void)
{
	register char *p1, *p2;

	p1 = linebuf;
	if ((p2 = linebp) == 0)
		return(EOF);
	while (*p1++ = *p2++)
		;
	linebp = 0;
	return(0);
}

static int
dosub(int really)
{
	register char *lp, *sp;
	register int i, j, k;
	int c;

	if (!really)
		goto copy;
	i = 0;
	j = 0;
	k = 0;
	while (&linebuf[i] < loc1)
		genbuf[j++] = linebuf[i++];
	while (c = rhsbuf[k++]&0377) {
		if (c=='&') {
			j = place(j, loc1, loc2);
			continue;
		} else if (c == '\\') {
			c = rhsbuf[k++]&0377;
			if (c >='1' && c < nbra+'1') {
				j = place(j, braslist[c-'1'], braelist[c-'1']);
				continue;
			}
		}
		if (j >= LBSIZE)
			growlb("line too long");
		genbuf[j++] = c;
	}
	i = loc2 - linebuf;
	loc2 = j + linebuf;
#if defined (SUS) || defined (SU3) || defined (S42)
	if (loc1 == &linebuf[i]) {
		int	n;
		wchar_t	wc;
		if (mb_cur_max > 1 && (n = mbtowc(&wc, loc2, mb_cur_max)) > 0)
			loc2 += n;
		else
			loc2++;
	}
#endif	/* SUS || SU3 || S42 */
	while (genbuf[j++] = linebuf[i++])
		if (j >= LBSIZE)
			growlb("line too long");
	if (really) {
		lp = linebuf;
		sp = genbuf;
	} else {
	copy:	sp = linebuf;
		lp = genbuf;
	}
	while (*lp++ = *sp++)
		;
	return really;
}

static int
place(register int j, register const char *l1, register const char *l2)
{

	while (l1 < l2) {
		genbuf[j++] = *l1++;
		if (j >= LBSIZE)
			growlb("line too long");
	}
	return(j);
}

static void
move(int cflag)
{
	register long *adt, *ad1, *ad2;

	setdot();
	nonzero();
	if ((adt = address())==0)
		error("illegal move destination");
	newline();
	checkpoint();
	if (cflag) {
		long *ozero;
		intptr_t delta;
		ad1 = dol;
		ozero = zero;
		append(getcopy, ad1++);
		ad2 = dol;
		delta = zero - ozero;
		ad1 += delta;
		adt += delta;
	} else {
		ad2 = addr2;
		for (ad1 = addr1; ad1 <= ad2;)
			*ad1++ &= ~01;
		ad1 = addr1;
	}
	ad2++;
	if (adt<ad1) {
		dot = adt + (ad2-ad1);
		if ((++adt)==ad1)
			return;
		reverse(adt, ad1);
		reverse(ad1, ad2);
		reverse(adt, ad2);
	} else if (adt >= ad2) {
		dot = adt++;
		reverse(ad1, ad2);
		reverse(ad2, adt);
		reverse(ad1, adt);
	} else
		error("illegal move destination");
	fchange = 1;
}

static void
reverse(register long *a1, register long *a2)
{
	register int t;

	for (;;) {
		t = *--a2;
		if (a2 <= a1)
			return;
		*a2 = *a1;
		*a1++ = t;
	}
}

static int
getcopy(void)
{
	if (addr1 > addr2)
		return(EOF);
	getline(*addr1++, 0);
	return(0);
}

static int
execute(int gf, long *addr, int subst)
{
	register char *p1, *p2, c;

	for (c=0; c<NBRA; c++) {
		braslist[c&0377] = 0;
		braelist[c&0377] = 0;
	}
	if (gf) {
		if (circf)
			return(0);
		p1 = linebuf;
		p2 = genbuf;
		while (*p1++ = *p2++)
			;
		locs = p1 = loc2;
	} else {
		if (addr==zero)
			return(0);
		p1 = getline(*addr, 1);
		locs = 0;
	}
	needsub = subst;
	return step(p1, expbuf);
}

static void
cmplerr(int c)
{
	const char	*msg;

#if !defined (SUS) && !defined (S42) && !defined (SU3)
	expbuf[0] = 0;
#endif
	switch (c) {
	case 11:
		msg = "Range endpoint too large";
		break;
	case 16:
		msg = "bad number";
		break;
	case 25:
		msg = "`\\digit' out of range";
		break;
	case 36:
		msg = "illegal or missing delimiter";
		break;
	case 41:
		msg = "no remembered search string";
		break;
	case 42:
		msg = "'\\( \\)' imbalance";
		break;
	case 43:
		msg = "Too many `\\(' s";
		break;
	case 44:
		msg = "more than 2 numbers given";
		break;
	case 45:
		msg = "'\\}' expected";
		break;
	case 46:
		msg = "first number exceeds second";
		break;
	case 49:
		msg = "'[ ]' imbalance";
		break;
	case 50:
		msg = "regular expression overflow";
		break;
	case 67:
		msg = "illegal byte sequence";
		break;
	default:
		msg = "regular expression error";
		break;
	}
	error(msg);
}

static void
doprnt(long *bot, long *top)
{
	long	*a1;

	a1 = bot;
	do {
		if (numbf ^ Nflag) {
			putd(a1-zero);
			putchr('\t');
		}
		nlputs(getline(*a1++, 0));
	} while (a1 <= top);
	pflag = 0;
	listf = 0;
	numbf = 0;
}

static void
putd(long c)
{
	register int r;

	r = c%10;
	c /= 10;
	if (c)
		putd(c);
	putchr(r + '0');
}

static void
nlputs(register const char *sp)
{
	if (listf)
		list(sp);
	else if (tabstops)
		expand(sp);
	else
		puts(sp);
}

static void
puts(register const char *sp)
{
	while (*sp) {
		if (*sp != '\n')
			putchr(*sp++ & 0377);
		else
			sp++, putchr('\0');
	}
	putchr('\n');
}

static void
list(const char *lp)
{
	int col, n;
	wchar_t c;

	col = numbf ^ Nflag ? 8 : 0;
	while (*lp) {
		if (mb_cur_max > 1 && *lp&0200)
			n = mbtowc(&c, lp, mb_cur_max);
		else {
			n = 1;
			c = *lp&0377;
		}
		if (col+1 >= 72) {
			col = 0;
			putchr('\\');
			putchr('\n');
		}
		if (n<0 ||
#if defined (SUS) || defined (S42) || defined (SU3)
				c == '\\' ||
#endif	/* SUS || S42 || SU3 */
				!(mb_cur_max>1 ? iswprint(c) : isprint(c))) {
			if (n<0)
				n = 1;
			while (n--)
				col += lstchr(*lp++&0377);
		} else if (mb_cur_max>1) {
			col += wcwidth(c);
			while (n--)
				putchr(*lp++&0377);
		} else {
			putchr(*lp++&0377);
			col++;
		}
	}
#if defined (SUS) || defined (S42) || defined (SU3)
	putchr('$');
#endif
	putchr('\n');
}

static int
lstchr(int c)
{
	int	cad = 1, d;

#if !defined (SUS) && !defined (S42) && !defined (SU3)
	if (c=='\t') {
		c = '>';
		goto esc;
	}
	if (c=='\b') {
		c = '<';
	esc:
		putchr('-');
		putchr('\b');
		putchr(c);
	} else if (c == '\n') {
		putchr('\\');
		putchr('0');
		putchr('0');
		putchr('0');
		cad = 4;
#else	/* !SUS, !S42, !SU3 */
	if (c == '\n')
		c = '\0';
	if (c == '\\') {
		putchr('\\');
		putchr('\\');
		cad = 2;
	} else if (c == '\a') {
		putchr('\\');
		putchr('a');
		cad = 2;
	} else if (c == '\b') {
		putchr('\\');
		putchr('b');
		cad = 2;
	} else if (c == '\f') {
		putchr('\\');
		putchr('f');
		cad = 2;
	} else if (c == '\r') {
		putchr('\\');
		putchr('r');
		cad = 2;
	} else if (c == '\t') {
		putchr('\\');
		putchr('t');
		cad = 2;
	} else if (c == '\v') {
		putchr('\\');
		putchr('v');
		cad = 2;
#endif	/* !SUS, !S42, !SU3 */
	} else {
		putchr('\\');
		putchr(((c&~077)>>6)+'0');
		c &= 077;
		d = c & 07;
		putchr(c > d ? ((c-d)>>3)+'0' : '0');
		putchr(d+'0');
		cad = 4;
	}
	return cad;
}

static void
putstr(const char *s)
{
	while (*s)
		putchr(*s++);
}

static char	line[70];
static char	*linp	= line;

static void
putchr(int ac)
{
	register char *lp;
	register int c;

	lp = linp;
	c = ac;
	*lp++ = c;
	if(c == '\n' || lp >= &line[64]) {
		linp = line;
		write(1, line, lp-line);
		return;
	}
	linp = lp;
}

static void
checkpoint(void)
{
	long	*a1, *a2;

	if (undzero && globp == NULL) {
		for (a1 = zero+1, a2 = undzero+1; a1 <= dol; a1++, a2++)
			*a2 = *a1;
		unddot = &undzero[dot-zero];
		unddol = &undzero[dol-zero];
		for (a1 = names, a2 = undnames; a1 < &names[26]; a1++, a2++)
			*a2 = *a1;
	}
}

#define	swap(a, b)	(t = a, a = b, b = t)

static void
undo(void)
{
	long	*t;

	if (undzero == NULL)
		error("no undo information saved");
	swap(zero, undzero);
	swap(dot, unddot);
	swap(dol, unddol);
	swap(names, undnames);
}

static int
maketf(int fd)
{
	char	*tmpdir;

	if (fd == -1) {
		if ((tmpdir = getenv("TMPDIR")) == NULL ||
				(fd = creatf(tmpdir)) < 0)
			if ((fd = creatf("/var/tmp")) < 0 &&
					(fd = creatf("/tmp")) < 0)
				error("cannot create temporary file");
	} else
		ftruncate(fd, 0);	/* blkio() will seek to 0 anyway */
	return fd;
}

static int
creatf(const char *tmpdir)
{
	if (strlen(tmpdir) >= sizeof tfname - 9)
		return -1;
	strcpy(tfname, tmpdir);
	strcat(tfname, "/eXXXXXX");
	return mkstemp(tfname);
}

static int
sopen(const char *fn, int rdwr)
{
	int	pf[2], fd = -1;

	if (fn[0] == '!') {
		fn++;
		if (pipe(pf) < 0)
			error("write or open on pipe failed");
		switch (pipid = fork()) {
		case 0:
			if (rdwr == READ)
				dup2(pf[1], 1);
			else
				dup2(pf[0], 0);
			close(pf[0]);
			close(pf[1]);
			sigset(SIGHUP, oldhup);
			sigset(SIGQUIT, oldquit);
			sigset(SIGPIPE, oldpipe);
			execl(SHELL, "sh", "-c", fn, NULL);
			_exit(0100);
		default:
			close(pf[rdwr == READ ? 1 : 0]);
			fd = pf[rdwr == READ ? 0 : 1];
			break;
		case -1:
			error("fork failed - try again");
		}
	} else if (rdwr == READ)
		fd = open(fn, O_RDONLY);
	else if (rdwr == EXIST)
		fd = open(fn, O_WRONLY);
	else /*if (rdwr == WRITE)*/
		fd = creat(fn, 0666);
	if (fd >= 0 && rdwr == READ)
		readop = 1;
	if (fd >= 0)
		fstat(fd, &fstbuf);
	return fd;
}

static void
sclose(int fd)
{
	int	status;

	close(fd);
	if (pipid >= 0) {
		while (wait(&status) != pipid);
		pipid = -1;
	}
	readop = 0;
}

static void
fspec(const char *lp)
{
	struct termios	ts;
	const char	*cp;

	freetabs();
	maxlength = 0;
	if (tcgetattr(1, &ts) < 0
#ifdef	TAB3
			|| (ts.c_oflag&TAB3) == 0
#endif
			)
		return;
	while (lp[0]) {
		if (lp[0] == '<' && lp[1] == ':')
			break;
		lp++;
	}
	if (lp[0]) {
		lp += 2;
		while ((cp = ftok(&lp)) != NULL) {
			switch (*cp) {
			case 't':
				freetabs();
				if ((tabstops = tabstring(&cp[1])) == NULL)
					goto err;
				break;
			case 's':
				maxlength = atoi(&cp[1]);
				break;
			case 'm':
			case 'd':
			case 'e':
				break;
			case ':':
				if (cp[1] == '>') {
					if (tabstops == NULL)
						if ((tabstops = tabstring("0"))
								== NULL)
							goto err;
					return;
				}
				/*FALLTHRU*/
			default:
			err:	freetabs();
				maxlength = 0;
				errput("PWB spec problem", NULL);
				return;
			}
		}
	}
}

static const char *
ftok(const char **lp)
{
	const char	*cp;

	while (**lp && **lp != ':' && (**lp == ' ' || **lp == '\t'))
		(*lp)++;
	cp = *lp;
	while (**lp && **lp != ':' && **lp != ' ' && **lp != '\t')
		(*lp)++;
	return cp;
}

static struct tabulator *
repetitive(int repetition)
{
	struct tabulator	*tp, *tabspec;
	int	col, i;

	if ((tp = tabspec = calloc(1, sizeof *tp)) == NULL)
		return NULL;
	tp->t_rep = repetition;
	if (repetition > 0) {
		 for (col = 1+repetition, i = 0; i < 22; col += repetition) {
			if ((tp->t_nxt = calloc(1, sizeof *tp)) == NULL)
				return NULL;
			tp = tp->t_nxt;
			tp->t_tab = col;
		}
	}
	return tabspec;
}

#define	blank(c)	((c) == ' ' || (c) == '\t')

static struct tabulator *
tablist(const char *s)
{
	struct tabulator	*tp, *tabspec;
	char	*x;
	int	prev = 0, val;

	if ((tp = tabspec = calloc(1, sizeof *tp)) == NULL)
		return NULL;
	for (;;) {
		while (*s == ',')
			s++;
		if (*s == '\0' || blank(*s) || *s == ':')
			break;
		val = strtol(s, &x, 10);
		if (*s == '+')
			val += prev;
		prev = val;
		if (*s == '-' || (*x != ',' && !blank(*x) && *x != ':' &&
					*x != '\0'))
			return NULL;
		s = x;
		if ((tp->t_nxt = calloc(1, sizeof *tp)) == NULL)
			return NULL;
		tp = tp->t_nxt;
		tp->t_tab = val;
	}
	return tabspec;
}

static struct tabulator *
tabstring(const char *s)
{
	const struct {
		const char	*c_nam;
		const char	*c_str;
	} canned[] = {
		{ "a",	"1,10,16,36,72" },
		{ "a2",	"1,10,16,40,72" },
		{ "c",	"1,8,12,16,20,55" },
		{ "c2",	"1,6,10,14,49" },
		{ "c3",	"1,6,10,14,18,22,26,30,34,38,42,46,50,54,58,62,67" },
		{ "f",	"1,7,11,15,19,23" },
		{ "p",	"1,5,9,13,17,21,25,29,33,37,41,45,49,53,57,61" },
		{ "s",	"1,10,55" },
		{ "u",	"1,12,20,44" },
		{ 0,	0 }
	};

	int	i, j;

	if (s[0] == '-') {
		if (s[1] >= '0' && s[1] <= '9' && ((i = atoi(&s[1])) != 0))
			return repetitive(i);
		for (i = 0; canned[i].c_nam; i++) {
			for (j = 0; canned[i].c_nam[j]; j++)
				if (s[j+1] != canned[i].c_nam[j])
					break;
			if ((s[j+1]=='\0' || s[j+1]==':' || blank(s[j+1])) &&
					canned[i].c_nam[j] == '\0')
				return tablist(canned[i].c_str);
		}
		return NULL;
	} else
		return tablist(s);
}

static void
freetabs(void)
{
	struct tabulator	*tp;

	tp = tabstops;
	while (tp) {
		tabstops = tp->t_nxt;
		free(tp);
		tp = tabstops;
	}
}

static void
expand(const char *s)
{
	struct tabulator	*tp = tabstops;
	int	col = 0, n = 1, m, tabcnt = 0, nspc;
	wchar_t	wc;

	while (*s) {
		nspc = 0;
		switch (*s) {
		case '\n':
			putchr('\0');
			s++;
			continue;
		case '\t':
			if (tp) {
				if (tp->t_rep) {
					if (col % tp->t_rep == 0) {
						nspc++;
						col++;
					}
					while (col % tp->t_rep) {
						nspc++;
						col++;
					}
					break;
				}
				while (tp && (col>tp->t_tab || tp->t_tab == 0))
					tp = tp->t_nxt;
				if (tp && col == tp->t_tab) {
					nspc++;
					col++;
					tp = tp->t_nxt;
				}
				if (tp) {
					while (col < tp->t_tab) {
						nspc++;
						col++;
					}
					tp = tp->t_nxt;
					break;
				}
			}
			tabcnt = 1;
			nspc++;
			break;
		default:
			if (mb_cur_max>1 && (n=mbtowc(&wc, s, mb_cur_max))>0) {
				if ((m = wcwidth(wc)) > 0)
					col += m;
			} else {
				col++;
				n = 1;
			}
		}
		if (maxlength && col > maxlength) {
			putstr("\ntoo long");
			break;
		}
		if (nspc) {
			while (nspc--)
				putchr(' ');
			s++;
		} else
			while (n--)
				putchr(*s++);
	}
	if (tabcnt)
		putstr("\ntab count");
	putchr('\n');
}

static wint_t
GETWC(char *mb)
{
	int	c, n;

	n = 1;
	mb[0] = c = GETC();
	mb[1] = '\0';
	if (mb_cur_max > 1 && c&0200 && c != EOF) {
		int	m;
		wchar_t	wc;

		while ((m = mbtowc(&wc, mb, mb_cur_max)) < 0 && n<mb_cur_max) {
			mb[n++] = c = GETC();
			mb[n] = '\0';
			if (c == '\n' || c == EOF)
				break;
		}
		if (m != n)
			ERROR(67);
		return wc;
	} else
		return c;
}

static void
growlb(const char *msg)
{
	char	*olb = linebuf;
	int	i;

	LBSIZE += 512;
	if ((linebuf = realloc(linebuf, LBSIZE)) == NULL ||
			(genbuf = realloc(genbuf, LBSIZE)) == NULL)
		error(msg);
	if (linebuf != olb) {
		loc1 += linebuf - olb;
		loc2 += linebuf - olb;
		for (i = 0; i < NBRA; i++) {
			if (braslist[i])
				braslist[i] += linebuf - olb;
			if (braelist[i])
				braelist[i] += linebuf - olb;
		}
	}
}

static void
growrhs(const char *msg)
{
	RHSIZE += 256;
	if ((rhsbuf = realloc(rhsbuf, RHSIZE)) == NULL)
		error(msg);
}

static void
growfn(const char *msg)
{
	FNSIZE += 64;
	if ((savedfile = realloc(savedfile, FNSIZE)) == NULL ||
			(file = realloc(file, FNSIZE)) == NULL)
		error(msg);
	if (FNSIZE == 64)
		file[0] = savedfile[0] = 0;
}

#if defined (SUS) || defined (S42) || defined (SU3)
union	ptrstore {
	void	*vp;
	char	bp[sizeof (void *)];
};

static void *
fetchptr(const char *bp)
{
	union ptrstore	u;
	int	i;

	for (i = 0; i < sizeof (void *); i++)
		u.bp[i] = bp[i];
	return u.vp;
}

static void
storeptr(void *vp, char *bp)
{
	union ptrstore	u;
	int	i;

	u.vp = vp;
	for (i = 0; i < sizeof (void *); i++)
		bp[i] = u.bp[i];
}

#define	add(c)	((i>=LBSIZE ? (growlb("regular expression overflow"),0) : 0), \
		genbuf[i++] = (c))

#define	copy(s)	{ \
	int	m; \
	for (m = 0; m==0 || s[m]; m++) \
		add(s[m]); \
}

static char *
compile(char *unused, char *ep, const char *endbuf, int seof)
{
	INIT
	int	c, d, i;
	regex_t	*rp;
	char	*op;
	char	mb[MB_LEN_MAX+1];

	op = ep;
	ep += 2;
	if ((rp = fetchptr(ep)) == NULL) {
		if ((rp = calloc(1, sizeof *rp)) == NULL)
			ERROR(50);
		storeptr(rp, ep);
	}
	ep += sizeof (void *);
	i = 0;
	nbra = 0;
	do {
		if ((c = GETWC(mb)) == seof)
			add('\0');
		else if (c == '\\') {
			copy(mb);
			c = GETWC(mb);
			if (c == '(')
				nbra++;
			goto normchar;
		} else if (c == '[') {
			add(c);
			d = EOF;
			do {
				c = GETWC(mb);
				if (c == EOF || c == '\n')
					ERROR(49);
				copy(mb);
				if (d=='[' && (c==':' || c=='.' || c=='=')) {
					d = c;
					do {
						c = GETWC(mb);
						if (c == EOF || c == '\n')
							ERROR(49);
						copy(mb);
					} while (c != d || PEEKC() != ']');
					c = GETWC(mb);
					copy(mb);
					c = EOF;
				}
				d = c;
			} while (c != ']');
		} else {
			if (c == EOF || c == '\n') {
				if (c == '\n')
					UNGETC(c);
				mb[0] = c = '\0';
			}
			if (c == '\0')
				nodelim = 1;
	normchar:	copy(mb);
		}
	} while (genbuf[i-1] != '\0');
	if (genbuf[0]) {
		int	reflags = 0;

#ifdef	REG_ANGLES
		reflags |= REG_ANGLES;
#endif
#if defined (SU3) && defined (REG_AVOIDNULL)
		reflags |= REG_AVOIDNULL;
#endif
		if (op[0])
			regfree(rp);
		op[0] = 0;
		switch (regcomp(rp, genbuf, reflags)) {
		case 0:
			break;
		case REG_ESUBREG:
			ERROR(25);
			/*NOTREACHED*/
		case REG_EBRACK:
			ERROR(49);
			/*NOTREACHED*/
		case REG_EPAREN:
			ERROR(42);
			/*NOTREACHED*/
		case REG_BADBR:
		case REG_EBRACE:
			ERROR(45);
			/*NOTREACHED*/
		case REG_ERANGE:
			ERROR(11);
			/*NOTREACHED*/
		case REG_ESPACE:
			ERROR(50);
			/*NOTREACHED*/
		default:
			ERROR(-1);
		}
		op[0] = 1;
		circf = op[1] = genbuf[0] == '^';
	} else if (op[0]) {
		circf = op[1];
	} else
		ERROR(41);
	return ep + sizeof (void *);
}

static int
step(const char *lp, const char *ep)
{
	regex_t	*rp;
	regmatch_t	bralist[NBRA+1];
	int	eflag = 0;
	int	res;
	int	i;

	rp = fetchptr(&ep[2]);
	if (ep[0] == 0)
		return 0;
	if (locs)
		eflag |= REG_NOTBOL;
	if ((res = regexec(rp, lp, needsub? NBRA+1 : 0, bralist, eflag)) == 0 &&
			needsub) {
		loc1 = (char *)lp + bralist[0].rm_so;
		loc2 = (char *)lp + bralist[0].rm_eo;
		for (i = 1; i <= NBRA; i++) {
			if (bralist[i].rm_so != -1) {
				braslist[i-1] = (char *)lp + bralist[i].rm_so;
				braelist[i-1] = (char *)lp + bralist[i].rm_eo;
			} else
				braslist[i-1] = braelist[i-1] = NULL;
		}
	}
	return res == 0;
}
#endif	/* SUS || S42 || SU3 */

static void
help(void)
{
	const char	*desc[] = {
		"(.)a            append up to .",
		"(.)b[n]         browse n lines",
		"(.,.)c          change up to .",
		"(.,.)d          delete lines",
		"e [file]        edit file",
		"E [file]        force edit",
		"f [file]        print or set file",
		"(1,$)g/RE/cmd   global cmd",
		"(1,$)G/RE/      interactive global",
		"h               print last error",
		"H               toggle error messages",
		"help            print this screen",
		"(.)i            insert up to .",
		"(.,.+1)j        join lines",
		"(.)kx           mark line with x",
		"(.,.)l          list lines",
		"(.,.)ma         move lines to a",
		"(.,.)n          number lines",
		"N               revert n and p",
		"(.)o[n]         show n lines of context",
		"(.,.)p          print lines",
		"P               toggle prompt",
		"q               quit",
		"Q               force quit",
		"($)r            read file",
		"(.,.)s/RE/repl/ search and replace",
		"(.,.)s/RE/rp/g  replace all occurrences",
		"(.,.)s/RE/rp/n  replace n-th occurrence",
		"(.,.)ta         transfer lines to a",
		"u               undo last change",
		"(1,$)v/RE/cmd   reverse global",
		"(1,$)V/RE/      reverse i/a global",
		"(1,$)w [file]   write file",
		"(1,$)W [file]   append to file",
		"z               write buffer and quit",
		"($)=            print line number",
		"!command        execute shell command",
		"(.+1)<newline>  print one line",
		"/RE             find RE forwards",
		"?RE             find RE backwards",
		"1               first line",
		".               current line",
		"$               last line",
		",               1,$",
		";               .,$",
		NULL
	};
	char	line[100];
	int	c, half, i, k;

	half = (sizeof desc / sizeof *desc) / 2;
	for (i = 0; i < half && desc[i]; i++) {
		c = 0;
		for (k = 0; desc[i][k]; k++)
			line[c++] = desc[i][k];
		if (desc[i+half]) {
			while (c < 40)
				line[c++] = ' ';
			for (k = 0; desc[i+half][k]; k++)
				line[c++] = desc[i+half][k];
		}
		line[c] = 0;
		puts(line);
	}
}
