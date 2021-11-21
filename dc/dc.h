/*	from Unix 7th Edition /usr/src/cmd/dc/dc.h	*/
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

/*	Sccsid @(#)dc.h	1.9 (gritter) 2/4/05>	*/

#include	<stdlib.h>
#include	<signal.h>

#define FATAL 0
#define NFATAL 1
#define BLK sizeof(struct blk)
#define PTRSZ sizeof(int *)
#define HEADSZ 1024
#define STKSZ 100
#define RDSKSZ 100
#define TBLSZ 256
#define ARRAYST 0241
#define NL 1
#define NG 2
#define NE 3
#define length(p) ((p)->wt-(p)->beg)
#define rewind(p) (p)->rd=(p)->beg
#define create(p)	(p)->rd = (p)->wt = (p)->beg
#define fsfile(p)	(p)->rd = (p)->wt
#define truncate(p)	(p)->wt = (p)->rd
#define sfeof(p)	(((p)->rd>=(p)->wt)?1:0)
#define sfbeg(p)	(((p)->rd==(p)->beg)?1:0)
#define sungetc(p,c)	*(--(p)->rd)=c
#ifdef interdata
#define NEGBYTE 0200
#define MASK (-1 & ~0377)
#define sgetc(p)	( ((p)->rd==(p)->wt) ? EOF :( ((*(p)->rd & NEGBYTE) != 0) ? ( *(p)->rd++ | MASK): *(p)->rd++ ))
#define slookc(p)	( ((p)->rd==(p)->wt) ? EOF :( ((*(p)->rd & NEGBYTE) != 0) ? (*(p)->rd | MASK) : *(p)->rd ))
#define sbackc(p)	( ((p)->rd==(p)->beg) ? EOF :( ((*(--(p)->rd) & NEGBYTE) != 0) ? (*(p)->rd | MASK): *(p)->rd ))
#endif
#ifndef interdata
#define sgetc(p)	(((p)->rd==(p)->wt)?EOF:*(p)->rd++)
#define slookc(p)	(((p)->rd==(p)->wt)?EOF:*(p)->rd)
#define sbackc(p)	(((p)->rd==(p)->beg)?EOF:*(--(p)->rd))
#endif
#define sputc(p,c)	{if((p)->wt==(p)->last)more(p); *(p)->wt++ = c; }
#define salterc(p,c)	{if((p)->rd==(p)->last)more(p); *(p)->rd++ = c; if((p)->rd>(p)->wt)(p)->wt=(p)->rd;}
#define sunputc(p)	(*( (p)->rd = --(p)->wt))
#define zero(p)	for(pp=(p)->beg;pp<(p)->last;)*pp++='\0'
#define OUTC(x) {int _c = (x); if (_c) {printf("%c",_c); if(--count == 0){printf("\\\n"); count=ll;} } }
#define TEST2(b)	{ OUTC(b[0] & 0377); OUTC(b[1] & 0377); }
#define EMPTY if(stkerr != 0){printf("stack empty\n"); continue; }
#define EMPTYR(x) if(stkerr!=0){pushp(x);printf("stack empty\n");continue;}
#define EMPTYS if(stkerr != 0){printf("stack empty\n"); return(1);}
#define EMPTYSR(x) if(stkerr !=0){printf("stack empty\n");pushp(x);return(1);}
#define error(p)	{printf(p); continue; }
#define errorrt(p)	{printf(p); return(1); }
struct blk {
	char	*rd;
	char	*wt;
	char	*beg;
	char	*last;
};
struct	blk *hfree;
struct	blk *arg1, *arg2;
int	svargc;
char	savk;
char	**svargv;
int	dbg;
int	ifile;
FILE	*curfile;
struct	blk *scalptr, *basptr, *tenptr, *inbas;
struct	blk *sqtemp, *chptr, *strptr, *divxyz;
struct	blk *stack[STKSZ];
struct	blk **stkptr,**stkbeg;
struct	blk **stkend;
int	stkerr;
int	lastchar;
struct	blk *readstk[RDSKSZ];
struct	blk **readptr;
struct	blk *rem;
int	k;
struct	blk *irem;
int	skd,skr;
int	neg;
struct	sym {
	struct	sym *next;
	struct	blk *val;
} symlst[TBLSZ];
struct	sym *stable[TBLSZ];
struct	sym *sptr,*sfree;
struct	wblk {
	struct blk **rdw;
	struct blk **wtw;
	struct blk **begw;
	struct blk **lastw;
};
FILE	*fsave;
long	rel;
long	nbytes;
long	all;
long	headmor;
long	obase;
int	fw,fw1,ll;
int	(*outdit)(struct blk *, int, int);
int	logo;
int	log_10;
int	count;
char	*pp;
char	*dummy;

#define	div(a, b)	dcdiv(a, b)
#define	sqrt(a)		dcsqrt(a)
#define	exp(a, b)	dcexp(a, b)
#define	getwd(a)	dcgetwd(a)
extern void commnds(void);
extern struct blk *div(struct blk *, struct blk *);
extern int dscale(void);
extern struct blk *removr(struct blk *, int);
extern struct blk *sqrt(struct blk *);
extern struct blk *exp(struct blk *, struct blk *);
extern void init(int, char *[]);
extern void onintr(int);
extern void pushp(struct blk *);
extern struct blk *pop(void);
extern struct blk *readin(void);
extern struct blk *add0(struct blk *, int);
extern struct blk *mult(struct blk *, struct blk *);
extern void chsign(struct blk *);
extern int readc(void);
extern void unreadc(char);
extern void binop(char);
extern void print(struct blk *);
extern struct blk *getdec(struct blk *, int);
extern void tenot(struct blk *, int);
extern void oneot(struct blk *, int, char);
extern void hexot(struct blk *, int, int);
extern void bigot(struct blk *, int, int);
extern struct blk *add(struct blk *, struct blk *);
extern int eqk(void);
extern struct blk *removc(struct blk *, int);
extern struct blk *scalint(struct blk *);
extern struct blk *scale(struct blk *, int);
extern int subt(void);
extern int command(void);
extern int cond(char);
extern void load(void);
extern int log_2(long);
extern struct blk *salloc(int);
extern struct blk *morehd(void);
extern struct blk *copy(struct blk *, int);
extern void sdump(char *, struct blk *);
extern void seekc(struct blk *, int);
extern void salterwd(struct wblk *, struct blk *);
extern void more(struct blk *);
extern void ospace(char *);
extern void garbage(char *);
extern void redef(struct blk *);
extern void release(register struct blk *);
extern struct blk *getwd(struct blk *);
extern void putwd(struct blk *, struct blk *);
extern struct blk *lookwd(struct blk *);
extern char *nalloc(register char *, unsigned);
extern void *srealloc(void *, size_t);

#if defined (__GLIBC__) && defined (_IO_getc_unlocked)
#undef	getc
#define	getc(f)		_IO_getc_unlocked(f)
#endif

#ifndef	BC_BASE_MAX
#define	BC_BASE_MAX	99
#endif
#ifndef	BC_DIM_MAX
#define	BC_DIM_MAX	2048
#endif
