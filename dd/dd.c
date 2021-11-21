/*
 * dd - convert and copy
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, January 2003.
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
static const char sccsid[] USED = "@(#)dd.sl	1.30 (gritter) 1/22/06";

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<malloc.h>
#include	<errno.h>
#include	<libgen.h>
#include	<ctype.h>
#include	<locale.h>
#include	<signal.h>
#include	"sigset.h"
#include	<wchar.h>
#include	<wctype.h>
#include	<limits.h>

#include	<sys/ioctl.h>

#if defined (__linux__) || defined (__sun) || defined (__FreeBSD__) || \
	defined (__hpux) || defined (_AIX) || defined (__NetBSD__) || \
	defined (__OpenBSD__) || defined (__DragonFly__) || defined (__APPLE__)
#include	<sys/mtio.h>
#else	/* SVR4.2MP */
#include	<sys/scsi.h>
#include	<sys/st01.h>
#endif	/* SVR4.2MP */

#include	"atoll.h"
#include	"memalign.h"
#include	"mbtowi.h"

/*
 * For 'conv=ascii'.
 */
static const unsigned char	c_ascii[] = {
0000,0001,0002,0003,0234,0011,0206,0177,0227,0215,0216,0013,0014,0015,0016,0017,
0020,0021,0022,0023,0235,0205,0010,0207,0030,0031,0222,0217,0034,0035,0036,0037,
0200,0201,0202,0203,0204,0012,0027,0033,0210,0211,0212,0213,0214,0005,0006,0007,
0220,0221,0026,0223,0224,0225,0226,0004,0230,0231,0232,0233,0024,0025,0236,0032,
0040,0240,0241,0242,0243,0244,0245,0246,0247,0250,0325,0056,0074,0050,0053,0174,
0046,0251,0252,0253,0254,0255,0256,0257,0260,0261,0041,0044,0052,0051,0073,0176,
0055,0057,0262,0263,0264,0265,0266,0267,0270,0271,0313,0054,0045,0137,0076,0077,
0272,0273,0274,0275,0276,0277,0300,0301,0302,0140,0072,0043,0100,0047,0075,0042,
0303,0141,0142,0143,0144,0145,0146,0147,0150,0151,0304,0305,0306,0307,0310,0311,
0312,0152,0153,0154,0155,0156,0157,0160,0161,0162,0136,0314,0315,0316,0317,0320,
0321,0345,0163,0164,0165,0166,0167,0170,0171,0172,0322,0323,0324,0133,0326,0327,
0330,0331,0332,0333,0334,0335,0336,0337,0340,0341,0342,0343,0344,0135,0346,0347,
0173,0101,0102,0103,0104,0105,0106,0107,0110,0111,0350,0351,0352,0353,0354,0355,
0175,0112,0113,0114,0115,0116,0117,0120,0121,0122,0356,0357,0360,0361,0362,0363,
0134,0237,0123,0124,0125,0126,0127,0130,0131,0132,0364,0365,0366,0367,0370,0371,
0060,0061,0062,0063,0064,0065,0066,0067,0070,0071,0372,0373,0374,0375,0376,0377
};

/*
 * For 'conv=ibm'.
 */
static const unsigned char	c_ibm[] = {
0000,0001,0002,0003,0067,0055,0056,0057,0026,0005,0045,0013,0014,0015,0016,0017,
0020,0021,0022,0023,0074,0075,0062,0046,0030,0031,0077,0047,0034,0035,0036,0037,
0100,0132,0177,0173,0133,0154,0120,0175,0115,0135,0134,0116,0153,0140,0113,0141,
0360,0361,0362,0363,0364,0365,0366,0367,0370,0371,0172,0136,0114,0176,0156,0157,
0174,0301,0302,0303,0304,0305,0306,0307,0310,0311,0321,0322,0323,0324,0325,0326,
0327,0330,0331,0342,0343,0344,0345,0346,0347,0350,0351,0255,0340,0275,0137,0155,
0171,0201,0202,0203,0204,0205,0206,0207,0210,0211,0221,0222,0223,0224,0225,0226,
0227,0230,0231,0242,0243,0244,0245,0246,0247,0250,0251,0300,0117,0320,0241,0007,
0040,0041,0042,0043,0044,0025,0006,0027,0050,0051,0052,0053,0054,0011,0012,0033,
0060,0061,0032,0063,0064,0065,0066,0010,0070,0071,0072,0073,0004,0024,0076,0341,
0101,0102,0103,0104,0105,0106,0107,0110,0111,0121,0122,0123,0124,0125,0126,0127,
0130,0131,0142,0143,0144,0145,0146,0147,0150,0151,0160,0161,0162,0163,0164,0165,
0166,0167,0170,0200,0212,0213,0214,0215,0216,0217,0220,0232,0233,0234,0235,0236,
0237,0240,0252,0253,0254,0255,0256,0257,0260,0261,0262,0263,0264,0265,0266,0267,
0270,0271,0272,0273,0274,0275,0276,0277,0312,0313,0314,0315,0316,0317,0332,0333,
0334,0335,0336,0337,0352,0353,0354,0355,0356,0357,0372,0373,0374,0375,0376,0377
};

/*
 * For 'conv=ebcdic'.
 */
static const unsigned char	c_ebcdic[] = {
0000,0001,0002,0003,0067,0055,0056,0057,0026,0005,0045,0013,0014,0015,0016,0017,
0020,0021,0022,0023,0074,0075,0062,0046,0030,0031,0077,0047,0034,0035,0036,0037,
0100,0132,0177,0173,0133,0154,0120,0175,0115,0135,0134,0116,0153,0140,0113,0141,
0360,0361,0362,0363,0364,0365,0366,0367,0370,0371,0172,0136,0114,0176,0156,0157,
0174,0301,0302,0303,0304,0305,0306,0307,0310,0311,0321,0322,0323,0324,0325,0326,
0327,0330,0331,0342,0343,0344,0345,0346,0347,0350,0351,0255,0340,0275,0232,0155,
0171,0201,0202,0203,0204,0205,0206,0207,0210,0211,0221,0222,0223,0224,0225,0226,
0227,0230,0231,0242,0243,0244,0245,0246,0247,0250,0251,0300,0117,0320,0137,0007,
0040,0041,0042,0043,0044,0025,0006,0027,0050,0051,0052,0053,0054,0011,0012,0033,
0060,0061,0032,0063,0064,0065,0066,0010,0070,0071,0072,0073,0004,0024,0076,0341,
0101,0102,0103,0104,0105,0106,0107,0110,0111,0121,0122,0123,0124,0125,0126,0127,
0130,0131,0142,0143,0144,0145,0146,0147,0150,0151,0160,0161,0162,0163,0164,0165,
0166,0167,0170,0200,0212,0213,0214,0215,0216,0217,0220,0152,0233,0234,0235,0236,
0237,0240,0252,0253,0254,0112,0256,0257,0260,0261,0262,0263,0264,0265,0266,0267,
0270,0271,0272,0273,0274,0241,0276,0277,0312,0313,0314,0315,0316,0317,0332,0333,
0334,0335,0336,0337,0352,0353,0354,0355,0356,0357,0372,0373,0374,0375,0376,0377
};

static char		*progname;	/* argv[0] to main() */

typedef	long long	d_type;

static char		*iblok;		/* input buffer */
static char		*oblok;		/* output buffer */
static char		*cblok;		/* conversion buffer */

static char		mblok[MB_LEN_MAX+1];	/* tow{upper|lower} buffer */
static char		*mbp;		/* points to remaining chars in mblok */
static int		mbrest;		/* number of remaining chars in mblok */

static const char	*iffile;	/* input file name */
static int		iffd;		/* input file descriptor */
static const char	*offile;	/* output file name */
static int		offd;		/* output file descriptor */
static struct stat	istat;		/* stat of input */
static struct stat	ostat;		/* stat of output */
static d_type		ibs = 512;	/* input block size */
static d_type		obs = 512;	/* output block size */
static d_type		bs;		/* size for both buffers */
static d_type		oflow;		/* remaining bytes in output buffer */
static d_type		cbs;		/* conversion block size */
static d_type		cflow;		/* remaining bytes in conv. buffer */
static int		ctrunc;		/* truncate current data (conv=block) */
static d_type		skip;		/* skip these blocks on input */
static d_type		count = -1;	/* no more than count blocks of input */
static int		files = 1;	/* read EOF this many times */
static d_type		iseek;		/* seek these blocks on input */
static d_type		oseek;		/* seek these blocks on output */
static int		mb_cur_max;	/* MB_CUR_MAX acceleration */

static d_type		iwhole;		/* statistics */
static d_type		ipartial;
static d_type		owhole;
static d_type		opartial;
static d_type		truncated;

static enum charconv {
	CHAR_NONE	= 0,
	CHAR_ASCII	= 1,
	CHAR_EBCDIC	= 2,
	CHAR_IBM	= 3
} chars = CHAR_NONE;

static enum conversion {
	CONV_NONE	= 0,
	CONV_BLOCK	= 01,
	CONV_UNBLOCK	= 02,
	CONV_LCASE	= 04,
	CONV_UCASE	= 010,
	CONV_SWAB	= 020,
	CONV_NOERROR	= 040,
	CONV_NOTRUNC	= 0100,
	CONV_IDIRECT	= 0200,
	CONV_ODIRECT	= 0400,
	CONV_DIRECT	= 0600,
	CONV_SYNC	= 01000
} convs = CONV_NONE;

static struct {
	const char	*c_name;
	enum conversion	c_conv;
	enum charconv	c_char;
} convtab[] = {
	{ "ascii",	CONV_UNBLOCK,	CHAR_ASCII	},
	{ "ebcdic",	CONV_BLOCK,	CHAR_EBCDIC	},
	{ "ibm",	CONV_BLOCK,	CHAR_IBM	},
	{ "block",	CONV_BLOCK,	CHAR_NONE	},
	{ "unblock",	CONV_UNBLOCK,	CHAR_NONE	},
	{ "lcase",	CONV_LCASE,	CHAR_NONE	},
	{ "ucase",	CONV_UCASE,	CHAR_NONE	},
	{ "swab",	CONV_SWAB,	CHAR_NONE	},
	{ "noerror",	CONV_NOERROR,	CHAR_NONE	},
	{ "notrunc",	CONV_NOTRUNC,	CHAR_NONE	},
#ifdef	O_DIRECT
	{ "idirect",	CONV_IDIRECT,	CHAR_NONE	},
	{ "odirect",	CONV_ODIRECT,	CHAR_NONE	},
#endif	/* O_DIRECT */
	{ "sync",	CONV_SYNC,	CHAR_NONE	},
	{ NULL,		CONV_NONE,	CHAR_NONE	}
};

static void *
bmalloc(size_t nbytes)
{
	static long	pagesize;
	void	*vp;

	if (pagesize == 0)
		if ((pagesize = sysconf(_SC_PAGESIZE)) < 0)
			pagesize = 4096;
	if ((vp = memalign(pagesize, nbytes)) == NULL) {
		fprintf(stderr, "%s: not enough memory\n", progname);
		fprintf(stderr, "Please use a smaller buffer size\n");
		exit(077);
	}
	return vp;
}

/************************** ARGUMENT SCANNING ***************************/
static void
badarg(const char *arg)
{
	fprintf(stderr, "%s: bad arg: \"%s\"\n", progname, arg);
	exit(2);
}

static void
badnumeric(const char *arg)
{
	fprintf(stderr, "%s: bad numeric arg: \"%s\"\n", progname, arg);
	exit(2);
}

static void
nozeroblok(void)
{
	fprintf(stderr, "%s: buffer sizes cannot be zero\n", progname);
	exit(2);
}

/*
 * Get the value of a numeric argument.
 */
static d_type
expr(const char *ap)
{
	d_type	val;
	char	*x;
	int	c;
	
	if (*ap == '-' || *ap == '+')
		badnumeric(ap);
	val = strtoull(ap, &x, 10);
	while ((c = *x++) != '\0') {
		switch (c) {
		case 'k':
			val *= 1024;
			break;
		case 'b':
			val *= 512;
			break;
		case 'w':
			val *= 2;
			break;
		case 'x':
		case '*':
			return val * expr(x);
		default:
			badnumeric(ap);
		}
	}
	return val;
}

static void
setin(const char *ap)
{
	iffile = ap;
}

static void
setof(const char *ap)
{
	offile = ap;
}

static void
setibs(const char *ap)
{
	ibs = expr(ap);
	if (ibs == 0)
		nozeroblok();
}

static void
setobs(const char *ap)
{
	obs = expr(ap);
	if (obs == 0)
		nozeroblok();
}

static void
setbs(const char *ap)
{
	bs = expr(ap);
}

static void
setcbs(const char *ap)
{
	cbs = expr(ap);
}

static void
setskip(const char *ap)
{
	skip = expr(ap);
}

static void
setcount(const char *ap)
{
	count = expr(ap);
}

static void
setconv(const char *ap)
{
	const char	*cp, *cq;
	int	i;

	for (;;) {
		while (*ap == ',')
			ap++;
		if (*ap == '\0')
			break;
		for (i = 0; convtab[i].c_name; i++) {
			for (cp = convtab[i].c_name, cq = ap;
					*cp && (*cp == *cq);
					cp++, cq++);
			if (*cp == '\0' && (*cq == ',' || *cq == '\0')) {
				convs |= convtab[i].c_conv;
				if (convtab[i].c_char != CHAR_NONE)
					chars = convtab[i].c_char;
				ap = cq;
				goto next;
			}
		}
		badarg(ap);
	next:;
	}
}

static void
setfiles(const char *ap)
{
	files = expr(ap);
}

static void
setiseek(const char *ap)
{
	iseek = expr(ap);
}

static void
setoseek(const char *ap)
{
	oseek = expr(ap);
}

static struct {
	const char	*a_name;
	void		(*a_func)(const char *);
} argtab[] = {
	{ "if=",	setin		},
	{ "of=",	setof		},
	{ "ibs=",	setibs		},
	{ "obs=",	setobs		},
	{ "bs=",	setbs		},
	{ "cbs=",	setcbs		},
	{ "skip=",	setskip		},
	{ "seek=",	setoseek	},
	{ "count=",	setcount	},
	{ "conv=",	setconv		},
	{ "files=",	setfiles	},
	{ "iseek=",	setiseek	},
	{ "oseek=",	setoseek	},
	{ NULL,		NULL		}
};

static const char *
thisarg(const char *sp, const char *ap)
{
	do {
		if (*sp != *ap)
			return NULL;
		if (*sp == '=')
			return &sp[1];
	} while (*sp++ && *ap++);
	return NULL;
}

/******************************* EXECUTION ********************************/
static void
stats(void)
{
	fprintf(stderr, "%llu+%llu records in\n",
			(unsigned long long)iwhole,
			(unsigned long long)ipartial);
	fprintf(stderr, "%llu+%llu records out\n",
			(unsigned long long)owhole,
			(unsigned long long)opartial);
	if (truncated) {
		fprintf(stderr, "%llu truncated record%s\n",
				(unsigned long long)truncated,
				truncated > 1 ? "s" : "");
	}
}

static void	charconv(char *data, size_t size);
static void	bflush(void);
static void	cflush(void);
static void	uflush(void);

static void
quit(int status)
{
	if (mbp)
		charconv(NULL, 0);
	cflush();
	uflush();
	bflush();
	stats();
	exit(status);
}

static void
onint(int sig)
{
	stats();
	exit(sig | 0200);
}

static int
ontape(void)
{
	static int	yes = -1;

	if (yes == -1) {
#if defined (__linux__) || defined (__FreeBSD__) || defined (__hpux) || \
	defined (_AIX) || defined (__NetBSD__) || defined (__OpenBSD__) || \
	defined (__DragonFly__) || defined (__APPLE__)
		struct mtget	mg;
		yes = (istat.st_mode&S_IFMT) == S_IFCHR &&
			ioctl(iffd, MTIOCGET, &mg) == 0;
#elif defined (__sun)
		struct mtdrivetype_request	mr;
		struct mtdrivetype	md;
		mr.size = sizeof md;
		mr.mtdtp = &md;
		yes = (istat.st_mode&S_IFMT) == S_IFCHR &&
			ioctl(iffd, MTIOCGETDRIVETYPE, &mr) == 0;
#else	/* SVR4.2MP */
		struct blklen	bl;
		yes = (istat.st_mode&S_IFMT) == S_IFCHR &&
			ioctl(iffd, T_RDBLKLEN, &bl) == 0;
#endif	/* SVR4.2MP */
	}
	return yes;
}

static void
seekconv(d_type count)
{
	ssize_t	sz;
	off_t	offs;

	if (lseek(offd, 0, SEEK_CUR) != (off_t)-1) {
		do {
			if ((offs = lseek(offd, obs, SEEK_CUR)) == (off_t)-1) {
			err:	fprintf(stderr, "%s: output seek error: %s\n",
						progname, strerror(errno));
				exit(3);
			}
		} while (--count);
		if ((convs & CONV_NOTRUNC) == 0 &&
				(ostat.st_mode&S_IFMT) == S_IFREG)
			ftruncate(offd, offs);
		return;
	}
	while (count) {
		if ((sz = read(offd, oblok, obs)) == 0)
			break;
		if (sz < 0)
			goto err;
		count--;
	}
	if (count) {
		memset(oblok, 0, obs);
		do {
			if ((sz = write(offd, oblok, obs)) < 0)
				goto err;
		} while (--count);
	}
}

static void
skipconv(int canseek, d_type count)
{
	ssize_t	rd = 0;

	if (canseek && lseek(iffd, 0, SEEK_CUR) == (off_t)-1)
		canseek = 0;
	while (count--) {
		if (canseek) {
			if (lseek(iffd, ibs, SEEK_CUR) != (off_t)-1)
				rd = ibs;
			else if (errno == EINVAL)
				rd = 0;
			else {
				fprintf(stderr, "%s: input seek error: %s\n",
					progname, strerror(errno));
				exit(3);
			}
		} else {
			if ((rd = read(iffd, iblok, ibs)) < 0) {
				fprintf(stderr,
					"%s: read error during skip: %s\n",
					progname, strerror(errno));
				exit(3);
			}
		}
		if (rd == 0 && files-- <= 1) {
			fprintf(stderr, "%s: cannot skip past end-of-file\n",
					progname);
			exit(3);
		}
	}
}

static void
prepare(void)
{
	int	flags;

	if (bs)
		ibs = obs = bs;
	iblok = bmalloc(ibs);
	if (!(bs && chars == CHAR_NONE &&
			(convs|CONV_SYNC|CONV_NOERROR|CONV_NOTRUNC|CONV_DIRECT)
			== (CONV_SYNC|CONV_NOERROR|CONV_NOTRUNC|CONV_DIRECT)))
		oblok = bmalloc(obs);
	if (cbs > 0) {
		if ((convs & (CONV_BLOCK|CONV_UNBLOCK)) == 0) {
			fprintf(stderr,
		"%s: cbs must be zero if no block conversion requested\n",
				progname);
			exit(2);
		}
		cblok = bmalloc(cbs + 1);
	} else
		convs &= ~(CONV_BLOCK|CONV_UNBLOCK);
	if ((iffd = iffile ? open(iffile, O_RDONLY) : dup(0)) < 0) {
		fprintf(stderr, "%s: cannot open %s: %s\n", progname,
				iffile ? iffile : "", strerror(errno));
		exit(1);
	}
	fstat(iffd, &istat);
#ifdef	O_DIRECT
	if (convs & CONV_IDIRECT) {
		int	flags;
		flags = fcntl(iffd, F_GETFL);
		fcntl(iffd, F_SETFL, flags | O_DIRECT);
	}
#endif	/* O_DIRECT */
	if (skip)
		skipconv(0, skip);
	else if (iseek)
		skipconv(1, iseek);
	flags = O_RDWR | O_CREAT;
	if ((convs & CONV_NOTRUNC) == 0 && oseek == 0)
		flags |= O_TRUNC;
	if ((offd = offile ? open(offile, flags, 0666) : dup(1)) < 0) {
		fprintf(stderr, "%s: cannot %s %s: %s\n",
				progname,
				flags & O_TRUNC ? "create" : "open",
				offile ? offile : "", strerror(errno));
		exit(1);
	}
	fstat(offd, &ostat);
#ifdef	O_DIRECT
	if (convs & CONV_ODIRECT) {
		int	flags;
		flags = fcntl(offd, F_GETFL);
		fcntl(offd, F_SETFL, flags | O_DIRECT);
	}
#endif	/* O_DIRECT */
	if (oseek)
		seekconv(oseek);
}

static void
swabconv(char *data, size_t size)
{
	char	c;

	while (size > 1) {
		c = data[0];
		data[0] = data[1];
		data[1] = c;
		size -= 2;
		data += 2;
	}
}

static void
ascconv(char *data, size_t size)
{
	while (size--) {
		*data = c_ascii[*data & 0377];
		data++;
	}
}

static ssize_t
swrite(const char *data, size_t size)
{
	ssize_t	wt;

	for (;;) {
		if ((wt = write(offd, data, size)) <= 0) {
			if (errno == EINTR)
				continue;
			fprintf(stderr, "%s: write error: %s\n",
					progname, strerror(errno));
			oflow = 0;
			offd = -1;
			quit(1);
		}
		break;
	}
	return wt;
}

/*
 * Write without output buffering (if bs= was specified).
 */
static void
dwrite(const char *data, size_t size)
{
	ssize_t	wrt;

	do {
		wrt = swrite(data, size);
		if (wrt == obs)
			owhole++;
		else
			opartial++;
		data += wrt;
		size -= wrt;
	} while (size > 0);
}

/*
 * Write to output buffer. On short write, remaining data is kept within
 * the buffer and written next time again. Might a warning be useful in
 * this case?
 */
static void
bwrite(const char *data, size_t size)
{
	ssize_t	wrt;
	size_t	di;

	while (oflow + size > obs) {
		di = obs - oflow;
		size -= di;
		if (oflow) {
			memcpy(&oblok[oflow], data, di);
			wrt = swrite(oblok, obs);
		} else
			wrt = swrite(data, obs);
		if (wrt != obs) {
			memcpy(oblok, &(oflow ? oblok : data)[wrt], obs - wrt);
			opartial++;
		} else
			owhole++;
		oflow = obs - wrt;
		data += di;
	}
	if (size == obs) {
		if ((wrt = swrite(data, obs)) == obs)
			owhole++;
		else
			opartial++;
		size -= wrt;
		data += wrt;
	}
	if (size) {
		memcpy(&oblok[oflow], data, size);
		oflow += size;
	}
}

static void
bflush(void)
{
	ssize_t	wrt;

	if (offd >= 0) {
		while (oflow) {
			if ((wrt = swrite(oblok, oflow)) != oflow)
				memcpy(oblok, &oblok[wrt], obs - wrt);
			oflow -= wrt;
			opartial++;
		}
		if (close(offd) < 0) {
			fprintf(stderr, "%s: write error: %s\n",
					progname, strerror(errno));
			offd = -1;
			quit(1);
		}
		offd = -1;
	}
}

/*
 * Handle conversions to EBCDIC.
 */
static void
ewrite(char *data, size_t size)
{
	char	*dt = data;
	size_t	sz = size;
	if (chars == CHAR_EBCDIC) {
		while (sz--) {
			*dt = c_ebcdic[*dt & 0377];
			dt++;
		}
	} else if (chars == CHAR_IBM) {
		while (sz--) {
			*dt = c_ibm[*dt & 0377];
			dt++;
		}
	}
	bwrite(data, size);
}

/*
 * Handle 'conv=block'.
 */
static void
cflush(void)
{
	if (convs & CONV_BLOCK && cflow) {
		while (cflow < cbs)
			cblok[cflow++] = ' ';
		ewrite(cblok, cbs);
		cflow = 0;
	}
}

static void
cwrite(const char *data, size_t size)
{
	while (size) {
		if (ctrunc == 0) {
			cblok[cflow] = *data++;
			if (cblok[cflow] == '\n') {
				if (cflow == 0)
					cblok[cflow++] = ' ';
				cflush();
			} else if (++cflow == cbs) {
				cflush();
				ctrunc = 1;
			}
		} else {
			if (*data++ == '\n')
				ctrunc = 0;
			else if (ctrunc == 1) {
				truncated++;
				ctrunc = 2;
			}
		}
		size--;
	}
}

/*
 * Handle 'conv=unblock'.
 */
static void
uflush(void)
{
	char	*cp;

	if (cflow) {
		for (cp = &cblok[cflow-1]; cp >= cblok && *cp == ' '; cp--);
		cp[1] = '\n';
		bwrite(cblok, cp - cblok + 2);
		cflow = 0;
	}
}

static void
uwrite(const char *data, size_t size)
{
	while (size) {
		while (cflow < cbs) {
			cblok[cflow++] = *data++;
			if (--size == 0)
				return;
		}
		uflush();
	}
}

static void
blokconv(char *data, size_t size)
{
	switch (chars) {
	case CHAR_EBCDIC:
	case CHAR_IBM:
		if ((convs & (CONV_BLOCK|CONV_UNBLOCK)) == 0) {
			ewrite(data, size);
			break;
		}
		/*FALLTHRU*/
	default:
		if (convs & CONV_BLOCK)
			cwrite(data, size);
		else if (convs & CONV_UNBLOCK)
			uwrite(data, size);
		else
			bwrite(data, size);
		break;
	}
}

static void
charconv(char *data, size_t size)
{
	if (convs & (CONV_LCASE|CONV_UCASE)) {
		if (mb_cur_max > 1) {
			/*
			 * Multibyte case conversion is somewhat ugly
			 * with dd as there is no guarantee that a
			 * character fits in an input block. We need
			 * another intermediate therefore to store
			 * incomplete multibyte sequences.
			 */
			int	i, n, len;
			wint_t	wc;
			int	flush = size == 0;

			while (size > 0 || (flush && mbrest)) {
				i = 0;
				if (mbrest && mbp && mbp > mblok) {
					do
						mblok[i] = mbp[i];
					while (i++, --mbrest);
				} else if (mbp == mblok) {
					i = mbrest;
					mbrest = 0;
				}
				if (i == 0 && size) {
					mblok[i++] = *data++;
					size--;
				}
				if (mblok[0] & 0200) {
					while (i < mb_cur_max && size) {
						mblok[i++] = *data++;
						size--;
					}
					if (!flush && i < mb_cur_max) {
						mbp = mblok;
						mbrest = i;
						return;
					}
					if ((n = mbtowi(&wc, mblok, i)) < 0) {
						len = 1;
						wc = WEOF;
					} else if (n == 0)
						len = 1;
					else
						len = n;
				} else {
					wc = mblok[0];
					len = n = 1;
				}
				if (i > 0) {
					mbrest = i - len;
					mbp = &mblok[len];
				} else {
					mbrest = 0;
					mbp = NULL;
				}
				if (wc != WEOF) {
					char	new[MB_LEN_MAX + 1];

					if (convs & CONV_LCASE)
						wc = wc & ~(wchar_t)0177 ?
							towlower(wc) :
							tolower(wc);
					if (convs & CONV_UCASE)
						wc = wc & ~(wchar_t)0177 ?
							towupper(wc) :
							toupper(wc);
					if ((n = wctomb(new, wc)) > 0)
						blokconv(new, n);
					else
						goto inv;
				} else
				inv:	blokconv(mblok, len);
			}
			return;
		} else {
			char	*dp = data;
			size_t	sz = size;

			while (sz--) {
				if (convs & CONV_LCASE)
					*dp = tolower(*dp & 0377);
				if (convs & CONV_UCASE)
					*dp = toupper(*dp & 0377);
				dp++;
			}
		}
	}
	blokconv(data, size);
}

static void
dd(void)
{
	ssize_t	rd;

	while (count == -1 || count > 0) {
		if ((rd = read(iffd, iblok, ibs)) < ibs) {
			if (rd < 0) {
				fprintf(stderr, "%s: read error: %s\n",
						progname, strerror(errno));
				if (convs & CONV_NOERROR) {
					stats();
					if (!ontape())
						lseek(iffd, ibs, SEEK_CUR);
					if (convs & CONV_SYNC)
						rd = 0;
					else
						continue;
				} else
					quit(1);
			} else if (rd == 0) {
				if (files-- <= 1)
					break;
				continue;
			} else if (rd > 0)
				ipartial++;
			if (convs & CONV_SYNC) {
				int	c;

				c = convs&(CONV_BLOCK|CONV_UNBLOCK) ? ' ' : 0;
				memset(&iblok[rd], c, ibs - rd);
				rd = ibs;
			}
		} else
			iwhole++;
		if (count > 0)
			count--;
		if (bs && chars == CHAR_NONE &&
			(convs|CONV_SYNC|CONV_NOERROR|CONV_NOTRUNC|CONV_DIRECT)
			== (CONV_SYNC|CONV_NOERROR|CONV_NOTRUNC|CONV_DIRECT))
			dwrite(iblok, rd);
		else {
			if (convs & CONV_SWAB)
				swabconv(iblok, rd);
			if (chars == CHAR_ASCII)
				ascconv(iblok, rd);
			charconv(iblok, rd);
		}
	}
}

int
main(int argc, char **argv)
{
	const char	*cp;
	int	o, i;

	progname = basename(argv[0]);
	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	if (argc > 1 && argv[1][0] == '-' && argv[1][1] == '-' &&
			argv[1][2] == '\0')
		o = 2;
	else
		o = 1;
	while (o < argc) {
		for (i = 0; argtab[i].a_name; i++) {
			if ((cp = thisarg(argv[o], argtab[i].a_name)) != 0) {
				argtab[i].a_func(cp);
				break;
			}
		}
		if (argtab[i].a_name == NULL)
			badarg(argv[o]);
		o++;
	}
	if ((sigset(SIGINT, SIG_IGN)) != SIG_IGN)
		sigset(SIGINT, onint);
	prepare();
	dd();
	quit(0);
	/*NOTREACHED*/
	return 0;
}
