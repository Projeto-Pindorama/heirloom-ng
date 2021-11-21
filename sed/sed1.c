/*	from Unix 7th Edition sed	*/
/*	Sccsid @(#)sed1.c	1.42 (gritter) 2/6/05>	*/
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

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<wchar.h>
#include	<wctype.h>
#include "sed.h"

#if !defined (SUS) && !defined (SU3) && !defined(S42)
#define	INIT		extern char *cp, *badp; \
			register char *sp = cp;
#define	GETC()		(*sp++)
#define	PEEKC()		(*sp)
#define	UNGETC(c)	(--sp)
#define	RETURN(c)	{ cp = sp; return ep; }
#define	ERROR(c)	{ cp = sp; return badp; }

#define	regexp_h_malloc(n)	smalloc(n)
#include <regexp.h>
#endif	/* !SUS && !SU3 && !S42 */

#ifndef	CCEOF
#ifdef	CEOF
#define	CCEOF	CEOF
#else	/* !CEOF */
#define	CCEOF	22
#endif	/* !CEOF */
#endif	/* !CCEOF */

int ceof = CCEOF;

#if !defined (SUS) && !defined (SU3)
static const char	*trans[]  = {
	"\\00",
	"\\01",
	"\\02",
	"\\03",
	"\\04",
	"\\05",
	"\\06",
	"\\07",
	"-<",
	"->",
	"\n",
	"\\13",
	"\\14",
	"\\15",
	"\\16",
	"\\17",
	"\\20",
	"\\21",
	"\\22",
	"\\23",
	"\\24",
	"\\25",
	"\\26",
	"\\27",
	"\\30",
	"\\31",
	"\\32",
	"\\33",
	"\\34",
	"\\35",
	"\\36",
	"\\37"
};
#endif	/* !SUS, !SU3 */

static char	*cbp;
static char	*ebp;
static int	dolflag;
static int	sflag;
static int	jflag;
static int	delflag;
static long long	lnum;
static char	ibuf[512];
static int	ibrd;
static int	mflag;
static int	f = -1;
static int	spend;
static int	genend;
static int	hspend;

static void command(struct reptr *);
static int match(char *, int, int);
static int substitute(struct reptr *);
static void dosub(char *);
static int place(int, int, int);
static int gline(int);
static void arout(void);
static void lcom(wint_t, int);
static void oout(int);
static void mout(const char *);
static void nout(wint_t);
static void wout(wint_t);
static void lout(int);

#if defined (SUS) || defined (SU3) || defined (S42)
#define	NBRA	9
int	sed;
int	nbra;
int	circf;
static char	*braslist[NBRA];
static char	*braelist[NBRA];
static char	*loc1, *loc2, *locs;

static int
step(char *line, char *pattern)
{
	struct re_emu	*re = (struct re_emu *)&pattern[-1];
	regmatch_t bralist[NBRA+1];
	int	eflag = 0;
	int	res;
	int	i, nsub;

	if (circf == 2)	/* empty pattern */
		return 0;
	if (locs)
		eflag |= REG_NOTBOL;
	/*
	 * Don't fetch more match locations than necessary since this
	 * might prevent use of DFA.
	 */
	nsub = mflag;
	if ((res = regexec(&re->r_preg, line, nsub, bralist, eflag)) == 0) {
		if (nsub > 0) {
			loc1 = line + bralist[0].rm_so;
			loc2 = line + bralist[0].rm_eo;
			for (i = 1; i < nsub; i++) {
				if (bralist[i].rm_so != -1) {
					braslist[i-1] = line + bralist[i].rm_so;
					braelist[i-1] = line + bralist[i].rm_eo;
				} else
					braslist[i-1] = braelist[i-1] = NULL;
			}
		}
	}
	return res == 0;
}
#endif	/* SUS || SU3 || S42 */

static int	lcomlen;
static int	Braslist[NBRA];
static int	Braelist[NBRA];
static int	Loc1, Loc2;

void
execute(const char *file)
{
	register char *p1, *p2;
	register struct reptr	*ipc;
	int	c;
	int	execc;

	if (f >= 0)
		close(f);
	if (file) {
		if ((f = open(file, O_RDONLY)) < 0) {
			nonfatal("Can't open %s", file);
			return;
		}
	} else
		f = 0;

	ebp = ibuf;
	cbp = ibuf;

	if(pending) {
		ipc = pending;
		pending = 0;
		goto yes;
	}

	for(;;) {
		if((execc = gline(0)) < 0) {
			if (f >= 0) {
				close(f);
				f = -1;
			}
			return;
		}
		spend = execc;

		for(ipc = ptrspace; ipc->command; ) {

			p1 = ipc->ad1;
			p2 = ipc->ad2;

			if(p1) {

				if(ipc->inar) {
					if(*p2 == CEND) {
						p1 = 0;
					} else if(*p2 == CLNUM) {
						c = glno(&p2[1]);
						if(lnum > tlno[c]) {
							ipc->inar = 0;
							if(ipc->negfl)
								goto yes;
							ipc++;
							continue;
						}
						if(lnum == tlno[c]) {
							ipc->inar = 0;
						}
					} else if(match(p2, 0, 0)) {
						ipc->inar = 0;
					}
				} else if(*p1 == CEND) {
					if(!dolflag) {
						if(ipc->negfl)
							goto yes;
						ipc++;
						continue;
					}

				} else if(*p1 == CLNUM) {
					c = glno(&p1[1]);
					if(lnum != tlno[c]) {
						if(ipc->negfl)
							goto yes;
						ipc++;
						continue;
					}
					if(p2) {
						ipc->inar = 1;
#if defined (SUS) || defined (SU3)
						goto ichk;
#endif	/* SUS, SU3 */
					}
				} else if(match(p1, 0, 0)) {
					if(p2) {
						ipc->inar = 1;
#if defined (SUS) || defined (SU3)
					ichk:	if (*p2 == CLNUM) {
							c = glno(&p2[1]);
							if (lnum >= tlno[c])
								ipc->inar = 0;
						}
#endif	/* SUS, SU3 */
					}
				} else {
					if(ipc->negfl)
						goto yes;
					ipc++;
					continue;
				}
			}

			if(ipc->negfl) {
				ipc++;
				continue;
			}
	yes:
			command(ipc);

			if(delflag)
				break;

			if(jflag) {
				jflag = 0;
				if((ipc = P(ipc->bptr.lb1)) == 0) {
					ipc = ptrspace;
					break;
				}
			} else
				ipc++;

		}
		if(!nflag && !delflag) {
			for(p1 = linebuf; p1 < &linebuf[spend]; p1++)
				putc(*p1&0377, stdout);
			putc('\n', stdout);
		}

		if(A(aptr) > abuf) {
			arout();
		}

		delflag = 0;

	}
}

static int
match(char *expbuf, int gf, int needloc)
{
	register char	*p1;
	int	i, val;

	if(gf) {
		if(*expbuf)	return(0);
#if defined (SUS) || defined (SU3) || defined (S42)
		if (loc1 == loc2) {
			int	n;
			wchar_t	wc;
			if (multibyte && (n = mbtowc(&wc, &linebuf[Loc2],
							MB_LEN_MAX)) > 0)
				Loc2 += n;
			else
				Loc2++;
		}
#endif
		locs = p1 = loc2 = &linebuf[Loc2];
	} else {
		p1 = linebuf;
		locs = 0;
	}

	mflag = needloc;
	circf = *expbuf++;
	val = step(p1, expbuf);
	for (i = 0; i < NBRA; i++) {
		Braslist[i] = braslist[i] - linebuf;
		Braelist[i] = braelist[i] - linebuf;
	}
	Loc1 = loc1 - linebuf;
	Loc2 = loc2 - linebuf;
	return val;
}

static int
substitute(struct reptr *ipc)
{
	int	matchcnt = 1;

	if (match(ipc->bptr.re1, 0, ipc->nsub + 1) == 0)
		return(0);

	sflag = 0;
	if (ipc->gfl >= -1 && ipc->gfl <= 1)
		dosub(ipc->rhs);

	if(ipc->gfl != 0) {
		while(linebuf[Loc2]) {
			if(match(ipc->bptr.re1, 1, ipc->nsub + 1) == 0)
				break;
			matchcnt++;
			if (ipc->gfl == -1 || ipc->gfl == matchcnt)
				dosub(ipc->rhs);
		}
	}
	return(1);
}

static void
dosub(char *rhsbuf)
{
	register int lc, sc;
	register char	*rp;
	int c;

	sflag = 1;
	lc = 0;	/*linebuf*/
	sc = 0;	/*genbuf*/
	rp = rhsbuf;
	while (lc < Loc1)
		genbuf[sc++] = linebuf[lc++];
	while((c = *rp++) != 0) {
		if (c == '&') {
			sc = place(sc, Loc1, Loc2);
			continue;
		} else if (c == '\\') {
			c = *rp++;
			if (c >= '1' && c < NBRA+'1') {
				sc = place(sc, Braslist[c-'1'],
						Braelist[c-'1']);
				continue;
			}
		}
		if (sc >= gbend)
			growsp("output line too long.");
		genbuf[sc++] = (char)c;
	}
	lc = Loc2;
	Loc2 = sc;
	do {
		if (sc >= gbend)
			growsp("Output line too long.");
	} while (genbuf[sc++] = linebuf[lc++], lc <= spend);
	genend = sc-1;
	lc = 0;	/*linebuf*/
	sc = 0;	/*genbuf*/
	while (linebuf[lc++] = genbuf[sc++], sc <= genend);
	spend = lc-1;
}

static int
place(int asc, int al1, int al2)
{
	register int sc;
	register int l1, l2;

	sc = asc;
	l1 = al1;
	l2 = al2;
	while (l1 < l2) {
		if (sc >= gbend)
			growsp("Output line too long.");
		genbuf[sc++] = linebuf[l1++];
	}
	return(sc);
}

static void
command(struct reptr *ipc)
{
	register int	i;
	wint_t	c;
	register char	*p1, *p2;
	int	k1, k2, k3;
	char	*lp;
	int	execc;


	switch(ipc->command) {

		case ACOM:
			*A(aptr) = ipc;
			aptr_inc();
			*A(aptr) = 0;
			break;

		case CCOM:
			delflag = 1;
			if(!ipc->inar || dolflag) {
				for(p1 = ipc->bptr.re1; *p1; ) {
					putc(*p1&0377, stdout);
					p1++;
				}
				putc('\n', stdout);
			}
			break;
		case DCOM:
			delflag++;
			break;
		case CDCOM:
			p1 = p2 = linebuf;

			while(*p1 != '\n') {
				if(p1++ == &linebuf[spend]) {
					delflag++;
					return;
				}
			}

			p1++;
			while(*p2++ = *p1++, p1 <= &linebuf[spend]);
			spend = p2-1 - linebuf;
			jflag++;
			break;

		case EQCOM:
			fprintf(stdout, "%lld\n", lnum);
			break;

		case GCOM:
			p1 = linebuf;
			p2 = holdsp;
			while(*p1++ = *p2++, p2 <= &holdsp[hspend]);
			spend = p1-1 - linebuf;
			break;

		case CGCOM:
			linebuf[spend++] = '\n';
			k1 = spend;
			k2 = 0;	/*holdsp*/
			do {
				if(k1 >= lbend)
					growsp(NULL);
			} while(linebuf[k1++] = holdsp[k2++], k2 <= hspend);
			spend = k1-1;
			break;

		case HCOM:
			p1 = holdsp;
			p2 = linebuf;
			while(*p1++ = *p2++, p2 <= &linebuf[spend]);
			hspend = p1-1 - holdsp;
			break;

		case CHCOM:
			holdsp[hspend++] = '\n';
			k1 = hspend;
			k2 = 0;	/*linebuf*/
			do {
				if(k1 >= hend)
					growsp("\1hold space overflow !");
			} while(holdsp[k1++] = linebuf[k2++], k2 <= spend);
			hspend = k1-1;
			break;

		case ICOM:
			for(p1 = ipc->bptr.re1; *p1; ) {
				putc(*p1&0377, stdout);
				p1++;
			}
			putc('\n', stdout);
			break;

		case BCOM:
			jflag = 1;
			break;

		case LCOM:
			lp = linebuf;
			lcomlen = 0;
			while (lp < &linebuf[spend]) {
				c = fetch(&lp);
				lcom(c, invchar == 0);
			}
#if defined (SUS) || defined (SU3)
			putc('$', stdout);
#endif	/* SUS, SU3 */
			putc('\n', stdout);
			break;

		case NCOM:
			if(!nflag) {
				for(p1 = linebuf; p1 < &linebuf[spend]; p1++)
					putc(*p1&0377, stdout);
				putc('\n', stdout);
			}

			if(A(aptr) > abuf)
				arout();
			if((execc = gline(0)) < 0) {
				pending = ipc;
				delflag = 1;
				break;
			}
			spend = execc;

			break;
		case CNCOM:
			if(A(aptr) > abuf)
				arout();
			linebuf[spend++] = '\n';
			if((execc = gline(spend)) < 0) {
				pending = ipc;
				delflag = 1;
				break;
			}
			spend = execc;
			break;

		case PCOM:
			for(p1 = linebuf; p1 < &linebuf[spend]; p1++)
				putc(*p1&0377, stdout);
			putc('\n', stdout);
			break;
		case CPCOM:
	cpcom:
			for(p1 = linebuf; *p1 != '\n' && p1<&linebuf[spend]; ) {
				putc(*p1&0377, stdout);
				p1++;
			}
			putc('\n', stdout);
			break;

		case QCOM:
			if(!nflag) {
				for(p1 = linebuf; p1 < &linebuf[spend]; p1++)
					putc(*p1&0377, stdout);
				putc('\n', stdout);
			}
			if(A(aptr) > abuf)	arout();
			fclose(stdout);
			if (ibrd > 0)
				lseek(f, -ibrd, SEEK_CUR);
			exit(0);
		case RCOM:

			*A(aptr) = ipc;
			aptr_inc();
			*A(aptr) = 0;

			break;

		case SCOM:
			i = substitute(ipc);
			if(ipc->pfl && i)
				if(ipc->pfl == 1) {
					for(p1 = linebuf; p1 < &linebuf[spend];
							p1++)
						putc(*p1&0377, stdout);
					putc('\n', stdout);
				}
				else
					goto cpcom;
			if(i && ipc->fcode)
				goto wcom;
			break;

		case TCOM:
			if(sflag == 0)	break;
			sflag = 0;
			jflag = 1;
			break;

		wcom:
		case WCOM:
			fprintf(ipc->fcode, "%s\n", linebuf);
			break;
		case XCOM:
			p1 = linebuf;
			p2 = genbuf;
			while(*p2++ = *p1++, p1 <= &linebuf[spend]);
			genend = p2-1 - genbuf;
			p1 = holdsp;
			p2 = linebuf;
			while(*p2++ = *p1++, p1 <= &holdsp[hspend]);
			spend = p2-1 - linebuf;
			p1 = genbuf;
			p2 = holdsp;
			while(*p2++ = *p1++, p1 <= &genbuf[genend]);
			hspend = p2-1 - holdsp;
			break;

		case YCOM:
			if (multibyte) {
				struct yitem	**yt, *yp;

				yt = (struct yitem **)ipc->bptr.re1;
				k1 = 0;	/*linebuf*/
				k2 = 0;	/*genbuf*/
				do {
					k3 = k1;
					lp = &linebuf[k1];
					c = fetch(&lp);
					k1 = lp - linebuf;
					if (invchar == 0 &&
					    (yp = ylook(c, yt, 0)) != NULL) {
						k3 = 0;	/*yp->y_mc*/
						do {
							if (k2 >= gbend)
								growsp("output "
								"line too "
								"long.");
							genbuf[k2] =
								yp->y_mc[k3++];
						} while (genbuf[k2++] != '\0');
						k2--;
					} else {
						while (k3 < k1) {
							if (k2 >= gbend)
								growsp("output "
								"line too "
								"long.");
							genbuf[k2++] =
								linebuf[k3++];
						}
					}
				} while (k1 <= spend);
				genend = k2-1;
				p1 = linebuf;
				p2 = genbuf;
				while (*p1++ = *p2++, p2 <= &genbuf[genend]);
				spend = p1-1 - linebuf;
			} else {
				p1 = linebuf;
				p2 = ipc->bptr.re1;
				while((*p1 = p2[*p1 & 0377]) != 0)	p1++;
			}
			break;
		case COCOM:
		case ECOM:
		case FCOM:
		case CWCOM:
			;
	}

}

static int
gline(int addr)
{
	register char	*p2;
	register int	c;
	register int	c1;
	c1 = addr;
	p2 = cbp;
	for (;;) {
		if (p2 >= ebp) {
			if (f < 0 || (c = read(f, ibuf, sizeof ibuf)) <= 0) {
				if (c1 > addr && dolflag == 0) {
					c = 1;
					ibuf[0] = '\n';
					close(f);
					f = -1;
				} else
					return(-1);
			} else
				ibrd += c;
			p2 = ibuf;
			ebp = ibuf+c;
		}
		if ((c = *p2++ & 0377) == '\n') {
			ibrd--;
			if(needdol && p2 >=  ebp) {
				if(f<0||(c = read(f, ibuf, sizeof ibuf)) <= 0) {
					close(f);
					f = -1;
					if(eargc == 0)
							dolflag = 1;
				} else
					ibrd += c;

				p2 = ibuf;
				ebp = ibuf + c;
			}
			break;
		}
		if(c1 >= lbend)
			growsp(NULL);
		linebuf[c1++] = (char)c;
		ibrd--;
	}
	lnum++;
	if(c1 >= lbend)
		growsp(NULL);
	linebuf[c1] = 0;
	cbp = p2;

	sflag = 0;
	return(c1);
}

static void
arout(void)
{
	register char	*p1;
	struct reptr **a;
	FILE	*fi;
	char	c;
	int	t;

	for (a = abuf; *a; a++) {
		if((*a)->command == ACOM) {
			for(p1 = (*a)->bptr.re1; *p1; ) {
				putc(*p1&0377, stdout);
				p1++;
			}
			putc('\n', stdout);
		} else {
			if((fi = fopen((*a)->bptr.re1, "r")) == NULL)
				continue;
			while((t = getc(fi)) != EOF) {
				c = t;
				putc(c&0377, stdout);
			}
			fclose(fi);
		}
	}
	aptr = 1;
	*A(aptr) = 0;
}

static void
lcom(wint_t c, int valid)
{
	if (!valid) {
		oout(c);
		return;
	}
#if defined (SUS) || defined (SU3)
	switch (c) {
	case '\\':
		mout("\\\\");
		return;
	case '\a':
		mout("\\a");
		return;
	case '\b':
		mout("\\b");
		return;
	case '\f':
		mout("\\f");
		return;
	case '\r':
		mout("\\r");
		return;
	case '\t':
		mout("\\t");
		return;
	case '\v':
		mout("\\v");
		return;
	}
#else	/* !SUS, !SU3 */
	if (c < 040) {
		mout(trans[c]);
		return;
	}
#endif	/* !SUS, !SU3 */
	if (multibyte) {
		if (iswprint(c))
			wout(c);
		else
			nout(c);
	} else {
		if (isprint(c))
			lout(c);
		else
			oout(c);
	}
}

static void
oout(int c)
{
	char lbuf[5], *p;
	int d;
	const char *nums = "01234567";

	p = lbuf;
	*p++ = '\\';
	*p++ = nums[(c & ~077) >> 6];
	c &= 077;
	d = c & 07;
	*p++ =  c > d ?  nums[(c-d)>>3] : nums[0];
	*p++ = nums[d];
	*p = '\0';
	mout(lbuf);
}

static void
mout(const char *p)
{
	while (*p != '\0') {
		lout(*p & 0377);
		p++;
	}
}

static void
nout(wint_t c)
{
	char	mb[MB_LEN_MAX+1];
	char	*p;
	int	i;

	if ((i = wctomb(mb, c)) > 0) {
		mb[i] = '\0';
		for (p = mb; *p; p++)
			oout(*p & 0377);
	}
}

static void
lout(int c)
{
	if (lcomlen++ > 70) {
		putc('\\', stdout);
		putc('\n', stdout);
		lcomlen = 1;
	}
	putc(c, stdout);
}

static void
wout(wint_t c)
{
	char	mb[MB_LEN_MAX+1], *p;
	int	i, w;

	if ((i = wctomb(mb, c)) > 0) {
		w = wcwidth(c);
		if (lcomlen + w > 70) {
			putc('\\', stdout);
			putc('\n', stdout);
			lcomlen = 0;
		}
		mb[i] = '\0';
		for (p = mb; *p; p++)
			putc(*p & 0377, stdout);
		lcomlen += w;
	}
}
