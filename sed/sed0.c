/*	from Unix 7th Edition sed	*/
/*	Sccsid @(#)sed0.c	1.64 (gritter) 3/12/05>	*/
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
#include <unistd.h>
#include <stdlib.h>
#include <locale.h>
#include <libgen.h>
#include <stdarg.h>
#include <wchar.h>
#include "sed.h"

int	ABUFSIZE;
struct reptr	**abuf;
int	aptr;
char	*genbuf;
int	gbend;
int	lbend;
int	hend;
char	*linebuf;
char	*holdsp;
int	nflag;
long long	*tlno;
char	*cp;

int	status;
int	multibyte;
int	invchar;
int	needdol;

int	eargc;

struct	reptr *ptrspace;
struct reptr	*pending;
char	*badp;

static const char	CGMES[]	= "\1command garbled: %s";
static const char	TMMES[]	= "Too much text: %s";
static const char	LTL[]	= "Label too long: %s";
static const char	LINTL[]	= "line too long";
static const char	AD0MES[]	= "No addresses allowed: %s";
static const char	AD1MES[]	= "Only one address allowed: %s";
static FILE	**fcode;
static FILE	*fin;
static char	*lastre;
static wchar_t	sed_seof;
static int	PTRSIZE;
static int	eflag;
static int	gflag;
static int	nlno;
static char	**fname;
static int	nfiles;
static int	rep;
static struct label  *ltab;
static int	lab;
static size_t	LABSIZE;
static int	labtab = 1;
static int	depth;
static char	**eargv;
static int	*cmpend;
static size_t	DEPTH;
static char	bad;
static char	compfl;
static char	*progname;
static char	*(*ycomp)(char **);
static int	executing;

static void fcomp(void);
static char *compsub(char **, char *);
static int rline(void);
static char *address(char **);
static int cmp(const char *, const char *);
static void text(char **);
static int search(struct label *);
static void dechain(void);
static char *ycomp_sb(char **);
static char *ycomp_mb(char **);
static void lab_inc(void);
static void rep_inc(void);
static void depth_check(void);
static void *srealloc(void *, size_t);
static void *scalloc(size_t, size_t);
static char *sed_compile(char **);
static void wfile(void);
static void morefiles(void);

static char	*null;
#define	check(p, buf, sz, incr, op) \
	if (&p[1] >= &(buf)[sz]) { \
		size_t ppos = p - buf; \
		size_t opos = op - buf; \
		buf = srealloc(buf, (sz += incr) * sizeof *(buf)); \
		p = &(buf)[ppos]; \
		if (op != NULL) \
			op = &(buf)[opos]; \
	}

int
main(int argc, char **argv)
{
	int c;
	const char optstr[] = "nf:e:g";

	sed = 1;
	progname = basename(argv[0]);
	eargc = argc;
	eargv = argv;

#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif	/* __GLIBC__ */
#if defined (SUS) || defined (SU3) || defined (S42)
	setlocale(LC_COLLATE, "");
#endif	/* SUS || SU3 || S42 */
	setlocale(LC_CTYPE, "");
	multibyte = MB_CUR_MAX > 1;
	ycomp = multibyte ? ycomp_mb : ycomp_sb;
	badp = &bad;
	aptr_inc();
	lab_inc();
	lab_inc();	/* 0 reserved for end-pointer -> labtab = 1 */
	growsp(NULL);
	rep_inc();
	pending = 0;
	depth = 0;
	morefiles();
	fcode[0] = stdout;
	nfiles = 1;
	morefiles();

	if(eargc == 1)
		exit(0);
	while ((c = getopt(eargc, eargv, optstr)) != EOF) {
		switch (c) {
		case 'n':
			nflag++;
			continue;

		case 'f':
			if((fin = fopen(optarg, "r")) == NULL)
				fatal("Cannot open pattern-file: %s", optarg);

			fcomp();
			fclose(fin);
			continue;

		case 'e':
			eflag++;
			fcomp();
			eflag = 0;
			continue;

		case 'g':
			gflag++;
			continue;

		default:
			exit(2);
		}
	}

	eargv += optind, eargc -= optind;


	if(compfl == 0 && *eargv) {
		optarg = *eargv++;
		eargc--;
		eflag++;
		fcomp();
		eflag = 0;
	}

	if(depth)
		fatal("Too many {'s");

	L(labtab)->address = rep;

	dechain();

/*	abort();	*/	/*DEBUG*/

	executing++;
	if(eargc <= 0)
		execute((char *)NULL);
	else while(--eargc >= 0) {
		execute(*eargv++);
	}
	fclose(stdout);
	return status;
}

static void
fcomp(void)
{

	register char	*op, *tp, *q;
	int	pt, pt1;
	int	lpt;

	compfl = 1;
	op = lastre;

	if(rline() < 0)	return;
	if(*linebuf == '#') {
		if(linebuf[1] == 'n')
			nflag = 1;
	}
	else {
		cp = linebuf;
		goto comploop;
	}

	for(;;) {
		if(rline() < 0)	break;

		cp = linebuf;

comploop:
/*	fprintf(stdout, "cp: %s\n", cp); */	/*DEBUG*/
		while(*cp == ' ' || *cp == '\t')	cp++;
		if(*cp == '\0' || *cp == '#')		continue;
		if(*cp == ';') {
			cp++;
			goto comploop;
		}

		q = address(&P(rep)->ad1);
		if(q == badp)
			fatal(CGMES, linebuf);

		if(q != 0 && q == P(rep)->ad1) {
			if(op)
				P(rep)->ad1 = op;
			else
				fatal("First RE may not be null");
		} else if(q == 0) {
			P(rep)->ad1 = 0;
		} else {
			op = P(rep)->ad1;
			if(*cp == ',' || *cp == ';') {
				cp++;
				q = address(&P(rep)->ad2);
				if(q == badp || q == 0)
					fatal(CGMES, linebuf);
				if(q == P(rep)->ad2)
					P(rep)->ad2 = op;
				else
					op = P(rep)->ad2;

			} else
				P(rep)->ad2 = 0;
		}

		while(*cp == ' ' || *cp == '\t')	cp++;

swit:
		switch(*cp++) {

			default:
				fatal("Unrecognized command: %s", linebuf);
				/*NOTREACHED*/

			case '!':
				P(rep)->negfl = 1;
				goto swit;

			case '{':
				P(rep)->command = BCOM;
				P(rep)->negfl = !(P(rep)->negfl);
				depth_check();
				cmpend[depth++] = rep;
				rep_inc();
				if(*cp == '\0')	continue;

				goto comploop;

			case '}':
				if(P(rep)->ad1)
					fatal(AD0MES, linebuf);

				if(--depth < 0)
					fatal("Too many }'s");
				P(cmpend[depth])->bptr.lb1 = rep;

				continue;

			case '=':
				P(rep)->command = EQCOM;
				if(P(rep)->ad2)
					fatal(AD1MES, linebuf);
				break;

			case ':':
				if(P(rep)->ad1)
					fatal(AD0MES, linebuf);

				while(*cp++ == ' ');
				cp--;


				tp = L(lab)->asc;
				while((*tp++ = *cp++))
					if(tp >= &(L(lab)->asc[sizeof
								L(lab)->asc]))
						fatal(LTL, linebuf);
				*--tp = '\0';

				if(lpt = search(L(lab))) {
					if(L(lpt)->address)
						fatal("Duplicate labels: %s",
								linebuf);
				} else {
					L(lab)->chain = 0;
					lpt = lab;
					lab_inc();
				}
				L(lpt)->address = rep;

				continue;

			case 'a':
				P(rep)->command = ACOM;
				if(P(rep)->ad2)
					fatal(AD1MES, linebuf);
				if(*cp == '\\')	cp++;
				if(*cp++ != '\n')
					fatal(CGMES, linebuf);
				text(&P(rep)->bptr.re1);
				break;
			case 'c':
				P(rep)->command = CCOM;
				if(*cp == '\\')	cp++;
				if(*cp++ != ('\n'))
					fatal(CGMES, linebuf);
				text(&P(rep)->bptr.re1);
				needdol = 1;
				break;
			case 'i':
				P(rep)->command = ICOM;
				if(P(rep)->ad2)
					fatal(AD1MES, linebuf);
				if(*cp == '\\')	cp++;
				if(*cp++ != ('\n'))
					fatal(CGMES, linebuf);
				text(&P(rep)->bptr.re1);
				break;

			case 'g':
				P(rep)->command = GCOM;
				break;

			case 'G':
				P(rep)->command = CGCOM;
				break;

			case 'h':
				P(rep)->command = HCOM;
				break;

			case 'H':
				P(rep)->command = CHCOM;
				break;

			case 't':
				P(rep)->command = TCOM;
				goto jtcommon;

			case 'b':
				P(rep)->command = BCOM;
jtcommon:
				while(*cp++ == ' ');
				cp--;

				if(*cp == '\0') {
					if((pt = L(labtab)->chain) != 0) {
						while((pt1 = P(pt)->bptr.lb1) != 0)
							pt = pt1;
						P(pt)->bptr.lb1 = rep;
					} else
						L(labtab)->chain = rep;
					break;
				}
				tp = L(lab)->asc;
				while((*tp++ = *cp++))
					if(tp >= &(L(lab)->asc[sizeof
								L(lab)->asc]))
						fatal(LTL, linebuf);
				cp--;
				*--tp = '\0';

				if(lpt = search(L(lab))) {
					if(L(lpt)->address) {
						P(rep)->bptr.lb1 = L(lpt)->address;
					} else {
						pt = L(lpt)->chain;
						while((pt1 = P(pt)->bptr.lb1) != 0)
							pt = pt1;
						P(pt)->bptr.lb1 = rep;
					}
				} else {
					L(lab)->chain = rep;
					L(lab)->address = 0;
					lab_inc();
				}
				break;

			case 'n':
				P(rep)->command = NCOM;
				break;

			case 'N':
				P(rep)->command = CNCOM;
				break;

			case 'p':
				P(rep)->command = PCOM;
				break;

			case 'P':
				P(rep)->command = CPCOM;
				break;

			case 'r':
				P(rep)->command = RCOM;
				if(P(rep)->ad2)
					fatal(AD1MES, linebuf);
#if !defined (SUS) && !defined (SU3)
				if(*cp++ != ' ')
					fatal(CGMES, linebuf);
#else	/* SUS, SU3 */
				while (*cp == ' ' || *cp == '\t')
					cp++;
#endif	/* SUS, SU3 */
				text(&P(rep)->bptr.re1);
				break;

			case 'd':
				P(rep)->command = DCOM;
				break;

			case 'D':
				P(rep)->command = CDCOM;
				P(rep)->bptr.lb1 = 1;
				break;

			case 'q':
				P(rep)->command = QCOM;
				if(P(rep)->ad2)
					fatal(AD1MES, linebuf);
				break;

			case 'l':
				P(rep)->command = LCOM;
				break;

			case 's':
				P(rep)->command = SCOM;
				sed_seof = fetch(&cp);
				q = sed_compile(&P(rep)->bptr.re1);
				if(q == badp)
					fatal(CGMES, linebuf);
				if(q == P(rep)->bptr.re1) {
					if (op == NULL)
						fatal("First RE may not be null");
					P(rep)->bptr.re1 = op;
				} else {
					op = P(rep)->bptr.re1;
				}

				if(compsub(&P(rep)->rhs, &P(rep)->nsub) == badp)
					fatal(CGMES, linebuf);
			sloop:	if(*cp == 'g') {
					cp++;
					P(rep)->gfl = -1;
					goto sloop;
				} else if(gflag)
					P(rep)->gfl = -1;
				if (*cp >= '0' && *cp <= '9') {
					while (*cp >= '0' && *cp <= '9') {
						if (P(rep)->gfl == -1)
							P(rep)->gfl = 0;
						P(rep)->gfl = P(rep)->gfl * 10
							+ *cp++ - '0';
					}
					goto sloop;
				}
#if !defined (SUS) && !defined (SU3)
				if (P(rep)->gfl > 0 && P(rep)->gfl > 512)
			fatal("Suffix too large - 512 max: %s", linebuf);
#endif

				if(*cp == 'p') {
					cp++;
					P(rep)->pfl = 1;
					goto sloop;
				}

				if(*cp == 'P') {
					cp++;
					P(rep)->pfl = 2;
					goto sloop;
				}

				if(*cp == 'w') {
					cp++;
					wfile();
				}
				break;

			case 'w':
				P(rep)->command = WCOM;
				wfile();
				break;

			case 'x':
				P(rep)->command = XCOM;
				break;

			case 'y':
				P(rep)->command = YCOM;
				sed_seof = fetch(&cp);
				if (ycomp(&P(rep)->bptr.re1) == badp)
					fatal(CGMES, linebuf);
				break;

		}
		rep_inc();

		if(*cp++ != '\0') {
			if(cp[-1] == ';')
				goto comploop;
			fatal(CGMES, linebuf);
		}

	}
	P(rep)->command = 0;
	lastre = op;
}

static char	*
compsub(char **rhsbuf, char *nsubp)
{
	register char	*p, *op, *oq;
	char	*q;
	wint_t	c;
	size_t	sz = 32;

	*rhsbuf = smalloc(sz);
	p = *rhsbuf;
	q = cp;
	*nsubp = 0;
	for(;;) {
		op = p;
		oq = q;
		if((c = fetch(&q)) == '\\') {
			check(p, *rhsbuf, sz, 32, op)
			*p = '\\';
			oq = q;
			c = fetch(&q);
			do {
				check(p, *rhsbuf, sz, 32, op)
				*++p = *oq++;
			} while (oq < q);
			if(c > nbra + '0' && c <= '9')
				return(badp);
			if (c > *nsubp + '0' && c <= '9')
				*nsubp = c - '0';
			check(p, *rhsbuf, sz, 32, op)
			p++;
			continue;
		} else {
			do {
				check(p, *rhsbuf, sz, 32, op)
				*p++ = *oq++;
			} while (oq < q);
			p--;
		}
		if(c == sed_seof) {
			check(p, *rhsbuf, sz, 32, op)
			*op++ = '\0';
			cp = q;
			return(op);
		}
		check(p, *rhsbuf, sz, 32, op)
		if(*p++ == '\0') {
			return(badp);
		}

	}
}

#define	rlinechk()	if (c >= lbend-2) \
				growsp(LINTL)

static int
rline(void)
{
	register char	*q;
	register int	c;
	register int	t;
	static char	*saveq;

	c = -1;

	if(eflag) {
		if(eflag > 0) {
			eflag = -1;
			q = optarg;
			rlinechk();
			while(linebuf[++c] = *q++) {
				rlinechk();
				if(linebuf[c] == '\\') {
					if((linebuf[++c] = *q++) == '\0') {
						rlinechk();
						saveq = 0;
						return(-1);
					} else
						continue;
				}
				if(linebuf[c] == '\n') {
					linebuf[c] = '\0';
					saveq = q;
					return(1);
				}
			}
			saveq = 0;
			return(1);
		}
		if((q = saveq) == 0)	return(-1);

		while(linebuf[++c] = *q++) {
			rlinechk();
			if(linebuf[c] == '\\') {
				if((linebuf[++c] = *q++) == '0') {
					rlinechk();
					saveq = 0;
					return(-1);
				} else
					continue;
			}
			if(linebuf[c] == '\n') {
				linebuf[c] = '\0';
				saveq = q;
				return(1);
			}
		}
		saveq = 0;
		return(1);
	}

	while((t = getc(fin)) != EOF) {
		rlinechk();
		linebuf[++c] = (char)t;
		if(linebuf[c] == '\\') {
			t = getc(fin);
			rlinechk();
			linebuf[++c] = (char)t;
		}
		else if(linebuf[c] == '\n') {
			linebuf[c] = '\0';
			return(1);
		}
	}
	linebuf[++c] = '\0';
	return(-1);
}

static char	*
address(char **expbuf)
{
	register char	*rcp, *ep;
	long long	lno;

	*expbuf = NULL;
	if(*cp == '$') {
		cp++;
		ep = *expbuf = smalloc(2 * sizeof *expbuf);
		*ep++ = CEND;
		*ep++ = ceof;
		needdol = 1;
		return(ep);
	}

	if(*cp == '/' || *cp == '\\') {
		if (*cp == '\\')
			cp++;
		sed_seof = fetch(&cp);
		return(sed_compile(expbuf));
	}

	rcp = cp;
	lno = 0;

	while(*rcp >= '0' && *rcp <= '9')
		lno = lno*10 + *rcp++ - '0';

	if(rcp > cp) {
		if (nlno > 020000000000 ||
		    (tlno = realloc(tlno, (nlno+1)*sizeof *tlno)) == NULL)
			fatal("Too many line numbers");
		ep = *expbuf = smalloc(6 * sizeof *expbuf);
		*ep++ = CLNUM;
		slno(ep, nlno);
		tlno[nlno++] = lno;
		*ep++ = ceof;
		cp = rcp;
		return(ep);
	}
	return(0);
}

static int
cmp(const char *a, const char *b)
{
	register const char	*ra, *rb;

	ra = a - 1;
	rb = b - 1;

	while(*++ra == *++rb)
		if(*ra == '\0')	return(0);
	return(1);
}

static void
text(char **textbuf)
{
	register char	*p, *oq;
	char *q;
	size_t sz = 128;

	*textbuf = smalloc(sz);
	p = *textbuf;
	q = cp;
	for(;;) {

		oq = q;
		if(fetch(&q) == '\\') {
			oq = q;
			fetch(&q);
		}
		while(oq < q)
			*p++ = *oq++;
		if(p[-1] == '\0') {
			cp = --q;
			return;
		}
		check(p, *textbuf, sz, 128, null)
	}
}

static int
search(struct label *ptr)
{
	struct label	*rp;

	rp = L(labtab);
	while(rp < ptr) {
		if(cmp(rp->asc, ptr->asc) == 0)
			return(rp - L(labtab) + 1);
		rp++;
	}

	return(0);
}


static void
dechain(void)
{
	struct label	*lptr;
	int	rptr, trptr;

	for(lptr = L(labtab); lptr < L(lab); lptr++) {

		if(lptr->address == 0)
			fatal("Undefined label: %s", lptr->asc);

		if(lptr->chain) {
			rptr = lptr->chain;
			while((trptr = P(rptr)->bptr.lb1) != 0) {
				P(rptr)->bptr.lb1 = lptr->address;
				rptr = trptr;
			}
			P(rptr)->bptr.lb1 = lptr->address;
		}
	}
}

static char *
ycomp_sb(char **expbuf)
{
	register int	c, d;
	register char	*ep, *tsp;
	char	*sp;

	*expbuf = smalloc(0400);
	ep = *expbuf;
	for(c = 0; !(c & 0400); c++)
		ep[c] = '\0';
	sp = cp;
	for(tsp = cp; *tsp != sed_seof; tsp++) {
		if(*tsp == '\\')
			tsp++;
		if(*tsp == '\n' || *tsp == '\0')
			return(badp);
	}
	tsp++;

	while((c = *sp++ & 0377) != sed_seof) {
		if(c == '\\') {
			c = *sp == 'n' ? '\n' : *sp;
			sp++;
		}
		if((ep[c] = d = *tsp++ & 0377) == '\\') {
			ep[c] = *tsp == 'n' ? '\n' : *tsp;
			tsp++;
		}
		if(d != '\\' && ep[c] == sed_seof || ep[c] == '\0')
			return(badp);
	}
	if(*tsp != sed_seof)
		return(badp);
	cp = ++tsp;

	for(c = 0; !(c & 0400); c++)
		if(ep[c] == 0)
			ep[c] = (char)c;

	return(ep + 0400);
}

static char *
ycomp_mb(char **expbuf)
{
	struct yitem	**yt, *yp;
	register wint_t	c, d;
	char	*otsp, *tsp, *sp, *mp;

	tsp = sp = cp;
	while ((c = fetch(&tsp)) != sed_seof) {
		if (c == '\\')
			c = fetch(&tsp);
		if (c == '\n' || c == '\0')
			return badp;
	}
	yt = scalloc(200, sizeof *yt);
	while ((c = fetch(&sp)) != sed_seof) {
		if (c == '\\') {
			if ((d = fetch(&sp)) == 'n')
				c = '\n';
			else
				c = d;
		}
		otsp = tsp;
		d = fetch(&tsp);
		yp = ylook(c, yt, 1);
		yp->y_oc = c;
		if ((yp->y_yc = d) == '\\') {
			otsp = tsp;
			if ((c = fetch(&tsp)) == 'n')
				yp->y_yc = '\n';
			else
				yp->y_yc = c;
		}
		if (d != '\\' && yp->y_yc == sed_seof || yp->y_yc == '\0')
			return badp;
		mp = yp->y_mc;
		if (yp->y_yc != '\n')
			while (otsp < tsp)
				*mp++ = *otsp++;
		else
			*mp++ = '\n';
		*mp = '\0';
	}
	if (fetch(&tsp) != sed_seof)
		return badp;
	cp = tsp;
	*expbuf = (char *)yt;
	return &(*expbuf)[1];
}

static void
rep_inc(void)
{
	register char	*p;
	const int	chunk = 16;

	if (++rep >= PTRSIZE) {
		ptrspace = srealloc(ptrspace,
				(PTRSIZE += chunk) * sizeof *ptrspace);
		for (p = (char *)&ptrspace[PTRSIZE - chunk];
				p < (char *)&ptrspace[PTRSIZE]; p++)
			*p = '\0';
	}
}

static void
lab_inc(void)
{
	register char	*p;
	const int	chunk = 8;

	if (++lab >= LABSIZE) {
		ltab = srealloc(ltab, (LABSIZE += chunk) * sizeof *ltab);
		for (p = (char *)&ltab[LABSIZE - chunk];
				p < (char *)&ltab[LABSIZE]; p++)
			*p = '\0';
	}
}

void
aptr_inc(void)
{
	register char	*p;
	const int	chunk = 8;

	if (++aptr > ABUFSIZE) {
		abuf = srealloc(abuf, (ABUFSIZE += chunk) * sizeof *abuf);
		for (p = (char *)&abuf[ABUFSIZE - chunk];
				p < (char *)&abuf[ABUFSIZE]; p++)
			*p = '\0';
	}
}

static void
depth_check(void)
{
	if (depth + 1 > DEPTH)
		cmpend = srealloc(cmpend, (DEPTH += 8) * sizeof *cmpend);
}

void
nonfatal(const char *afmt, ...)
{
	va_list ap;
	const char	*fmt;

	if (*afmt == '\1') {
		fprintf(stderr, "%s: ", progname);
		fmt = &afmt[1];
	} else
		fmt = afmt;
	va_start(ap, afmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
	status |= 1;
}

void
fatal(const char *afmt, ...)
{
	va_list ap;
	const char	*fmt;

	if (*afmt == '\1') {
		fprintf(stderr, "%s: ", progname);
		fmt = &afmt[1];
	} else
		fmt = afmt;
	va_start(ap, afmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
	exit(2);
}

static void *
srealloc(void *vp, size_t nbytes)
{
	void *p;

	if ((p = realloc(vp, nbytes)) == NULL)
		fatal(TMMES, linebuf);
	return p;
}

void *
smalloc(size_t nbytes)
{
	return srealloc(NULL, nbytes);
}

static void *
scalloc(size_t nmemb, size_t size)
{
	void *p;

	if ((p = calloc(nmemb, size)) == NULL)
		fatal(TMMES, linebuf);
	return p;
}

#if defined (SUS) || defined (SU3) || defined (S42)
static char *
sed_compile(char **ep)
{
	struct re_emu	*re;
	static char	*pat;
	static size_t	patsz;
	register char	*p, *oc;
	wint_t	c, d;

	if (*cp != sed_seof)
		nbra = 0;
	if (patsz == 0)
		pat = smalloc(patsz = 32);
	p = pat;
	do {
		oc = cp;
		if ((c = fetch(&cp)) == sed_seof)
			*p = '\0';
		else if (c == '\\') {
			oc = cp;
			if ((c = fetch(&cp)) == 'n')
				*p = '\n';
			else {
				check(p, pat, patsz, 32, null);
				*p++ = '\\';
				if (c == '(')
					nbra++;
				goto normchar;
			}
		} else if (c == '[') {
			check(p, pat, patsz, 32, null);
			*p++ = c;
			d = WEOF;
			do {
				oc = cp;
				c = fetch(&cp);
				if (c == '\0')
					goto normchar;
				do {
					check(p, pat, patsz, 32, null);
					*p++ = *oc++;
				} while (oc < cp);
				if (d == '[' && (c == ':' || c == '.' ||
							c == '=')) {
					d = c;
					do {
						oc = cp;
						c = fetch(&cp);
						if (c == '\0')
							goto normchar;
						do {
							check(p, pat, patsz,32,
									null);
							*p++ = *oc++;
						} while (oc < cp);
					} while (c != d || peek(&cp) != ']');
					oc = cp;
					c = fetch(&cp);
					do {
						check(p, pat, patsz, 32, null);
						*p++ = *oc++;
					} while (oc < cp);
					c = WEOF; /* == reset d and continue */
				}
				d = c;
			} while (c != ']');
			p--;
		} else {
	normchar:	do {
				check(p, pat, patsz, 32, null)
				*p++ = *oc++;
			} while (oc < cp);
			p--;
		}
		check(p, pat, patsz, 32, null);
	} while (*p++ != '\0');
	re = scalloc(1, sizeof *re);
	*ep = (char *)re;
	if (*pat == '^')
		**ep = 1;
	if (*pat != '\0') {
		int reflags = 0;

#ifdef	REG_ANGLES
		reflags |= REG_ANGLES;
#endif	/* REG_ANGLES */
#if defined (SU3) && defined (REG_AVOIDNULL)
		reflags |= REG_AVOIDNULL;
#endif	/* SU3 && AVOIDNULL */
		if (regcomp(&re->r_preg, pat, reflags) != 0)
			re = (struct re_emu *)badp;
	} else
		**ep = 2;
	p = (char *)re;
	if (p != badp && *pat)
		p++;
	return p;
}
#else	/* !SUS, !SU3, !S42 */
static char *
sed_compile(char **ep)
{
	extern char *compile(char *, char *, char *, int);
	register char *p;
	size_t sz;

	for (sz = 0, p = cp; *p; p++)
		if (*p == '[')
			sz += 32;
	sz += 2 * (p - cp) + 5;
	*ep = smalloc(sz);
	(*ep)[1] = '\0';
	p = compile(NULL, &(*ep)[1], &(*ep)[sz], sed_seof);
	if (p == &(*ep)[1])
		return *ep;
	**ep = circf;
	return p;
}
#endif	/* !SUS, !SU3, !S42 */

wint_t
wc_get(char **sc, int move)
{
	wint_t	c;
	char	*p = *sc;
	wchar_t	wcbuf;
	int	len;

	if ((*p & 0200) == 0) {
		c = *p;
		p += (len = 1);
		invchar = 0;
	} else if ((len = mbtowc(&wcbuf, p, MB_LEN_MAX)) < 0) {
		if (!executing)
			fatal("invalid multibyte character: %s", p);
		c = (*p++ & 0377);
		mbtowc(NULL, NULL, 0);
		invchar = 1;
	} else if (len == 0) {
		c = '\0';
		p++;
		invchar = 0;
	} else {
		c = wcbuf;
		p += len;
		invchar = 0;
	}
	if (move)
		*sc = p;
	return c;
}

/*
 * Note that this hash is not optimized to distribute the items
 * equally to all buckets. y commands typically handle only a
 * small part of the alphabet, thus most characters will have
 * no entry in the hash table. If no list exists in the bucket
 * for the hash of these characters, the function can return
 * quickly.
 */
#define	yhash(c)	(c & 0177)

struct yitem *
ylook(wint_t c, struct yitem **yt, int make)
{
	struct yitem	*yp;
	int	h;

	yp = yt[h = yhash(c)];
	while (yp != NULL) {
		if (yp->y_oc == c)
			break;
		yp = yp->y_nxt;
	}
	if (make && yp == NULL) {
		yp = scalloc(1, sizeof *yp);
		yp->y_oc = c;
		yp->y_nxt = yt[h];
		yt[h] = yp;
	}
	return yp;
}

void
growsp(const char *msg)
{
	const int	incr = 128;
	int	olbend, ogbend, ohend;

	olbend = lbend;
	ogbend = gbend;
	ohend = hend;
	if ((linebuf = realloc(linebuf, lbend += incr)) == NULL ||
			(genbuf = realloc(genbuf, gbend += incr)) == NULL ||
			(holdsp = realloc(holdsp, hend += incr)) == NULL)
		fatal(msg ? msg : "Cannot malloc space");
	while (olbend < lbend)
		linebuf[olbend++] = '\0';
	while (ogbend < gbend)
		genbuf[ogbend++] = '\0';
	while (ohend < hend)
		holdsp[ohend++] = '\0';
}

static void
wfile(void)
{
	int	i;

#if !defined (SUS) && !defined (SU3)
	if(*cp++ != ' ')
		fatal(CGMES, linebuf);
#else	/* SUS, SU3 */
	while (*cp == ' ' || *cp == '\t')
		cp++;
#endif	/* SUS, SU3 */

	text(&fname[nfiles]);
	for(i = nfiles - 1; i >= 0; i--)
		if(fname[i] != NULL && cmp(fname[nfiles], fname[i]) == 0) {
			P(rep)->fcode = fcode[i];
			free(fname[nfiles]);
			return;
		}

	if((P(rep)->fcode = fopen(fname[nfiles], "w")) == NULL)
		fatal("Cannot create %s", fname[nfiles]);
	fcode[nfiles++] = P(rep)->fcode;
	morefiles();
}

static void
morefiles(void)
{
	if ((fname = realloc(fname, (nfiles+1) * sizeof *fname)) == 0 ||
	    (fcode = realloc(fcode, (nfiles+1) * sizeof *fcode)) == 0)
		fatal("Too many files in w commands");
	fname[nfiles] = 0;
	fcode[nfiles] = 0;
}
