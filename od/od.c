/*
 * od - octal dump
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, December 2002.
 */
/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute
 * it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 */

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
#if defined (SUS)
static const char sccsid[] USED = "@(#)od_sus.sl	1.26 (gritter) 5/29/05";
#else
static const char sccsid[] USED = "@(#)od.sl	1.26 (gritter) 5/29/05";
#endif

#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>
#include	<inttypes.h>
#include	<locale.h>
#include	<ctype.h>
#include	<wctype.h>
#include	<wchar.h>
#include	<limits.h>
#include	"asciitype.h"
#include	"atoll.h"

#ifdef	__GLIBC__
#ifdef	_IO_getc_unlocked
#undef	getc
#define	getc(f)		_IO_getc_unlocked(f)
#endif	/* _IO_getc_unlocked */
#ifdef	_IO_putc_unlocked
#undef	putc
#define	putc(c, f)	_IO_putc_unlocked(c, f)
#endif	/* _IO_putc_unlocked */
#endif	/* __GLIBC__ */

enum	{
	BLOCK = 16
};

/*
 * An input block.
 */
union	block {
	char		b_c[BLOCK];
	int16_t		b_16[8];
	int32_t		b_32[4];
	int64_t		b_64[2];
	float		b_f[4];
	double		b_d[2];
};

/*
 * Format type as given with the -t option.
 */
struct	type {
	struct type	*t_nxt;		/* next type */
	const char	*t_prf;		/* format string */
	int		t_rst;		/* rest of multibyte character */
	char		t_cnt;		/* word size */
	char		t_fmt;		/* format character */
	char		t_pad;		/* space padding length */
	char		t_000;		/* currently unused */
};

/*
 * An input buffer.
 */
struct	buffer {
	union block	bu_blk;		/* input data */
	int		bu_cnt;		/* valid bytes in input data */
};

/*
 * Maps -t format to printf strings.
 */
static const struct	{
	char		p_cnt;
	char		p_fmt;
	char		p_pad;
	char		p_000;
	const char	*p_prf;
} prf[] = {
	{	4,	'f',	1,	0,	" %14.7e"	},
	{	8,	'f',	10,	0,	" %21.14le"	},
	{	1,	'd',	0,	0,	" %3d",		},
	{	2,	'd',	0,	0,	"\205\212",	},
	{	4,	'd',	4,	0,	"\12\212",	},
	{	8,	'd',	10,	0,	"\24\212",	},
	{	1,	'o',	0,	0,	"\3\10"		},
	{	2,	'o',	1,	0,	"\6\10"		},
	{	4,	'o',	4,	0,	"\13\10"	},
	{	8,	'o',	9,	0,	"\26\10"	},
	{	1,	'u',	0,	0,	"\3\12"		},
	{	2,	'u',	2,	0,	"\5\12"		},
	{	4,	'u',	5,	0,	"\12\12"	},
	{	8,	'u',	11,	0,	"\24\12"	},
	{	1,	'x',	1,	0,	"\2\20"		},
	{	2,	'x',	3,	0,	"\4\20"		},
	{	4,	'x',	7,	0,	"\10\20"	},
	{	8,	'x',	15,	0,	"\20\20"	},
	{	1,	'a',	0,	0,	""		},
	{	1,	'c',	0,	0,	""		},
	{	1,	'\0',	0,	0,	""		},
	{	0,	0,	0,	0,	NULL		}
};

static unsigned		errcnt;		/* count of errors */
static char		*progname;	/* argv[0] to main() */
static int		offset_base = 8;/* base of offset to be printed */
static int		offset_oflo = 07777777;	/* max offs. in regular width */
static long long	skip;		/* skip bytes of input */
static long long	limit = -1;	/* print no more bytes than limit */
static long long	total;		/* total bytes of input */
static long long	offset;		/* offset to print */
static int		vflag;		/* print all lines */
static int		Cflag;		/* Cray -C option */
static char		**files;	/* files to read */
static const char	*skipstr;	/* skip format string for error msg */
static FILE		*curfile;	/* current file */
static struct type	*types;		/* output formats */
static int		mb_cur_max;	/* MB_CUR_MAX */
static int		hadinput;	/* did actually read from a file */
static int		stretch;	/* stretch output columns */
static int		expensive;	/* need to compare output lines */

/*
 * For -t a.
 */
static const char	*const ctab_a[] = {
	"nul",	"soh",	"stx",	"etx",	"eot",	"enq",	"ack",	"bel",
	" bs",	" ht",	" nl",	" vt",	" ff",	" cr",	" so",	" si",
	"dle",	"dc1",	"dc2",	"dc3",	"dc4",	"nak",	"syn",	"etb",
	"can",	" em",	"sub",	"esc",	" fs",	" gs",	" rs",	" us",
	" sp"
};

/*
 * For -c.
 */
static const char	*const ctab_0[] = {
	" \\0",	"001",	"002",	"003",	"004",	"005",	"006",	"007",
	" \\b",	" \\t",	" \\n",	"013",	" \\f",	" \\r",	"016",	"017",
	"020",	"021",	"022",	"023",	"024",	"025",	"026",	"027",
	"030",	"031",	"032",	"033",	"034",	"035",	"036",	"037",
	"   "
};

/*
 * For -t c.
 */
static const char	*const ctab_c[] = {
	" \\0",	"001",	"002",	"003",	"004",	"005",	"006",	" \\a",
	" \\b",	" \\t",	" \\n",	" \\v",	" \\f",	" \\r",	"016",	"017",
	"020",	"021",	"022",	"023",	"024",	"025",	"026",	"027",
	"030",	"031",	"032",	"033",	"034",	"035",	"036",	"037",
	"   "
};

/******************************* HELPERS ********************************/
static void *
scalloc(size_t nmemb, size_t size)
{
	void	*p;

	if ((p = calloc(nmemb, size)) == NULL) {
		write(2, "No storage\n", 11);
		exit(077);
	}
	return p;
}

static void *
srealloc(void *vp, size_t nbytes)
{
	void	*p;

	if ((p = realloc(vp, nbytes)) == NULL) {
		write(2, "No storage\n", 11);
		exit(077);
	}
	return p;
}

/*static void *
smalloc(size_t nbytes)
{
	return srealloc(NULL, nbytes);
}*/

/******************************* EXECUTION ********************************/
/*
 * Return the next file in the argument list, or NULL if there are
 * no further files.
 */
static FILE *
nextfile(void)
{
	FILE	*fp;

	if (curfile && curfile != stdin)
		fclose(curfile);
	do {
		if (files == NULL || files[0] == NULL)
			return NULL;
		if (files[0][0] == '-' && files[0][1] == '\0') {
			fp = stdin;
			if (limit >= 0)
				setvbuf(stdin, NULL, _IONBF, 0);
		} else {
			if ((fp = fopen(files[0], "r")) == NULL) {
				fprintf(stderr, "%s: cannot open %s\n",
						progname, files[0]);
				errcnt |= 1;
			}
		}
		files++;
	} while (fp == NULL);
	if (hadinput == 0 && fp != NULL)
		hadinput++;
	return fp;
}

/*
 * Skip bytes of input.
 */
static void
doskip(void)
{
	while (skip > 0) {
		if (curfile == NULL || getc(curfile) == EOF) {
			if ((curfile = nextfile()) == NULL) {
				fprintf(stderr, "%s: %s is too large.\n",
						progname, skipstr);
				exit(2);
			}
			continue;
		}
		total++;
		skip--;
	}
	if (limit >= 0)
		limit += total;
}

/*
 * Fill an input buffer.
 */
static int
fill(struct buffer *bp)
{
	int	c, i;

	i = 0;
	while (i < sizeof bp->bu_blk && (limit <= 0 || total < limit)) {
		if (curfile == NULL || (c = getc(curfile)) == EOF) {
			if ((curfile = nextfile()) == NULL)
				break;
			continue;
		}
		bp->bu_blk.b_c[i++] = (char)c;
		total++;
	}
	bp->bu_cnt = i;
	while (i < sizeof bp->bu_blk)
		bp->bu_blk.b_c[i++] = '\0';
	return bp->bu_cnt;
}

/*
 * Print a value to the passed buffer. As 64-bit arithmethics requires
 * more than twice the time of 32-bit arithmetics on 32-bit platforms,
 * generate different function sets for int, long, and long long.
 */
#define	digit(T, type)	static size_t \
T ## digit(char *buf, int size, int base, unsigned type n) \
{ \
	char	*cp; \
	int	d; \
\
	if (size == 0) \
		return 0; \
	cp = buf + T ## digit(buf, size - 1, base, n / base); \
	*cp = (d = n % base) > 9 ? d - 10 + 'a' : d + '0'; \
	return cp - buf + 1; \
}

#define	number(T, type)	static size_t \
T ## number(char *buf, const char *fmt, unsigned type n) \
{ \
	int	size = fmt[0] & 0377, base = fmt[1] & 0377; \
	int	add = 1; \
\
	buf[0] = ' '; \
	if (size & 0200) { \
		size &= 0177; \
		buf[add++] = ' '; \
	} \
	if (base & 0200) { \
		base &= 0177; \
		if ((type)n < 0) { \
			buf[add] = '-'; \
			n = 0 - (type)n; \
		} else \
			buf[add] = ' '; \
		add++; \
	} \
	return T ## digit(&buf[add], size, base, n) + add; \
}

#define	mkfuncs(T, type)	digit(T, type) number(T, type)

mkfuncs(i, int)
mkfuncs(l, long)
mkfuncs(ll, long long)

/*
 * Print the offset at the start of each row.
 */
static void
prna(long long addr, int c)
{
	unsigned long long	a;
	char	buf[30];
	int	m, n, s;

	if (offset_base != 0) {
		if (addr <= offset_oflo) {
			/*
			 * Address fits in 7 characters and is preceded
			 * by '0' characters.
			 */
			if (addr > UINT_MAX)
				n = lldigit(buf, 7, offset_base, addr);
			else
				n = idigit(buf, 7, offset_base, addr);
			for (m = 0; m < n; m++)
				putc(buf[m], stdout);
		} else {
			/*
			 * Precompute the length of the address in
			 * characters if possible (speed improvement).
			 */
			switch (offset_base) {
			case 8:
				a = addr;
				for (s = 0; a != 0; s++)
					a >>= 3;
				break;
			case 16:
				a = addr;
				for (s = 0; a != 0; s++)
					a >>= 4;
				break;
			default:
				s = sizeof buf;
			}
			if (addr > UINT_MAX)
				n = lldigit(buf, s, offset_base, addr);
			else
				n = idigit(buf, s, offset_base, addr);
			for (m = 0; buf[m] == '0'; m++);
			while (m < n) {
				putc(buf[m], stdout);
				m++;
			}
		}
	}
	if (c != '\0')
		putc(c, stdout);
}

/*
 * Print a number of output lines, each preceded by the offset column.
 */
static void
prnt(long long addr, const char *s)
{
	int	lc = 0;

	do {
		if (lc++ == 0)
			prna(addr, '\0');
		else
			fputs("       ", stdout);
		do
			putc(*s, stdout);
		while (*s++ != '\n');
	} while (*s != '\0');
}

/*
 * Append a string to a group of output lines, or flush if s == NULL.
 */
static void
put(const char *s)
{
	static char	*ob, *Ob;
	static size_t	os, Os, ol;
	static int	eq;

	if (s == NULL) {
		if (Ob && !vflag && expensive && strcmp(ob, Ob) == 0) {
			if (eq++ == 0)
				printf("*\n");
		} else {
			prnt(offset, ob);
			if (ol + 1 > Os)
				Ob = srealloc(Ob, Os = ol + 1);
			strcpy(Ob, ob);
			eq = 0;
		}
		ol = 0;
	} else {
		size_t	l = strlen(s);

		if (ol + l + 1 >= os)
			ob = srealloc(ob, os = ol + l + 1);
		strcpy(&ob[ol], s);
		ol += l;
	}
}

/*
 * Format the data within the buffers according to tp.
 */
static void
format(struct type *tp, struct buffer *b1, struct buffer *b2)
{
	char	buf[200];
	int	i, j, n, l = 0;

	switch (tp->t_fmt) {
	case 'a':
	case '\0':
	case 'c':
		for (i = 0; i < b1->bu_cnt; i++) {
			int	c = b1->bu_blk.b_c[i] & 0377;

			if (tp->t_fmt == 'a')
				c &= 0177;
			if (tp->t_rst) {
				strcpy(&buf[l], "  **");
				tp->t_rst--;
				l += 4;
			} else if (tp->t_fmt != 'a' && c > 040 &&
					mb_cur_max > 1) {
				char	mb[MB_LEN_MAX];
				struct buffer	*bp;
				int	m, n;
				wchar_t	wc;

				m = i;
				bp = b1;
				for (n = 0; n < mb_cur_max; n++) {
					mb[n] = bp->bu_blk.b_c[m++];
					if (m >= bp->bu_cnt) {
						if (bp == b1) {
							bp = b2;
							m = 0;
						} else
							break;
					}
				}
				mb[n] = '\0';
				if ((n = mbtowc(&wc, mb, mb_cur_max)) <= 0
						|| !iswprint(wc))
					goto spec;
				m = wcwidth(wc);
				do
					buf[l++] = ' ';
				while (++m < 4);
				for (m = 0; m < n; m++)
					buf[l++] = mb[m];
				if (n > 1)
					tp->t_rst = n - 1;
			} else if (c > 040 && isprint(c)) {
				buf[l++] = ' ';
				buf[l++] = ' ';
				buf[l++] = ' ';
				buf[l++] = c;
			} else {
			spec:	if (c <= 040) {
					buf[l] = ' ';
					switch (tp->t_fmt) {
					case 'a':
						strcpy(&buf[l+1], ctab_a[c]);
						break;
					case '\0':
						strcpy(&buf[l+1], ctab_0[c]);
						break;
					case 'c':
						strcpy(&buf[l+1], ctab_c[c]);
						break;
					}
					l += 4;
				} else if (tp->t_fmt == 'a' && c == '\177') {
					strcpy(&buf[l], " del");
					l += 4;
				} else
					l += inumber(&buf[l], "\3\10", c);
			}
		}
		break;
	case 'f':
	case 'd':
	case 'o':
	case 'u':
	case 'x':
		for (i = 0, n = 0;
				i < BLOCK / tp->t_cnt && n < b1->bu_cnt;
				i++, n += tp->t_cnt) {
			if (stretch) {
				for (j = 0; j < tp->t_pad + stretch - 1; j++)
					buf[l++] = ' ';
			}
			if (tp->t_fmt == 'f') {
				switch (tp->t_cnt) {
				case 4:
					l += sprintf(&buf[l], tp->t_prf,
						b1->bu_blk.b_f[i]);
					break;
				case 8:
					l += sprintf(&buf[l], tp->t_prf,
						b1->bu_blk.b_d[i]);
					break;
				}
			} else {
				switch (tp->t_cnt) {
				case 1:
					if (tp->t_fmt == 'd')
						l += sprintf(&buf[l], tp->t_prf,
							b1->bu_blk.b_c[i]);
					else
						l += inumber(&buf[l], tp->t_prf,
							b1->bu_blk.b_c[i]&0377);
					break;
				case 2:
					if (tp->t_fmt == 'd')
						l += inumber(&buf[l], tp->t_prf,
							b1->bu_blk.b_16[i]);
					else
						l += inumber(&buf[l], tp->t_prf,
						b1->bu_blk.b_16[i] & 0177777U);
					break;
				case 4:
					if (tp->t_fmt == 'd')
						l += lnumber(&buf[l], tp->t_prf,
							b1->bu_blk.b_32[i]);
					else
						l += lnumber(&buf[l], tp->t_prf,
							b1->bu_blk.b_32[i] &
							037777777777UL);
					break;
				case 8:
					if (tp->t_fmt == 'd')
						l+= llnumber(&buf[l], tp->t_prf,
							b1->bu_blk.b_64[i]);
					else
						l+= llnumber(&buf[l], tp->t_prf,
							b1->bu_blk.b_64[i] &
						01777777777777777777777ULL);
					break;
				}
			}
		}
	}
	if (Cflag && b1->bu_cnt > 0) {
		static int	max;
		int	c;
		if (max == 0)
			max = l * (BLOCK/tp->t_cnt) /
				((b1->bu_cnt+tp->t_cnt-1) / tp->t_cnt);
		while (l < max)
			buf[l++] = ' ';
		buf[l++] = ' ';
		for (i = 0; i < b1->bu_cnt || i % 8; i++) {
			c = i < b1->bu_cnt ? b1->bu_blk.b_c[i] & 0377 : '.';
			buf[l++] = isprint(c) ? c : '.';
		}
	}
	buf[l++] = '\n';
	buf[l] = '\0';
	put(buf);
}

/*
 * Main execution loop. Two input buffers are necessary because multibyte
 * characters for the -c option do not always end at a buffer boundary.
 */
static void
od(void)
{
	struct buffer	b1, b2, *bp, *bq;
	struct type	*tp;
	int	star = 0;

	offset = total;
	fill(bp = &b1);
	fill(bq = &b2);
	if (hadinput == 0)
		return;
	do {
		if (star == 0) {
			for (tp = types; tp; tp = tp->t_nxt)
				format(tp, bp, bq);
			put(NULL);
		}
		offset += bp->bu_cnt;
		bp = (bp == &b1 ? &b2 : &b1);
		bq = (bq == &b1 ? &b2 : &b1);
		/*
		 * If no multibyte characters are to be printed, identical
		 * input blocks always lead to identical output lines. It
		 * is thus not necessary to format them for comparison;
		 * comparing at this point saves a lot of time for files
		 * that contain many identical lines.
		 */
		if (!vflag && !expensive && bp->bu_cnt &&
				bp->bu_cnt == bq->bu_cnt &&
				memcmp(bp->bu_blk.b_c, bq->bu_blk.b_c,
					bp->bu_cnt) == 0) {
			if (star == 0)
				printf("*\n");
			star = 1;
		} else
			star = 0;
	} while (fill(bq) > 0 || bp->bu_cnt > 0);
	if (total > 0)
		prna(total, '\n');
}

/*************************** OPTION SCANNING *****************************/
static void
usage(void)
{
	fprintf(stderr, "usage: %s [-bcdDfFoOsSvxX] [file] [[+]offset[.][b]]\n",
               progname);
	exit(2);
}

static void
setfiles(char **av)
{
	if (*av)
		files = av;
	else {
		curfile = stdin;
		hadinput = 1;
		if (limit >= 0)
			setvbuf(stdin, NULL, _IONBF, 0);
	}
}

static void
invarg(int c)
{
	fprintf(stderr, "%s: invalid argument to option -%c\n", progname, c);
	usage();
}

/*
 * Compute output column alignment.
 */
static void
align(void)
{
	struct type	*tp, *tq;

	for (tp = types; tp && tp->t_nxt; tp = tp->t_nxt) {
		tq = tp->t_nxt;

		if (tp->t_pad != tq->t_pad) {
			stretch = 1;
			break;
		}
	}
}

/*
 * Add an element to the list of types.
 */
static void
addtype(char fmt, char cnt)
{
	struct type	*tp, *tq;
	int	i;

	tp = scalloc(1, sizeof *tp);
	tp->t_fmt = fmt;
	tp->t_cnt = cnt;
	for (i = 0; prf[i].p_prf; i++) {
		if (prf[i].p_cnt == cnt && prf[i].p_fmt == fmt) {
			tp->t_prf = prf[i].p_prf;
			tp->t_pad = prf[i].p_pad;
			tp->t_000 = prf[i].p_000;
			break;
		}
	}
	if (types) {
		for (tq = types; tq->t_nxt; tq = tq->t_nxt);
		tq->t_nxt = tp;
	} else
		types = tp;
}

/*
 * Handle the argument to -t.
 */
static int
settype(const char *s)
{
	char	fmt, cnt;

	if (s == NULL) {
		expensive = mb_cur_max > 1;
		addtype('\0', 1);
		return 0;
	}
	while (*s) {
		switch (fmt = *s++) {
		case 'c':
			expensive = mb_cur_max > 1;
			/*FALLTHRU*/
		case 'a':
			addtype(fmt, 1);
			break;
		case 'f':
			switch (*s) {
			case 'F':
			case '4':
				cnt = 4;
				s++;
				break;
			case 'D':
			case 'L':
			case '8':
				cnt = 8;
				s++;
				break;
			default:
				cnt = 8;
			}
			addtype(fmt, cnt);
			break;
		case 'd':
		case 'o':
		case 'u':
		case 'x':
			switch (*s) {
			case '1':
				cnt = 1;
				s++;
				break;
			case 'C':
				cnt = sizeof (char);
				s++;
				break;
			case '2':
				cnt = 2;
				s++;
				break;
			case 'S':
				cnt = sizeof (short);
				s++;
				break;
			case '4':
				cnt = 4;
				s++;
				break;
			case 'I':
				cnt = sizeof (int);
				s++;
				break;
			case '8':
				cnt = 8;
				s++;
				break;
			case 'L':
				cnt = sizeof (long);
				s++;
				break;
			default:
				cnt = sizeof (int);
			}
			addtype(fmt, cnt);
			break;
		default:
			return -1;
		}
	}
	return 0;
}

/*
 * Handle a traditional offset argument.
 */
static int
setoffset(const char *s)
{
	long long	o;
	const char	*sp;
	int	base = 8;
	int	mult = 1;

	skipstr = s;
	if (*s == '+')
		s++;
	for (sp = s; digitchar(*sp & 0377); sp++);
	if (sp > s) {
		if (*sp == '.') {
			base = 10;
			sp++;
		}
		if (*sp == 'b' || *sp == 'B') {
			mult = 512;
			sp++;
		}
		if (*sp != '\0')
			return -1;
	} else
		return -1;
	o = strtoll(s, NULL, base);
	skip = o * mult;
	return 0;
}

/*
 * Handle the argument to -j.
 */
static int
setskip(const char *s)
{
	const char	*sp = NULL;
	long long	o;
	int	base = 10;
	int	mult = 1;

	skipstr = s;
	if (s[0] == '0' && s[1]) {
		s++;
		if (*s == 'x' || *s == 'X') {
			s++;
			base = 16;
		} else
			base = 8;
	}
	switch (base) {
	case 8:
		for (sp = s; octalchar(*sp & 0377); sp++);
		break;
	case 10:
		for (sp = s; digitchar(*sp & 0377); sp++);
		break;
	case 16:
		for (sp = s; digitchar(*sp & 0377) ||
				*sp == 'a' || *sp == 'A' ||
				*sp == 'b' || *sp == 'B' ||
				*sp == 'c' || *sp == 'C' ||
				*sp == 'd' || *sp == 'D' ||
				*sp == 'e' || *sp == 'E' ||
				*sp == 'f' || *sp == 'F';
			sp++);
		break;
	}
	if (sp > s) {
		switch (*sp) {
		case 'b':
			mult = 512;
			sp++;
			break;
		case 'k':
			mult = 1024;
			sp++;
			break;
		case 'm':
			mult = 1048576;
			sp++;
			break;
		case '\0':
			break;
		default:
			return -1;
		}
		if (*sp != '\0')
			return -1;
	} else
		return -1;
	o = strtoull(s, NULL, base);
	skip = o * mult;
	return 0;
}

/*
 * Handle the argument to -N.
 */
static int
setlimit(const char *s)
{
	long long	o;
	char	*x;
	int	base = 10;

	if (*s == '0') {
		s++;
		if (*s == 'x' || *s == 'X') {
			s++;
			base = 16;
		} else
			base = 8;
	}
	o = strtoll(s, &x, base);
	if (*x != '\0')
		return -1;
	limit = o;
	return 0;
}

int
main(int argc, char **argv)
{
	const char	optstring[] = ":A:bcCdDfFj:N:oOsSt:vxX";
	int	i, newopt = 0;;

	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	if (sizeof (union block) != BLOCK || mb_cur_max > BLOCK)
		abort();
	progname = basename(argv[0]);
#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	while ((i = getopt(argc, argv, optstring)) != EOF) {
		switch (i) {
		case 'A':
			switch (optarg[0]) {
			case 'd':
				offset_base = 10;
				offset_oflo = 9999999;
				break;
			case 'o':
				offset_base = 8;
				offset_oflo = 07777777;
				break;
			case 'x':
				offset_base = 16;
				offset_oflo = 0xfffffff;
				break;
			case 'n':
				offset_base = 0;
				break;
			default:
				invarg(i);
			}
			if (optarg[1] != '\0')
				invarg(i);
			newopt = 1;
			break;
		case 'b':
			settype("o1");
			break;
		case 'c':
			settype(NULL);
			break;
		case 'd':
			settype("u2");
			break;
		case 'D':
			settype("u4");
			break;
		case 'f':
			settype("fF");
			break;
		case 'F':
			settype("fD");
			break;
		case 'j':
			if (setskip(optarg) < 0)
				invarg(i);
			newopt = 1;
			break;
		case 'N':
			if (setlimit(optarg) < 0)
				invarg(i);
			newopt = 1;
			break;
		case 'o':
			settype("o2");
			break;
		case 'O':
			settype("o4");
			break;
		case 's':
			settype("d2");
			break;
		case 'S':
			settype("d4");
			break;
		case 't':
			if (settype(optarg) < 0)
				invarg('t');
			newopt = 1;
			break;
		case 'v':
			vflag = 1;
			break;
		case 'x':
			settype("x2");
			break;
		case 'X':
			settype("x4");
			break;
		case ':':
			fprintf(stderr,
				"%s: option requires an argument -- %c\n",
				progname, optopt);
			usage();
		case 'C':
			Cflag = 1;
			break;
		case '?':
			fprintf(stderr, "%s: bad flag -%c\n",
					progname, optopt);
			/*FALLTHRU*/
		default:
			usage();
		}
	}
	if (newopt == 0 && ((optind>=argc-2 && argc &&argv[argc-1][0] == '+') ||
#ifndef	SUS
				(optind>=argc-2 && argc &&
#else	/* SUS */
				(optind == argc-1 &&
#endif	/* SUS */
				digitchar(argv[argc-1][0] & 0377))) &&
			setoffset(argv[argc-1]) >= 0) {
		argc--;
		argv[argc] = NULL;
	}
	setfiles(argc ? &argv[optind] : &argv[0]);
	if (types == NULL)
		settype("oS");
	align();
	if (skip > 0)
		doskip();
	od();
	return errcnt;
}
