/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, May 2003.
 */
/*	from Unix 32V /usr/src/cmd/pr.c	*/
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
/*
 *   print file with headings
 *  2+head+2+page[56]+5
 */

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
#if defined (SUS)
static const char sccsid[] USED = "@(#)pr_sus.sl	1.29 (gritter) 5/29/05";
#else	/* !SUS */
static const char sccsid[] USED = "@(#)pr.sl	1.29 (gritter) 5/29/05";
#endif	/* !SUS */

#include <stdio.h>
#include <signal.h>
#include "sigset.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <libgen.h>
#include <locale.h>
#include <wchar.h>
#include <ctype.h>
#include <limits.h>

#ifdef	__GLIBC__
#ifdef	_IO_getc_unlocked
#undef	getc
#define	getc(f)		_IO_getc_unlocked(f)
#endif
#ifdef	_IO_putc_unlocked
#undef	putc
#define	putc(c, f)	_IO_putc_unlocked(c, f)
#undef	putchar
#define	putchar(c)	_IO_putc_unlocked(c, stdout)
#endif
#endif	/* __GLIBC__ */

#include <atoll.h>
#include <iblok.h>
#include <mbtowi.h>

#define	next(wc, s, n)	(mb_cur_max > 1 && *(s) & 0200 ? \
		((n) = mbtowi(&(wc), (s), mb_cur_max), \
		 (n) = ((n) > 0 ? (n) : (n) < 0 ? (wc=WEOF, 1) : 1)) :\
		((wc) = *(s) & 0377, (n) = 1))

static struct	chargap {
	wint_t	c_c;
	int	c_g;
	int	c_u;
} eflg =  { '\t', 8, 0 }, iflg =  { '\t', 8, 0 }, nflg =  { '\t', 5, 0 };

static int	status;
static const char	*progname;
static int	ncol	= 1;
static const char	*header;
static int	col;
static long	NCOL;
static int	*icol;
static int	*scol;
static struct iblok	*file;
static long	BUFS;
static wint_t	*buffer;	/* for multi-column output */
#define	FF	014
static int	line;
static wint_t	**colp;
static int	nofile;
static char	isclosed[10];
static struct iblok	*ifile[10];
static const char	**lastarg;
static wint_t	*peekc, *peek2c;
static long long	fpage;
static long long	page;
static int	colw;
static int	nspace;
static int	width	= 72;
static int	length	= 66;
static int	plength = 61;
static int	margin	= 10;
static int	ntflg;
static int	mflg;
static wint_t	tabc;
static char	*tty;
static int	mode;
static int	oneof;
static int	mb_cur_max;
static const char	*curfile;
static int	aflg;
static int	dflg;
static int	fflg;
static int	oflg;
static int	wflg;
static int	Fflg;
static int	rflg;
static int	pflg = -1;
static int	numwidth;

static void	usage(int);
static void	eitmcol(void);
static wint_t	argchar(int, const char *);
static struct chargap getcg(int, const char *, struct chargap);
static void	done(void);
static void	onintr(int);
static void	fixtty(void);
static void	print(const char *, const char **);
static void	mopen(const char **);
static wint_t	putpage(void);
static wint_t	readc(void);
static wint_t	tpgetc(int);
static wint_t	pgetc(int);
static void	put(wint_t);
static void	putcp(wint_t);
static void	putcs(const char *);
static int	putnum(long long);
static int	cnumwidth(void);
static void	delaym(const char *, const char *);
static void	printm(void);

int
main(int argc, char **argv)
{
	int nfdone, hadtd;
	int ac;
	const char **av, *ap;

	progname = basename(argv[0]);
	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	setlocale(LC_TIME, "");
	if (sigset(SIGINT, SIG_IGN) != SIG_IGN)
		sigset(SIGINT, onintr);
	lastarg = (const char **)&argv[argc-1];
	fixtty();
	ac = argc, av = (const char **)argv;
	for (nfdone=0; ac>1; ac--) {
		av++;
		if (**av == '-' && av[0][1]) {
			if (av[0][1] == '-' && av[0][2] == '\0')
				break;
			ap = *av;
		nc:	switch (*++ap) {
			case 'a':
				aflg++;
				goto nc;

			case 'd':
				dflg++;
				goto nc;

			case 'F':
#ifndef	SUS
				Fflg++;
				goto nc;
#endif	/* SUS */

			case 'f':
				fflg++;
				goto nc;

			case 'r':
				rflg++;
				goto nc;

			case 'p':
				pflg = open("/dev/tty", O_RDONLY);
				goto nc;

			case 'e':
				eflg = getcg('e', &ap[1], eflg);
				continue;

			case 'i':
				iflg = getcg('i', &ap[1], iflg);
				continue;

			case 'n':
				nflg = getcg('n', &ap[1], nflg);
				continue;

			case 'o':
				if (ap[1])
					oflg = atoi(++ap);
				else if (ac>2) {
					oflg = atoi(*++av);
					*av = NULL;
					ac--;
				}
				continue;

			case 'h':
				if (ap[1])
					header = &ap[1];
				else if (ac>2) {
					header = *++av;
					*av = NULL;
					ac--;
				}
				continue;

			case 't':
				ntflg++;
				goto nc;

			case 'l':
				if (ap[1])
					length = atoi(++ap);
				else if (ac>2) {
					length = atoi(*++av);
					*av = NULL;
					ac--;
				}
				continue;

			case 'w':
				wflg++;
				if (ap[1])
					width = atoi(++ap);
				else if (ac>2) {
					width = atoi(*++av);
					*av = NULL;
					ac--;
				}
#ifdef	SUS
				else
					usage(0);
#endif	/* SUS */
				continue;

			case 's':
				if (*++ap)
					tabc = argchar('s', ap);
				else
					tabc = '\t';
				if (wflg == 0)
					width = 512;
				continue;

			case 'm':
				if (ncol > 1)
					eitmcol();
				mflg++;
				goto nc;

			case '0':
			case '1': case '2': case '3':
			case '4': case '5': case '6':
			case '7': case '8': case '9':
				if (mflg)
					eitmcol();
				ncol = atoi(ap);
				continue;

			case '\0':
				continue;

			default:
				usage(*ap);
			}
		} else if (**av == '+') {
			fpage = atoll(&(*av)[1]);
		} else {
			nfdone++;
			if (mflg)
				break;
		}
	}
	if (aflg && ncol <= 1) {
		fprintf(stderr, "%s: -a valid only with -column\n", progname);
		usage(0);
	}
	numwidth = cnumwidth();
	if ((nflg.c_u&(1|2)) == 1 && ncol > 1)
		nflg.c_c = ' ';
	ac = argc, av = (const char **)argv;
	if (nfdone==0)
		mflg = 0;
	if ((aflg||mflg) == 0) {
		BUFS = ncol > 1 ? length*3*(width+1) : 2;
		if ((buffer = calloc(BUFS+1, sizeof *buffer)) == 0) {
			write(2, "out of space\n", 13);
			exit(1);
		}
	}
	NCOL = mflg ? 10 : ncol;
	if ((icol = calloc(NCOL, sizeof *icol)) == NULL ||
			(scol = calloc(NCOL, sizeof *scol)) == NULL ||
			(colp = calloc(NCOL, sizeof *colp)) == NULL ||
			(peekc = calloc(NCOL, sizeof *peekc)) == NULL ||
			(peek2c = calloc(NCOL, sizeof *peek2c)) == NULL) {
		free(buffer);
		fprintf(stderr, "%s: No room for columns.\n", progname);
		exit(1);
	}
	for (nfdone=0, hadtd=0; ac>1; ac--) {
		av++;
		if (*av) {
			if (**av != '-' && **av != '+' || hadtd ||
					**av == '-' && av[0][1] == '\0') {
				print(*av, av);
				nfdone++;
				if (mflg)
					break;
			} else if (av[0][0] == '-' && av[0][1] == '-' &&
					av[0][2] == '\0')
				hadtd++;
		}
	}
	if (nfdone==0)
		print(NULL, NULL);
	done();
	/*NOTREACHED*/
	return 0;
}

static void
usage(int c)
{
	if (c)
		fprintf(stderr, "%s: unknown option, %c\n", progname, c);
	fprintf(stderr, "\
usage: %s [-column [-wwidth] [-a]] [-ect] [-ict] [-drtfp] [+page] [-nsk]  \\\n\
          [-ooffset] [-llength] [-sseparator] [-h header] [-F] [file ...]\n\
\n\
       %s [-m      [-wwidth]]      [-ect] [-ict] [-drtfp] [+page] [-nsk]  \\\n\
          [-ooffset] [-llength] [-sseparator] [-h header] [-F] file1 file2 ...\
\n",
	progname, progname);
	exit(1);
}

static void
eitmcol(void)
{
	fprintf(stderr, "%s: only one of either -m or -column allowed\n",
			progname);
	usage(0);
}

static wint_t
argchar(int optc, const char *ap)
{
	wint_t	wc;
	int	n;

	next(wc, ap, n);
	if (wc == WEOF) {
		fprintf(stderr,
			"%s: illegal byte sequence in argument to -%c\n",
				progname, optc);
		exit(1);
	}
	return wc;
}

static struct chargap
getcg(int optc, const char *ap, struct chargap dg)
{
	struct chargap	cg;
	int	n;

	cg.c_u = 0;
	if (*ap && !isdigit(*ap)) {
		next(cg.c_c, ap, n);
		if (cg.c_c == WEOF) {
			fprintf(stderr,
			"%s: illegal byte sequence in argument to -%c\n",
				progname, optc);
			exit(1);
		}
		ap += n;
		cg.c_u |= 2;
	} else
		cg.c_c = dg.c_c;
	if (*ap == '\0' || (cg.c_g = atoi(ap)) <= 0)
		cg.c_g = dg.c_g;
	cg.c_u |= 1;
	return cg;
}

static void
done(void)
{
	printm();
	if (tty)
		chmod(tty, mode);
	exit(status);
}

/*ARGSUSED*/
static void
onintr(int signo)
{

	printm();
	if (tty)
		chmod(tty, mode);
	_exit(1);
}

static void
fixtty(void)
{
	struct stat sbuf;

	tty = ttyname(1);
	if (tty == 0) {
		close(pflg);
		pflg = -1;
		return;
	}
	stat(tty, &sbuf);
	mode = sbuf.st_mode&0777;
	chmod(tty, 0600);
}

static void
print(const char *fp, const char **argp)
{
	struct stat sbuf;
	register int sncol, i;
	register const char *sheader;
	char cbuf[100], linebuf[100];
	wint_t c;

	curfile = fp ? fp : "";
	if (ntflg)
		margin = 0;
	else
		margin = 10;
	if (length <= margin) {
		margin = 0;
		ntflg = 1;
	}
	if (width <= 0)
		width = 72;
	if (width <= numwidth || ncol*2>width) {
		fprintf(stderr, "%s: width too small\n", progname);
		status |= 1;
		done();
	}
	if (mflg) {
		mopen(argp);
		ncol = nofile;
	}
	colw = (width-numwidth)/(ncol==0? 1 : ncol);
	sncol = ncol;
	sheader = header;
	plength = length-5;
	if (--ncol<0)
		ncol = 0;
	if (ntflg)
		plength = length;
	if (ncol)
		iflg.c_u |= 1, eflg.c_u |= 1;
	if (mflg || fp && fp[0] == '-' && fp[1] == '\0')
		fp = 0;
	oneof = 2;
	if (fp) {
		if((file=ib_open(fp, 0))==NULL) {
			delaym("%s: can't open %s\n", fp);
			ncol = sncol;
			header = sheader;
			return;
		}
		fstat(file->ib_fd, &sbuf);
		if (aflg) {
			ifile[0] = file;
			nofile = 1;
		}
	} else {
		if ((file=ib_alloc(dup(0), 0))==NULL) {
			delaym("%s: cannot open stdin\n", NULL);
			return;
		}
		time(&sbuf.st_mtime);
	}
	if (header == 0)
		header = fp?fp:"";
	strftime(cbuf, sizeof cbuf, "%b %e %H:%M %Y",
			localtime(&sbuf.st_mtime));
	page = 1;
	for (i = 0; i<NCOL; i++)
		scol[i] = icol[i] = 0;
	if (buffer)
		*buffer = 0;
	colp[ncol] = buffer;
	while ((mflg||aflg)&&nofile || (!mflg&&!aflg)&&tpgetc(ncol)!=WEOF) {
		if (pflg >= 0) {
			putc('\a', stderr);
			while (read(pflg, linebuf, 1) == 1 &&
					*linebuf != '\n');
		}
		if (mflg==0&&aflg==0) {
			colp[ncol]--;
			if (colp[ncol] < buffer)
				colp[ncol] = &buffer[BUFS];
		}
		line = 0;
		if (ntflg==0) {
			putcs("\n\n");
			putcs(cbuf);
			putcs("  ");
			putcs(header);
			snprintf(linebuf, sizeof linebuf,
					" Page %lld\n\n\n", page);
			putcs(linebuf);
		}
		c = putpage();
		if (ntflg==0 || c==FF) {
			if (fflg)
				put(FF);
			else while(line<length)
				put('\n');
		}
		page++;
	}
	if (oneof == 0)
		ib_close(file);
	ncol = sncol;
	header = sheader;
}

static void
mopen(const char **ap)
{
	register const char **p, *p1;

	p = ap;
	while((p1 = *p) && p++ <= lastarg) {
		if((ifile[nofile] = p1[0]=='-'&&p1[1]=='\0' ?
					ib_alloc(dup(0), 0) :
					ib_open(p1, 0)) == NULL) {
			delaym(p1[0]=='-'&&p1[1]=='\0' ?
					"%s: cannot open stdin\n" :
					"%s: can't open %s\n",
				p1);
			isclosed[nofile] = 1;
			nofile--;
		}
		else
			isclosed[nofile] = 0;
		if(++nofile>=10) {
			fprintf(stderr, "%s: Too many args\n", progname);
			status |= 2;
			done();
		}
	}
}

static wint_t
putpage(void)
{
	static long long	cnt;
	long long	ocnt;
	register int lastcol, i;
	register wint_t c = 0;
	register wint_t *cp;
	int j, k, l, xplength, content;

	if (ncol==0) {
		while (line<plength) {
			for (i=0; i<oflg; i++)
				put(' ');
			c = pgetc(0);
			if (nflg.c_u && c!=0 && c!=WEOF)
				putnum(++cnt);
			while(c && c!='\n' && c!=FF && c!=WEOF) {
				put(c);
				c = pgetc(0);
			}
			if (c==0 || c==WEOF)
				break;
			put('\n');
			if (dflg && line<plength)
				put('\n');
			if (c==FF)
				break;
		}
		return c;
	}
	ocnt = cnt;
	colp[0] = colp[ncol];
	xplength = plength;
	if (mflg==0&&aflg==0) {
		k=0;
		for (i=1; i<=ncol; i++) {
			colp[i] = colp[i-1];
			for (j = margin; j<length; j++) {
				while((c=pgetc(i))!='\n')
					if (c==0 || c==WEOF)
						break;
				if (c=='\n')
					k++;
			}
		}
		cp = colp[ncol];
		if (k == ncol * (length-margin))
			for (j = margin; j<length; j++) {
				while((c=pgetc(ncol))!='\n')
					if (c==0 || c==WEOF)
						break;
				if (c=='\n')
					k++;
			}
		colp[ncol] = cp;
		if (k < (ncol+1) * (length-margin)) {
			l = k;
			l = (k + ncol) / (ncol+1);
			xplength = plength - ((length-margin) - l);
			for (i = 0; i < ncol; i++) {
				l = (k + ncol-i) / (ncol-i+1);
				cp = colp[i];
				for (j = 0; j < l; j++) {
					while((c=pgetc(i))!='\n')
						if (c==0 || c==WEOF)
							break;
					if (c=='\n')
						k--;
				}
				colp[i][-1] = WEOF;
				colp[i+1] = colp[i];
				colp[i] = cp;
			}
		}
	}
	l = 0;
	k = xplength - line;
	while (line<xplength) {
		content = 0;
		if (mflg && !aflg) {
			for (i=0; i<=ncol; i++) {
				c = pgetc(i);
				peek2c[i] = c;
				if (c!='\0' && c!='\n')
					content = i+1;
			}
		}
		l++;
		lastcol = colw+numwidth;
		for (i=0; i<oflg; i++)
			put(' ');
		for (i=0; i<ncol; i++) {
			c = pgetc(i);
			if ((mflg==0||content&&i==0) && nflg.c_u && c!=0) {
				cnt++;
				putnum((aflg||mflg) ? cnt : ocnt + l + k*i);
			}
			while (c && c!='\n' && c!=WEOF) {
				if (col<lastcol-1 || tabc!=0)
					put(c);
				c = pgetc(i);
			}
			if (c==0) {
				if (aflg || mflg && nofile <= 0)
					break;
				continue;
			}
			if (aflg) {
				c = pgetc(i+1);
				peek2c[i+1] = c;
				if (c == 0) {
					put('\n');
					continue;
				}
			}
			if (tabc)
				put(tabc);
			else while (col<lastcol)
				put(' ');
			lastcol += colw;
		}
		c = pgetc(ncol);
		if (mflg==0 && nflg.c_u && c!=0) {
			cnt++;
			putnum(aflg ? cnt : ocnt + l + k*i);
		}
		while (c && c!='\n' && c!=WEOF) {
			if (col<lastcol-1 || tabc!=0)
				put(c);
			c = pgetc(ncol);
		}
		if (c==0 && (aflg || mflg && nofile <= 0))
			break;
		put('\n');
		if (dflg && line<plength)
			put('\n');
	}
	return 0;
}

static wint_t
readc(void)
{
	wint_t	c;
	int	m;

	do {
		if (mb_cur_max > 1 ? ib_getw(file, &c, &m) == NULL :
				(c = ib_get(file)) == (wint_t)EOF) {
			ib_close(file);
			if (oneof)
				delaym("%s: %s -- empty file\n", curfile);
			oneof = 1;
			return WEOF;
		}
		oneof = 0;
	} while (c == WEOF || c == 0);
	return c;
}

static wint_t
tpgetc(int ai)
{
	struct iblok *ip;
	register wint_t **p;
	wint_t c;
	register int i;
	int m;

	i = ai;
	if (mflg||aflg) {
		do {
			ip = aflg ? file : ifile[i];
			if(isclosed[i] || ( mb_cur_max > 1 ?
						ib_getw(ip, &c, &m) == NULL :
						(c=ib_get(ip))==(wint_t)EOF)) {
				if (mflg&&isclosed[i]==0) {
					isclosed[i] = 1;
					if (--nofile <= 0)
						return(0);
				} else if (aflg) {
					nofile = 0;
					return(0);
				}
				if (nofile <= 0)
					return(0);
				return('\n');
			}
		} while (c == WEOF || c == 0);
		if (c==FF && ncol>0)
			c = '\n';
		return(c);
	}
	c = **(p = &colp[i]);
	if (c == WEOF || c == 0) {
		if (oneof == 1 || (c = readc()) == WEOF)
			return(c);
		 /* For multi-column output, store at least three times the
		    column width so that cc sequences remain complete. */
		if (ncol==0 || scol[i]<=colw*3 || c=='\n') {
			**p = c;
			(*p)++;
			if (*p >= &buffer[BUFS]) {
				*p = buffer;
				if (ncol==0)
					*(*p)++ = c;
			}
			**p = WEOF;
		}
	} else {
		(*p)++;
		if (*p >= &buffer[BUFS])
			*p = buffer;
	}
	if (c==FF && ncol>0)
		c = '\n';
	return(c);
}

static wint_t
pgetc(int ai)
{
	register wint_t c;
	int i;

	i = aflg?0:ai;
	if (peek2c[ai]) {
		c = peek2c[ai];
		peek2c[ai] = 0;
		return c;
	} else if (peekc[i]) {
		c = peekc[i];
		peekc[i] = 0;
	} else {
		c = tpgetc(i);
		scol[i]++;
	}
	if (Fflg && icol[i] >= colw - (ncol != 0) && c != 0 && c != '\n' &&
			c != WEOF) {
		peekc[i] = c;
		c = '\n';
	}
	if (tabc) {
		if (c == '\n')
			scol[i] = 0;
		return(c);
	}
	if (eflg.c_u && c == eflg.c_c) {
		icol[i]++;
		if (icol[i] % eflg.c_g != 0)
			peekc[i] = eflg.c_c;
		return(' ');
	} else switch (c) {

	case '\n':
		scol[i] = 0;
		/*FALLTHRU*/

	case '\r':
		icol[i] = 0;
		break;

	case 010:
	case 033:
		if (icol[i] > 0)
			icol[i]--;
		break;

	case '\t':
		icol[i]++;
		while (icol[i] & 07)
			icol[i]++;

	default:
		if (mb_cur_max > 1 && c & ~(wchar_t)0177)
			icol[i] += wcwidth(c);
		else if (isprint(c))
			icol[i]++;
	}
	return(c);
}
static void
put(wint_t ac)
{
	register int ns;
	register wint_t c;

	c = ac;
	if (tabc) {
		putcp(c);
		if (c=='\n')
			line++;
		return;
	}
	switch (c) {

	case ' ':
		nspace++;
		col++;
		return;

	case '\n':
		line++;
		/*FALLTHRU*/

	case '\r':
		col = 0;
		nspace = 0;
		break;

	case 010:
	case 033:
		if (--col<0)
			col = 0;
		if (--nspace<0)
			nspace = 0;

	}
	while(nspace) {
		if (iflg.c_u && nspace >= (ns=iflg.c_g-(col-nspace)%iflg.c_g)
#ifdef	SUS
				&& (ns > 1 || nspace > 1)
#endif	/* SUS */
				) {
			nspace -= ns;
			putcp(iflg.c_c);
		} else {
			nspace--;
			putcp(' ');
		}
	}
	if (c == '\t') {
		col++;
		while (col & 07)
			col++;
	} else if (mb_cur_max > 1 && c & ~(wchar_t)0177)
		col += wcwidth(c);
	else if (isprint(c))
		col++;
	putcp(c);
}

static void
putcp(wint_t c)
{
	if (page >= fpage) {
		if (mb_cur_max > 1 && c & ~(wchar_t)0177) {
			char	mb[MB_LEN_MAX];
			int	i, n;

			n = wctomb(mb, c);
			for (i = 0; i < n; i++)
				putchar(mb[i]);
		} else
			putchar(c);
	}
}

static void
putcs(const char *s)
{
	wint_t	c;
	int	n;

	while (*s) {
		next(c, s, n);
		if (c != WEOF)
			put(c);
		s += n;
	}
}

static int
putnum(long long n)
{
	char	buf[40], *bp;
	int	i;

	i = col;
	snprintf(buf, sizeof buf, "%*lld", nflg.c_g, n);
	for (bp = buf; *bp; bp++)
		put(*bp);
	put(nflg.c_c);
	return col - i;
}

static int
cnumwidth(void)
{
	int	i;

	if (nflg.c_u == 0 || ncol > 1 && mflg == 0)
		return 0;
	i = nflg.c_g;
	if (nflg.c_c == '\t') {
		i++;
		while (i & 07)
			i++;
	} else if (mb_cur_max > 1 && nflg.c_c & ~(wchar_t)0177)
		i += wcwidth(nflg.c_c);
	else if (i >= 040)
		i++;
	return i;
}

static struct	message {
	struct message	*m_nxt;
	const char	*m_fmt;
	const char	*m_arg;
} *messages;

static void
delaym(const char *fmt, const char *arg)
{
	struct message	*mp, *mq;

	status |= 1;
	if (tty == NULL) {
		if (rflg == 0)
			fprintf(stderr, fmt, progname, arg);
		return;
	}
	if ((mp = calloc(1, sizeof *mp)) == NULL) {
		write(2, "pr: malloc failed\n", 18);
		exit(1);
	}
	mp->m_fmt = fmt;
	mp->m_arg = arg;
	if (messages) {
		for (mq = messages; mq->m_nxt; mq = mq->m_nxt);
		mq->m_nxt = mp;
	} else
		messages = mp;
}

static void
printm(void)
{
	struct message	*mp;

	if (rflg == 0 && tty)
		for (mp = messages; mp; mp = mp->m_nxt)
			fprintf(stderr, mp->m_fmt, progname, mp->m_arg);
}
