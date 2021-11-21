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

/*	Sccsid @(#)diffreg.c	1.30 (gritter) 3/15/07>	*/
/*	from 4.3BSD diffreg.c 4.16 3/29/86	*/

#include "diff.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include "sigset.h"
#include <wchar.h>
#include <wctype.h>
#include <inttypes.h>
#include "mbtowi.h"
/*
 * diff - compare two files.
 */

/*
 *	Uses an algorithm due to Harold Stone, which finds
 *	a pair of longest identical subsequences in the two
 *	files.
 *
 *	The major goal is to generate the match vector J.
 *	J[i] is the index of the line in file1 corresponding
 *	to line i file0. J[i] = 0 if there is no
 *	such line in file1.
 *
 *	Lines are hashed so as to work in core. All potential
 *	matches are located by sorting the lines of each file
 *	on the hash (called ``value''). In particular, this
 *	collects the equivalence classes in file1 together.
 *	Subroutine equiv replaces the value of each line in
 *	file0 by the index of the first element of its 
 *	matching equivalence in (the reordered) file1.
 *	To save space equiv squeezes file1 into a single
 *	array member in which the equivalence classes
 *	are simply concatenated, except that their first
 *	members are flagged by changing sign.
 *
 *	Next the indices that point into member are unsorted into
 *	array class according to the original order of file0.
 *
 *	The cleverness lies in routine stone. This marches
 *	through the lines of file0, developing a vector klist
 *	of "k-candidates". At step i a k-candidate is a matched
 *	pair of lines x,y (x in file0 y in file1) such that
 *	there is a common subsequence of length k
 *	between the first i lines of file0 and the first y 
 *	lines of file1, but there is no such subsequence for
 *	any smaller y. x is the earliest possible mate to y
 *	that occurs in such a subsequence.
 *
 *	Whenever any of the members of the equivalence class of
 *	lines in file1 matable to a line in file0 has serial number 
 *	less than the y of some k-candidate, that k-candidate 
 *	with the smallest such y is replaced. The new 
 *	k-candidate is chained (via pred) to the current
 *	k-1 candidate so that the actual subsequence can
 *	be recovered. When a member has serial number greater
 *	that the y of all k-candidates, the klist is extended.
 *	At the end, the longest subsequence is pulled out
 *	and placed in the array J by unravel
 *
 *	With J in hand, the matches there recorded are
 *	check'ed against reality to assure that no spurious
 *	matches have crept in due to hashing. If they have,
 *	they are broken, and "jackpot" is recorded--a harmless
 *	matter except that a true match for a spuriously
 *	mated line may now be unnecessarily reported as a change.
 *
 *	Much of the complexity of the program comes simply
 *	from trying to minimize core utilization and
 *	maximize the range of doable problems by dynamically
 *	allocating what is needed and reusing what is not.
 *	The core requirements for problems larger than somewhat
 *	are (in words) 2*length(file0) + length(file1) +
 *	3*(number of k-candidates installed),  typically about
 *	6n words for files of length n. 
 */

#define	prints(s)	fputs(s,stdout)

static FILE	*input[2];
static char	mbuf[2][MB_LEN_MAX+1];
static char	*mcur[2];
static char	*mend[2];
static int	incompl[2];

struct cand {
	long	x;
	long	y;
	long	pred;
} cand;
static struct line {
	long	serial;
	long	value;
} *file[2], line;
static long	len[2];
static struct	line *sfile[2];/*shortened by pruning common prefix and suffix*/
static long	slen[2];
static long	pref, suff;	/* length of prefix and suffix */
static long	*class;		/* will be overlaid on file[0] */
static long	*member;	/* will be overlaid on file[1] */
static long	*klist;		/* will be overlaid on file[0] after class */
static struct	cand *clist;	/* merely a free storage pot for candidates */
static long	clen = 0;
static long	*J;		/* will be overlaid on class */
static off_t	*ixold;		/* will be overlaid on klist */
static off_t	*ixnew;		/* will be overlaid on file[1] */
static int	(*chrtran)(int);/* translation for case-folding */
static long	pstart;		/* start of last search for -p */
static long	plast;		/* match of last search for -p */
static long	*saveJ;		/* saved J for -p */

/* chrtran points to one of 3 translation functions:
 *	cup2low if folding upper to lower case
 *	clow2low if not folding case
 *	wlow2low if not folding case and MB_CUR_MAX > 1
 */
static int
clow2low(int c)
{
	return c;
}

static int
cup2low(int c)
{
	return tolower(c);
}

static int
wup2low(int c)
{
	return c & ~(wchar_t)0177 ? towlower(c) : tolower(c);
}

static char	*copytemp(char **, const char *);
#undef	splice
#define	splice	xxsplice
static char	*splice(const char *, char *);
static void	prepare(int);
static void	prune(void);
static void	equiv(struct line *, long, struct line *, long, long *);
static long	stone(long *, long, long *, long *);
static long	newcand(long, long, long);
static long	search(long *, long, long);
static void	unravel(long);
static void	check_sb(void);
static void	check_mb(void);
static void	sort(struct line *, long);
static void	unsort(struct line *, long, long *);
static long	skipline(int);
static long	wskipline(int);
static void	output(void);
static void	change(long, long, long, long);
static void	range(long, long, const char *);
static void	fetch(off_t *, long, long, FILE *, const char *, int);
static int	readhash(int);
static int	asciifile(FILE *);
static void	dump_context_vec(void);
static void	sdone(int);
static char	*wcget(int, wint_t *, int *);
static void	missnl(int);
static void	pdump(long);

#define	notseekable(m)	(((m)&S_IFMT) != S_IFREG && ((m)&S_IFMT) != S_IFBLK)

void
diffreg(void)
{
	register long i, j;
	char buf1[BUFSIZ], buf2[BUFSIZ];

	if (hflag) {
		diffargv[0] = "diffh";
		execvp(diffh, diffargv);
		if (sysv3)
			fprintf(stderr, "%s: cannot find diffh\n", progname);
		else
			fprintf(stderr, "%s: %s: %s\n", progname, diffh,
					strerror(errno));
		done();
	}
	chrtran = (iflag? mb_cur_max>1 ? wup2low : cup2low : clow2low);
	if ((stb1.st_mode & S_IFMT) == S_IFDIR) {
		file1 = splice(file1, file2);
		if (stat(file1, &stb1) < 0) {
			if (sysv3)
				stb1.st_mode = S_IFREG;
			else {
				fprintf(stderr, "%s: %s: %s\n", progname, file1,
						strerror(errno));
				done();
			}
		}
	} else if ((stb2.st_mode & S_IFMT) == S_IFDIR) {
		file2 = splice(file2, file1);
		if (stat(file2, &stb2) < 0) {
			if (sysv3)
				stb2.st_mode = S_IFREG;
			else {
				fprintf(stderr, "%s: %s: %s\n", progname, file2,
						strerror(errno));
				done();
			}
		}
	}
	if (!strcmp(file1, "-") || (notseekable(stb1.st_mode) &&
				strcmp(file1, "/dev/null"))) {
		if (!strcmp(file2, "-")) {
			fprintf(stderr, "%s: can't specify - -\n", progname);
			done();
		}
		file1 = copytemp(&tempfile1, file1);
		if (stat(file1, &stb1) < 0) {
			fprintf(stderr, "%s: %s: %s\n", progname, file1,
					strerror(errno));
			done();
		}
	} else if (!strcmp(file2, "-") || (notseekable(stb2.st_mode) &&
				strcmp(file2, "/dev/null"))) {
		file2 = copytemp(&tempfile2, file2);
		if (stat(file2, &stb2) < 0) {
			fprintf(stderr, "%s: %s: %s\n", progname, file2,
					strerror(errno));
			done();
		}
	}
	if ((input[0] = fopen(file1, "r")) == NULL) {
		if (sysv3)
			fprintf(stderr, "%s: cannot open %s\n",
					progname, file1);
		else
			fprintf(stderr, "%s: %s: %s\n", progname, file1,
					strerror(errno));
		done();
	}
	mcur[0] = mend[0] = NULL;
	if ((input[1] = fopen(file2, "r")) == NULL) {
		if (sysv3)
			fprintf(stderr, "%s: cannot open %s\n",
					progname, file2);
		else
			fprintf(stderr, "%s: %s: %s\n", progname, file2,
					strerror(errno));
		fclose(input[0]);
		done();
	}
	mcur[1] = mend[1] = NULL;
	if (stb1.st_size != stb2.st_size)
		goto notsame;
	for (;;) {
		i = fread(buf1, 1, BUFSIZ, input[0]);
		j = fread(buf2, 1, BUFSIZ, input[1]);
		if (i < 0 || j < 0 || i != j)
			goto notsame;
		if (i == 0 && j == 0) {
			fclose(input[0]);
			fclose(input[1]);
			status = 0;		/* files don't differ */
			goto same;
		}
		for (j = 0; j < i; j++)
			if (buf1[j] != buf2[j])
				goto notsame;
	}
notsame:
	/*
	 *	Files certainly differ at this point; set status accordingly
	 */
	status = 1;
	if (!aflag && (!asciifile(input[0]) || !asciifile(input[1]))) {
		printf("Binary files %s and %s differ\n", file1, file2);
		fclose(input[0]);
		fclose(input[1]);
		done();
	}
	prepare(0);
	prepare(1);
	fclose(input[0]);
	fclose(input[1]);
	prune();
	sort(sfile[0],slen[0]);
	sort(sfile[1],slen[1]);

	member = (long *)file[1];
	equiv(sfile[0], slen[0], sfile[1], slen[1], member);
	member = ralloc(member,(slen[1]+2)*sizeof(*member));

	class = (long *)file[0];
	unsort(sfile[0], slen[0], class);
	class = ralloc(class,(slen[0]+2)*sizeof(*class));

	klist = talloc((slen[0]+2)*sizeof(*klist));
	clist = talloc(sizeof(cand));
	i = stone(class, slen[0], member, klist);
	tfree(member);
	tfree(class);

	J = talloc((len[0]+2)*sizeof(*J));
	unravel(klist[i]);
	tfree(clist);
	tfree(klist);

	ixold = talloc((len[0]+2)*sizeof(*ixold));
	ixnew = talloc((len[1]+2)*sizeof(*ixnew));
	if (mb_cur_max > 1)
		check_mb();
	else
		check_sb();
	pstart = plast = 0;
	output();
	status = anychange;
same:
	if (opt == D_CONTEXT && anychange == 0)
		printf("No differences encountered\n");
	done();
}

static char *
copytemp(char **tf, const char *fn)
{
	const char	templ[] = "/tmp/dXXXXXX";
	char buf[BUFSIZ];
	register int i, f, sfd;

	if (*tf) {
		unlink(*tf);
		strcpy(*tf, templ);
	} else {
		sigset(SIGHUP,sdone);
		sigset(SIGINT,sdone);
		sigset(SIGPIPE,sdone);
		sigset(SIGTERM,sdone);
		*tf = strdup(templ);
	}
	f = mkstemp(*tf);
	if (f < 0) {
		fprintf(stderr, "%s: cannot create %s\n", progname, *tf);
		done();
	}
	if (strcmp(fn, "-")) {
		if ((sfd = open(fn, O_RDONLY)) < 0) {
			fprintf(stderr, "%s: %s: %s\n", progname, fn,
					strerror(errno));
			done();
		}
	} else
		sfd = 0;
	while ((i = read(sfd,buf,BUFSIZ)) > 0)
		if (write(f,buf,i) != i) {
			fprintf(stderr, "%s: write failed %s\n", progname, *tf);
			done();
		}
	close(f);
	if (sfd > 0)
		close(sfd);
	return (*tf);
}

static char *
splice(const char *dir, char *file)
{
	const char *tail;
	char *buf;

	if (!strcmp(file, "-")) {
		fprintf(stderr,
			"%s: can't specify - with other arg directory\n",
			progname);
		done();
	}
	tail = basename(file);
	buf = talloc(strlen(dir) + strlen(tail) + 2);
	sprintf(buf, "%s/%s", dir, tail);
	return (buf);
}

static void
prepare(int i)
{
	register struct line *p;
	register long j;
	register long h;

	fseeko(input[i], 0, SEEK_SET);
	mcur[i] = mend[i] = NULL;
	p = talloc(3*sizeof(line));
	for(j=0; h=readhash(i);) {
		p = ralloc(p,(++j+3)*sizeof(line));
		p[j].value = h;
	}
	len[i] = j;
	file[i] = p;
}

static void
prune(void)
{
	register long i;
	register int j;
	for(pref=0;pref<len[0]&&pref<len[1]&&
		file[0][pref+1].value==file[1][pref+1].value;
		pref++ ) ;
	for(suff=0;suff<len[0]-pref&&suff<len[1]-pref&&
		file[0][len[0]-suff].value==file[1][len[1]-suff].value;
		suff++) ;
	for(j=0;j<2;j++) {
		sfile[j] = file[j]+pref;
		slen[j] = len[j]-pref-suff;
		for(i=0;i<=slen[j];i++)
			sfile[j][i].serial = i;
	}
}

static void
equiv(struct line *a,long n,struct line *b,long m,long *c)
{
	register long i, j;
	i = j = 1;
	while(i<=n && j<=m) {
		if(a[i].value <b[j].value)
			a[i++].value = 0;
		else if(a[i].value == b[j].value)
			a[i++].value = j;
		else
			j++;
	}
	while(i <= n)
		a[i++].value = 0;
	b[m+1].value = 0;
	j = 0;
	while(++j <= m) {
		c[j] = -b[j].serial;
		while(b[j+1].value == b[j].value) {
			j++;
			c[j] = b[j].serial;
		}
	}
	c[j] = -1;
}

static long
stone(long *a,long n,long *b,register long *c)
{
	register long i, k,y;
	long j, l;
	long oldc, tc;
	long oldl;
	k = 0;
	c[0] = newcand(0,0,0);
	for(i=1; i<=n; i++) {
		j = a[i];
		if(j==0)
			continue;
		y = -b[j];
		oldl = 0;
		oldc = c[0];
		do {
			if(y <= clist[oldc].y)
				continue;
			l = search(c, k, y);
			if(l!=oldl+1)
				oldc = c[l-1];
			if(l<=k) {
				if(clist[c[l]].y <= y)
					continue;
				tc = c[l];
				c[l] = newcand(i,y,oldc);
				oldc = tc;
				oldl = l;
			} else {
				c[l] = newcand(i,y,oldc);
				k++;
				break;
			}
		} while((y=b[++j]) > 0);
	}
	return(k);
}

static long
newcand(long x,long y,long pred)
{
	register struct cand *q;
	clist = ralloc(clist,++clen*sizeof(cand));
	q = clist + clen -1;
	q->x = x;
	q->y = y;
	q->pred = pred;
	return(clen-1);
}

static long
search(long *c, long k, long y)
{
	register long i, j, l;
	long t;
	if(clist[c[k]].y<y)	/*quick look for typical case*/
		return(k+1);
	i = 0;
	j = k+1;
	while (1) {
		l = i + j;
		if ((l >>= 1) <= i) 
			break;
		t = clist[c[l]].y;
		if(t > y)
			j = l;
		else if(t < y)
			i = l;
		else
			return(l);
	}
	return(l+1);
}

static void
unravel(long p)
{
	register long i;
	register struct cand *q;
	for(i=0; i<=len[0]; i++)
		J[i] =	i<=pref ? i:
			i>len[0]-suff ? i+len[1]-len[0]:
			0;
	for(q=clist+p;q->y!=0;q=clist+q->pred)
		J[q->x+pref] = q->y+pref;
}

/* check does double duty:
1.  ferret out any fortuitous correspondences due
to confounding by hashing (which result in "jackpot")
2.  collect random access indexes to the two files */

static void
check_sb(void)
{
	register long i, j;
	long jackpot;
	off_t ctold, ctnew;
	register int c,d;

	if ((input[0] = fopen(file1,"r")) == NULL) {
		perror(file1);
		done();
	}
	if ((input[1] = fopen(file2,"r")) == NULL) {
		perror(file2);
		fclose(input[0]);
		done();
	}
	j = 1;
	ixold[0] = ixnew[0] = 0;
	jackpot = 0;
	ctold = ctnew = 0;
	for(i=1;i<=len[0];i++) {
		if(J[i]==0) {
			ixold[i] = ctold += skipline(0);
			continue;
		}
		while(j<J[i]) {
			ixnew[j] = ctnew += skipline(1);
			j++;
		}
		if(bflag || wflag || iflag) {
			for(;;) {
				c = getc(input[0]);
				d = getc(input[1]);
				ctold++;
				ctnew++;
				if(bflag && isspace(c) && isspace(d)) {
					do {
						if(c=='\n')
							break;
						ctold++;
					} while(isspace(c=getc(input[0])));
					do {
						if(d=='\n')
							break;
						ctnew++;
					} while(isspace(d=getc(input[1])));
				} else if ( wflag ) {
					while( isspace(c) && c!='\n' ) {
						c=getc(input[0]);
						ctold++;
					}
					while( isspace(d) && d!='\n' ) {
						d=getc(input[1]);
						ctnew++;
					}
				}
				if(chrtran(c) != chrtran(d)) {
					jackpot++;
					J[i] = 0;
					if(c!='\n')
						ctold += skipline(0);
					if(d!='\n')
						ctnew += skipline(1);
					break;
				}
				if(c=='\n' || c==EOF)
					break;
			}
		} else {
			for(;;) {
				ctold++;
				ctnew++;
				if((c=getc(input[0])) != (d=getc(input[1]))) {
					/* jackpot++; */
					J[i] = 0;
					if(c!='\n')
						ctold += skipline(0);
					if(d!='\n')
						ctnew += skipline(1);
					break;
				}
				if(c=='\n' || c==EOF)
					break;
			}
		}
		ixold[i] = ctold;
		ixnew[j] = ctnew;
		j++;
	}
	for(;j<=len[1];j++) {
		ixnew[j] = ctnew += skipline(1);
	}
	fclose(input[0]);
	fclose(input[1]);
/*
	if(jackpot)
		fprintf(stderr, "jackpot\n");
*/
}

static void
check_mb(void)
{
	register long i, j;
	long jackpot;
	off_t ctold, ctnew;
	wint_t	wc, wd;
	int	nc, nd;
	char	*cc, *cd;

	if ((input[0] = fopen(file1,"r")) == NULL) {
		perror(file1);
		done();
	}
	mcur[0] = mend[0] = NULL;
	if ((input[1] = fopen(file2,"r")) == NULL) {
		perror(file2);
		fclose(input[0]);
		done();
	}
	mcur[1] = mend[1] = NULL;
	j = 1;
	ixold[0] = ixnew[0] = 0;
	jackpot = 0;
	ctold = ctnew = 0;
	for(i=1;i<=len[0];i++) {
		if(J[i]==0) {
			ixold[i] = ctold += wskipline(0);
			continue;
		}
		while(j<J[i]) {
			ixnew[j] = ctnew += wskipline(1);
			j++;
		}
		if(bflag || wflag || iflag) {
			for(;;) {
				cc = wcget(0, &wc, &nc);
				cd = wcget(1, &wd, &nd);
				if(bflag && iswspace(wc) && iswspace(wd)) {
					do {
						if(wc=='\n')
							break;
						ctold += nc;
					} while(cc = wcget(0, &wc, &nc),
							iswspace(wc));
					do {
						if(wd=='\n')
							break;
						ctnew += nd;
					} while(cd = wcget(1, &wd, &nd),
							iswspace(wd));
					ctold += nc;
					ctnew += nd;
				} else if ( wflag ) {
					ctold += nc;
					ctnew += nd;
					while( iswspace(wc) && wc!='\n' && cc) {
						cc = wcget(0, &wc, &nc);
						ctold += nc;
					}
					while( iswspace(wd) && wd!='\n' && cd) {
						cd = wcget(1, &wd, &nd);
						ctnew += nd;
					}
				} else {
					ctold += nc;
					ctnew += nd;
				}
				if(chrtran(wc) != chrtran(wd) ||
						wc == WEOF && wd == WEOF &&
						(cc == NULL && cd && *cd ||
						 cc && *cc && cd == NULL ||
						 cc && cd && *cc != *cd)) {
					jackpot++;
					J[i] = 0;
					if(wc!='\n')
						ctold += wskipline(0);
					if(wd!='\n')
						ctnew += wskipline(1);
					break;
				}
				if(wc=='\n' || cc == NULL)
					break;
			}
		} else {
			for(;;) {
				cc = wcget(0, &wc, &nc);
				cd = wcget(1, &wd, &nd);
				ctold += nc;
				ctnew += nd;
				if (wc != wd || wc == WEOF && wd == WEOF &&
						(cc == NULL && cd && *cd ||
						 cc && *cc && cd == NULL ||
						 cc && cd && *cc != *cd)) {
					/* jackpot++; */
					J[i] = 0;
					if(wc!='\n')
						ctold += wskipline(0);
					if(wd!='\n')
						ctnew += wskipline(1);
					break;
				}
				if(wc=='\n' || cc == NULL)
					break;
			}
		}
		ixold[i] = ctold;
		ixnew[j] = ctnew;
		j++;
	}
	for(;j<=len[1];j++) {
		ixnew[j] = ctnew += wskipline(1);
	}
	fclose(input[0]);
	fclose(input[1]);
/*
	if(jackpot)
		fprintf(stderr, "jackpot\n");
*/
}

static void
sort(struct line *a,long n)	/*shellsort CACM #201*/
{
	struct line w;
	register long j,m = 0;
	struct line *ai;
	register struct line *aim;
	long k;

	if (n == 0)
		return;
	for(j=1;j<=n;j*= 2)
		m = 2*j - 1;
	for(m/=2;m!=0;m/=2) {
		k = n-m;
		for(j=1;j<=k;j++) {
			for(ai = &a[j]; ai > a; ai -= m) {
				aim = &ai[m];
				if(aim < ai)
					break;	/*wraparound*/
				if(aim->value > ai[0].value ||
				   aim->value == ai[0].value &&
				   aim->serial > ai[0].serial)
					break;
				w.value = ai[0].value;
				ai[0].value = aim->value;
				aim->value = w.value;
				w.serial = ai[0].serial;
				ai[0].serial = aim->serial;
				aim->serial = w.serial;
			}
		}
	}
}

static void
unsort(struct line *f, long l, long *b)
{
	register long *a;
	register long i;
	a = talloc((l+1)*sizeof(*a));
	for(i=1;i<=l;i++)
		a[f[i].serial] = f[i].value;
	for(i=1;i<=l;i++)
		b[i] = a[i];
	tfree(a);
}

static long
skipline(int f)
{
	register long i;
	register int c;

	for(i=1;(c=getc(input[f]))!='\n';i++)
		if (c == EOF)
			return(i);
	return(i);
}

static long
wskipline(int f)
{
	long	i;
	int	n;
	wint_t	wc;
	char	*cp;

	for (i = 1; cp = wcget(f, &wc, &n), wc != '\n'; i += n)
		if (cp == NULL)
			return (i);
	return (i);
}

static void
output(void)
{
	long m;
	register long i0, i1, j1;
	long j0;
	if ((input[0] = fopen(file1,"r")) == NULL) {
		perror(file1);
		done();
	}
	if ((input[1] = fopen(file2,"r")) == NULL) {
		perror(file2);
		fclose(input[0]);
		done();
	}
	m = len[0];
	J[0] = 0;
	J[m+1] = len[1]+1;
	if (pflag) {
		saveJ = talloc((len[0]+2)*sizeof(*saveJ));
		memcpy(saveJ, J, (len[0]+2)*sizeof(*saveJ));
	}
	if(opt!=D_EDIT) for(i0=1;i0<=m;i0=i1+1) {
		while(i0<=m&&J[i0]==J[i0-1]+1) i0++;
		j0 = J[i0-1]+1;
		i1 = i0-1;
		while(i1<m&&J[i1+1]==0) i1++;
		j1 = J[i1+1]-1;
		J[i1] = j1;
		change(i0,i1,j0,j1);
	} else for(i0=m;i0>=1;i0=i1-1) {
		while(i0>=1&&J[i0]==J[i0+1]-1&&J[i0]!=0) i0--;
		j0 = J[i0+1]-1;
		i1 = i0+1;
		while(i1>1&&J[i1-1]==0) i1--;
		j1 = J[i1-1]+1;
		J[i1] = j1;
		change(i1,i0,j1,j0);
	}
	if(m==0)
		change(1,0,1,len[1]);
	if (opt==D_IFDEF) {
		for (;;) {
#define	c i0
			c = getc(input[0]);
			if (c == EOF)
				goto end;
			putchar(c);
		}
#undef c
	}
	if (anychange && (opt == D_CONTEXT || opt == D_UNIFIED))
		dump_context_vec();
end:	fclose(input[0]);
	fclose(input[1]);
}

static int
allblank(off_t *f, long a, long b, FILE *lb)
{
	long	i;

	if (a > b)
		return 1;
	for (i = a; i <= b; i++)
		if (f[i]-f[i-1] != 1)
			return 0;
	return 1;
}

/*
 * The following struct is used to record change information when
 * doing a "context" diff.  (see routine "change" to understand the
 * highly mneumonic field names)
 */
struct context_vec {
	long	a;	/* start line in old file */
	long	b;	/* end line in old file */
	long	c;	/* start line in new file */
	long	d;	/* end line in new file */
};

static struct	context_vec	*context_vec_start,
				*context_vec_end,
				*context_vec_ptr;

#define	MAX_CONTEXT	129

/* indicate that there is a difference between lines a and b of the from file
   to get to lines c to d of the to file.
   If a is greater then b then there are no lines in the from file involved
   and this means that there were lines appended (beginning at b).
   If c is greater than d then there are lines missing from the to file.
*/
static void
change(long a,long b,long c,long d)
{
	int ch;
	struct stat stbuf;

	if (opt != D_IFDEF && a>b && c>d)
		return;
	if (Bflag && allblank(ixold,a,b,input[0]) &&
			allblank(ixnew,c,d,input[1]))
		return;
	if (anychange == 0) {
		anychange = 1;
		if(opt == D_CONTEXT || opt == D_UNIFIED) {
			if (opt == D_CONTEXT) {
				printf("*** %s\t", file1);
				stat(file1, &stbuf);
				printf("%s--- %s\t",
					ctime(&stbuf.st_mtime), file2);
				stat(file2, &stbuf);
				printf("%s", ctime(&stbuf.st_mtime));
			} else {	/* opt == D_UNIFIED */
				printf("--- %s\t", file1);
				stat(file1, &stbuf);
				printf("%s+++ %s\t",
					ctime(&stbuf.st_mtime), file2);
				stat(file2, &stbuf);
				printf("%s", ctime(&stbuf.st_mtime));
			}

			context_vec_start = talloc(MAX_CONTEXT *
						   sizeof(*context_vec_start));
			context_vec_end = context_vec_start + MAX_CONTEXT;
			context_vec_ptr = context_vec_start - 1;
		}
	}
	if (a <= b && c <= d)
		ch = 'c';
	else
		ch = (a <= b) ? 'd' : 'a';
	if(opt == D_CONTEXT || opt == D_UNIFIED) {
		/*
		 * if this new change is within 'context' lines of
		 * the previous change, just add it to the change
		 * record.  If the record is full or if this
		 * change is more than 'context' lines from the previous
		 * change, dump the record, reset it & add the new change.
		 */
		if ( context_vec_ptr >= context_vec_end-1 ||
		     ( context_vec_ptr >= context_vec_start &&
		       a > (context_vec_ptr->b + 2*context) &&
		       c > (context_vec_ptr->d + 2*context) ) )
			dump_context_vec();

		context_vec_ptr++;
		context_vec_ptr->a = a;
		context_vec_ptr->b = b;
		context_vec_ptr->c = c;
		context_vec_ptr->d = d;
		return;
	}
	switch (opt) {

	case D_NORMAL:
	case D_EDIT:
		range(a,b,",");
		putchar(a>b?'a':c>d?'d':'c');
		if(opt==D_NORMAL)
			range(c,d,",");
		putchar('\n');
		break;
	case D_REVERSE:
		putchar(a>b?'a':c>d?'d':'c');
		range(a,b," ");
		putchar('\n');
		break;
        case D_NREVERSE:
                if (a>b)
                        printf("a%ld %ld\n",b,d-c+1);
                else {
                        printf("d%ld %ld\n",a,b-a+1);
                        if (!(c>d))
                           /* add changed lines */
                           printf("a%ld %ld\n",b, d-c+1);
                }
                break;
	}
	if(opt == D_NORMAL || opt == D_IFDEF) {
		fetch(ixold,a,b,input[0],"< ", 1);
		if(a<=b&&c<=d && opt == D_NORMAL)
			prints("---\n");
	}
	fetch(ixnew,c,d,input[1],opt==D_NORMAL?"> ":"", 0);
	if ((opt ==D_EDIT || opt == D_REVERSE) && c<=d)
		prints(".\n");
	if (inifdef) {
		fprintf(stdout, "#endif %s\n", endifname);
		inifdef = 0;
	}
}

static void
range(long a,long b,const char *separator)
{
	printf("%ld", a>b?b:a);
	if(a<b || opt==D_UNIFIED) {
		printf("%s%ld", separator, opt==D_UNIFIED ? b-a+1 : b);
	}
}

static void
fetch(off_t *f,long a,long b,FILE *lb,const char *s,int oldfile)
{
	register long i, j;
	register int c;
	register long col;
	register long nc;
	int oneflag = (*ifdef1!='\0') != (*ifdef2!='\0');

	/*
	 * When doing #ifdef's, copy down to current line
	 * if this is the first file, so that stuff makes it to output.
	 */
	if (opt == D_IFDEF && oldfile){
		off_t curpos = ftello(lb);
		/* print through if append (a>b), else to (nb: 0 vs 1 orig) */
		nc = f[a>b? b : a-1 ] - curpos;
		for (i = 0; i < nc; i++) {
			c = getc(lb);
			if (c == EOF)
				break;
			putchar(c);
		}
	}
	if (a > b)
		return;
	if (opt == D_IFDEF) {
		if (inifdef)
			fprintf(stdout, "#else %s%s\n", oneflag && oldfile==1 ? "!" : "", ifdef2);
		else {
			if (oneflag) {
				/* There was only one ifdef given */
				endifname = ifdef2;
				if (oldfile)
					fprintf(stdout, "#ifndef %s\n", endifname);
				else
					fprintf(stdout, "#ifdef %s\n", endifname);
			}
			else {
				endifname = oldfile ? ifdef1 : ifdef2;
				fprintf(stdout, "#ifdef %s\n", endifname);
			}
		}
		inifdef = 1+oldfile;
	}

	for(i=a;i<=b;i++) {
		fseeko(lb,f[i-1],SEEK_SET);
		nc = f[i]-f[i-1];
		if (opt != D_IFDEF)
			prints(s);
		col = 0;
		for(j=0;j<nc;j++) {
			c = getc(lb);
			if (c == '\t' && tflag)
				do
					putchar(' ');
				while (++col & 7);
			else if (c == EOF) {
				if (aflag)
					printf("\n\\ No newline at "
							"end of file\n");
				else
					putchar('\n');
				break;
			} else {
				putchar(c);
				col++;
			}
		}
	}

	if (inifdef && !wantelses) {
		fprintf(stdout, "#endif %s\n", endifname);
		inifdef = 0;
	}
}

#define POW2			/* define only if HALFLONG is 2**n */
#define HALFLONG 16
#define low(x)	(x&((1L<<HALFLONG)-1))
#define high(x)	(x>>HALFLONG)

/*
 * hashing has the effect of
 * arranging line in 7-bit bytes and then
 * summing 1-s complement in 16-bit hunks 
 */
static int
readhash(register int f)
{
	register int32_t sum;
	register unsigned shift;
	register int t;
	register int space;
	int	content;
	wint_t	wt;
	int	n;
	char	*cp;

	sum = 1;
	space = 0;
	content = 0;
	if(!bflag && !wflag) {
		if(iflag) {
			if (mb_cur_max > 1) {
				for (shift = 0; cp = wcget(f, &wt, &n),
						wt != '\n'; shift += 7) {
					if (cp == NULL) {
						if (content) {
							missnl(f);
							break;
						}
						return (0);
					}
					content = 1;
					sum += (int32_t)chrtran(wt) << (shift
#ifdef POW2
			    		&= HALFLONG - 1);
#else
			    		%= HALFLONG);
#endif
				}
			} else {
				for(shift=0;(t=getc(input[f]))!='\n';shift+=7) {
					if(t==EOF) {
						if (content) {
							missnl(f);
							break;
						}
						return(0);
					}
					content = 1;
					sum += (int32_t)chrtran(t) << (shift
#ifdef POW2
				    	&= HALFLONG - 1);
#else
				    	%= HALFLONG);
#endif
				}
			}
		} else {
			for(shift=0;(t=getc(input[f]))!='\n';shift+=7) {
				if(t==EOF) {
					if (content) {
						missnl(f);
						break;
					}
					return(0);
				}
				content = 1;
				sum += (int32_t)t << (shift
#ifdef POW2
				    &= HALFLONG - 1);
#else
				    %= HALFLONG);
#endif
			}
		}
	} else {
		if (mb_cur_max > 1) {
			for(shift=0;;) {
				if ((cp = wcget(f, &wt, &n)) == NULL) {
					if (content) {
						missnl(f);
						break;
					}
					return(0);
				}
				content = 1;
				switch (wt) {
				default:
					if (iswspace(wt)) {
						space++;
						continue;
					}
					if(space && !wflag) {
						shift += 7;
						space = 0;
					}
					sum += (int32_t)chrtran(wt) << (shift
#ifdef POW2
				    	&= HALFLONG - 1);
#else
				    	%= HALFLONG);
#endif
					shift += 7;
					continue;
				case '\n':
					break;
				}
				break;
			}
		} else {
			for(shift=0;;) {
				switch(t=getc(input[f])) {
				case EOF:
					if (content) {
						missnl(f);
						break;
					}
					return(0);
				default:
					content = 1;
					if (isspace(t)) {
						space++;
						continue;
					}
					if(space && !wflag) {
						shift += 7;
						space = 0;
					}
					sum += (int32_t)chrtran(t) << (shift
#ifdef POW2
				    	&= HALFLONG - 1);
#else
				    	%= HALFLONG);
#endif
					shift += 7;
					continue;
				case '\n':
					break;
				}
				break;
			}
		}
	}
	sum = low(sum) + high(sum);
	return((int16_t)low(sum) + (int16_t)high(sum));
}

static int
asciifile(FILE *f)
{
	char buf[BUFSIZ];
	register int cnt;
	register char *cp;

	fseeko(f, 0, SEEK_SET);
	cnt = fread(buf, 1, BUFSIZ, f);
	cp = buf;
	while (--cnt >= 0)
		if (*cp++ == '\0')
			return (0);
	return (1);
}


/* dump accumulated "context" diff changes */
static void
dump_context_vec(void)
{
	register long	a, b = 0, c, d = 0;
	register char	ch;
	register struct	context_vec *cvp = context_vec_start;
	register long	lowa, upb, lowc, upd;
	register int	do_output;

	if ( cvp > context_vec_ptr )
		return;

	lowa = max(1, cvp->a - context);
	upb  = min(len[0], context_vec_ptr->b + context);
	lowc = max(1, cvp->c - context);
	upd  = min(len[1], context_vec_ptr->d + context);

	if (opt == D_UNIFIED) {
		printf("@@ -");
		range(lowa, upb, ",");
		printf(" +");
		range(lowc, upd, ",");
		printf(" @@");
		if (pflag)
			pdump(lowa-1);
		printf("\n");

		while (cvp <= context_vec_ptr) {
			a = cvp->a; b = cvp->b; c = cvp->c; d = cvp->d;

			if (a <= b && c <= d)
				ch = 'c';
			else
				ch = (a <= b) ? 'd' : 'a';

			switch (ch) {
			case 'a':
				fetch(ixold,lowa,b,input[0]," ", 0);
				fetch(ixnew,c,d,input[1],"+",0);
				break;
			case 'c':
				fetch(ixold,lowa,a-1,input[0]," ", 0);
				fetch(ixold,a,b,input[0],"-",0);
				fetch(ixnew,c,d,input[1],"+",0);
				break;
			case 'd':
				fetch(ixold,lowa,a-1,input[0]," ", 0);
				fetch(ixold,a,b,input[0],"-",0);
				break;
			}
			lowa = b + 1;
			cvp++;
		}
		fetch(ixold, b+1, upb, input[0], " ", 0);
	}

	if (opt == D_CONTEXT) {
		printf("***************");
		if (pflag)
			pdump(lowa-1);
		printf("\n*** ");
		range(lowa,upb,",");
		printf(" ****\n");

		/*
		 * output changes to the "old" file.  The first loop suppresses
		 * output if there were no changes to the "old" file (we'll see
		 * the "old" lines as context in the "new" list).
		 */
		do_output = 0;
		for ( ; cvp <= context_vec_ptr; cvp++)
			if (cvp->a <= cvp->b) {
				cvp = context_vec_start;
				do_output++;
				break;
			}
	
		if ( do_output ) {
			while (cvp <= context_vec_ptr) {
				a = cvp->a; b = cvp->b; c = cvp->c; d = cvp->d;

				if (a <= b && c <= d)
					ch = 'c';
				else
					ch = (a <= b) ? 'd' : 'a';

				if (ch == 'a')
					fetch(ixold,lowa,b,input[0],"  ", 0);
				else {
					fetch(ixold,lowa,a-1,input[0],"  ", 0);
					fetch(ixold,a,b,input[0],
							ch == 'c' ? "! " : "- ",
							0);
				}
				lowa = b + 1;
				cvp++;
			}
			fetch(ixold, b+1, upb, input[0], "  ", 0);
		}

		/* output changes to the "new" file */
		printf("--- ");
		range(lowc,upd,",");
		printf(" ----\n");

		do_output = 0;
		for (cvp = context_vec_start; cvp <= context_vec_ptr; cvp++)
			if (cvp->c <= cvp->d) {
				cvp = context_vec_start;
				do_output++;
				break;
			}
	
		if (do_output) {
			while (cvp <= context_vec_ptr) {
				a = cvp->a; b = cvp->b; c = cvp->c; d = cvp->d;

				if (a <= b && c <= d)
					ch = 'c';
				else
					ch = (a <= b) ? 'd' : 'a';

				if (ch == 'd')
					fetch(ixnew,lowc,d,input[1],"  ", 0);
				else {
					fetch(ixnew,lowc,c-1,input[1],"  ", 0);
					fetch(ixnew,c,d,input[1],
							ch == 'c' ? "! " : "+ ",
							0);
				}
				lowc = d + 1;
				cvp++;
			}
			fetch(ixnew, d+1, upd, input[1], "  ", 0);
		}
	}

	context_vec_ptr = context_vec_start - 1;
}

/*ARGSUSED*/
static void
sdone(int signo)
{
	done();
}

static char *
wcget(int f, wint_t *wc, int *len)
{
	size_t	rest;
	int	c, i, n;

	i = 0;
	rest = mend[f] - mcur[f];
	if (rest && mcur[f] > mbuf[f]) {
		do
			mbuf[f][i] = mcur[f][i];
		while (i++, --rest);
	} else if (incompl[f]) {
		incompl[f] = 0;
		*wc = WEOF;
		mend[f] = mcur[f] = NULL;
		return NULL;
	}
	if (i == 0) {
		c = getc(input[f]);
		if (c == EOF) {
			*wc = WEOF;
			mend[f] = mcur[f] = NULL;
			return NULL;
		}
		mbuf[f][i++] = c;
	}
	if (mbuf[f][0] & 0200) {
		while (mbuf[f][i-1] != '\n' && i < mb_cur_max &&
				incompl[f] == 0) {
			c = getc(input[f]);
			if (c != EOF)
				mbuf[f][i++] = c;
			else
				incompl[f] = 1;
		}
		n = mbtowi(wc, mbuf[f], i);
		if (n < 0) {
			*len = 1;
			*wc = WEOF;
		} else if (n == 0) {
			*len = 1;
			*wc = '\0';
		} else
			*len = n;
	} else {
		*wc = mbuf[f][0];
		*len = n = 1;
	}
	mcur[f] = &mbuf[f][*len];
	mend[f] = &mcur[f][i - *len];
	return mbuf[f];
}

static void
missnl(int f)
{
	if (aflag == 0)
		fprintf(stderr, "Warning: missing newline at end of file %s\n",
			f == 0 ? file1 : file2);
}

/*
 * Find and dump the name of the C function with the -p option. The
 * search begins at line sa.
 */
static void
pdump(long sa)
{
#define	psize	40
	static char	lbuf[psize*MB_LEN_MAX+1];
	char	mbuf[MB_LEN_MAX+1];
	int	c, i, j;
	wchar_t	wc;
	long	a = sa;

	while (a-- > pstart) {
		if (saveJ[a+1] == 0)
			continue;
		fseeko(input[0], ixold[a], SEEK_SET);
		i = 0;
		do {
			if ((c=getc(input[0])) == EOF || c == '\n')
				break;
			mbuf[i] = c;
		} while (++i<mb_cur_max);
		if (mb_cur_max>1) {
			mbuf[i] = 0;
			if (((c=mbuf[0])&0200)==0)
				wc = mbuf[0];
			else if (mbtowc(&wc, mbuf, i) < 0)
				continue;
		}
		if ((mb_cur_max>1 && mbuf[0]&0200 ? iswalpha(wc):isalpha(c)) ||
				c == '$' || c == '_') {
			plast = a+1;
			for (j = 0; j < i; j++)
				lbuf[j] = mbuf[j];
			while (i < sizeof lbuf - 1) {
				if ((c=getc(input[0])) == EOF || c == '\n')
					break;
				lbuf[i++] = c;
			}
			for (j=0;j<i&&j<psize;) {
				if (mb_cur_max==1 || (lbuf[j]&0200) == 0)
					j++;
				else {
					c = mbtowc(NULL, &lbuf[j], i-j);
					j += c>0 ? c:1;
				}
			}
			lbuf[j] = 0;
			break;
		}
	}
	pstart = sa;
	if (plast) {
		putchar(' ');
		for (i = 0; lbuf[i]; i++)
			putchar(lbuf[i] & 0377);
	}
}
