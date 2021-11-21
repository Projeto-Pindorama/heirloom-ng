/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, March 2003.
 */
/*	from Unix 32V /usr/src/cmd/sort.c	*/
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
#if defined (SUS)
static const char sccsid[] USED = "@(#)sort_sus.sl	1.37 (gritter) 5/29/05";
#else
static const char sccsid[] USED = "@(#)sort.sl	1.37 (gritter) 5/29/05";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <ctype.h>
#include <signal.h>
#include "sigset.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <locale.h>
#include <wchar.h>
#include <wctype.h>
#include <langinfo.h>
#include <inttypes.h>
#include <errno.h>

#include <blank.h>

#if defined (__GLIBC__)
#if defined (_IO_getc_unlocked)
#undef	getc
#define	getc(f)		_IO_getc_unlocked(f)
#endif
#endif

static int	L = 512;
enum {
	N	= 7
};
enum {
	MINMEM	= 8 * 2048,
	MAXMEM	= 512 * 2048
};
static int	MEM = 16 * 2048;
static int	NF = 10;
enum {
	ALIGN	= 07
};

static FILE	*is, *os;
static char	*dirtry[] = {"/var/tmp", "/var/tmp", "/tmp", NULL};
static char	**dirs;
static char	*file1;
static char	*file;
static char	*filep;
static int	nfiles;
static unsigned	nlines;
static unsigned	ntext;
static intptr_t	*lspace;
static char	*tspace;
static int 	mflg;
static int	cflg;
static int	uflg;
static int	Mflg;	/* had any +M -M */
static int	ccoll;	/* LC_COLLATE is C or POSIX */
static char	*collba;
static char	*collbb;
static int	cblank;	/* isblank(c) == (c == '\t' || c == ' ') */
static char	*outfil;
static int unsafeout;	/*kludge to assure -m -o works*/
static wchar_t	tabchar;
static int 	eargc;
static char	**eargv;
static char	*progname;
static int	mb_cur_max;
static wchar_t	radixchar;
static wchar_t	thousep;
static int	phase;

#define next(wc, s, n) (*(s) & 0200 ? ((n) = mbtowc(&(wc), (s), mb_cur_max), \
		 (n) = ((n) > 0 ? (n) : (n) < 0 ? (wc=WEOF, 1) : 1)) :\
		((wc) = *(s) & 0377, (n) = 1))

static int
fold_sb(const char **s)
{
	int	c;

	c = *(*s)++ & 0377;
	return toupper(c);
}

static int
fold_mb(const char **s)
{
	int	n;
	wchar_t	wc;

	next(wc, *s, n);
	*s += n;
	return wc & ~(wchar_t)0177 ? towupper(wc) : toupper(wc);
}

static int
nofold_sb(const char **s)
{
	return *(*s)++ & 0377;
}

static int
nofold_mb(const char **s)
{
	int	n;
	wchar_t	wc;

	next(wc, *s, n);
	*s += n;
	return wc;
}

static int
nonprint_sb(const char *s)
{
	return isprint(*s & 0377) ? 0 : 1;
}

static int
nonprint_mb(const char *s)
{
	int	n;
	wchar_t	wc;

	next(wc, s, n);
	return iswprint(wc) ? 0 : n;
}

static int
dict_sb(const char *s)
{
	return (*s == '\n' || isblank(*s & 0377) || isalnum(*s & 0377)) ? 0 : 1;
}

static int
dict_mb(const char *s)
{
	int	n;
	wchar_t	wc;

	next(wc, s, n);
	return (wc == '\n' || iswblank(wc) || iswalnum(wc)) ? 0 : n;
}

static struct	field {
	int (*code)(const char **);
	int (*ignore)(const char *);
	int nflg;
	int rflg;
	int Mflg;
	int bflg[2];
	int m[2];
	int n[2];
	int posix;
}	*fields;
static struct field proto = {
	nofold_sb,
	0,
	0,
	1,
	0,
	{ 0, 0 },
	{ 0, -1 },
	{ 0, 0 },
	0
};
static int	nfields;
static int 	error = 1;

struct merg
{
	FILE	*b;
	size_t	s;
	char	l[1];
};

static void	inval(void);
static void	addnl(const char *);
static void	setmem(const char *);
static void	setlinesize(const char *);
static void	newfield(void);
static void	sort(void);
static void	merge(int, int);
static int	rline(struct merg *, const char *);
static void	disorder(const char *, char *);
static void	newfile(void);
static char	*setfil(int);
static void	oldfile(void);
static void	safeoutfil(void);
static void	cant(const char *, int);
static void	ccnt(const char *, int);
static void	diag(const char *, const char *);
static void	rderr(const char *, int);
static void	wrerr(int);
static void	term(int);
static int	cmpnum(const char *, const char *, const char *, const char *,
			int);
static int	cmp(const char *, const char *);
static int	cmpa(register const char *, register const char *);
static int	cmpm(const char *, const char *);
static int	cmpl(const char *, const char *);
static const char	*skip(const char *, struct field *, int);
static const char	*eol(register const char *);
static void	copyproto(void);
static int	field(const char *, const char *, int, int);
static int	number(const char **);
static void	keydef(char *);
static void	quicksort(char **, char **, int);
static void	*srealloc(void *, size_t);
static void	*smalloc(size_t);
static int	ecpy(char *, const char *, int);
static int	elicpy(char *, const char *, const char *, int,
			int (*)(const char *), int (*)(const char **));
static int	monthcmp(const char *, const char *);
static void	chkblank(void);

static int	(*compare)(const char *, const char *);
static int	(*cmpf)(const char *, const char *);

int
main(int argc, char **argv)
{
	register int a;
	char *arg;
	struct field *p, *q;
	int i;
	unsigned pid = getpid();

	progname = basename(argv[0]);
	arg = setlocale(LC_COLLATE, "");
	ccoll = arg == 0 || strcmp(arg, "C") == 0 || strcmp(arg, "POSIX") == 0;
	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	if (mb_cur_max > 1)
		proto.code = nofold_mb;
	else
		chkblank();
	compare = cmpf = ccoll ? mb_cur_max > 1 ? cmpm : cmpa : cmpl;
	setlocale(LC_NUMERIC, "");
	arg = nl_langinfo(RADIXCHAR);
	if (mb_cur_max > 1)
		next(radixchar, arg, i);
	else
		radixchar = *arg & 0377;
	arg = nl_langinfo(THOUSEP);
	if (mb_cur_max > 1)
		next(thousep, arg, i);
	else
		thousep = *arg & 0377;
	setlocale(LC_TIME, "");
	fields = smalloc(NF * sizeof *fields);
	copyproto();
	eargv = argv;
	while (--argc > 0) {
		if(**++argv == '-') for(arg = *argv;;) {
			switch(*++arg) {
			case '\0':
				if (&arg[-1] == *argv)
					eargv[eargc++] = "-";
				break;

			case 'k':
				if (arg[1]) {
					keydef(&arg[1]);
					while (arg[1])
						arg++;
				} else if (--argc > 0)
					keydef(*++argv);
				else
					inval();
				continue;

			case 'o':
				if(arg[1]) {
					outfil = &arg[1];
					while (arg[1])
						arg++;
				} else if(--argc > 0)
					outfil = *++argv;
				else {
					fprintf(stderr,
					"%s: can't identify output file\n",
						progname);
					exit(2);
				}
				continue;

			case 'T':
				if (arg[1]) {
					dirtry[0] = &arg[1];
					while (arg[1])
						arg++;
				} else if (--argc > 0)
					dirtry[0] = *++argv;
				continue;

			case 'y':
				setmem(&arg[1]);
				while (arg[1])
					arg++;
				continue;

			case 'z':
				if (arg[1]) {
					setlinesize(&arg[1]);
					while (arg[1])
						arg++;
				} else if (--argc > 0)
					setlinesize(*++argv);
				else
					inval();
				continue;

			default:
				if (field(&argv[0][1],argv[1],nfields>0,0))
					argv++, argc--;
				break;
			}
			break;
		} else if (**argv == '+') {
			newfield();
			if (field(&argv[0][1],argv[1],0,0))
				argv++, argc--;
		} else
			eargv[eargc++] = *argv;
	}
	q = &fields[0];
	for(a=1; a<=nfields; a++) {
		p = &fields[a];
		if(p->code != proto.code) continue;
		if(p->ignore != proto.ignore) continue;
		if(p->nflg != proto.nflg) continue;
		if(p->rflg != proto.rflg) continue;
		if(p->bflg[0] != proto.bflg[0]) continue;
		if(p->bflg[1] != proto.bflg[1]) continue;
		p->code = q->code;
		p->ignore = q->ignore;
		p->nflg = q->nflg;
		p->rflg = q->rflg;
		p->bflg[0] = p->bflg[1] = q->bflg[0];
	}
	if(eargc == 0)
		eargv[eargc++] = "-";
	if(cflg && eargc>1) {
		diag("can check only 1 file","");
		exit(2);
	}
	safeoutfil();

	if (MEM < (L + sizeof (struct merg) + ALIGN + 1) * 12)
		MEM = (L + sizeof (struct merg) + ALIGN + 1) * 12;
	while ((lspace = malloc(MEM)) == NULL)
		MEM -= 512;
	a = MEM;
	nlines = (a - (L + sizeof (size_t) + ALIGN));
	nlines /= (5*(sizeof(char *)/sizeof(char)));
	tspace = (char *)(lspace + nlines);
	ntext = &((char *)lspace)[MEM] - tspace -
		L - sizeof (size_t) - ALIGN - 2;
	if (!ccoll || Mflg) {
		collba = smalloc(L+1);
		collbb = smalloc(L+1);
	}
	a = -1;
	if ((arg = getenv("TMPDIR")) != NULL)
		dirtry[0] = strdup(arg);
	for(dirs=dirtry; *dirs; dirs++) {
		file1 = srealloc(file1, strlen(*dirs) + 30);
		file = file1;
		sprintf(filep=file1, "%s/stm%05uaa", *dirs, pid);
		while (*filep)
			filep++;
		filep -= 2;
		if ((a=open(file, O_CREAT|O_EXCL|O_WRONLY, 0600)) >= 0)
			break;
	}
	if(a < 0) {
		diag("can't locate temp","");
		exit(1);
	}
	close(a);
	unlink(file);
	if ((sigset(SIGHUP, SIG_IGN)) != SIG_IGN)
		sigset(SIGHUP, term);
	if (sigset(SIGINT, SIG_IGN) != SIG_IGN)
		sigset(SIGINT, term);
	sigset(SIGPIPE,term);
	if (sigset(SIGTERM, SIG_IGN) != SIG_IGN)
		sigset(SIGTERM, term);
	nfiles = eargc;
	if(!mflg && !cflg) {
		sort();
		fclose(stdin);
	}
	phase = 1;
	for(a = mflg|cflg?0:eargc; a+N<nfiles || unsafeout&&a<eargc; a=i) {
		i = a+N;
		if(i>=nfiles)
			i = nfiles;
		newfile();
		merge(a, i);
	}
	if(a != nfiles) {
		oldfile();
		merge(a, nfiles);
	}
	error = 0;
	term(0);
	/*NOTREACHED*/
	return 0;
}

static void
setmem(const char *s)
{
	char	*x;

	if (s[0]) {
		MEM = strtol(s, &x, 10) * 1024;
		if (*x != '\0')
			inval();
	} else
		MEM = MAXMEM;
	if (MEM < MINMEM)
		MEM = MINMEM;
	else if (MEM > MAXMEM)
		MEM = MAXMEM;
}

static void
setlinesize(const char *s)
{
	char	*x;

	if (s[0] == '\0' || (L = strtol(s, &x, 10), *x != '\0'))
		inval();
	if (L < 128)
		L = 128;
}

static void
newfield(void)
{
	if(++nfields>=NF) {
		NF += 10;
		fields = realloc(fields, NF * sizeof *fields);
		if (fields == 0) {
			diag("too many keys","");
			exit(1);
		}
	}
	copyproto();
}

static void
morespace(char **cp, size_t **sz, int ln)
{
	intptr_t	*olspc = lspace, diff, i, incr;

	i = nlines / 8;
	if (nlines - i >= 12/* && ln + i < nlines*/)
		nlines -= i;
	incr = nlines * 128;
	MEM += incr;
	L += 128;
	if ((lspace = realloc(lspace, MEM)) == NULL) {
	rtl:	write(2, progname, strlen(progname));
		write(2, ": record too large\n", sizeof ": record too large");
		term(0);
	}
	if (olspc != lspace) {
		diff = (char *)lspace - (char *)olspc;
		tspace = &tspace[diff];
		for (i = 0; i < ln; i++)
			((char **)lspace)[i] += diff;
		*cp = &(*cp)[diff];
		*sz = (size_t *)((char *)*sz + diff);
	}
	ntext = &((char *)lspace)[MEM] - tspace - L -
		sizeof (size_t) - ALIGN - 2;
	if (!ccoll) {
		free(collba);
		free(collbb);
		collba = malloc(L+1);
		collbb = malloc(L+1);
		if (collba == NULL || collbb == NULL)
			goto rtl;
	}
}

static void
sort(void)
{
	char *cp;
	register int c, ln;
	int done;
	int i, lastc;
	char *f = NULL, *of;
	size_t *sz;

	done = 0;
	i = 0;
	c = EOF;
	lastc = EOF;
	do {
		cp = tspace;
		ln = 0;
		while(ln < nlines && cp < tspace+ntext) {
			while ((intptr_t)cp & ALIGN)
				cp++;
			((char **)lspace)[ln++] = cp;
			sz = (size_t *)cp;
			cp += sizeof (size_t);
			*sz = 0;
			while(c != '\n') {
				if(c != EOF) {
					if (++(*sz) >= L-1)
						morespace(&cp, &sz, ln);
					*cp++ = (char)c;
					lastc = c;
					c = getc(is);
					continue;
				} else if(is) {
					if (ferror(is))
						rderr(f, errno);
					fclose(is);
				}
				if(i < eargc) {
					of = f;
					if((f = setfil(i++)) == 0)
						is = stdin;
					else if((is = fopen(f, "r")) == NULL)
						cant(f, errno);
					if (lastc != EOF && lastc != '\n') {
						lastc = c;
						c = '\n';
						addnl(of);
					} else {
						lastc = c;
						c = getc(is);
					}
				} else {
					if (lastc != EOF && lastc != '\n') {
						addnl(f);
						ln++;
					}
					break;
				}
			}
			*cp++ = '\n';
			(*sz)++;
			if(c == EOF) {
				done++;
				ln--;
				break;
			}
			lastc = c;
			c = getc(is);
		}
		quicksort((char **)lspace, &((char **)lspace)[ln],
				sizeof (size_t));
		if(done == 0 || nfiles != eargc)
			newfile();
		else
			oldfile();
		while(ln-- > 0) {
			if (fwrite(((char **)lspace)[ln] + sizeof (size_t),
					1, *(size_t *)((char **)lspace)[ln], os)
					!= *(size_t *)((char **)lspace)[ln])
				wrerr(errno);
		}
		if (fclose(os) != 0)
			wrerr(errno);
	} while(done == 0);
}

static void
merge(int a, int b)
{
	static struct merg	**ibuf;
	static int	nibufs;
	struct	merg	*p;
	register int	i;
	struct merg **ip, *jp;
	char	*f = NULL;
	int	j;
	int	k, l;
	int	muflg;

	if (b > nibufs) {
		if (ibuf)
			free(ibuf);
		ibuf = smalloc(nibufs = nfiles * sizeof *ibuf);
	}
	p = (struct merg *)lspace;
	j = 0;
	for(i=a; i < b; i++) {
		f = setfil(i);
		if(f == 0)
			p->b = stdin;
		else if((p->b = fopen(f, "r")) == NULL)
			cant(f, errno);
		ibuf[j] = p;
		if(!rline(p, f))	j++;
		p = (struct merg *)((intptr_t) (
			(char *)p+sizeof(FILE *)+sizeof(size_t)+L+2+ALIGN
					) & ~(intptr_t)ALIGN);
	}

	do {
		i = j;
		quicksort((char **)ibuf, (char **)(ibuf+i),
				sizeof (FILE *) + sizeof (size_t));
		l = 0;
		while(i--) {
			if(ibuf[i]->s == 0) {
				l = 1;
				if(rline(ibuf[i], f)) {
					k = i;
					while(++k < j)
						ibuf[k-1] = ibuf[k];
					j--;
				}
			}
		}
	} while(l);

	muflg = mflg & uflg | cflg;
	i = j;
	while(i > 0) {
		if(!cflg && (uflg == 0 || muflg || ibuf[i-1]->s && (i == 1 ||
				(*compare)(ibuf[i-1]->l,ibuf[i-2]->l)))) {
			if (fwrite(ibuf[i-1]->l, 1, ibuf[i-1]->s, os)
					!= ibuf[i-1]->s)
				wrerr(errno);
		}
		if(muflg){
			memcpy(p->l, ibuf[i-1]->l, ibuf[i-1]->s);
			p->s = ibuf[i-1]->s;
			muflg |= 0400;
		}
		for(;;) {
			if(rline(ibuf[i-1], f)) {
				i--;
				if(i == 0)
					break;
				if(i == 1)
					muflg = uflg;
			}
			ip = &ibuf[i];
			while(--ip>ibuf&&(*compare)(ip[0]->l,ip[-1]->l)<0){
				jp = *ip;
				*ip = *(ip-1);
				*(ip-1) = jp;
			}
			if(!(muflg&0400))
				break;
			j = (*compare)(ibuf[i-1]->l,p->l);
			if(cflg) {
				if(j > 0)
					disorder("disorder: ",ibuf[i-1]->l);
				else if(uflg && j==0)
					disorder("non-unique: ",ibuf[i-1]->l);
			} else if(j == 0)
				continue;
			break;
		}
	}
	p = (struct merg *)lspace;
	for(i=a; i<b; i++) {
		fclose(p->b);
		p = (struct merg *)((intptr_t) (
			(char *)p+sizeof(FILE *)+sizeof(size_t)+L+2+ALIGN
					) & ~(intptr_t)ALIGN);
		if(i >= eargc)
			unlink(setfil(i));
	}
	if (fclose(os) != 0)
		wrerr(errno);
}

static int
rline(struct merg *mp, const char *fn)
{
	register char *cp;
	register char *ce;
	FILE *bp;
	register int c = EOF;
	int	lastc;

	bp = mp->b;
	cp = mp->l;
	ce = cp+L;
	do {
		lastc = c;
		c = getc(bp);
		if(c == EOF) {
			if (ferror(bp))
				rderr(fn, errno);
			if (lastc != EOF && lastc != '\n') {
				addnl(0);
				c = '\n';
			} else
				return(1);
		}
		if(cp>=ce) {
			diag("fatal: line too long", "");
			term(0);
		}
		*cp++ = (char)c;
	} while(c!='\n');
	mp->s = cp - mp->l;
	return(0);
}

static void
disorder(const char *s, char *t)
{
	register char *u;
	for(u=t; *u!='\n';u++) ;
	*u = 0;
	diag(s,t);
	term(0);
}

static void
newfile(void)
{
	register char *f;
	int	fd;

	f = setfil(nfiles);
	unlink(f);
	if ((fd = open(f, O_CREAT|O_EXCL|O_WRONLY, 0600)) < 0 ||
			(os = fdopen(fd, "w")) == NULL)
		ccnt(f, errno);
	nfiles++;
}

static char *
setfil(int i)
{
	int	k;

	if(i < eargc)
		if(eargv[i][0] == '-' && eargv[i][1] == '\0')
			return(0);
		else
			return(eargv[i]);
	i -= eargc;
	k = 2;
	while (i >= 26 * 26) {
		filep[k] = i%26 + 'a';
		k++;
		i /= 26;
	}
	filep[k] = '\0';
	filep[0] = i/26 + 'a';
	filep[1] = i%26 + 'a';
	return(file);
}

static void
oldfile(void)
{

	if(outfil) {
		if((os=fopen(outfil, "w")) == NULL)
			ccnt(outfil, errno);
	} else
		os = stdout;
}

static void
safeoutfil(void)
{
	register int i;
	struct stat obuf,ibuf;

	if(!mflg||outfil==0)
		return;
	if(stat(outfil,&obuf)==-1)
		return;
	for(i=eargc-N;i<eargc;i++) {	/*-N is suff., not nec.*/
		if(stat(eargv[i],&ibuf)==-1)
			continue;
		if(obuf.st_dev==ibuf.st_dev&&
		   obuf.st_ino==ibuf.st_ino)
			unsafeout++;
	}
}

static void
inval(void)
{
	fprintf(stderr, "%s: invalid use of command line options\n", progname);
	exit(2);
}

static void
addnl(const char *s)
{
	if (s && !(s[0] == '-' && s[1] == '\0'))
		diag("warning: missing NEWLINE added at end of input file ", s);
	else
		diag("warning: missing NEWLINE at EOF added", "");
}

static void
cant(const char *f, int en)
{
	fprintf(stderr, "%s: can't open %s: %s\n", progname,
			f, strerror(en));
	term(0);
}

static void
ccnt(const char *f, int en)
{
	fprintf(stderr, "%s: can't create %s: %s\n", progname,
			f, strerror(en));
	term(0);
}

static void
diag(const char *s, const char *t)
{
	fprintf(stderr, "%s: %s%s\n", progname, s, t);
}

static void
rderr(const char *fn, int en)
{
	fprintf(stderr, "%s: read error on %s: %s\n", progname,
			fn ? fn : "stdin", strerror(en));
	term(0);
}

static void
wrerr(int en)
{
	fprintf(stderr, "%s: write error while %s: %s\n", progname,
			phase ? "merging" : "sorting", strerror(en));
	term(0);
}

/*ARGSUSED*/
static void
term(int signo)
{
	register int i;

	if(nfiles == eargc)
		nfiles++;
	for(i=eargc; i<=nfiles; i++) {	/*<= in case of interrupt*/
		unlink(setfil(i));	/*with nfiles not updated*/
	}
	exit(error);
}

static int
cmpnum(const char *pa, const char *pb, const char *la, const char *lb,
		int rflg)
{
	register int sa;
	int sb;
	int a, b;
	int n;
	const char *ipa, *ipb, *jpa, *jpb;
	wchar_t	wc;

	if (mb_cur_max > 1) {
		while (next(wc, pa, n), iswblank(wc))
			pa += n;
		while (next(wc, pb, n), iswblank(wc))
			pb += n;
	} else if (cblank) {
		while (*pa == ' ' || *pa == '\t')
			pa++;
		while (*pb == ' ' || *pb == '\t')
			pb++;
	} else {
		while(isblank(*pa & 0377))
			pa++;
		while(isblank(*pb & 0377))
			pb++;
	}
	sa = sb = rflg;
	if(*pa == '-') {
		pa++;
		sa = -sa;
	}
	if(*pb == '-') {
		pb++;
		sb = -sb;
	}
	if (thousep) {
		if (mb_cur_max > 1) {
			for (ipa = pa; n = 1, ipa<la && (isdigit(*ipa&0377) ||
				(next(wc, ipa, n), wc == thousep)); ipa += n);
			for (ipb = pb; n = 1, ipb<lb && (isdigit(*ipb&0377) ||
				(next(wc, ipb, n), wc == thousep)); ipb += n);
		} else {
			for (ipa = pa; ipa<la && (isdigit(*ipa&0377) ||
				(*ipa&0377) == thousep); ipa++);
			for (ipb = pb; ipb<lb && (isdigit(*ipb&0377) ||
				(*ipb&0377) == thousep); ipb++);
		}
	} else {
		for(ipa = pa; ipa<la&&isdigit(*ipa&0377); ipa++) ;
		for(ipb = pb; ipb<lb&&isdigit(*ipb&0377); ipb++) ;
	}
	jpa = ipa;
	jpb = ipb;
	a = 0;
	if(sa==sb) {
		if (thousep) {
			while(ipa > pa && ipb > pb) {
				if (!isdigit(ipa[-1]&0377)) {
					ipa--;
					continue;
				}
				if (!isdigit(ipb[-1]&0377)) {
					ipb--;
					continue;
				}
				if(b = *--ipb - *--ipa)
					a = b;
			}
		} else {
			while(ipa > pa && ipb > pb)
				if(b = *--ipb - *--ipa)
					a = b;
		}
	}
	if (thousep) {
		while(ipa > pa) {
			if (!isdigit(ipa[-1]&0377)) {
				ipa--;
				continue;
			}
			if(*--ipa != '0')
				return(-sa);
		}
		while(ipb > pb) {
			if (!isdigit(ipb[-1]&0377)) {
				ipb--;
				continue;
			}
			if(*--ipb != '0')
				return(sb);
		}
	} else {
		while(ipa > pa)
			if(*--ipa != '0')
				return(-sa);
		while(ipb > pb)
			if(*--ipb != '0')
				return(sb);
	}
	if(a) return(a*sa);
	if (mb_cur_max > 1) {
		pa = jpa;
		pb = jpb;
		if (next(wc, pa, n), wc == radixchar)
			pa += n;
		if (next(wc, pb, n), wc == radixchar)
			pb += n;
	} else {
		if((*(pa=jpa) & 0377) == radixchar)
			pa++;
		if((*(pb=jpb) & 0377) == radixchar)
			pb++;
	}
	if(sa==sb)
		while(pa<la && isdigit(*pa&0377)
		   && pb<lb && isdigit(*pb&0377))
			if(a = *pb++ - *pa++)
				return(a*sa);
	while(pa<la && isdigit(*pa&0377))
		if(*pa++ != '0')
			return(-sa);
	while(pb<lb && isdigit(*pb&0377))
		if(*pb++ != '0')
			return(sb);
	return 0;
}

static int
cmp(const char *i, const char *j)
{
	const char *pa, *pb;
	int (*code)(const char **);
	int (*ignore)(const char *);
	int n, k;
	register int sa;
	int sb;
	const char *la, *lb;
	struct field *fp;

	for(k = nfields>0; k<=nfields; k++) {
		fp = &fields[k];
		pa = i;
		pb = j;
		if(k) {
			la = skip(pa, fp, 1);
			pa = skip(pa, fp, 0);
			lb = skip(pb, fp, 1);
			pb = skip(pb, fp, 0);
		} else {
			la = eol(pa);
			lb = eol(pb);
		}
		if(fp->nflg) {
			if ((n = cmpnum(pa, pb, la, lb, fp->rflg)) != 0)
				return n;
			continue;
		}
		code = fp->code;
		ignore = fp->ignore;
		if (ccoll && !fp->Mflg) {
		loop:	if (ignore) {
				while((n = ignore(pa)) > 0)
					pa += n;
				while((n = ignore(pb)) > 0)
					pb += n;
			}
			if(pa>=la || *pa=='\n')
				if(pb<lb && *pb!='\n')
					return(fp->rflg);
				else continue;
			if(pb>=lb || *pb=='\n')
				return(-fp->rflg);
			if((sa = code(&pb)-code(&pa)) == 0)
				goto loop;
			return(sa*fp->rflg);
		} else {
			sa = elicpy(collba, pa, la, '\n', ignore, code);
			sb = elicpy(collbb, pb, lb, '\n', ignore, code);
			n = fp->Mflg ? monthcmp(collba, collbb) :
				strcoll(collba, collbb);
			if (n)
				return n > 0 ? -fp->rflg : fp->rflg;
			pa = &pa[sa];
			pb = &pb[sb];
		}
	}
	if(uflg)
		return(0);
	return(cmpf(i, j));
}

static int
cmpa(register const char *pa, register const char *pb)
{
	while(*pa == *pb) {
		if(*pa++ == '\n')
			return(0);
		pb++;
	}
	return(
		*pa == '\n' ? fields[0].rflg:
		*pb == '\n' ?-fields[0].rflg:
#ifndef	__ICC
		(*pb & 0377) > (*pa & 0377)  ? fields[0].rflg:
		-fields[0].rflg
#else	/* __ICC */	/* work around optimizer bug */
		*(const unsigned char *)pb > *(const unsigned char *)pa  ?
		fields[0].rflg: -fields[0].rflg
#endif	/* __ICC */
	);
}

static int
cmpm(const char *pa, const char *pb)
{
	wchar_t	wa, wb;
	int	ma, mb;

	while (next(wa, pa, ma), next(wb, pb, mb), wa == wb) {
		if (wa == '\n')
			return 0;
		pa += ma;
		pb += mb;
	}
	return (
		wa == '\n' ? fields[0].rflg :
		wb == '\n' ?-fields[0].rflg :
		wb > wa ? fields[0].rflg : -fields[0].rflg
	);
}

static int
cmpl(const char *pa, const char *pb)
{
	int	n;

	ecpy(collba, pa, '\n');
	ecpy(collbb, pb, '\n');
	n = strcoll(collba, collbb);
	return n ? n > 0 ? -fields[0].rflg : fields[0].rflg : 0;
}

static const char *
skip(const char *pp, struct field *fp, int j)
{
	register int i;
	const char *p;
	int	runs = 0;

	p = pp;
	if( (i=fp->m[j]) < 0)
		return(eol(p));
loop:	if (mb_cur_max > 1) {
		int	n;
		wchar_t	wc;

		if(tabchar != 0) {
			while(i-- > 0) {
				while (next(wc, p, n), wc != tabchar) {
					if (wc != '\n')
						p += n;
					else goto ret;
				}
				p += n;
			}
		} else {
			while(i-- > 0) {
				while (next(wc, p, n), iswblank(wc))
					p += n;
				while (next(wc, p, n), !iswblank(wc))
					if (wc != '\n')
						p += n;
					else goto ret;
			}
		}
		if(fp->bflg[j] && runs == 0)
			while (next(wc, p, n), iswblank(wc))
				p += n;
	} else {	/* mb_cur_max == 1 */
		if(tabchar != 0) {
			while(i-- > 0) {
				while((*p & 0377) != tabchar)
					if(*p != '\n')
						p++;
					else goto ret;
				p++;
			}
		} else if (cblank) {
			while(i-- > 0) {
				while(*p == ' ' || *p == '\t')
					p++;
				while(!(*p == ' ' || *p == '\t'))
					if(*p != '\n')
						p++;
					else goto ret;
			}
		} else {
			while(i-- > 0) {
				while(isblank(*p & 0377))
					p++;
				while(!isblank(*p & 0377))
					if(*p != '\n')
						p++;
					else goto ret;
			}
		}
		if(fp->bflg[j] && runs == 0) {
			if (cblank)
				while(*p == ' ' || *p == '\t')
					p++;
			else
				while(isblank(*p & 0377))
					p++;
		}
	}
	i = fp->n[j];
	while(i-- > 0) {
		if(*p != '\n')
			p++;
		else goto ret;
	} 
	if (fp->posix && fp->n[j] == 0 && j == 1 && runs++ == 0) {
		i = 1;
		goto loop;
	}
ret:
	return(p);
}

static const char *
eol(register const char *p)
{
	while(*p != '\n') p++;
	return(p);
}

static void
copyproto(void)
{
	register int i;
	register int *p, *q;

	p = (int *)&proto;
	q = (int *)&fields[nfields];
	for(i=0; i<sizeof(proto)/sizeof(*p); i++)
		*q++ = *p++;
}

static int
field(const char *s, const char *nxt, int k, int posix)
{
	register struct field *p;
	register int d;
	const char *arg;
	int cons = 0;
	p = &fields[nfields];
	d = 0;
	for(; *s!=0; s++) {
		switch(*s) {
		case '\0':
			return cons;

		case 'M':
			p->Mflg = 1;
			Mflg = 1;
			/*FALLTHRU*/

		case 'b':
			p->bflg[k]++;
			break;

		case 'd':
			p->ignore = mb_cur_max > 1 ? dict_mb : dict_sb;
			break;

		case 'f':
			p->code = mb_cur_max > 1 ? fold_mb : fold_sb;
			break;
		case 'i':
			p->ignore = mb_cur_max > 1 ? nonprint_mb : nonprint_sb;
			break;

		case 'c':
			cflg = 1;
			L = 2048;
			continue;

		case 'm':
			mflg = 1;
			L = 2048;
			continue;

		case 'n':
			p->nflg++;
			break;
		case 't':
			if (s[1])
				arg = ++s;
			else if (nxt) {
				arg = nxt;
				cons = 1;
			} else {
#ifdef	SUS
				inval();
#endif	/* SUS */
				tabchar = 0;
				continue;
			}
			if (mb_cur_max > 1) {
				int	n;

				next(tabchar, arg, n);
				if (tabchar && arg == s)
					s += n - 1;
			} else
				tabchar = *arg & 0377;
			continue;

		case 'r':
			p->rflg = -1;
			continue;
		case 'u':
			uflg = 1;
			break;

		case '.':
			if(p->m[k] == -1)	/* -m.n with m missing */
				p->m[k] = 0;
			d = &fields[0].n[0]-&fields[0].m[0];
			break;

		default:
			inval();
			/*NOTREACHED*/

		case '0': case '1': case '2': case '3':
			  case '4': case '5': case '6':
			  case '7': case '8': case '9':
			if ((p->m[k+d] = number(&s) - posix) < 0) {
				if (posix && k == 1 && d > 0 && p->m[k+d] == -1)
					p->m[k+d] = 0;
				else
					inval();
			}
		}
		compare = cmp;
	}
	p->posix = posix;
	return cons;
}

static int
number(const char **ppa)
{
	int n;
	register const char *pa;
	pa = *ppa;
	n = 0;
	while(isdigit(*pa&0377)) {
		n = n*10 + *pa - '0';
		*ppa = pa++;
	}
	return(n);
}

static void
keydef(char *s)
{
	char	*comma;

	if ((comma = strchr(s, ',')) != NULL)
		*comma++ = '\0';
	newfield();
	field(s, NULL, 0, 1);
	if (comma)
		field(comma, NULL, 1, 1);
}

#define qsexc(p,q) t= *p;*p= *q;*q=t
#define qstexc(p,q,r) t= *p;*p= *r;*r= *q;*q=t

static void
quicksort(char **a, char **l, int offs)
{
	register char **i, **j;
	char **k;
	char **lp, **hp;
	int c;
	char *t;
	unsigned n;



start:
	if((n=l-a) <= 1)
		return;


	n /= 2;
	hp = lp = a+n;
	i = a;
	j = l-1;


	for(;;) {
		if(i < lp) {
			if((c = (*compare)(*i + offs, *lp + offs)) == 0) {
				--lp;
				qsexc(i, lp);
				continue;
			}
			if(c < 0) {
				++i;
				continue;
			}
		}

loop:
		if(j > hp) {
			if((c = (*compare)(*hp + offs, *j + offs)) == 0) {
				++hp;
				qsexc(hp, j);
				goto loop;
			}
			if(c > 0) {
				if(i == lp) {
					++hp;
					qstexc(i, hp, j);
					i = ++lp;
					goto loop;
				}
				qsexc(i, j);
				--j;
				++i;
				continue;
			}
			--j;
			goto loop;
		}


		if(i == lp) {
			if(uflg)
				for(k=lp+1; k<=hp; k++)
					*(size_t *)(*k+offs-sizeof (size_t))=0;
			if(lp-a >= l-hp) {
				quicksort(hp+1, l, offs);
				l = lp;
			} else {
				quicksort(a, lp, offs);
				a = hp+1;
			}
			goto start;
		}


		--lp;
		qstexc(j, lp, i);
		j = --hp;
	}
}

static void *
srealloc(void *op, size_t size)
{
	void	*np;
	const char	*msg;

	if ((np = realloc(op, size)) == NULL) {
		msg = phase ? "allocation error before merge\n" :
			"allocation error before sort\n";
		write(2, msg, strlen(msg));
		term(0);
	}
	return np;
}

static void *
smalloc(size_t size)
{
	return srealloc(NULL, size);
}

static int
ecpy(char *dst, const char *src, int c)
{
	char	*dp = dst;
	while (*src && (*src & 0377) != c)
		*dp++ = *src++;
	*dp = '\0';
	return dp - dst;
}

static int
elicpy(char *dst, const char *src, const char *send, int c,
		int (*ignore)(const char *), int (*code)(const char **))
{
	const char	*sp = src, *osp;
	wchar_t	wc;
	char	mb[MB_LEN_MAX+1];
	int	i, n;

	for (;;) {
		if (ignore)
			while ((n = ignore(sp)) > 0)
				sp += n;
		if (sp >= send || *sp == '\0' || (*sp & 0377) == c)
			break;
		osp = sp;
		wc = code(&sp);
		if (mb_cur_max > 1) {
			if ((n = wctomb(mb, wc)) > 0) {
				for (i = 0; i < n; i++)
					*dst++ = mb[i];
			} else {
				while (osp < sp)
					*dst++ = *osp++;
			}
		} else
			*dst++ = wc;
	}
	*dst = '\0';
	return sp - src;
}

static void
cpcu3(char *dst, const char *src)
{
	const char	*osp;
	wchar_t	wc;
	char	mb[MB_LEN_MAX+1];
	int	i, j, n;

	for (i = 0; i < 3; i++) {
		if (mb_cur_max > 1) {
			osp = src;
			wc = fold_mb(&src);
			if ((n = wctomb(mb, wc)) > 0) {
				for (j = 0; j < n; j++)
					*dst++ = mb[j];
			} else {
				while (osp < src)
					*dst++ = *osp++;
			}
		} else
			*dst++ = fold_sb(&src);
	}
	*dst = '\0';
}

static char *
upcdup(const char *s)
{
	char	*r;

	r = smalloc(MB_LEN_MAX*3 + 1);
	cpcu3(r, s);
	return r;
}

static const char	*months[12];

#define	COPY_ABMON(m)	months[m-1] = upcdup(nl_langinfo(ABMON_##m))

static void
fillmonths(void)
{
	COPY_ABMON(1);
	COPY_ABMON(2);
	COPY_ABMON(3);
	COPY_ABMON(4);
	COPY_ABMON(5);
	COPY_ABMON(6);
	COPY_ABMON(7);
	COPY_ABMON(8);
	COPY_ABMON(9);
	COPY_ABMON(10);
	COPY_ABMON(11);
	COPY_ABMON(12);
}

static int
monthcoll(const char *s)
{
	int	i;
	char	u[MB_LEN_MAX*3+1];

	cpcu3(u, s);
	for (i = 0; i < 12; i++)
		if (strcmp(u, months[i]) == 0)
			return i;
	return 0;
}


static int
monthcmp(const char *pa, const char *pb)
{
	if (months[0] == NULL)
		fillmonths();
	return monthcoll(pa) - monthcoll(pb);
}

/*
 * isblank() consumes half of execution time (in skip()) with
 * glibc 2.3.1. Check if it contains only space and tab, and
 * if so, avoid calling it.
 */
static void
chkblank(void)
{
	int	c;

	for (c = 1; c <= 255; c++) {
		if (c == ' ' || c == '\t')
			continue;
		if (isblank(c))
			return;
	}
	cblank = 1;
}
