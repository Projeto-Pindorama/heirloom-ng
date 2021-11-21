/*
 * This code contains changes by
 * Gunnar Ritter, Freiburg i. Br., Germany, March 2003. All rights reserved.
 *
 * Conditions 1, 2, and 4 and the no-warranty notice below apply
 * to these changes.
 *
 *
 * Copyright (c) 1991
 * 	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 	This product includes software developed by the University of
 * 	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
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

/*	Sccsid @(#)diff.c	1.24 (gritter) 3/27/05>	*/
/*	from 4.3BSD diff.c 4.6 4/3/86	*/

#include "diff.h"
#include <unistd.h>
#include <locale.h>
#include <iblok.h>
/*
 * diff - driver and subroutines
 */

const char	diff[] = "diff";
const char	diffh[] = DIFFH;
const char	pr[] = "pr";
const char	*progname;
const char	*argv0;

static void	usage(void);
static void	xadd(const char *);
static void	Xadd(const char *);

int
main(int argc, char **argv)
{
	int	i, invalid = 0;

	progname = basename(argv[0]);
	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	if (getenv("SYSV3") != NULL)
		sysv3 = 1;
	ifdef1 = "FILE1"; ifdef2 = "FILE2";
	status = 2;
	argv0 = argv[0];
	diffargv = argv;
	while ((i = getopt(argc, argv, ":D:efnbBwitcC:uU:hS:rslNx:a12pX:"))
			!= EOF) {
		switch (i) {
#ifdef notdef
		case 'I':
			opt = D_IFDEF;
			wantelses = 0;
			break;
		case 'E':
			opt = D_IFDEF;
			wantelses = 1;
			break;
		case '1':
			opt = D_IFDEF;
			ifdef1 = optarg;
			break;
#endif
		case 'D':
			/* -Dfoo = -E -1 -2foo */
			wantelses = 1;
			ifdef1 = "";
			/* fall through */
#ifdef notdef
		case '2':
#endif
			opt = D_IFDEF;
			ifdef2 = optarg;
			break;
		case 'e':
			opt = D_EDIT;
			break;
		case 'f':
			opt = D_REVERSE;
			break;
		case 'n':
			opt = D_NREVERSE;
			break;
		case 'b':
			bflag = 1;
			break;
		case 'B':
			Bflag = 1;
			break;
		case 'w':
			wflag = 1;
			break;
		case 'i':
			iflag = 1;
			break;
		case 't':
			tflag = 1;
			break;
		case 'c':
			opt = D_CONTEXT;
			context = 3;
			break;
		case 'C': {
			char	*x;

			opt = D_CONTEXT;
			context = strtol(optarg, &x, 10);
			if (*x != '\0' || *optarg == '+' || *optarg == '-') {
				fprintf(stderr, "%s: use -C num\n", progname);
				done();
			}
			break;
		}
		case 'u':
			opt = D_UNIFIED;
			context = 3;
			break;
		case 'U': {
			char	*x;

			opt = D_UNIFIED;
			context = strtol(optarg, &x, 10);
			if (*x != '\0' || *optarg == '+' || *optarg == '-') {
				fprintf(stderr, "%s: use -U num\n", progname);
				done();
			}
			break;
		}
		case 'h':
			hflag++;
			break;
		case 'S':
			start = optarg;
			break;
		case 'r':
			rflag++;
			break;
		case 's':
			sflag++;
			break;
		case 'l':
			lflag++;
			break;
		case 'N':
			Nflag |= 3;
			break;
		case '1':
			Nflag |= 1;
			break;
		case '2':
			Nflag |= 2;
			break;
		case 'x':
			xadd(optarg);
			break;
		case 'a':
			aflag++;
			break;
		case 'p':
			pflag++;
			break;
		case 'X':
			Xadd(optarg);
			break;
		default:
			if (invalid == 0 && !sysv3)
				invalid = optopt;
		}
	}
	argv += optind, argc -= optind;
	if (argc != 2) {
		fprintf(stderr, sysv3 ? "%s: arg count\n" :
				"%s: two filename arguments required\n",
				progname);
		done();
	}
	file1 = argv[0];
	file2 = argv[1];
	if (invalid) {
		fprintf(stderr, "%s: invalid option -%c\n", progname, invalid);
		usage();
	}
	if (pflag) {
		if (opt == D_UNIFIED || opt == D_CONTEXT)
			/*EMPTY*/;
		else if (opt == 0) {
			opt = D_CONTEXT;
			context = 3;
		} else {
			fprintf(stderr,
				"%s: -p doesn't support -e, -f, -n, or -I\n",
		    		progname);
			done();
		}
	}
	if (hflag && opt) {
		fprintf(stderr,
		    "%s: -h doesn't support -e, -f, -n, -c, -u, or -I\n",
		    	progname);
		done();
	}
	diffany(argv);
	/*NOTREACHED*/
	return 0;
}

void
diffany(char **argv)
{
	if (!strcmp(file1, "-"))
		stb1.st_mode = S_IFREG;
	else if (stat(file1, &stb1) < 0) {
		if (sysv3)
			stb1.st_mode = S_IFREG;
		else {
			fprintf(stderr, "%s: %s: %s\n", progname, file1,
					strerror(errno));
			done();
		}
	}
	if (!strcmp(file2, "-"))
		stb2.st_mode = S_IFREG;
	else if (stat(file2, &stb2) < 0) {
		if (sysv3)
			stb2.st_mode = S_IFREG;
		else {
			fprintf(stderr, "%s: %s: %s\n", progname, file2,
					strerror(errno));
			done();
		}
	}
	if ((stb1.st_mode & S_IFMT) == S_IFDIR &&
	    (stb2.st_mode & S_IFMT) == S_IFDIR) {
		diffdir(argv);
	} else
		diffreg();
	done();
}

static void
usage(void)
{
	fprintf(stderr, "\
usage: %s [ -bcefhilnrstw -C num -D string -S name ] file1 file2\n",
		progname);
	done();
}

int
min(int a,int b)
{

	return (a < b ? a : b);
}

int
max(int a,int b)
{

	return (a > b ? a : b);
}

void
done(void)
{
	if (tempfile1) {
		unlink(tempfile1);
		tempfile1 = NULL;
	}
	if (tempfile2) {
		unlink(tempfile2);
		tempfile2 = NULL;
	}
	if (recdepth == 0)
		exit(status);
	else
		longjmp(recenv, 1);
}

static void	noroom(void);

void *
dalloc(size_t n)
{
	struct stackblk *sp;

	if ((sp = malloc(n + sizeof *sp)) != NULL) {
		sp->s_prev = NULL;
		sp->s_next = curstack;
		if (curstack)
			curstack->s_prev = sp;
		curstack = sp;
		return (char *)sp + sizeof *sp;
	} else
		return NULL;
}

void *
talloc(size_t n)
{
	register void *p;

	if ((p = dalloc(n)) == NULL)
		noroom();
	return p;
}

void *
ralloc(void *p,size_t n)
{
	struct stackblk	*sp, *sq;

	if (p == NULL)
		return talloc(n);
	sp = (struct stackblk *)((char *)p - sizeof *sp);
	if ((sq = realloc(sp, n + sizeof *sp)) == NULL)
		noroom();
	if (sq->s_prev)
		sq->s_prev->s_next = sq;
	if (sq->s_next)
		sq->s_next->s_prev = sq;
	if (curstack == sp)
		curstack = sq;
	return (char *)sq + sizeof *sq;
}

void
tfree(void *p)
{
	struct stackblk	*sp;

	if (p == NULL)
		return;
	sp = (struct stackblk *)((char *)p - sizeof *sp);
	if (sp->s_prev)
		sp->s_prev->s_next = sp->s_next;
	if (sp->s_next)
		sp->s_next->s_prev = sp->s_prev;
	if (sp == curstack)
		curstack = sp->s_next;
	free(sp);
}

void
purgestack(void)
{
	struct stackblk *sp = curstack, *sq = NULL;

	do {
		free(sq);
		sq = sp;
		if (sp)
			sp = sp->s_next;
	} while (sq);
}

static void
noroom(void)
{
	oomsg(": files too big, try -h\n");
	status = 2;
	done();
}

static void
xadd(const char *cp)
{
	struct xclusion	*xp;

	xp = talloc(sizeof *xp);
	xp->x_pat = cp;
	xp->x_nxt = xflag;
	xflag = xp;
}

static void
Xadd(const char *name)
{
	struct iblok	*ip;
	char	*line = NULL;
	size_t	size = 0, len;

	if (name[0] == '-' && name[1] == '\0')
		ip = ib_alloc(0, 0);
	else
		ip = ib_open(name, 0);
	if (ip == NULL) {
		fprintf(stderr, "%s: -X %s: %s\n", progname, name,
				strerror(errno));
		done();
	}
	while ((len = ib_getlin(ip, &line, &size, realloc)) != 0) {
		if (line[len-1] == '\n')
			line[--len] = '\0';
		xadd(line);
		line = NULL;
		size = 0;
	}
	free(line);
	if (ip->ib_fd)
		ib_close(ip);
	else
		ib_free(ip);
}

void
oomsg(const char *s)
{
	write(2, progname, strlen(progname));
	write(2, s, strlen(s));
}
