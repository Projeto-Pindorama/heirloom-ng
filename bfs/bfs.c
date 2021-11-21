/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*
 * Copyright 2005 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */

/*	from OpenSolaris "bfs.c	1.14	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 */
#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)bfs.c	1.15 (gritter) 7/2/05";

#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <regexpr.h>
#include <limits.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <inttypes.h>
#include <wchar.h>
#include <limits.h>
#include <errno.h>
#include "sigset.h"
static jmp_buf env;

#define	BRKTYP	long
#define	BRKSIZ	8192
#define	BRKTWO	4
#define	BFSTRU	-1L
#define	BFSBUF	512

struct Comd {
	int Cnumadr;
	long Cadr[2];
	char Csep;
	char Cop;
};

static long Dot, Dollar;
static long markarray[26], *mark;
static int fstack[15] = {1, -1};
static int infildes = 0;
static int outfildes = 1;
static char *internal, *intptr;
static size_t internalsize;
static char *comdlist;
static size_t comdlistsize;
static char charbuf = '\n';
static int peeked;
static char *currex;
static size_t currexsize;
static long trunc = BFSTRU;
static int crunch = -1;
static off_t *segblk, prevblk;
static short *segoff, prevoff;
static int txtfd;
static int oldfd = 0;
static int flag4 = 0;
static int flag3 = 0;
static int flag2 = 0;
static int flag1 = 0;
static int flag = 0;
static int lprev = 1;
static int status[1];
static BRKTYP *lincnt;
static char *perbuf;
static char *glbuf;
static char tty, *bigfile;
static char *fle;
static size_t flesize;
static char prompt = 1;
static char verbose = 1;	/* 1=print # of bytes read in; 0=silent. */
static char *varray[10];	/* Holds xv cmd parameters. */
static size_t varraysize[10];
static long long outcnt;
static char strtmp[PATH_MAX+32];
static int mb_cur_max;
static int errcnt;

static void reset(int);
static void begin(struct Comd *p);
static int  bigopen(const char *file);
static void sizeprt(long long blk, int off);
static long bigread(long l, char **rec, size_t *recsize);
static int gcomd(struct Comd *p, int k);
static int fcomd(struct Comd *p);
static void ecomd(void);
static int kcomd(struct Comd *p);
static int xncomd(struct Comd *p);
static int pcomd(struct Comd *p);
static int qcomd(void);
static int xcomds(struct Comd *p);
static int xbcomd(struct Comd *p);
static int xccomd(struct Comd *p);
static int xfcomd(struct Comd *p);
static int xocomd(struct Comd *p);
static int xtcomd(struct Comd *p);
static int xvcomd(void);
static int wcomd(struct Comd *p);
static int nlcomd(struct Comd *p);
static int eqcomd(struct Comd *p);
static int colcomd(struct Comd *p);
static int excomd(void);
static int xcomdlist(struct Comd *p);
static int defaults(struct Comd *p, int prt, int max,
		    long def1, long def2, int setdot, int errsok);
static int getcomd(struct Comd *p, int prt);
static int getadrs(struct Comd *p, int prt);
static int getadr(struct Comd *p, int prt);
static int getnumb(struct Comd *p, int prt);
static int rdnumb(int prt);
static int getrel(struct Comd *p, int prt);
static int getmark(struct Comd *p, int prt);
static int getrex(struct Comd *p, int prt, char c);
static int hunt(int prt, char rex[], long start,
		int down, int wrap, int errsok);
static int jump(int prt, char label[]);
static int getstr(int prt, char **buf, size_t *size,
		char brk, char ignr, int nonl);
static int regerr(int c);
static int err(int prt, const char msg[]);
static char mygetc(void);
static int readc(int f, char *c);
static int percent(char **line, size_t *linesize);
static int newfile(int prt, char f[]);
static void push(int s[], int d);
static int pop(int s[]);
static int peekc(void);
static void eat(void);
static int more(void);
static void quit(void);
static void out(char *ln, long length);
static char *untab(const char *l);
static int patoi(const char *b);
static int equal(const char *a, const char *b);
static void intcat(const char *);
static void *grow(void *ptr, size_t size, const char *msg);

int
main(int argc, char *argv[])
{
	struct Comd comdstruct, *p;
	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	if (argc < 2 || argc > 3) {
		err(1, "arg count");
		quit();
	}
	mark = markarray-'a';
	if (argc == 3) {
		verbose = 0;
	}
	setbuf(stdout, 0);
	if (bigopen(bigfile = argv[argc-1]))
		quit();
	tty = isatty(0);
	p = &comdstruct;
	/* Look for 0 or more non-'%' char followed by a '%' */
	perbuf = compile("[^%]*%", NULL, NULL);
	if (regerrno)
		regerr(regerrno);
	setjmp(env);
	sigset(SIGINT, reset);
	err(0, "");
	printf("\n");
	flag = 0;
	prompt = 0;
	/*CONSTCOND*/	for (;;)
		begin(p);
	/*NOTREACHED*/
	return 0;
}

static void
reset(int signo)		/* for longjmp on signal */
{
	sigrelse(signo);
	longjmp(env, 1);
}

static void
begin(struct Comd *p)
{
	char	*line = NULL;
	size_t	linesize = 0;
	strtagn:
	if (flag == 0)
		eat();
	if (infildes != 100) {
		if (infildes == 0 && prompt)
			printf("*");
		flag3 = 1;
		if (getstr(1, &line, &linesize, 0, 0, 0))
			exit(1);
		flag3 = 0;
		if (percent(&line, &linesize) < 0)
			goto strtagn;
		newfile(1, "");
	}
	if (!(getcomd(p, 1) < 0)) {
		switch (p->Cop) {
		case 'e':
			if (!flag)
				ecomd();
			else
				err(0, "");
			break;

		case 'f':
			fcomd(p);
			break;

		case 'g':
			if (flag == 0)
				gcomd(p, 1);
			else
				err(0, "");
			break;

		case 'k':
			 kcomd(p);
			break;

		case 'p':
			pcomd(p);
			break;

		case 'q':
			qcomd();
			break;

		case 'v':
			if (flag == 0)
				gcomd(p, 0);
			else
				err(0, "");
			break;

		case 'x':
			if (!flag)
				xcomds(p);
			else
				err(0, "");
			break;

		case 'w':
			wcomd(p);
			break;

		case '\n':
			 nlcomd(p);
			break;

		case '=':
			eqcomd(p);
			break;

		case ':':
			colcomd(p);
			break;

		case '!':
			excomd();
			break;

		case 'P':
			prompt = !prompt;
			break;

		default:
			if (flag)
				err(0, "");
			else
				err(1, "bad command");
			break;
		}
	}
	free(line);
}

static int
bigopen(const char *file)
{
	long l, off, cnt;
	off_t blk;
	long s;
	int newline, n;
	char block[512];
	size_t totsiz;
	const char	toomany[] = "too many lines";
	if ((txtfd = open(file, O_RDONLY)) < 0)
		return (err(1, "can't open"));
	blk = -1;
	newline = 1;
	l = cnt = s = 0;
	off = 512;
	totsiz = 0;
	lincnt = grow(lincnt, BRKSIZ * sizeof *lincnt, toomany);
	segblk = grow(segblk, 512 * sizeof *segblk, toomany);
	segoff = grow(segoff, 512 * sizeof *segblk, toomany);
	totsiz += BRKSIZ;
	while ((n = read(txtfd, block, 512)) > 0) {
		blk++;
		for (off = 0; off < n; off++) {
			if (newline) {
				newline = 0;
				if (l > 0 && !(l&07777)) {
					totsiz += BRKSIZ;
					lincnt = grow(lincnt,
						totsiz * sizeof *lincnt,
						toomany);
				}
				lincnt[l] = cnt;
				cnt = 0;
				if (l == LONG_MAX)
					return err(1, toomany);
				if (!(l++ & 077)) {
					if (!(l-1 & 077777)) {
						if (s >= LONG_MAX - 512)
							return err(1, toomany);
						segblk = grow(segblk,
							(s + 512) *
							sizeof *segblk,
							toomany);
						segoff = grow(segoff,
							(s + 512) *
							sizeof *segoff,
							toomany);
					}
					segblk[s] = blk;
					segoff[s++] = off;
				}
			}
			if (block[off] == '\n') newline = 1;
			if (cnt == LONG_MAX)
				return err(1, "line too long");
			cnt++;
		}
	}
	if (n < 0) {
		errcnt = 1;
		puts("i/o error");
	}
	if (!(l&07777)) {
		totsiz += BRKTWO;
		lincnt = grow(lincnt, totsiz * sizeof *lincnt, toomany);
	}
	lincnt[Dot = Dollar = l] = cnt;
	sizeprt(blk, off);
	return (0);
}

static void
sizeprt(long long blk, int off)
{
	if (verbose)
		printf("%lld", 512*blk+off);
}

static off_t saveblk = -1;

static long
bigread(long l, char **rec, size_t *recsize)
{
	long i;
	char *b;
	long r;
	int off;
	static char savetxt[512];
	static int rd;

	if ((i = l-lprev) == 1) prevoff += lincnt[lprev];
	else if (i >= 0 && i <= 32)
		for (i = lprev; i < l; i++) prevoff += lincnt[i];
	else if (i < 0 && i >= -32)
		for (i = lprev-1; i >= l; i--) prevoff -= lincnt[i];
	else {
		prevblk = segblk[i = (l-1)>>6];
		prevoff = segoff[i];
		for (i = (i<<6)+1; i < l; i++) prevoff += lincnt[i];
	}

	prevblk += prevoff>>9;
	prevoff &= 0777;
	lprev = l;
	if (prevblk != saveblk) {
		lseek(txtfd, (saveblk = prevblk)<<9, 0);
		rd = read(txtfd, savetxt, 512);
	}
	r = 0;
	off = prevoff;
	while (rd > 0) {
		for (b = savetxt+off; b < savetxt+rd; b++) {
			if (r+1 >= *recsize) {
				char	*nrec;
				nrec = realloc(*rec, *recsize += BFSBUF);
				if (nrec == NULL) {
					write(2, "Line too long--"
						 "output truncated\n", 32);
					goto out;
				}
				*rec = nrec;
			}
			if (*b == '\n') {
			out:	(*rec)[r] = '\0';
				return r;
			}
			(*rec)[r++] = *b;
		}
		if ((i = read(txtfd, savetxt, 512)) == 0) {
			puts("'\\n' appended");
			goto out;
		}
		rd = i;
		off = 0;
		saveblk++;
	}
	if (rd < 0) {
		puts("i/o error");
		saveblk = -1;
	}
	if (*rec == 0)
		*rec = grow(*rec, *recsize = 1, "no space");
	goto out;
}

static void
ecomd(void)
{
	int i = 0;
	while (peekc() == ' ')
		mygetc();
	do {
		if (i >= flesize)
			fle = grow(fle, flesize += 80, "file name too long");
	} while ((fle[i++] = mygetc()) != '\n');
	fle[--i] = '\0';
	/* Without this, ~20 "e" cmds gave "can't open" msg. */
	close(txtfd);
	free(lincnt);
	lincnt = NULL;
	/* Reset parameters. */
	lprev = 1;
	prevblk = 0;
	prevoff = 0;
	saveblk = -1;
	if (bigopen(bigfile  = fle))
		quit();
	printf("\n");
}

static int
fcomd(struct Comd *p)
{
	if (more() || defaults(p, 1, 0, 0, 0, 0, 0))
		return (-1);
	printf("%s\n", bigfile);
	return (0);
}

static int
gcomd(struct Comd *p, int k)
{
	char d;
	long i, end;
	char *line = NULL;
	size_t linesize = 0;
	if (defaults(p, 1, 2, 1, Dollar, 0, 0))
		return (-1);
	if ((d = mygetc()) == '\n')
		return (err(1, "syntax"));
	if (peekc() == d)
		mygetc();
	else
		if (getstr(1, &currex, &currexsize, d, 0, 1))
			return (-1);
	glbuf = compile(currex, NULL, NULL);
	if (regerrno) {
		regerr(regerrno);
		return (-1);
	}

	if (getstr(1, &comdlist, &comdlistsize, 0, 0, 0)) {
		free(glbuf);
		return (-1);
	}
	i = p->Cadr[0];
	end = p->Cadr[1];
	while (i <= end) {
		bigread(i, &line, &linesize);
		if (!(step(line, glbuf))) {
			if (!k) {
				Dot = i;
				if (xcomdlist(p)) {
					free(glbuf);
					free(line);
					return (err(1, "bad comd list"));
				}
			}
			i++;
		} else {
			if (k) {
				Dot = i;
				if (xcomdlist(p)) {
					free(glbuf);
					free(line);
					return (err(1, "bad comd list"));
				}
			}
			i++;
		}
	}
	free(line);
	free(glbuf);
	return (0);
}

static int
kcomd(struct Comd *p)
{
	int c;
	if ((c = peekc()) < 'a' || c > 'z')
		return (err(1, "bad mark"));
	mygetc();
	if (more() || defaults(p, 1, 1, Dot, 0, 1, 0))
		return (-1);
	mark[c] = Dot = p->Cadr[0];
	return (0);
}

static int
xncomd(struct Comd *p)
{
	int c;
	if (more() || defaults(p, 1, 0, 0, 0, 0, 0))
		return (-1);

	for (c = 'a'; c <= 'z'; c++)
		if (mark[c])
			printf("%c\n", c);

	return (0);
}

static int
pcomd(struct Comd *p)
{
	long i, n;
	char *line = NULL;
	size_t linesize = 0;
	if (more() || defaults(p, 1, 2, Dot, Dot, 1, 0))
		return (-1);
	for (i = p->Cadr[0]; i <= p->Cadr[1] && i > 0; i++) {
		n = bigread(i, &line, &linesize);
		out(line, n);
	}
	free(line);
	return (0);
}

static int
qcomd(void)
{
	if (more())
		return (-1);
	quit();
	return (0);
}

static int
xcomds(struct Comd *p)
{
	switch (mygetc()) {
	case 'b':	return (xbcomd(p));
	case 'c':	return (xccomd(p));
	case 'f':	return (xfcomd(p));
	case 'n':	return (xncomd(p));
	case 'o':	return (xocomd(p));
	case 't':	return (xtcomd(p));
	case 'v':	return (xvcomd());
	default:	return (err(1, "bad command"));
	}
}

#define	Return	free(str); return
static int
xbcomd(struct Comd *p)
{
	int fail,  n = 0;
	char d;
	char *str = NULL;
	size_t strsize = 0;

	fail = 0;
	if (defaults(p, 0, 2, Dot, Dot, 0, 1))
		fail = 1;
	else {
		if ((d = mygetc()) == '\n') {
			Return (err(1, "syntax"));
		}
		if (d == 'z') {
			if (status[0] != 0) {
				Return (0);
			}
			mygetc();
			if (getstr(1, &str, &strsize, 0, 0, 0)) {
				Return (-1);
			}
			Return (jump(1, str));
		}
		if (d == 'n') {
			if (status[0] == 0) {
				Return (0);
			}
			mygetc();
			if (getstr(1, &str, &strsize, 0, 0, 0)) {
				Return (-1);
			}
			Return (jump(1, str));
		}
		if (getstr(1, &str, &strsize, d, ' ', 0)) {
			Return (-1);
		}
		if ((n = hunt(0, str, p->Cadr[0]-1, 1, 0, 1)) < 0)
			fail = 1;
		if (getstr(1, &str, &strsize, 0, 0, 0)) {
			Return (-1);
		}
		if (more()) {
			Return (err(1, "syntax"));
		}
	}
	if (!fail) {
		Dot = n;
		Return (jump(1, str));
	}
	Return (0);
}
#undef	Return

static int
xccomd(struct Comd *p)
{
	char *arg = NULL;
	size_t argsize = 0;
	if (getstr(1, &arg, &argsize, 0, ' ', 0) ||
			defaults(p, 1, 0, 0, 0, 0, 0)) {
		free(arg);
		return (-1);
	}
	if (equal(arg, ""))
		crunch = -crunch;
	else if (equal(arg, "0"))
		crunch = -1;
	else if (equal(arg, "1"))
		crunch = 1;
	else {
		free(arg);
		return (err(1, "syntax"));
	}

	free(arg);
	return (0);
}

static int
xfcomd(struct Comd *p)
{
	char fl[100];
	char *f;
	if (defaults(p, 1, 0, 0, 0, 0, 0))
		return (-1);

	while (peekc() == ' ')
		mygetc();
	for (f = fl; (*f = mygetc()) != '\n'; f++);
	if (f == fl)
		return (err(1, "no file"));
	*f = '\0';

	return (newfile(1, fl));
}

#define	Return	free(arg); return
static int
xocomd(struct Comd *p)
{
	int fd;
	char *arg = NULL;
	size_t argsize = 0;

	if (getstr(1, &arg, &argsize, 0, ' ', 0) ||
			defaults(p, 1, 0, 0, 0, 0, 0)) {
		Return (-1);
	}

	if (!arg[0]) {
		if (outfildes == 1) {
			Return (err(1, "no diversion"));
		}
		close(outfildes);
		outfildes = 1;
	} else {
		if (outfildes != 1) {
			Return (err(1, "already diverted"));
		}
		if ((fd = open(arg, O_WRONLY|O_CREAT|O_TRUNC, 0666)) < 0) {
			Return (err(1, "can't create"));
		}
		outfildes = fd;
	}
	Return (0);
}
#undef	Return

static int
xtcomd(struct Comd *p)
{
	int t;

	while (peekc() == ' ')
		mygetc();
	if ((t = rdnumb(1)) < 0 || more() || defaults(p, 1, 0, 0, 0, 0, 0))
		return (-1);

	trunc = t;
	return (0);
}

static int
xvcomd(void)
{
	int c;
	int i;
	int temp0 = -1, temp1 = -1, temp2 = -1;
	int fildes[2];

	if ((c = peekc()) < '0' || c > '9')
		return (err(1, "digit required"));
	mygetc();
	c -= '0';
	while (peekc() == ' ')
		mygetc();
	if (peekc() == '\\')
		mygetc();
	else if (peekc()  ==  '!') {
		if (pipe(fildes) < 0) {
			printf("Try again");
			return (-1);
		}
		temp0 = dup(0);
		temp1 = dup(1);
		temp2 = infildes;
		close(0);
		dup(fildes[0]);
		close(1);
		dup(fildes[1]);
		close(fildes[0]);
		close(fildes[1]);
		mygetc();
		flag4 = 1;
		excomd();
		close(1);
		infildes = 0;
	}
	i = 0;
	do {
		if (i >= varraysize[c])
			varray[c] = grow(varray[c], varraysize[c] += 100,
					"command too long");
	} while ((varray[c][i++] = mygetc()) != '\n');
	varray[c][i-1] = '\0';
	if (flag4) {
		infildes = temp2;
		close(0);
		dup(temp0);
		close(temp0);
		dup(temp1);
		close(temp1);
		flag4 = 0;
		charbuf = ' ';
	}
	return (0);
}

#define	Return	free(arg); free(line); return
static int
wcomd(struct Comd *p)
{
	long i, n;
	int fd, savefd;
	int savecrunch, savetrunc;
	char *arg = NULL, *line = NULL;
	size_t argsize = 0, linesize = 0;

	if (getstr(1, &arg, &argsize, 0, ' ', 0) ||
			defaults(p, 1, 2, 1, Dollar, 1, 0)) {
		Return (-1);
	}
	if (!arg[0]) {
		Return (err(1, "no file name"));
	}
	if (equal(arg, bigfile)) {
		Return (err(1, "no change indicated"));
	}
	if ((fd = open(arg, O_WRONLY|O_CREAT|O_TRUNC, 0666)) < 0) {
		Return (err(1, "can't create"));
	}

	savefd = outfildes;
	savetrunc = trunc;
	savecrunch = crunch;
	outfildes = fd;
	trunc = BFSTRU;
	crunch = -1;

	outcnt = 0;
	for (i = p->Cadr[0]; i <= p->Cadr[1] && i > 0; i++) {
		n = bigread(i, &line, &linesize);
		out(line, n);
	}
	if (verbose)
		printf("%lld\n", outcnt);
	close(fd);

	outfildes = savefd;
	trunc = savetrunc;
	crunch = savecrunch;
	Return (0);
}
#undef	Return

static int
nlcomd(struct Comd *p)
{
	if (defaults(p, 1, 2, Dot+1, Dot+1, 1, 0)) {
		mygetc();
		return (-1);
	}
	return (pcomd(p));
}

static int
eqcomd(struct Comd *p)
{
	if (more() || defaults(p, 1, 1, Dollar, 0, 0, 0))
		return (-1);
	printf("%ld\n", p->Cadr[0]);
	return (0);
}

static int
colcomd(struct Comd *p)
{
	return (defaults(p, 1, 0, 0, 0, 0, 0));
}

static int
xcomdlist(struct Comd *p)
{
	flag = 1;
	flag2 = 1;
	newfile(1, "");
	while (flag2)
		begin(p);
	if (flag == 0)
		return (1);
	flag = 0;
	return (0);
}

static int
excomd(void)
{
	pid_t i;
	int j;
	if (infildes != 100)
		charbuf = '\n';
	while ((i = fork()) < (pid_t)0)
		sleep(10);
	if (i == (pid_t)0) {
		/* Guarantees child can be intr. */
		sigset(SIGINT, SIG_DFL);
		if (infildes == 100 || flag4) {
			execl(SHELL, "sh", "-c", intptr, NULL);
			_exit(0);
		}
		if (infildes != 0) {
			close(0);
			dup(infildes);
		}
		for (j = 3; j < 15; j++) close(j);
		execl(SHELL, "sh", "-t", NULL);
		_exit(0);
	}
	sigset(SIGINT, SIG_IGN);
	while (wait(status) != i);
	status[0] = status[0] >> 8;

	sigset(SIGINT, reset);	/* Restore signal to previous status */

	if (((infildes == 0) || ((infildes  == 100) &&
	    (fstack[fstack[0]] == 0)))&& verbose && (!flag4))
		printf("!\n");
	return (0);
}

static int
defaults(struct Comd *p, int prt, int max,
    long def1, long def2, int setdot, int errsok)
{
	if (!def1)
		def1 = Dot;
	if (!def2)
		def2 = def1;
	if (p->Cnumadr >= max)
		return (errsok?-1:err(prt, "adr count"));
	if (p->Cnumadr < 0) {
		p->Cadr[++p->Cnumadr] = def1;
		p->Cadr[++p->Cnumadr] = def2;
	} else if (p->Cnumadr < 1)
		p->Cadr[++p->Cnumadr] = p->Cadr[0];
	if (p->Cadr[0] < 1 || p->Cadr[0] > Dollar ||
	    p->Cadr[1] < 1 || p->Cadr[1] > Dollar)
		return (errsok?-1:err(prt, "range"));
	if (p->Cadr[0] > p->Cadr[1])
		return (errsok?-1:err(prt, "adr1 > adr2"));
	if (setdot)
		Dot = p->Cadr[1];
	return (0);
}

static int
getcomd(struct Comd *p, int prt)
{
	int r;
	int c;

	p->Cnumadr = -1;
	p->Csep = ' ';
	switch (c = peekc()) {
	case ',':
	case ';':	p->Cop = mygetc();
		return (0);
	}

	if ((r = getadrs(p, prt)) < 0)
		return (r);

	if ((c = peekc()) < 0)
		return (err(prt, "syntax"));
	if (c == '\n')
		p->Cop = '\n';
	else
		p->Cop = mygetc();

	return (0);
}

static int
getadrs(struct Comd *p, int prt)
{
	int r;
	char c;

	if ((r = getadr(p, prt)) < 0)
		return (r);

	switch (c = peekc()) {
		case ';':
			Dot = p->Cadr[0];
			mygetc();
			p->Csep = c;
			return (getadr(p, prt));
		case ',':
			mygetc();
			p->Csep = c;
			return (getadr(p, prt));
		}

	return (0);
}

static int
getadr(struct Comd *p, int prt)
{
	int r;
	char c;

	r = 0;
	while (peekc() == ' ')
		mygetc();	/* Ignore leading spaces */
	switch (c = peekc()) {
		case '\n':
		case ',':
		case ';':	return (0);
		case '\'':	mygetc();
			r = getmark(p, prt);
			break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':	r = getnumb(p, prt);
			break;
		case '.':	mygetc();
			p->Cadr[++p->Cnumadr] = Dot;
			break;
		case '+':
		case '-':	p->Cadr[++p->Cnumadr] = Dot;
			break;
		case '$':	mygetc();
			p->Cadr[++p->Cnumadr] = Dollar;
			break;
		case '^':	mygetc();
			p->Cadr[++p->Cnumadr] = Dot - 1;
			break;
		case '/':
		case '?':
		case '>':
		case '<':	mygetc();
			r = getrex(p, prt, c);
			break;
		default:	return (0);
		}

	if (r == 0)
		r = getrel(p, prt);
	return (r);
}

static int
getnumb(struct Comd *p, int prt)
{
	long i;

	if ((i = rdnumb(prt)) < 0)
		return (-1);
	p->Cadr[++p->Cnumadr] = i;
	return (0);
}

static int
rdnumb(int prt)
{
	char num[20],  *n;
	int i;

	n = num;
	while ((*n = peekc()) >= '0' && *n <= '9') {
		n++;
		mygetc();
	}

	*n = '\0';
	if ((i = patoi(num)) >= 0)
		return (i);
	return (err(prt, "bad num"));
}

static int
getrel(struct Comd *p, int prt)
{
	int op, n;
	char c;
	int j;

	n = 0;
	op = 1;
	while ((c = peekc()) == '+' || c == '-') {
		if (c == '+')
			n++;
		else
			n--;
		mygetc();
	}
	j = n;
	if (n < 0)
		op = -1;
	if (c == '\n')
		p->Cadr[p->Cnumadr] += n;
	else {
		if ((n = rdnumb(0)) > 0 && p->Cnumadr >= 0) {
			p->Cadr[p->Cnumadr] += op*n;
			getrel(p, prt);
		} else {
			p->Cadr[p->Cnumadr] += j;
		}
	}
	return (0);
}

static int
getmark(struct Comd *p, int prt)
{
	int c;

	if ((c = peekc()) < 'a' || c > 'z')
		return (err(prt, "bad mark"));
	mygetc();

	if (!mark[c])
		return (err(prt, "undefined mark"));
	p->Cadr[++p->Cnumadr] = mark[c];
	return (0);
}

static int
getrex(struct Comd *p, int prt, char c)
{
	int down = -1, wrap = -1;
	long start;

	if (peekc() == c)
		mygetc();
	else if (getstr(prt, &currex, &currexsize, c, 0, 1))
		return (-1);

	switch (c) {
	case '/':	down = 1; wrap = 1; break;
	case '?':	down = 0; wrap = 1; break;
	case '>':	down = 1; wrap = 0; break;
	case '<':	down = 0; wrap = 0; break;
	}

	if (p->Csep == ';')
		start = p->Cadr[0];
	else
		start = Dot;

	if ((p->Cadr[++p->Cnumadr] = hunt(prt, currex, start, down, wrap, 0))
	    < 0)
		return (-1);
	return (0);
}

static int
hunt(int prt, char rex[], long start, int down, int wrap, int errsok)
{
	long i, end1, incr;
	long start1, start2;
	char *line = NULL;
	size_t linesize = 0;
	char *rebuf;

	if (down) {
		start1 = start + 1;
		end1 = Dollar;
		start2 = 1;
		incr = 1;
	} else {
		start1 = start  - 1;
		end1 = 1;
		start2 = Dollar;
		incr = -1;
	}

	rebuf = compile(rex, NULL, NULL);
	if (regerrno)
		regerr(regerrno);

	for (i = start1; i != end1+incr; i += incr) {
		bigread(i, &line, &linesize);
		if (step(line, rebuf)) {
			free(rebuf);
			free(line);
			return (i);
		}
	}

	if (!wrap) {
		free(rebuf);
		free(line);
		return (errsok?-1:err(prt, "not found"));
	}

	for (i = start2; i != start1; i += incr) {
		bigread(i, &line, &linesize);
		if (step(line, rebuf)) {
			free(rebuf);
			free(line);
			return (i);
		}
	}

	free(rebuf);
	free(line);
	return (errsok?-1:err(prt, "not found"));
}

static int
jump(int prt, char label[])
{
	size_t n;
	char *line = NULL;
	size_t linesize = 0;
	char *rebuf;

	if (infildes == 0 && tty)
		return (err(prt, "jump on tty"));
	if (infildes == 100)
		intptr = internal;
	else
		lseek(infildes, 0L, 0);

	snprintf(strtmp, sizeof strtmp, "^: *%s$", label);
	rebuf = compile(strtmp, NULL, NULL);
	if (regerrno) {
		regerr(regerrno);
		return (-1);
	}

	n = 0;
	for (;;) {
		if (n >= linesize)
			line = grow(line, linesize += 256, "line too long");
		if (readc(infildes, &line[n]) == 0)
			break;
		if (line[n] == '\n') {
			line[n] = '\0';
			if (step(line, rebuf)) {
				charbuf = '\n';
				free(rebuf);
				free(line);
				return (peeked = 0);
			}
			n = 0;
		} else
			n++;
	}

	free(line);
	free(rebuf);
	return (err(prt, "label not found"));
}

#define	check(t)	\
	if ((t) - 1 >= *size) { \
		*buf = grow(*buf, *size += 100, "command too long"); \
	}
static int
getstr(int prt, char **buf, size_t *size, char brk, char ignr, int nonl)
{
	char c, prevc;
	size_t	n;

	if (*buf == NULL)
		*buf = grow(*buf, *size = 100, "command too long");
	prevc = 0;
	for (n = 0; c = peekc(); prevc = c) {
		if (c == '\n') {
			if (prevc == '\\' && (!flag3)) {
				check(n-1);
				(*buf)[n-1] = mygetc();
			}
			else if (prevc == '\\' && flag3) {
				check(n);
				(*buf)[n++] = mygetc();
			} else if (nonl)
				break;
			else {
				check(n);
				return ((*buf)[n] = '\0');
			}
		} else {
			mygetc();
			if (c == brk) {
				if (prevc == '\\') {
					check(n-1);
					(*buf)[n-1] = c;
				}
				else {
					check(n);
					return ((*buf)[n] = '\0');
				}
			} else if (n != 0 || c != ignr) {
				check(n);
				(*buf)[n++] = c;
			}
		}
	}
	return (err(prt, "syntax"));
}

static int
regerr(int c)
{
	if (prompt) {
		switch (c) {
		case 11: printf("Range endpoint too large.\n");
			break;
		case 16: printf("Bad number.\n");
			break;
		case 25: printf("``\\digit'' out of range.\n");
			break;
		case 41: printf("No remembered search string.\n");
			break;
		case 42: printf("() imbalance.\n");
			break;
		case 43: printf("Too many (.\n");
			break;
		case 44: printf("More than 2 numbers given in { }.\n");
			break;
		case 45: printf("} expected after \\.\n");
			break;
		case 46: printf("First number exceeds second in { }.\n");
			break;
		case 49: printf("[] imbalance.\n");
			break;
		case 50: printf("Regular expression overflow.\n");
			break;
		case 67: printf("Illegal byte sequence.\n");
			break;
		default: printf("RE error.\n");
			break;
		}
	} else {
		printf("?\n");
	}
	return (-1);
}

static int
err(int prt, const char msg[])
{
	if (prt) (prompt? printf("%s\n", msg): printf("?\n"));
	if (infildes != 0) {
		infildes = pop(fstack);
		charbuf = '\n';
		peeked = 0;
		flag3 = 0;
		flag2 = 0;
		flag = 0;
	}
	return (-1);
}

static char
mygetc(void)
{
	if (!peeked) {
		while ((!(infildes == oldfd && flag)) && (!flag1) &&
		    (!readc(infildes, &charbuf))) {
			if (infildes == 100 && (!flag)) flag1 = 1;
			if ((infildes = pop(fstack)) == -1) quit();
			if ((!flag1) && infildes == 0 && flag3 && prompt)
				printf("*");
		}
		if (infildes == oldfd && flag) flag2 = 0;
		flag1 = 0;
	} else peeked = 0;
	return (charbuf);
}

static int
readc(int f, char *c)
{
	if (f == 100) {
		if (!(*c = *intptr++)) {
			intptr--;
			charbuf = '\n';
			return (0);
		}
	} else if (read(f, c, 1) != 1) {
		close(f);
		charbuf = '\n';
		return (0);
	}
	return (1);
}

static int
percent(char **line, size_t *linesize)
{
	char *var;
	char *front, *per, c[2], *olp, p[2], *fr = NULL;
	int i, j;

	per = p;
	var = c;
	j = 0;
	while (!j) {
		fr = grow(fr, strlen(*line) + 1, "command too long");
		front = fr;
		j = 1;
		olp = *line;
		intptr = internal;
		while (step(olp, perbuf)) {
			while (loc1 < loc2) *front++ = *loc1++;
			*(--front) = '\0';
			front = fr;
			*per++ = '%';
			*per = '\0';
			per = p;
			*var = *loc2;
			if ((i = 1 + strlen(front)) >= 2 && fr[i-2] == '\\') {
				intcat(front);
				--intptr;
				intcat(per);
			} else {
				if (!(*var >= '0' && *var <= '9')) {
					free(fr);
					return (err(1, "usage: %digit"));
				}
				intcat(front);
				intcat(varray[*var-'0']);
				j  = 0;
				loc2++;	/* Compensate for removing --lp above */
			}
			olp = loc2;
		}
		intcat(olp);
		if (!j) {
			intptr = internal;
			i = 0;
			do {
				if (i >= *linesize)
					*line = grow(*line, *linesize += 100,
							"command too long");
				(*line)[i] = intptr[i];
			} while (intptr[i++]);
		}
	}
	free(fr);
	return (0);
}

static int
newfile(int prt, char f[])
{
	int fd;

	if (!*f) {
		if (flag != 0) {
			oldfd = infildes;
			intptr = comdlist;
		} else intptr = internal;
		fd = 100;
	} else if ((fd = open(f, O_RDONLY)) < 0) {
		snprintf(strtmp, sizeof strtmp, "cannot open %s", f);
		return (err(prt, strtmp));
	}

	push(fstack, infildes);
	if (flag4) oldfd = fd;
	infildes = fd;
	return (peeked = 0);
}

static void
push(int s[], int d)
{
	s[++s[0]] = d;
}

static int
pop(int s[])
{
	return (s[s[0]--]);
}

static int
peekc(void)
{
	int c;

	c = mygetc();
	peeked = 1;

	return (c);
}

static void
eat(void)
{
	if (charbuf != '\n')
		while (mygetc() != '\n');
	peeked = 0;
}

static int
more(void)
{
	if (mygetc() != '\n')
		return (err(1, "syntax"));
	return (0);
}

static void
quit(void)
{
	exit(errcnt);
}

static void
out(char *ln, long length)
{
	char *rp, *wp, prev;
	int w, width;
	char *oldrp;
	wchar_t cl;
	int p;
	long lim;
	if (crunch > 0) {

		ln = untab(ln);
		rp = wp = ln - 1;
		prev = ' ';

		while (*++rp) {
			if (prev != ' ' || *rp != ' ')
				*++wp = *rp;
			prev = *rp;
		}
		*++wp = '\n';
		lim = wp - ln;
		*++wp = '\0';

		if (*ln == '\n')
			return;
	} else
		ln[lim = length] = '\n';

	if (trunc < 0)
		/*EMPTY*/;
	else if (mb_cur_max <= 1) {
		if (lim > trunc)
			ln[lim = trunc] = '\n';
	} else {
		if (lim > trunc) {
			w = 0;
			oldrp = rp = ln;
			while (rp < &ln[lim]) {
				p = mbtowc(&cl, rp, mb_cur_max);
				if (p == -1) {
					width = p = 1;
				} else {
					width = wcwidth(cl);
					if (width < 0)
						width = 1;
				}
				if ((w += width) > trunc)
					break;
				rp += p;
			}
			*rp = '\n';
			lim = rp - oldrp;
		}
	}

	w = 0;
	while (w < lim+1) {
		if ((p = write(outfildes, &ln[w], lim+1)) < 0) {
			if (errno == EAGAIN)
				continue;
			puts("i/o error");
			errcnt = 1;
			break;
		}
		w += p;
	}
	outcnt += w;
}

static char *
untab(const char *l)
{
	static char *line;
	static size_t linesize;
	const char *s;
	long q;

	free(line);
	line = NULL;
	linesize = 0;
	s = l;
	q = 0;
	do {
		if (q + 9 >= linesize)
			line = grow(line, linesize += 256, "line too long");
		if (*s == '\t')
			do
				line[q++] = ' ';
			while (q%8);
		else line[q++] = *s;
	} while (*s++);
	return (line);
}

/*
 *	Function to convert ascii string to integer.  Converts
 *	positive numbers only.  Returns -1 if non-numeric
 *	character encountered.
 */

static int
patoi(const char *b)
{
	int i;
	const char *a;

	a = b;
	i = 0;
	while (*a >= '0' && *a <= '9') i = 10 * i + *a++ - '0';

	if (*a)
		return (-1);
	return (i);
}

/*
 *	Compares 2 strings.  Returns 1 if equal, 0 if not.
 */

static int
equal(const char *a, const char *b)
{
	const char *x, *y;

	x = a;
	y = b;
	while (*x == *y++)
		if (*x++ == 0)
			return (1);
	return (0);
}

static void
intcat(const char *cp)
{
	size_t	diff;

	if (cp == NULL)
		return;
	do {
		if ((diff = intptr - internal) >= internalsize) {
			internal = grow(internal, internalsize += 100,
					"command too long");
			intptr = &internal[diff];
		}
	} while (*intptr++ = *cp++);
	intptr--;
}

static void *
grow(void *ptr, size_t size, const char *msg)
{
	if ((ptr = realloc(ptr, size)) == NULL) {
		err(1, msg);
		_exit(077);
	}
	return ptr;
}
