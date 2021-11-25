/*
 * determine type of file
 */
/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, September 2003.
 */
/*	from Unix 7th Edition /usr/src/cmd/file.c	*/
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
static const char sccsid[] USED = "@(#)file_sus.sl	1.33 (gritter) 4/14/06";
#else	/* !SUS */
static const char sccsid[] USED = "@(#)file.sl	1.33 (gritter) 4/14/06";
#endif	/* !SUS */

#ifdef	__GLIBC__
#include <sys/sysmacros.h>
#endif
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <inttypes.h>
#ifndef	major
#include <sys/sysmacros.h>
#endif
#include "iblok.h"
#include "asciitype.h"
#ifndef	S_IFDOOR
#define	S_IFDOOR	0xD000		/* Solaris door */
#endif
#ifndef	S_IFNAM
#define	S_IFNAM		0x5000		/* XENIX special named file */
#endif
#ifndef	S_INSEM
#define	S_INSEM		0x1		/* XENIX semaphore subtype of IFNAM */
#endif
#ifndef	S_INSHD
#define	S_INSHD		0x2		/* XENIX shared data subtype of IFNAM */
#endif
#ifndef	S_IFNWK
#define	S_IFNWK		0x9000		/* HP-UX network special file */
#endif
static int in;
static int i  = 0;
static char buf[512];
static const char *const fort[] = {
	"function","subroutine","common","dimension","block","integer",
	"real","data","double",0};
static const char *const asc[] = {
	"sys","mov","tst","clr","jmp",0};
static const char *const c[] = {
	"int","char","float","double","short","long",
	"unsigned","register","static","struct","extern",0};
static const char *const as[] = {
	"globl","byte","even","text","data","bss","comm",0};
static int	ifile;
static int	status;
static const char	*progname;
extern int	sysv3;

static struct	match {
	int	m_offs;			/* offset of item */
	enum {
		M_STRING = 0x08,	/* string value */
		M_BYTE   = 0x00,	/* byte value */
		M_SHORT  = 0x02,	/* two-byte value */
		M_LONG   = 0x04,	/* four-byte value */
		M_BE     = 0x10,	/* big endian */
		M_LE     = 0x00		/* little endian */
	} m_type;
	enum {				/* printf string in m_print */
		MS_NONE  = 0x00,
		MS_SHORT = 0x01,	/* %hd */
		MS_INT   = 0x02,	/* %d */
		MS_LONG  = 0x04		/* %ld */
	} m_subst;
	char	m_oper;			/* operand ( = < > & ^ ) */
	char	m_level;		/* level (> in first column */
	union {
		char	*m_string;
		unsigned long	m_number;
	} m_value;			/* value to match */
	char	*m_print;		/* string to print */
	struct match	*m_sub;		/* next level 1 match */
	struct match	*m_next;	/* next level 0 match */
} *matches;

static int		cflag;
static const char	*fflag;
static int		(*statfn)(const char *, struct stat *) = stat;
static const char	*mflag = MAGIC;
static int		zflag;

static void	type(const char *);
static int	lookup(const char *const *);
static int	ccom(void);
static int	ascom(void);
static int	english(const char *, int);
static int	magic(void);
static int	gotcha(struct match *, const char *);
static void	getmagic(const char *);
static void	split(char *, struct match **, struct match **, int);
static char	*col1(char *, struct match *);
static char	*col2(char *, struct match *);
static char	*col3(char *, struct match *);
static char	*col4(char *, struct match *);
static void	symlnk(const char *, struct stat *);
static void	prfn(const char *);
static void	*scalloc(size_t, size_t);
static void	usage(void);
static void	prmagic(void);
static void	pritem(struct match *, int);
static void	fmterr(const char *, int);
static int	tar(void);
static int	sccs(void);
static int	utf8(const char *);
static int	endianess(void);
static int	redirect(const char *, const char *);

int
main(int argc, char **argv)
{
	const char optstring[] = "cf:hm:z";
	struct iblok *fl;
	char	*ap = NULL;
	size_t	size = 0, l;

	progname = basename(argv[0]);
	if (getenv("SYSV3") != NULL)
		sysv3 = 1;
	while ((i = getopt(argc, argv, optstring)) != EOF) {
		switch (i) {
		case 'c':
			cflag = 1;
			break;
		case 'f':
			fflag = optarg;
			break;
		case 'h':
			statfn = lstat;
			break;
		case 'm':
			mflag = optarg;
			break;
		case 'z':
			zflag = 1;
			break;
		default:
			status = 2;
		}
	}
	if (status)
		usage();
	getmagic(mflag);
	if (cflag) {
		prmagic();
		exit(status);
	}
	argc -= optind-1, argv += optind-1;
	if (fflag) {
		if ((fl = ib_open(fflag, 0)) == NULL) {
			fprintf(stderr, "cannot open %s\n", fflag);
			usage();
		}
		while ((l = ib_getlin(fl, &ap, &size, realloc)) != 0) {
			if (ap[l-1] == '\n')
				ap[--l] = '\0';
			prfn(ap);
			type(ap);
			if (ifile>=0)
				close(ifile);
		}
		ib_close(fl);
		free(ap);
	} else if (argc == 1)
		usage();
	while(argc > 1) {
		prfn(argv[1]);
		type(argv[1]);
		argc--;
		argv++;
		if (ifile >= 0)
			close(ifile);
	}
	return status;
}

static void
type(const char *file)
{
	int j,nl,zipped = 0;
	char ch;
	struct stat mbuf;

	ifile = -1;
	if(statfn(file, &mbuf) < 0) {
		if (sysv3) {
			printf("cannot open, %s\n", strerror(errno));
#ifndef	SUS
			status |= 1;
#endif	/* !SUS */
			return;
		}
		goto cant;
	}
	switch (mbuf.st_mode & S_IFMT) {

	case S_IFCHR:
		printf("character");
		goto spcl;

	case S_IFDIR:
		printf("directory\n");
		return;

	case S_IFIFO:
		printf("fifo\n");
		return;

	case S_IFSOCK:
		printf("socket\n");
		return;

	case S_IFDOOR:
		printf("door\n");
		return;

	case S_IFNWK:
		printf("network\n");
		return;

	case S_IFNAM:
		switch (mbuf.st_rdev) {
		case S_INSEM:
			printf("Xenix semaphore\n");
			break;
		case S_INSHD:
			printf("Xenix shared memory handle\n");
			break;
		default:
			printf("unknown Xenix name special file\n");
		}
		return;

	case S_IFLNK:
		symlnk(file, &mbuf);
		return;

	case S_IFBLK:
		printf("block");

spcl:
		printf(" special (%d/%d)\n",
				(int)major(mbuf.st_rdev),
				(int)minor(mbuf.st_rdev));
		return;
	}

	ifile = open(file, O_RDONLY);
	if(ifile < 0) {
	cant:	if (sysv3)
			printf("cannot open for reading\n");
		else
			printf("cannot open: %s\n", strerror(errno));
#ifndef	SUS
		status |= 1;
#endif	/* !SUS */
		return;
	}
rd:	in = read(ifile, buf, sizeof buf);
	if(in == 0){
		printf("empty\n");
		return;
	} else if (in < 0) {
		printf("cannot read: %s\n", strerror(errno));
		status |= 1;
		return;
	}
	if (zflag && !zipped) {
		zipped = 1;
		if (in > 3 && memcmp(buf, "BZh", 3) == 0 &&
				redirect("bzip2", "-cd"))
			goto rd;
		else if (in > 2 && memcmp(buf, "\37\235", 2) == 0 &&
				redirect("zcat", NULL))
			goto rd;
		else if (in > 2 && memcmp(buf, "\37\213", 2) == 0 &&
				redirect("gzip", "-cd"))
			goto rd;
	}
	if (magic() || tar() || sccs())
		return;
	i = 0;
	if(ccom() == 0)goto notc;
	if (i >= in)goto isc;
	while(buf[i] == '#'){
		j = i;
		while(buf[i++] != '\n'){
			if(i - j > 255){
				if (!utf8(NULL))
					printf("data\n"); 
				goto out;
			}
			if(i >= in)goto notc;
		}
		if(ccom() == 0)goto notc;
	}
check:
	if(lookup(c) == 1){
		while((ch = buf[i++]) != ';' && ch != '{')if(i >= in)goto notc;
isc:		printf("c program text");
		goto outa;
	}
	nl = 0;
	while(buf[i] != '('){
		if(buf[i] <= 0)
			goto notas;
		if(buf[i] == ';'){
			i++; 
			goto check; 
		}
		if(buf[i++] == '\n')
			if(nl++ > 6)goto notc;
		if(i >= in)goto notc;
	}
	while(buf[i] != ')'){
		if(buf[i++] == '\n')
			if(nl++ > 6)goto notc;
		if(i >= in)goto notc;
	}
	while(buf[i] != '{'){
		if(buf[i++] == '\n')
			if(nl++ > 6)goto notc;
		if(i >= in)goto notc;
	}
	printf("c program text");
	goto outa;
notc:
	i = 0;
	while(buf[i] == 'c' || buf[i] == '#'){
		while(buf[i++] != '\n')if(i >= in)goto notfort;
	}
	if(lookup(fort) == 1){
		printf("fortran program text");
		goto outa;
	}
notfort:
	i=0;
	if(ascom() == 0)goto notas;
	j = i-1;
	if(buf[i] == '.'){
		i++;
		if(lookup(as) == 1){
			printf("assembler program text"); 
			goto outa;
		}
		else if(j>=0 && buf[j] == '\n' && (alphachar(buf[j+2]&0377) ||
					buf[j+2] == '\\' && buf[j+3] == '"')){
			printf("[nt]roff, tbl, or eqn input text");
			goto outa;
		}
	}
	while(lookup(asc) == 0){
		if(ascom() == 0)goto notas;
		while(buf[i] != '\n' && buf[i++] != ':')
			if(i >= in)goto notas;
		while(buf[i] == '\n' || buf[i] == ' ' || buf[i] == '\t')if(i++ >= in)goto notas;
		j = i-1;
		if(buf[i] == '.'){
			i++;
			if(lookup(as) == 1){
				printf("assembler program text"); 
				goto outa; 
			}
			else if(buf[j] == '\n' && (alphachar(buf[j+2]&0377) ||
					buf[j+2] == '\\' && buf[j+3] == '"')){
				printf("[nt]roff, tbl, or eqn input text");
				goto outa;
			}
		}
	}
	printf("assembler program text");
	goto outa;
notas:
	for(i=0; i < in; i++)if(buf[i]&0200){
		if (buf[0]=='\100' && buf[1]=='\357') {
			printf("troff output\n");
			goto out;
		}
		if (!utf8(NULL))
			printf("data\n"); 
		goto out; 
	}
	if (mbuf.st_mode&0111)
		printf("commands text");
	else
	    if (english(buf, in))
		printf("English text");
	else
	    printf("ascii text");
outa:
	while(i < in)
		if((buf[i++]&0377) > 127){
			if (!utf8(" with utf-8 characters\n"))
				printf(" with garbage\n");
			goto out;
		}
	/* if next few lines in then read whole file looking for nulls ...
		while((in = read(ifile,buf,sizeof buf)) > 0)
			for(i = 0; i < in; i++)
				if((buf[i]&0377) > 127){
					printf(" with garbage\n");
					goto out;
				}
		.... */
	printf("\n");
out:;
}
static int
lookup(const char *const *tab)
{
	char r;
	int k,j,l;
	while(buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\n')i++;
	for(j=0; tab[j] != 0; j++){
		l=0;
		for(k=i; ((r=tab[j][l++]) == buf[k] && r != '\0');k++);
		if(r == '\0')
			if(buf[k] == ' ' || buf[k] == '\n' || buf[k] == '\t'
			    || buf[k] == '{' || buf[k] == '/'){
				i=k;
				return(1);
			}
	}
	return(0);
}
static int
ccom(void){
	char cc;
	while((cc = buf[i]) == ' ' || cc == '\t' || cc == '\n')if(i++ >= in)return(0);
	if(buf[i] == '/' && buf[i+1] == '*'){
		i += 2;
		while(buf[i] != '*' || buf[i+1] != '/'){
			if(buf[i] == '\\')i += 2;
			else i++;
			if(i >= in)return(1);
		}
		if((i += 2) >= in)return(1);
	}
	if(buf[i] == '\n')if(ccom() == 0)return(0);
	return(1);
}
static int
ascom(void){
	while(buf[i] == '/'){
		i++;
		while(buf[i++] != '\n')if(i >= in)return(0);
		while(buf[i] == '\n')if(i++ >= in)return(0);
	}
	return(1);
}

static int
english (const char *bp, int n)
{
# define NASC 128
	int ct[NASC], j, vow, freq, rare;
	int badpun = 0, punct = 0;
	if (n<50) return(0); /* no point in statistics on squibs */
	for(j=0; j<NASC; j++)
		ct[j]=0;
	for(j=0; j<n; j++)
	{
		if ((bp[j]&0377)<NASC)
			ct[bp[j]|040]++;
		switch (bp[j])
		{
		case '.': 
		case ',': 
		case ')': 
		case '%':
		case ';': 
		case ':': 
		case '?':
			punct++;
			if ( j < n-1 &&
			    bp[j+1] != ' ' &&
			    bp[j+1] != '\n')
				badpun++;
		}
	}
	if (badpun*5 > punct)
		return(0);
	vow = ct['a'] + ct['e'] + ct['i'] + ct['o'] + ct['u'];
	freq = ct['e'] + ct['t'] + ct['a'] + ct['i'] + ct['o'] + ct['n'];
	rare = ct['v'] + ct['j'] + ct['k'] + ct['q'] + ct['x'] + ct['z'];
	if (2*ct[';'] > ct['e']) return(0);
	if ( (ct['>']+ct['<']+ct['/'])>ct['e']) return(0); /* shell file test */
	return (vow*5 >= n-ct[' '] && freq >= 10*rare);
}

static int
magic(void)
{
	struct match	*mp;

	for (mp = matches; mp; mp = mp->m_next)
		if (gotcha(mp, "")) {
			for (mp = mp->m_sub; mp; mp = mp->m_sub)
				gotcha(mp, " ");
			printf("\n");
			return 1;
		}
	return 0;
}

static int
gotcha(struct match *mp, const char *pfx)
{
	unsigned long	n = 0;
	const char	*p, *q;

	if (mp->m_type == M_STRING) {
		q = mp->m_value.m_string;
		for (p = &buf[mp->m_offs]; p < &buf[in]; p++, q++)
			if (*q == '\0' || *p != *q)
				break;
		if (*q)
			return 0;
	} else {
		switch (mp->m_type) {
		case M_LONG|M_LE:
			if (mp->m_offs > in - 4)
				return 0;
			n = (buf[mp->m_offs]&0377) +
				((unsigned long)(buf[mp->m_offs+1]&0377)<<8) +
				((unsigned long)(buf[mp->m_offs+2]&0377)<<16) +
				((unsigned long)(buf[mp->m_offs+3]&0377)<<24);
			break;
		case M_LONG|M_BE:
			if (mp->m_offs > in - 4)
				return 0;
			n = (buf[mp->m_offs+3]&0377) +
				((unsigned long)(buf[mp->m_offs+2]&0377)<<8) +
				((unsigned long)(buf[mp->m_offs+1]&0377)<<16) +
				((unsigned long)(buf[mp->m_offs]&0377)<<24);
			break;
		case M_SHORT|M_LE:
			if (mp->m_offs > in - 2)
				return 0;
			n = (buf[mp->m_offs]&0377) +
				((unsigned long)(buf[mp->m_offs+1]&0377)<<8);
			break;
		case M_SHORT|M_BE:
			if (mp->m_offs > in - 2)
				return 0;
			n = (buf[mp->m_offs+1]&0377) +
				((unsigned long)(buf[mp->m_offs]&0377)<<8);
			break;
		case M_BYTE:
			if (mp->m_offs > in - 1)
				return 0;
			n = buf[mp->m_offs]&0377;
			break;
		}
		switch (mp->m_oper) {
		case 0:
		case '=':
			if (n != mp->m_value.m_number)
				return 0;
			break;
		case '<':
			if (n >= mp->m_value.m_number)
				return 0;
			break;
		case '>':
			if (n <= mp->m_value.m_number)
				return 0;
			break;
		case '&':
			if ((n & mp->m_value.m_number) != mp->m_value.m_number)
				return 0;
			break;
		case '^':
			if ((n & mp->m_value.m_number) == mp->m_value.m_number)
				return 0;
			break;
		case 'x':
			break;
		default:
			return 0;
		}
	}
	if (mp->m_print) {
		printf("%s", pfx);
		if (mp->m_subst == MS_SHORT)
			printf(mp->m_print, (short)n);
		else if (mp->m_subst == MS_INT)
			printf(mp->m_print, (int)n);
		else if (mp->m_subst == MS_LONG)
			printf(mp->m_print, n);
		else
			printf("%s", mp->m_print);
	}
	return 1;
}

static void
getmagic(const char *fn)
{
	struct iblok	*ip;
	char	*line = NULL;
	size_t	size = 0, length;
	struct match	*mcur = NULL, *msub = NULL;
	int	lineno = 0;

	if ((ip = ib_open(fn, 0)) == NULL) {
		fprintf(stderr, "cannot open magic file <%s>.\n", fn);
		exit(2);
	}
	while ((length = ib_getlin(ip, &line, &size, realloc)) != 0) {
		lineno++;
		if (line[length-1] == '\n')
			line[--length] = '\0';
		if (line[0] == '#')
			continue;
		split(line, &mcur, &msub, lineno);
	}
	ib_close(ip);
	free(line);
}

static void
split(char *line, struct match **mcur, struct match **msub, int lineno)
{
	struct match	*mp;
	char	*op;

	mp = scalloc(1, sizeof *mp);
	op = line;
	if ((line = col1(line, mp)) == NULL) {
		fmterr(op, lineno);
		return;
	}
	op = line;
	if ((line = col2(line, mp)) == NULL) {
		fmterr(op, lineno);
		return;
	}
	op = line;
	if ((line = col3(line, mp)) == NULL) {
		fmterr(op, lineno);
		return;
	}
	op = line;
	if ((line = col4(line, mp)) == NULL) {
		fmterr(op, lineno);
		return;
	}
	if (mp->m_level) {
		if (*msub == 0)
			return;
		(*msub)->m_sub = mp;
		*msub = mp;
	} else {
		if (matches == 0)
			matches = mp;
		else
			(*mcur)->m_next = mp;
		*mcur = *msub = mp;
	}
}

static char *
col1(char *line, struct match *mp)
{
	int	base;

	if (line[0] == '>') {
		mp->m_level = 1;
		line++;
	}
	if (*line == '0') {
		base = 8;
		if (*++line == 'x') {
			base = 16;
			line++;
		}
	} else
		base = 10;
	mp->m_offs = strtol(line, &line, base);
	while (*line == ' ')
		line++;
	if (*line++ != '\t')
		return NULL;
	while (*line == '\t')
		line++;
	return line;
}

static char *
col2(char *line, struct match *mp)
{
	if (strncmp(line, "string", 6) == 0) {
		mp->m_type = M_STRING;
		line += 6;
	} else if (strncmp(line, "byte", 4) == 0) {
		mp->m_type = M_BYTE;
		line += 4;
	} else if (strncmp(line, "short", 5) == 0) {
		mp->m_type = M_SHORT|endianess();
		line += 5;
	} else if (strncmp(line, "long", 4) == 0) {
		mp->m_type = M_LONG|endianess();
		line += 4;
	} else if (strncmp(line, "beshort", 7) == 0) {
		mp->m_type = M_SHORT|M_BE;
		line += 7;
	} else if (strncmp(line, "belong", 6) == 0) {
		mp->m_type = M_LONG|M_BE;
		line += 6;
	} else if (strncmp(line, "leshort", 7) == 0) {
		mp->m_type = M_SHORT|M_LE;
		line += 7;
	} else if (strncmp(line, "lelong", 6) == 0) {
		mp->m_type = M_LONG|M_LE;
		line += 6;
	} else
		return NULL;
	while (*line != '\t')
		line++;
	while (*line == '\t')
		line++;
	return line;
}

static char *
col3(char *line, struct match *mp)
{
	char	*lp;
	int	base;

	if (mp->m_type != M_STRING) {
		if (!digitchar(*line&0377))
			mp->m_oper = *line++;
		if (*line == '0') {
			base = 8;
			if (*++line == 'x') {
				base = 16;
				line++;
			}
		} else
			base = 10;
		mp->m_value.m_number = strtoul(line, &line, base);
		while (*line == ' ')
			line++;
		if (*line++ != '\t')
			return NULL;
	} else {
		for (lp = line; *lp && *lp != '\t'; lp++);
		if (*lp != '\t')
			return NULL;
		mp->m_value.m_string = scalloc(lp-line+1, 1);
		memcpy(mp->m_value.m_string, line, lp-line);
		line = &lp[1];
	}
	while (*line == '\t')
		line++;
	return line;
}

static char *
col4(char *line, struct match *mp)
{
	const char	fmt[] = "diouxX";
	char	*lp, *sp;

	if (*line != '\0') {
		for (lp = line; *lp; lp++)
			if (lp[0] == '%') {
				sp = &lp[1];
				while (*sp == '-' || *sp == '+' || *sp == ' ' ||
						*sp == '#' || *sp == '0' ||
						*sp == '\'')
					sp++;
				while (digitchar(*sp&0377))
					sp++;
				if (*sp == '.') {
					sp++;
					while (digitchar(*sp&0377))
						sp++;
				}
				if (sp[0] == 'l' && strchr(fmt, sp[1]&0377)) {
					if (mp->m_subst != MS_NONE) {
						mp->m_subst = MS_NONE;
						break;
					}
					mp->m_subst = MS_LONG;
				} else if (sp[0] == 'h' &&
						strchr(fmt, sp[1]&0377)) {
					if (mp->m_subst != MS_NONE) {
						mp->m_subst = MS_NONE;
						break;
					}
					mp->m_subst = MS_SHORT;
				} else if (sp[0] == 'c' ||
						strchr(fmt, sp[0]&0377)) {
					if (mp->m_subst != MS_NONE) {
						mp->m_subst = MS_NONE;
						break;
					}
					mp->m_subst = MS_INT;
				}
			}
		mp->m_print = strdup(line);
	}
	return line;
}

static void
symlnk(const char *fn, struct stat *sp)
{
	char	*b;
	size_t	len = sp->st_size ? sp->st_size : PATH_MAX;

	b = scalloc(len+1, 1);
	if (readlink(fn, b, len) < 0) {
		printf("readlink error: %s\n", strerror(errno));
		status |= 1;
	} else
		printf("symbolic link to %s\n", b);
	free(b);
}

static void
prfn(const char *fn)
{
	printf("%s:\t", fn);
	if (strlen(fn) < 7)
		printf("\t");
}

static void *
scalloc(size_t nelem, size_t elsize)
{
	void	*vp;

	if ((vp = calloc(nelem, elsize)) == NULL) {
		write(2, "no memory\n", 10);
		_exit(077);
	}
	return vp;
}

static void
usage(void)
{
	fprintf(stderr,
		"usage: %s [-c] [-h] [-f ffile] [-m mfile] file...\n",
		progname);
	exit(2);
}

static void
prmagic(void)
{
	struct match	*mc, *ms;

	printf("level\toff\ttype\topcode\tvalue\tstring\n");
	for (mc = matches; mc; mc = mc->m_next) {
		pritem(mc, 0);
		for (ms = mc->m_sub; ms; ms = ms->m_sub)
			pritem(ms, 1);
	}
}

static void
pritem(struct match *mp, int level)
{
	printf("%d\t%d\t%d\t", level, mp->m_offs, mp->m_type);
	switch (mp->m_oper) {
	case 0:
	case '=':
		i = 0;
		break;
	case '>':
		i = 1;
		break;
	case '<':
		i = 2;
		break;
	case '^':
		i = 6;
		break;
	case '&':
		i = 5;
		break;
	case 'x':
		i = 4;
		break;
	default:
		i = 077;
	}
	if (mp->m_subst != MS_NONE)
		i |= 0100;
	printf("%d\t", i);
	if (mp->m_type == M_STRING)
		printf("%s\t", mp->m_value.m_string);
	else
		printf("%lo\t", mp->m_value.m_number);
	if (mp->m_print)
		printf("%s", mp->m_print);
	if (mp->m_subst != MS_NONE)
		printf("\tsubst");
	printf("\n");
}

static void
fmterr(const char *cp, int n)
{
	if (cflag) {
		fprintf(stderr, "fmt error, no tab after %s\non line %d\n",
				cp, n);
		status |= 1;
	}
}

static int
tar(void)
{
	int	si = 0, us = 0, su = 0, m = 1;

	if (in < 512 || buf[0] == 0)
		return 0;
	for (i = 155; i >= 148; i--) {
		if (buf[i] == ' ' || buf[i] == '\0')
			continue;
		if (!octalchar(buf[i]&0377))
			return 0;
		su += (buf[i] - '0') * m;
		m *= 8;
	}
	for (i = 0; i < 512; i++) {
		if (i < 148 || i >= 156) {
			si += (signed char)buf[i];
			us += (unsigned char)buf[i];
		} else {
			si += ' ';
			us += ' ';
		}
	}
	if (si == su || us == su) {
		printf("tar\n");
		return 1;
	}
	return 0;
}

static int
sccs(void)
{
	if (in > 8 && buf[0] == '\1' && buf[1] == 'h' &&
			digitchar(buf[2]&0377) && digitchar(buf[3]&0377) &&
			digitchar(buf[4]&0377) && digitchar(buf[5]&0377) &&
			digitchar(buf[6]&0377) && buf[7] == '\n') {
		printf("sccs\n");
		return 1;
	}
	return 0;
}

static int
utf8(const char *msg)
{
	int	c, d, i, j, n;
	int	found = 0;

	for (i = 0; i < in; i++) {
		if ((c = buf[i] & 0377) & 0200) {
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
			if (i + n >= in)
				break;
			for (j = 1; j <= n; j++) {
				d = buf[i+j] & 0377;
				if (d != (d & 077 | 0200))
					return 0;
			}
			i += n;
			found++;
		}
	}
	if (found) {
		printf("%s", msg ? msg : "utf-8 text\n");
		return 1;
	}
	return 0;
}

static int
endianess(void)
{
	union {
		char	u_c[4];
		int32_t	u_i;
	} u;
	u.u_i = 1;
	return u.u_c[3] == 1 ? M_BE : M_LE;
}

static int
redirect(const char *arg0, const char *arg1)
{
	int	pd[2];

	if (pipe(pd) < 0)
		return 0;
	switch (fork()) {
	case 0:
		lseek(ifile, -in, SEEK_CUR);
		dup2(ifile, 0);
		close(ifile);
		dup2(pd[1], 1);
		close(pd[0]);
		close(pd[1]);
		execlp(arg0, arg0, arg1, NULL);
		fprintf(stderr, "could not execute %s: %s\n",
				arg0, strerror(errno));
		_exit(0177);
		/*NOTREACHED*/
	default:
		dup2(pd[0], ifile);
		close(pd[0]);
		close(pd[1]);
		break;
	case -1:
		return 0;
	}
	return 1;
}
