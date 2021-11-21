/*
 * cpio - copy file archives in and out
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, April 2003.
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

/*	Sccsid @(#)cpio.h	1.29 (gritter) 3/26/07	*/

#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>

enum	{
	FMT_NONE	= 00000000,	/* no format chosen yet */

	TYP_PAX		= 00000010,	/* uses pax-like extended headers */
	TYP_BE		= 00000100,	/* this binary archive is big-endian */
	TYP_SGI		= 00000200,	/* SGI cpio -K flag binary archive */
	TYP_SCO		= 00000200,	/* SCO UnixWare 7.1 extended archive */
	TYP_CRC		= 00000400,	/* this has a SVR4 'crc' checksum */
	TYP_BINARY	= 00001000,	/* this is a binary cpio type */
	TYP_OCPIO	= 00002000,	/* this is an old cpio type */
	TYP_NCPIO	= 00004000,	/* this is a SVR4 cpio type */
	TYP_CRAY	= 00010000,	/* this is a Cray cpio archive */
	TYP_CPIO	= 00077000,	/* this is a cpio type */
	TYP_OTAR	= 00100000,	/* this is an old tar type */
	TYP_USTAR	= 00200000,	/* this is a ustar type */
	TYP_BAR		= 00400000,	/* this is a bar type */
	TYP_TAR		= 00700000,	/* this is a tar type */

	FMT_ODC		= 00002001,	/* POSIX ASCII cpio format */
	FMT_DEC		= 00002002,	/* DEC extended cpio format */
	FMT_BINLE	= 00003001,	/* binary (default) cpio format LE */
	FMT_BINBE	= 00003101,	/* binary (default) cpio format BE */
	FMT_SGILE	= 00003201,	/* IRIX-style -K binary format LE */
	FMT_SGIBE	= 00003301,	/* IRIX-style -K binary format BE */
	FMT_ASC		= 00004001,	/* SVR4 ASCII cpio format */
	FMT_SCOASC	= 00004201,	/* UnixWare 7.1 ASCII cpio format */
	FMT_CRC		= 00004401,	/* SVR4 ASCII cpio format w/checksum */
	FMT_SCOCRC	= 00004601,	/* UnixWare 7.1 ASCII cpio w/checksum */
	FMT_CRAY	= 00010001,	/* Cray cpio, UNICOS 6 and later */
	FMT_CRAY5	= 00010002,	/* Cray cpio, UNICOS 5 and earlier */
	FMT_OTAR	= 00100001,	/* obsolete tar format */
	FMT_TAR		= 00200001,	/* our tar format type */
	FMT_USTAR	= 00200002,	/* ustar format */
	FMT_GNUTAR	= 00200003,	/* GNU tar format type */
	FMT_PAX		= 00200011,	/* POSIX.1-2001 pax format type */
	FMT_SUN		= 00200012,	/* Sun extended tar format type */
	FMT_BAR		= 00400001,	/* bar format type */

	FMT_ZIP		= 01000000	/* zip format */
} fmttype;

/*
 * Zip compression method.
 */
enum	cmethod {
	C_STORED	= 0,		/* no compression */
	C_SHRUNK	= 1,
	C_REDUCED1	= 2,
	C_REDUCED2	= 3,
	C_REDUCED3	= 4,
	C_REDUCED4	= 5,
	C_IMPLODED	= 6,
	C_TOKENIZED	= 7,
	C_DEFLATED	= 8,
	C_ENHDEFLD	= 9,
	C_DCLIMPLODED	= 10,
	C_PKRESERVED	= 11,
	C_BZIP2		= 12,
};

/*
 * A collection of the interesting facts about a file in copy-in mode.
 */
struct	file {
	struct stat	f_st;		/* file stat */
	long long	f_rmajor;	/* st_rdev major */
	long long	f_rminor;	/* st_rdev minor */
	long long	f_dsize;	/* display size */
	long long	f_csize;	/* compressed size */
	long long	f_Kbase;	/* base size for -K */
	long long	f_Krest;	/* rest size for -K */
	long long	f_Ksize;	/* faked -K size field */
	char		*f_name;	/* file name */
	size_t		f_nsiz;		/* file name size */
	char		*f_lnam;	/* link name */
	size_t		f_lsiz;		/* link name size */
	uint32_t	f_chksum;	/* checksum */
	int		f_pad;		/* padding size */
	int		f_fd;		/* file descriptor (for pass mode) */
	enum cmethod	f_cmethod;	/* zip compression method */
	enum {
		FG_CRYPT	= 0001,	/* encrypted zip file */
		FG_BIT1		= 0002,
		FG_BIT2		= 0004,
		FG_DESC		= 0010	/* zip file with data descriptor */
	}		f_gflag;	/* zip general flag */
	enum {
		OF_ZIP64	= 0001	/* is a zip64 archive entry */
	}		f_oflag;	/* other flags */
};

/*
 * Patterns for gmatch().
 */
struct glist {
	struct glist	*g_nxt;
	const char	*g_pat;
	unsigned	g_gotcha : 1;
	unsigned	g_not    : 1;
	unsigned	g_art    : 1;
};

extern int		aflag;
extern int		Aflag;
extern int		bflag;
extern int		Bflag;
extern int		cflag;
extern int		Cflag;
extern int		dflag;
extern int		Dflag;
extern int		eflag;
extern int		cray_eflag;
extern const char	*Eflag;
extern int		fflag;
extern int		Hflag;
extern const char	*Iflag;
extern int		kflag;
extern int		Kflag;
extern int		lflag;
extern int		Lflag;
extern int		mflag;
extern const char	*Mflag;
extern const char	*Oflag;
extern int		Pflag;
extern int		rflag;
extern const char	*Rflag;
extern int		sflag;
extern int		Sflag;
extern int		tflag;
extern int		uflag;
extern int		hp_Uflag;
extern int		vflag;
extern int		Vflag;
extern int		sixflag;
extern int		action;
extern long long	errcnt;
extern int		blksiz;
extern int		sysv3;
extern int		printsev;
extern char		*progname;
extern struct glist	*patterns;

enum {			/* type of pax command this is */
	PAX_TYPE_CPIO		= 0,	/* not a pax command */
	PAX_TYPE_PAX1992	= 1,	/* POSIX.2 pax command */
	PAX_TYPE_PAX2001	= 2	/* POSIX.1-2001 pax command */
} pax;
extern int		pax_dflag;
extern int		pax_kflag;
extern int		pax_nflag;
extern int		pax_sflag;
extern int		pax_uflag;
extern int		pax_Xflag;

enum {
	PAX_P_NONE	= 0000,
	PAX_P_ATIME	= 0001,
	PAX_P_MTIME	= 0004,
	PAX_P_OWNER	= 0010,
	PAX_P_MODE	= 0020,
	PAX_P_EVERY	= 0400
} pax_preserve;

extern size_t		(*ofiles)(char **, size_t *);
extern void		(*prtime)(time_t);

extern ssize_t	bread(char *, size_t);
extern void	bunread(const char *, size_t);
extern void	swap(char *, size_t, int, int);
extern void	msg(int, int, const char *, ...);
extern void	emsg(int, const char *, ...);
extern void	unexeoa(void);
extern int	setfmt(char *);
extern char	*oneintfmt(const char *);
extern int	setreassign(const char *);
extern void	addg(const char *, int);
extern void	*srealloc(void *, size_t);
extern void	*smalloc(size_t);
extern void	*scalloc(size_t, size_t);
extern void	*svalloc(size_t, int);
extern char	*sstrdup(const char *);
extern int	pax_options(char *, int);

extern int	zipunshrink(struct file *, const char *, int, int, uint32_t *);
extern int	zipexplode(struct file *, const char *, int, int, uint32_t *);
extern int	zipexpand(struct file *, const char *, int, int, uint32_t *);
extern int	zipinflate(struct file *, const char *, int, int, uint32_t *);
extern int	zipunbz2(struct file *, const char *, int, int, uint32_t *);
extern int	zipblast(struct file *, const char *, int, int, uint32_t *);

extern uint32_t	zipcrc(uint32_t, const uint8_t *, size_t);

extern void	flags(int, char **);
extern void	usage(void);

extern int	pax_track(const char *, time_t);
extern void	pax_prlink(struct file *);
extern int	pax_sname(char **, size_t *);
extern void	pax_onexit(void);
