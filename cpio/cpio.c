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

/*
 * Sccsid @(#)cpio.c	1.303 (gritter) 4/14/07
 */

#include <sys/types.h>
#include <sys/stat.h>
#ifdef	__linux__
#if !defined (__UCLIBC__) && !defined (__dietlibc__)
#include <linux/fs.h>
#endif	/* !__UCLIBC__, !__dietlibc__ */
#include <linux/fd.h>
#undef	WNOHANG
#undef	WUNTRACED
#undef	P_ALL
#undef	P_PID
#undef	P_PGID
#ifdef	__dietlibc__
#undef	NR_OPEN
#undef	PATH_MAX
#endif	/* __dietlibc__ */
#endif	/* __linux__ */
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "sigset.h"
#include <time.h>
#include <utime.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <stdio.h>
#include <libgen.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <locale.h>
#include <ctype.h>
#include "memalign.h"

#if USE_ZLIB
#include <zlib.h>
#endif	/* USE_ZLIB */

#if USE_BZLIB
#include <bzlib.h>
#endif	/* USE_BZLIB */

#include <sys/ioctl.h>

#if defined (__linux__) || defined (__sun) || defined (__FreeBSD__) || \
	defined (__hpux) || defined (_AIX) || defined (__NetBSD__) || \
	defined (__OpenBSD__) || defined (__DragonFly__) || defined (__APPLE__)
#include <sys/mtio.h>
#else	/* SVR4.2MP */
#include <sys/scsi.h>
#include <sys/st01.h>
#endif	/* SVR4.2MP */

#include <iblok.h>
#include <sfile.h>
#include <atoll.h>

#ifdef	_AIX
#include <sys/sysmacros.h>
#endif	/* _AIX */

#ifndef	major
#include <sys/sysmacros.h>
#endif	/* !major */

#include "cpio.h"
#include "blast.h"

#ifdef	__GLIBC__
#ifdef	_IO_putc_unlocked
#undef	putc
#define	putc(c, f)	_IO_putc_unlocked(c, f)
#undef	putchar
#define	putchar(c)	_IO_putc_unlocked(c, stdout)
#endif	/* _IO_putc_unlocked */
#endif	/* __GLIBC__ */

/*
 * The cpio code assumes that all following S_IFMT bits are the same as
 * those of the mode fields in cpio headers. The only real Unix system
 * known to deviate from this de facto standard is UNICOS which uses
 * 0130000 for S_IFLNK. But this software does not run on UNICOS for
 * a variety of other reasons anyway, so this should not be of much
 * concern.
 */
#if	S_IFIFO	!= 0010000 || \
	S_IFCHR	!= 0020000 || \
	S_IFDIR	!= 0040000 || \
	S_IFBLK	!= 0060000 || \
	S_IFREG	!= 0100000 || \
	S_IFLNK	!= 0120000 || \
	S_IFSOCK!= 0140000 || \
	S_IFMT	!= 0170000
#error non-standard S_IFMT bits
#endif

/*
 * File types that are not present on all systems but that we want to
 * recognize nevertheless.
 */
#ifndef	S_IFDOOR
#define	S_IFDOOR	0150000		/* Solaris door */
#endif
#ifndef	S_IFNAM
#define	S_IFNAM		0050000		/* XENIX special named file */
#endif
#ifndef	S_INSEM
#define	S_INSEM		0x1		/* XENIX semaphore subtype of IFNAM */
#endif
#ifndef	S_INSHD
#define	S_INSHD		0x2		/* XENIX shared data subtype of IFNAM */
#endif
#ifndef	S_IFNWK
#define	S_IFNWK		0110000		/* HP-UX network special file */
#endif

#if defined (__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) || \
	defined (__DragonFly__) || defined (__APPLE__)
/*
 * For whatever reason, FreeBSD casts the return values of major() and
 * minor() to signed values so that normal limit comparisons will fail.
 */
static unsigned long
mymajor(long dev)
{
	return major(dev) & 0xFFFFFFFFUL;
}
#undef	major
#define	major(a)	mymajor(a)
static unsigned long
myminor(long dev)
{
	return minor(dev) & 0xFFFFFFFFUL;
}
#undef	minor
#define	minor(a)	myminor(a)
#endif	/* __FreeBSD__, __NetBSD__, __OpenBSD__, __DragonFly__, __APPLE__ */

/*
 * Device and inode counts in cpio formats are too small to store the
 * information used to detect hard links on today's systems. Keep track
 * of all devices and inodes and store fake counts in the archive.
 */
struct ilink {
	struct ilink	*l_nxt;	/* next link to same i-node */
	char		*l_nam;	/* link name */
	size_t		l_siz;	/* link name size */
};

struct islot {
	struct islot	*i_lln;	/* left link */
	struct islot 	*i_rln;	/* right link */
	struct ilink	*i_lnk;	/* links list */
	struct stat	*i_st;	/* stat information */
	char		*i_name;/* name of first link encountered */
	ino_t		i_ino;	/* real inode number */
	uint32_t	i_fino;	/* fake inode number */
	nlink_t		i_nlk;	/* number of remaining links */
};

struct dslot {
	struct dslot	*d_nxt;	/* next device */
	struct islot	*d_isl;	/* inode slots */
	uint32_t	d_cnt;	/* used inode number count */
	uint32_t	d_fake;	/* faked device id */
	dev_t		d_dev;	/* real device id */
};

union types2 {
	uint8_t		byte[2];
	uint16_t	sword;
};

union types4 {
	uint8_t		byte[4];
	uint16_t	sword[2];
	uint32_t	lword;
};

/*
 * Store and retrieve integers in a defined endian order.
 */
static uint16_t
ple16(const char *cp)
{
	return (uint16_t)(cp[0]&0377) +
		((uint16_t)(cp[1]&0377) << 8);
}

static uint16_t
pbe16(const char *cp)
{
	return (uint16_t)(cp[1]&0377) +
		((uint16_t)(cp[0]&0377) << 8);
}

static uint32_t
ple32(const char *cp)
{
	return (uint32_t)(cp[0]&0377) +
		((uint32_t)(cp[1]&0377) << 8) +
		((uint32_t)(cp[2]&0377) << 16) +
		((uint32_t)(cp[3]&0377) << 24);
}

static uint32_t
pbe32(const char *cp)
{
	return (uint32_t)(cp[3]&0377) +
		((uint32_t)(cp[2]&0377) << 8) +
		((uint32_t)(cp[1]&0377) << 16) +
		((uint32_t)(cp[0]&0377) << 24);
}

static uint32_t
pme32(const char *cp)
{
	return (uint32_t)(cp[2]&0377) +
		((uint32_t)(cp[3]&0377) << 8) +
		((uint32_t)(cp[0]&0377) << 16) +
		((uint32_t)(cp[1]&0377) << 24);
}

static uint64_t
ple64(const char *cp)
{
	return (uint64_t)(cp[0]&0377) +
		((uint64_t)(cp[1]&0377) << 8) +
		((uint64_t)(cp[2]&0377) << 16) +
		((uint64_t)(cp[3]&0377) << 24) +
		((uint64_t)(cp[4]&0377) << 32) +
		((uint64_t)(cp[5]&0377) << 40) +
		((uint64_t)(cp[6]&0377) << 48) +
		((uint64_t)(cp[7]&0377) << 56);
}

static uint64_t
pbe64(const char *cp)
{
	return (uint64_t)(cp[7]&0377) +
		((uint64_t)(cp[6]&0377) << 8) +
		((uint64_t)(cp[5]&0377) << 16) +
		((uint64_t)(cp[4]&0377) << 24) +
		((uint64_t)(cp[3]&0377) << 32) +
		((uint64_t)(cp[2]&0377) << 40) +
		((uint64_t)(cp[1]&0377) << 48) +
		((uint64_t)(cp[0]&0377) << 56);
}

static void
le16p(uint16_t n, char *cp)
{
	cp[0] = (n&0x00ff);
	cp[1] = (n&0xff00) >> 8;
}

static void
be16p(uint16_t n, char *cp)
{
	cp[1] = (n&0x00ff);
	cp[0] = (n&0xff00) >> 8;
}

static void
le32p(uint32_t n, char *cp)
{
	cp[0] = (n&0x000000ff);
	cp[1] = (n&0x0000ff00) >> 8;
	cp[2] = (n&0x00ff0000) >> 16;
	cp[3] = (n&0xff000000) >> 24;
}

static void
be32p(uint32_t n, char *cp)
{
	cp[3] = (n&0x000000ff);
	cp[2] = (n&0x0000ff00) >> 8;
	cp[1] = (n&0x00ff0000) >> 16;
	cp[0] = (n&0xff000000) >> 24;
}

static void
me32p(uint32_t n, char *cp)
{
	cp[2] = (n&0x000000ff);
	cp[3] = (n&0x0000ff00) >> 8;
	cp[0] = (n&0x00ff0000) >> 16;
	cp[1] = (n&0xff000000) >> 24;
}

static void
le64p(uint64_t n, char *cp)
{
	cp[0] = (n&0x00000000000000ffLL);
	cp[1] = (n&0x000000000000ff00LL) >> 8;
	cp[2] = (n&0x0000000000ff0000LL) >> 16;
	cp[3] = (n&0x00000000ff000000LL) >> 24;
	cp[4] = (n&0x000000ff00000000LL) >> 32;
	cp[5] = (n&0x0000ff0000000000LL) >> 40;
	cp[6] = (n&0x00ff000000000000LL) >> 48;
	cp[7] = (n&0xff00000000000000LL) >> 56;
}

static void
be64p(uint64_t n, char *cp)
{
	cp[7] = (n&0x00000000000000ffLL);
	cp[6] = (n&0x000000000000ff00LL) >> 8;
	cp[5] = (n&0x0000000000ff0000LL) >> 16;
	cp[4] = (n&0x00000000ff000000LL) >> 24;
	cp[3] = (n&0x000000ff00000000LL) >> 32;
	cp[2] = (n&0x0000ff0000000000LL) >> 40;
	cp[1] = (n&0x00ff000000000000LL) >> 48;
	cp[0] = (n&0xff00000000000000LL) >> 56;
}

#define	TNAMSIZ	100
#define	TPFXSIZ	155
#define	TMAGSIZ	6
#define	TTIMSIZ	12

/*
 * Structure of an archive header.
 */
union bincpio {
	char		data[4096];
#define	SIZEOF_hdr_cpio	26
	struct hdr_cpio {
		char		c_magic[2];
		char		c_dev[2];
		char		c_ino[2];
		char		c_mode[2];
		char		c_uid[2];
		char		c_gid[2];
		char		c_nlink[2];
		char		c_rdev[2];
		char		c_mtime[4];
		char		c_namesize[2];
		char		c_filesize[4];
	} Hdr;
#define	SIZEOF_cray_hdr	152
	struct cray_hdr {	/* with thanks to Cray-Cyber.org */
		char		C_magic[8];
		char		C_dev[8];
		char		C_ino[8];
		char		C_mode[8];
		char		C_uid[8];
		char		C_gid[8];
		char		C_nlink[8];
		char		C_rdev[8];
				/*
				 * The C_param field was introduced with
				 * UNICOS 6 and is simply not present in
				 * the earlier format. Following fields
				 * are the same for both revisions again.
				 */
#define	CRAY_PARAMSZ	(8*8)
		char		C_param[CRAY_PARAMSZ];
		char		C_mtime[8];
		char		C_namesize[8];
		char		C_filesize[8];
	} Crayhdr;
#define	SIZEOF_c_hdr	76
	struct c_hdr {
		char		c_magic[6];
		char		c_dev[6];
		char		c_ino[6];
		char		c_mode[6];
		char		c_uid[6];
		char		c_gid[6];
		char		c_nlink[6];
		char		c_rdev[6];
		char		c_mtime[11];
		char		c_namesz[6];
		char		c_filesz[11];
	} Cdr;
#define	SIZEOF_d_hdr	86
	struct d_hdr {
		char		d_magic[6];
		char		d_dev[6];
		char		d_ino[6];
		char		d_mode[6];
		char		d_uid[6];
		char		d_gid[6];
		char		d_nlink[6];
		char		d_rmaj[8];
		char		d_rmin[8];
		char		d_mtime[11];
		char		d_namesz[6];
		char		d_filesz[11];
	} Ddr;
#define	SIZEOF_Exp_cpio_hdr	110
	struct Exp_cpio_hdr {
		char		E_magic[6];
		char		E_ino[8];
		char		E_mode[8];
		char		E_uid[8];
		char		E_gid[8];
		char		E_nlink[8];
		char		E_mtime[8];
		char		E_filesize[8];
		char		E_maj[8];
		char		E_min[8];
		char		E_rmaj[8];
		char		E_rmin[8];
		char		E_namesize[8];
		char		E_chksum[8];
	} Edr;
	struct tar_header {
		char		t_name[TNAMSIZ];
		char		t_mode[8];
		char		t_uid[8];
		char		t_gid[8];
		char		t_size[12];
		char		t_mtime[TTIMSIZ];
		char		t_chksum[8];
		char		t_linkflag;
		char		t_linkname[TNAMSIZ];
		char		t_magic[TMAGSIZ];
		char		t_version[2];
		char		t_uname[32];
		char		t_gname[32];
		char		t_devmajor[8];
		char		t_devminor[8];
		char		t_prefix[TPFXSIZ];
	} Tdr;
#define	SIZEOF_bar_header	84
	struct bar_header {
		char		b_mode[8];
		char		b_uid[8];
		char		b_gid[8];
		char		b_size[12];
		char		b_mtime[12];
		char		b_chksum[8];
		char		b_rdev[8];
		char		b_linkflag;
		char		b_bar_magic[2];
		char		b_volume_num[4];
		char		b_compressed;
		char		b_date[12];
	} Bdr;
#define	SIZEOF_zip_header	30
	struct zip_header {
		char		z_signature[4];
		char		z_version[2];
		char		z_gflag[2];
		char		z_cmethod[2];
		char		z_modtime[2];
		char		z_moddate[2];
		char		z_crc32[4];
		char		z_csize[4];
		char		z_nsize[4];
		char		z_namelen[2];
		char		z_extralen[2];
	} Zdr;
};

#define	BCUT	0177777
#define	OCUT	0777777
#define	ECUT	0xFFFFFFFFUL

static const char	trailer[] = "TRAILER!!!";

/*
 * Structure of per-file extra data for zip format.
 */
union zextra {
	char	data[1];
#define	SIZEOF_zextra_gn	4
	struct	zextra_gn {
		char	ze_gn_tag[2];
		char	ze_gn_tsize[2];
	} Ze_gn;
#define	SIZEOF_zextra_64	32		/* regular size */
#define	SIZEOF_zextra_64_a	28		/* size without startn field */
#define	SIZEOF_zextra_64_b	20		/* size without reloff field */
	struct	zextra_64 {
		char	ze_64_tag[2];
		char	ze_64_tsize[2];
		char	ze_64_nsize[8];
		char	ze_64_csize[8];
		char	ze_64_reloff[8];
		char	ze_64_startn[4];
	} Ze_64;
#define	SIZEOF_zextra_pk	16
	struct	zextra_pk {
		char	ze_pk_tag[2];
		char	ze_pk_tsize[2];
		char	ze_pk_atime[4];
		char	ze_pk_mtime[4];
		char	ze_pk_uid[2];
		char	ze_pk_gid[2];
	} Ze_pk;
#define	SIZEOF_zextra_ek	17
	struct	zextra_et {
		char	ze_et_tag[2];
		char	ze_et_tsize[2];
		char	ze_et_flags[1];
		char	ze_et_mtime[4];
		char	ze_et_atime[4];
		char	ze_et_ctime[4];
	} Ze_et;
#define	SIZEOF_zextra_i1	16
	struct	zextra_i1 {
	char	ze_i1_tag[2];
		char	ze_i1_tsize[2];
		char	ze_i1_atime[4];
		char	ze_i1_mtime[4];
		char	ze_i1_uid[2];
		char	ze_i1_gid[2];
	} Ze_i1;
#define	SIZEOF_zextra_i2	8
	struct	zextra_i2 {
		char	ze_i2_tag[2];
		char	ze_i2_tsize[2];
		char	ze_i2_uid[2];
		char	ze_i2_gid[2];
	} Ze_i2;
#define	SIZEOF_zextra_as	16
	struct	zextra_as {
		char	ze_as_tag[2];
		char	ze_as_tsize[2];
		char	ze_as_crc[4];
		char	ze_as_mode[2];
		char	ze_as_sizdev[4];
		char	ze_as_uid[2];
		char	ze_as_gid[2];
	} Ze_as;
#define	SIZEOF_zextra_cp	40
	struct	zextra_cp {
		char	ze_cp_tag[2];
		char	ze_cp_tsize[2];
		char	ze_cp_dev[4];
		char	ze_cp_ino[4];
		char	ze_cp_mode[4];
		char	ze_cp_uid[4];
		char	ze_cp_gid[4];
		char	ze_cp_nlink[4];
		char	ze_cp_rdev[4];
		char	ze_cp_mtime[4];
		char	ze_cp_atime[4];
	} Ze_cp;
};

static struct	zipstuff {		/* stuff for central directory at EOF */
	struct zipstuff	*zs_next;
	char	*zs_name;		/* file name */
	long long	zs_size;	/* file size */
	long long	zs_relative;	/* offset of local header */
	long long	zs_csize;	/* compressed size */
	uint32_t	zs_crc32;	/* CRC */
	time_t		zs_mtime;	/* modification time */
	enum cmethod	zs_cmethod;	/* compression method */
	int		zs_gflag;	/* general flag */
	mode_t		zs_mode;	/* file mode */
} *zipbulk;

/*
 * Structure of the central zip directory at the end of the file. This
 * (obligatory) part of a zip file is written by this implementation,
 * but is completely ignored on copy-in. This means that we miss the
 * mode_t stored in zc_extralen if zc_versionmade[1] is 3 (Unix). We
 * have to do this since it contains the S_IFMT bits, thus telling us
 * whether something is a symbolic link and resulting in different
 * behavior - but as the input had to be seekable in order to do this,
 * we had to store entire archives in temporary files if input came
 * from a pipe to be consistent.
 */
#define	SIZEOF_zipcentral	46
struct	zipcentral {
	char	zc_signature[4];
	char	zc_versionmade[2];
	char	zc_versionextr[2];
	char	zc_gflag[2];
	char	zc_cmethod[2];
	char	zc_modtime[2];
	char	zc_moddate[2];
	char	zc_crc32[4];
	char	zc_csize[4];
	char	zc_nsize[4];
	char	zc_namelen[2];
	char	zc_extralen[2];
	char	zc_commentlen[2];
	char	zc_disknstart[2];
	char	zc_internal[2];
	char	zc_external[4];
	char	zc_relative[4];
};

#define	SIZEOF_zip64end		56
struct	zip64end {
	char	z6_signature[4];
	char	z6_recsize[8];
	char	z6_versionmade[2];
	char	z6_versionextr[2];
	char	z6_thisdiskn[4];
	char	z6_alldiskn[4];
	char	z6_thisentries[8];
	char	z6_allentries[8];
	char	z6_dirsize[8];
	char	z6_startsize[8];
};

#define	SIZEOF_zip64loc		20
struct	zip64loc {
	char	z4_signature[4];
	char	z4_startno[4];
	char	z4_reloff[8];
	char	z4_alldiskn[4];
};

#define	SIZEOF_zipend		22
struct	zipend {
	char	ze_signature[4];
	char	ze_thisdiskn[2];
	char	ze_alldiskn[2];
	char	ze_thisentries[2];
	char	ze_allentries[2];
	char	ze_dirsize[4];
	char	ze_startsize[4];
	char	ze_commentlen[2];
};

#define	SIZEOF_zipddesc		16
struct	zipddesc {
	char	zd_signature[4];
	char	zd_crc32[4];
	char	zd_csize[4];
	char	zd_nsize[4];
};

#define	SIZEOF_zipddesc64	24
struct	zipddesc64 {
	char	zd_signature[4];
	char	zd_crc32[4];
	char	zd_csize[8];
	char	zd_nsize[8];
};

/*
 * Magic numbers and strings.
 */
static const uint16_t	mag_bin = 070707;
/*static const uint16_t	mag_bbs = 0143561;*/
static const char	mag_asc[6] = "070701";
static const char	mag_crc[6] = "070702";
static const char	mag_odc[6] = "070707";
static const long	mag_sco = 0x7ffffe00;

static const char	mag_ustar[6] = "ustar\0";
static const char	mag_gnutar[8] = "ustar  \0";
static const char	mag_bar[2] = "V\0";

static const char	mag_zipctr[4] = "PK\1\2";
static const char	mag_zipsig[4] = "PK\3\4";
static const char	mag_zipend[4] = "PK\5\6";
static const char	mag_zip64e[4] = "PK\6\6";
static const char	mag_zip64l[4] = "PK\6\7";
static const char	mag_zipdds[4] = "PK\7\10";
#define			mag_zip64f	0x0001
#define			mag_zipcpio	0x0707

/*
 * Fields for the extended pax header.
 */
static enum paxrec {
	PR_NONE		= 0000,
	PR_ATIME	= 0001,
	PR_GID		= 0002,
	PR_LINKPATH	= 0004,
	PR_MTIME	= 0010,
	PR_PATH		= 0020,
	PR_SIZE		= 0040,
	PR_UID		= 0100,
	PR_SUN_DEVMAJOR	= 0200,
	PR_SUN_DEVMINOR	= 0400
} paxrec, globrec;

/*
 * Prototype structure, collecting user-defined information
 * about a file.
 */
struct prototype {
	mode_t	pt_mode;	/* type and permission bits */
	uid_t	pt_uid;		/* owner */
	gid_t	pt_gid;		/* group owner */
	time_t	pt_atime;	/* time of last access */
	time_t	pt_mtime;	/* time of last modification */
	dev_t	pt_rdev;	/* device major/minor */
	enum {
		PT_NONE	 = 0000,
		PT_TYPE	 = 0001,
		PT_OWNER = 0002,
		PT_GROUP = 0004,
		PT_MODE  = 0010,
		PT_ATIME = 0020,
		PT_MTIME = 0040,
		PT_RDEV  = 0100
	}	pt_spec;	/* specified information */
};

static struct stat	globst;

/*
 * This sets a sanity check limit on path names. If a longer path name
 * occurs in an archive, it is treated as corrupt. This is because no
 * known Unix system can handle path names of arbitrary length; limits
 * are typically between 1024 and 4096. Trying to extract longer path
 * names would fail anyway and will cpio eventually fail to allocate
 * memory.
 */
#define			SANELIMIT	0177777

char			*progname;	/* argv[0] to main() */
static struct dslot	*devices;	/* devices table */
static struct dslot	*markeddevs;	/* unusable device numbers */
static char		*blkbuf;	/* block buffer */
int			blksiz;		/* block buffer size */
static int		blktop;		/* top of filled part of buffer */
static int		curpos;		/* position in blkbuf */
static uint32_t		fakedev;	/* fake device for single link inodes */
static uint32_t		fakeino;	/* fake inode for single link inodes */
static uint32_t		harddev;	/* fake device used for hard links */
static unsigned long long	maxsize;/* maximum size for format */
static unsigned long long	maxrdev;/* maximum st_rdev for format */
static unsigned long long	maxmajor;/* maximum major(st_rdev) for format */
static unsigned long long	maxminor;/* maximum minor(st_rdev) for format */
static unsigned long long	maxuid;	/* maximum user id for format */
static unsigned long long	maxgid;	/* maximum group id for format */
static unsigned long long	maxnlink;/* maximum link count for format */
static int		mt;		/* magtape file descriptor */
static int		mfl;		/* magtape flags */
static struct stat	mtst;		/* fstat() on mt */
int			aflag;		/* reset access times */
int			Aflag;		/* append to archive */
int			bflag;		/* swap bytes */
int			Bflag;		/* 5120 blocking */
int			cflag;		/* ascii format */
int			Cflag;		/* user-defined blocking */
int			dflag;		/* create directories */
int			Dflag;		/* do not ask for next device */
int			eflag;		/* DEC format */
int			cray_eflag;	/* do not archive if values too large */
const char		*Eflag;		/* filename for files to be extracted */
int			fflag;		/* pattern excludes */
int			Hflag;		/* header format */
const char		*Iflag;		/* input archive name */
int			kflag;		/* skipt corrupted parts */
int			Kflag;		/* IRIX-style large file support */
int			lflag;		/* link of possible */
int			Lflag;		/* follow symbolic links */
int			mflag;		/* retain modification times */
const char		*Mflag;		/* message when switching media */
const char		*Oflag;		/* output archive name */
int			Pflag;		/* prototype file list */
int			rflag;		/* rename files */
const char		*Rflag;		/* reassign ownerships */
static uid_t		Ruid;		/* uid to assign */
static gid_t		Rgid;		/* gid to assign */
int			sflag;		/* swap half word bytes */
int			Sflag;		/* swap word bytes */
int			tflag;		/* print toc */
int			uflag;		/* overwrite files unconditionally */
int			hp_Uflag;	/* use umask when creating files */
int			vflag;		/* verbose */
int			Vflag;		/* special verbose */
int			sixflag;	/* 6th Edition archives */
int			action;		/* -i -o -p */
long long		errcnt;		/* error status */
static unsigned long long	maxpath;/* maximum path length with -i */
static uint32_t		maxino;		/* maximum inode number with -i */
static uid_t		myuid;		/* user id of caller */
static gid_t		mygid;		/* group id of caller */
static long long	blocks;		/* copying statistics: full blocks */
static long long	bytes;		/* copying statistics: partial blocks */
static long long	nwritten;	/* bytes written to archive */
static off_t		aoffs;		/* offset in archive */
static off_t		poffs;		/* physical offset in archive */
static int		tapeblock = -1;	/* physical tape block size */
struct glist		*patterns;	/* patterns for -i */
static int		tty;		/* terminal file descriptor */
static const char	*cur_ofile;	/* current original file */
static const char	*cur_tfile;	/* current temporary file */
static mode_t		umsk;		/* user's umask */
static int		zipclevel;	/* zip compression level */
static struct islot	*inull;		/* splay tree null element */
int			printsev;	/* print message severity strings */
static int		compressed_bar;	/* this is a compressed bar archive */
static int		formatforced;	/* -k -i -Hfmt forces a format */
static long long	lineno;		/* input line number */

int			pax_dflag;	/* directory matches only itself */
int			pax_kflag;	/* do not overwrite files */
int			pax_nflag;	/* select first archive member only */
int			pax_sflag;	/* substitute file names */
int			pax_uflag;	/* add only recent files to archive */
int			pax_Xflag;	/* do not cross device boundaries */
static enum {
	PO_NONE		= 0,
	PO_LINKDATA	= 01,		/* include link data in type 2 */
	PO_TIMES	= 02,		/* create atime and mtime fields */
} pax_oflag;				/* recognized -o options */

static void	copyout(int (*)(const char *, struct stat *));
static size_t	ofiles_cpio(char **, size_t *);
static void	dooutp(void);
static int	outfile(const char *, struct stat *);
static int	addfile(const char *, struct stat *, uint32_t, uint32_t, int,
			const char *);
static void	iflush(struct islot *, uint32_t);
static void	lflush(void);
static int	bigendian(void);
static void	getbuf(char **, size_t *, size_t);
static void	prdot(int);
static void	newmedia(int);
static void	mclose(void);
static ssize_t	mwrite(int);
static void	bwrite(const char *, size_t);
static void	bflush(void);
static int	sum(int, const char *, struct stat *, char *);
static int	rstime(const char *, struct stat *, const char *);
static struct islot	*isplay(ino_t, struct islot *);
static struct islot	*ifind(ino_t, struct islot **);
static void	iput(struct islot *, struct islot **);
static struct dslot	*dfind(struct dslot **, dev_t);
static void	done(int);
static void	dopass(const char *);
static int	passdata(struct file *, const char *, int);
static int	passfile(const char *, struct stat *);
static int	filein(struct file *, int (*)(struct file *, const char *, int),
			char *);
static int	linkunlink(const char *, const char *);
static void	tunlink(char **);
static int	filet(struct file *, int (*)(struct file *, const char *, int));
static void	filev(struct file *);
static int	typec(struct stat *);
static void	permbits(mode_t);
static void	prtime_cpio(time_t);
static void	getpath(const char *, char **, char **, size_t *, size_t *);
static void	setpath(const char *, char **, char **,
			size_t, size_t *, size_t *);
static int	imdir(char *);
static int	setattr(const char *, struct stat *);
static int	setowner(const char *, struct stat *);
static int	canlink(const char *, struct stat *, int);
static void	doinp(void);
static void	storelink(struct file *);
static void	flushlinks(struct file *);
static void	flushnode(struct islot *, struct file *);
static void	flushrest(int);
static void	flushtree(struct islot *, int);
static int	inpone(struct file *, int);
static int	readhdr(struct file *, union bincpio *);
static void	whathdr(void);
static int	infile(struct file *);
static int	skipfile(struct file *);
static int	skipdata(struct file *f,
			int (*)(struct file *, const char *, int));
static int	indata(struct file *, const char *, int);
static int	totrailer(void);
static long long	rdoct(const char *, int);
static long long	rdhex(const char *, int);
static ssize_t	mread(void);
static void	mstat(void);
static int	skippad(unsigned long long, int);
static int	allzero(const char *, int);
static const char	*getuser(uid_t);
static const char	*getgroup(gid_t);
static struct glist	*want(struct file *, struct glist **);
static void	patfile(void);
static int	ckodd(long long, int, const char *, const char *);
static int	rname(char **, size_t *);
static int	redirect(const char *, const char *);
static char	*tnameof(struct tar_header *, char *);
static int	tmkname(struct tar_header *, const char *);
static void	tlinkof(struct tar_header *, struct file *);
static int	tmklink(struct tar_header *, const char *);
static int	tlflag(struct stat *);
static void	tchksum(union bincpio *);
static int	tcssum(union bincpio *, int);
static int	trdsum(union bincpio *);
static mode_t	tifmt(int);
static void	bchksum(union bincpio *);
static int	bcssum(union bincpio *);
static void	blinkof(const char *, struct file *, int);
static void	dump_barhdr(void);
static int	zcreat(const char *, mode_t);
static int	zclose(int);
static void	markdev(dev_t);
static int	marked(dev_t);
static void	cantsup(int, const char *);
static void	onint(int);
static int	zipread(struct file *, const char *, int, int);
static void	zipreaddesc(struct file *);
static int	cantunzip(struct file *, const char *);
static time_t	gdostime(const char *, const char *);
static void	mkdostime(time_t, char *, char *);
static ssize_t	ziprxtra(struct file *, struct zip_header *);
static void	ziptrailer(void);
static void	zipdefer(const char *, struct stat *, long long,
			uint32_t, long long, const struct zip_header *);
static int	zipwrite(int, const char *, struct stat *,
			union bincpio *, size_t, uint32_t, uint32_t,
			uint32_t *, long long *);
static int	zipwtemp(int, const char *, struct stat *,
			union bincpio *, size_t, uint32_t, uint32_t,
			uint32_t *, long long *);
#if USE_ZLIB
static int	zipwdesc(int, const char *, struct stat *,
			union bincpio *, size_t, uint32_t, uint32_t,
			uint32_t *, long long *);
#endif	/* USE_ZLIB */
static int	zipwxtra(const char *, struct stat *, uint32_t, uint32_t);
static void	zipinfo(struct file *);
static void	readK2hdr(struct file *);
static int	readgnuname(char **, size_t *, long);
static void	writegnuname(const char *, long, int);
static void	tgetpax(struct tar_header *, struct file *);
static enum paxrec	tgetrec(char **, char **, char **);
static void	wrpax(const char *, const char *, struct stat *);
static void	addrec(char **, long *, long *,
			const char *, const char *, long long);
static void	paxnam(struct tar_header *, const char *);
static char	*sequence(void);
static char	*joinpath(const char *, char *);
static int	utf8(const char *);
static char	*getproto(char *, struct prototype *);

size_t		(*ofiles)(char **, size_t *) = ofiles_cpio;
void		(*prtime)(time_t) = prtime_cpio;

int
main(int argc, char **argv)
{
	myuid = getuid();
	mygid = getgid();
	umask(umsk = umask(0));
	progname = basename(argv[0]);
	setlocale(LC_CTYPE, "");
	setlocale(LC_TIME, "");
	inull = scalloc(1, sizeof *inull);
	inull->i_lln = inull->i_rln = inull;
	flags(argc, argv);
	switch (action) {
	case 'i':
		if (sigset(SIGINT, SIG_IGN) != SIG_IGN)
			sigset(SIGINT, onint);
		doinp();
		break;
	case 'o':
		dooutp();
		break;
	case 'p':
		if (sigset(SIGINT, SIG_IGN) != SIG_IGN)
			sigset(SIGINT, onint);
		dopass(argv[optind]);
		break;
	}
	if (tflag)
		fflush(stdout);
	else if (Vflag)
		prdot(1);
	if (pax != PAX_TYPE_CPIO)
		pax_onexit();
	fprintf(stderr, "%llu blocks\n", blocks + ((bytes + 0777) >> 9));
	mclose();
	if (errcnt && sysv3 == 0)
		fprintf(stderr, "%llu error(s)\n", errcnt);
	return errcnt ? sysv3 ? 1 : 2 : 0;
}

static size_t
ofiles_cpio(char **name, size_t *namsiz)
{
	static struct iblok	*ip;

	if (ip == NULL)
		ip = ib_alloc(0, 0);
	return ib_getlin(ip, name, namsiz, srealloc);
}

/*
 * Read the file name list for -o and -p and do initial processing
 * for each name.
 */
static void
copyout(int (*copyfn)(const char *, struct stat *))
{
	char	*name = NULL, *np;
	size_t	namsiz = 0, namlen;
	struct stat	st;
	struct prototype	pt;

	while ((namlen = ofiles(&name, &namsiz)) != 0) {
		lineno++;
		if (name[namlen-1] == '\n')
			name[--namlen] = '\0';
		if (Pflag)
			np = getproto(name, &pt);
		else
			np = name;
		while (np[0] == '.' && np[1] == '/') {
			np += 2;
			while (*np == '/')
				np++;
			if (*np == '\0') {
				np = name;
				break;
			}
		}
		if (lstat(np, &st) < 0) {
			if (Pflag && *np && ((pt.pt_spec &
			    (PT_TYPE|PT_OWNER|PT_GROUP|PT_MODE|PT_RDEV) &&
					((pt.pt_mode&S_IFMT) == S_IFBLK ||
					 (pt.pt_mode&S_IFMT) == S_IFCHR)) ||
				      (pt.pt_spec &
		            (PT_TYPE|PT_OWNER|PT_GROUP|PT_MODE) &&
			    		((pt.pt_mode&S_IFMT) == S_IFDIR ||
					 (pt.pt_mode&S_IFMT) == S_IFIFO ||
					 (pt.pt_mode&S_IFMT) == S_IFREG)))) {
				memset(&st, 0, sizeof st);
				st.st_mode = pt.pt_mode;
				st.st_blksize = 4096;
				st.st_nlink = 1;
				goto missingok;
			}
			else if (sysv3 < 0)
				msg(2, 0, "< %s > ?\n", np);
			else if (sysv3 > 0)
				msg(2, 0, "Cannot obtain information "
						"about file:  \"%s\".\n",
						np);
			else
				emsg(2, "Error with lstat of \"%s\"", np);
			errcnt++;
			continue;
		}
	missingok:
		if (Lflag && (st.st_mode&S_IFMT) == S_IFLNK) {
			if (stat(np, &st) < 0) {
				emsg(2, "Cannot follow \"%s\"", np);
				errcnt++;
				continue;
			}
		}
		/*
		 * These file types are essentially useless in an archive
		 * since they are recreated by any process that needs them.
		 * We thus ignore them and do not even issue a warning,
		 * because that would only displace more important messages
		 * on a terminal and confuse people who just want to copy
		 * directory hierarchies.--But for pax, POSIX.1-2001 requires
		 * us to fail!
		 */
		if ((st.st_mode&S_IFMT) == S_IFSOCK ||
				(st.st_mode&S_IFMT) == S_IFDOOR) {
			if (pax >= PAX_TYPE_PAX2001) {
				msg(2, 0, "Cannot handle %s \"%s\".\n",
					(st.st_mode&S_IFMT) == S_IFSOCK ?
						"socket" : "door", np);
				errcnt++;
			}
			continue;
		}
		if (Pflag) {
			if (pt.pt_spec & PT_TYPE)
				if ((st.st_mode&S_IFMT) != (pt.pt_mode&S_IFMT))
					msg(4, 0, "line %lld: types "
						"do not match\n", lineno);
			if (pt.pt_spec & PT_OWNER)
				st.st_uid = pt.pt_uid;
			if (pt.pt_spec & PT_GROUP)
				st.st_gid = pt.pt_gid;
			if (pt.pt_spec & PT_MODE) {
				st.st_mode &= ~(mode_t)07777;
				st.st_mode |= pt.pt_mode;
			}
			if (pt.pt_spec & PT_ATIME)
				st.st_atime = pt.pt_atime;
			if (pt.pt_spec & PT_MTIME)
				st.st_mtime = pt.pt_mtime;
			if (pt.pt_spec & PT_RDEV) {
				if ((st.st_mode&S_IFMT) != S_IFBLK &&
				    (st.st_mode&S_IFMT) != S_IFCHR)
					msg(4, 0, "line %lld: device type "
						"specified for non-device "
						"file\n", lineno);
				st.st_rdev = pt.pt_rdev;
			}
		}
		if (pax_track(np, st.st_mtime) == 0)
			continue;
		if ((fmttype == FMT_ZIP ||
					fmttype & TYP_BAR ||
					fmttype == FMT_GNUTAR)
				&& (st.st_mode&S_IFMT) == S_IFDIR &&
				name[namlen-1] != '/') {
			if (namlen+2 >= namsiz) {
				size_t	diff = np - name;
				name = srealloc(name, namsiz = namlen+2);
				np = &name[diff];
			}
			name[namlen++] = '/';
			name[namlen] = '\0';
		}
		errcnt += copyfn(np, &st);
	}
}

/*
 * Execution for -o.
 */
static void
dooutp(void)
{
	if (Oflag) {
		if ((mt = Aflag ? open(Oflag, O_RDWR, 0666) :
					creat(Oflag, 0666)) < 0) {
			if (sysv3) {
				emsg(013, "Cannot open <%s> for %s.", Oflag,
					Aflag ? "append" : "output");
				done(1);
			} else
				msg(3, -2, "Cannot open \"%s\" for %s\n", Oflag,
					Aflag ? "append" : "output");
		}
	} else
		mt = dup(1);
	mstat();
	blkbuf = svalloc(blksiz, 1);
	if (Aflag) {
		if (totrailer() != 0)
			return;
	} else if (fmttype == FMT_NONE)
		fmttype = bigendian() ? FMT_BINBE : FMT_BINLE;
	if (fmttype & TYP_BINARY) {
		maxino = 0177777;
		fakeino = 0177777;
		maxpath = 256;
		if (fmttype & TYP_SGI) {
			maxsize = 0x7FFFFFFFFFFFFFFFLL;
			maxmajor = 037777;
			maxminor = 0777777;
		} else {
			maxsize = 0x7FFFFFFFLL;
			maxrdev = 0177777;
		}
		maxuid = 0177777;
		maxgid = 0177777;
		maxnlink = 0177777;
	} else if (fmttype == FMT_ODC) {
		maxino = 0777777;
		fakeino = 0777777;
		maxpath = 256;
		maxsize = 077777777777LL;
		maxrdev = 0777777;
		maxuid = 0777777;
		maxgid = 0777777;
		maxnlink = 0777777;
	} else if (fmttype == FMT_DEC) {
		maxino = 0777777;
		fakeino = 0777777;
		maxpath = 256;
		maxsize = 077777777777LL;
		maxmajor = 077777777;
		maxminor = 077777777;
		maxuid = 0777777;
		maxgid = 0777777;
		maxnlink = 0777777;
	} else if (fmttype & TYP_NCPIO) {
		maxino = 0xFFFFFFFFUL;
		fakeino = 0xFFFFFFFFUL;
		maxpath = 1024;
		maxsize = fmttype&TYP_SCO ? 0x7FFFFFFFFFFFFFFFLL : 0xFFFFFFFFUL;
		maxmajor = 0xFFFFFFFFUL;
		maxminor = 0xFFFFFFFFUL;
		maxuid = 0xFFFFFFFFUL;
		maxgid = 0xFFFFFFFFUL;
		maxnlink = 0xFFFFFFFFUL;
	} else if (fmttype & TYP_CRAY) {
		maxino = 0xFFFFFFFFUL;
		fakeino = 0xFFFFFFFFUL;
		maxpath = SANELIMIT;
		maxsize = 0x7FFFFFFFFFFFFFFFLL;
		maxrdev = 0x7FFFFFFFFFFFFFFFLL;
		maxuid = 0x7FFFFFFFFFFFFFFFLL;
		maxgid = 0x7FFFFFFFFFFFFFFFLL;
		maxnlink = 0x7FFFFFFFFFFFFFFFLL;
	} else if (fmttype == FMT_GNUTAR) {
		maxino = 0xFFFFFFFFUL;
		fakeino = 0xFFFFFFFFUL;
		maxpath = SANELIMIT;
		maxsize = 0x7FFFFFFFFFFFFFFFLL;
		maxmajor = 0x7FFFFFFFFFFFFFFFLL;
		maxminor = 0x7FFFFFFFFFFFFFFFLL;
		maxuid = 0x7FFFFFFFFFFFFFFFLL;
		maxgid = 0x7FFFFFFFFFFFFFFFLL;
		maxnlink = 0x7FFFFFFFFFFFFFFFLL;
	} else if (fmttype & TYP_PAX) {
		maxino = 0xFFFFFFFFUL;
		fakeino = 0xFFFFFFFFUL;
		maxpath = SANELIMIT;
		maxsize = 0x7FFFFFFFFFFFFFFFLL;
		maxmajor = fmttype==FMT_SUN ? 0x7FFFFFFFFFFFFFFFLL : 07777777;
		maxminor = fmttype==FMT_SUN ? 0x7FFFFFFFFFFFFFFFLL : 07777777;
		maxuid = 0x7FFFFFFFFFFFFFFFLL;
		maxgid = 0x7FFFFFFFFFFFFFFFLL;
		maxnlink = 0x7FFFFFFFFFFFFFFFLL;
		if (pax_oflag & PO_TIMES)
			globrec |= PR_ATIME|PR_MTIME;
	} else if (fmttype & TYP_BAR) {
		maxino = 0xFFFFFFFFUL;
		fakeino = 0xFFFFFFFFUL;
		maxpath = 512 - SIZEOF_bar_header - 1;
		maxsize = 077777777777LL;
		maxrdev = 07777777;
		maxuid = 07777777;
		maxgid = 07777777;
		maxnlink = 0x7FFFFFFFFFFFFFFFLL;
		if (nwritten == 0)
			dump_barhdr();
	} else if (fmttype & TYP_USTAR) {
		maxino = 0xFFFFFFFFUL;
		fakeino = 0xFFFFFFFFUL;
		maxpath = 256;
		maxsize = 077777777777LL;
		maxmajor = 07777777;
		maxminor = 07777777;
		maxuid = 07777777;
		maxgid = 07777777;
		maxnlink = 0x7FFFFFFFFFFFFFFFLL;
	} else if (fmttype & TYP_OTAR) {
		maxino = 0xFFFFFFFFUL;
		fakeino = 0xFFFFFFFFUL;
		maxpath = 99;
		maxsize = 077777777777LL;
		maxuid = 07777777;
		maxgid = 07777777;
		maxnlink = 0x7FFFFFFFFFFFFFFFLL;
	} else if (fmttype == FMT_ZIP) {
		maxino = 0xFFFFFFFFUL;
		fakeino = 0xFFFFFFFFUL;
		maxpath = 60000;
		maxsize = 0x7FFFFFFFFFFFFFFFLL;
		maxrdev = 0xFFFFFFFFUL;
		maxuid = 0xFFFFFFFFUL;
		maxgid = 0xFFFFFFFFUL;
		maxnlink = 0xFFFFFFFFUL;
	} else
		abort();
	fakedev = 0177777;
	harddev = 1;
	copyout(outfile);
	if (fmttype & TYP_NCPIO)
		lflush();
	if (fmttype & TYP_CPIO) {
		struct stat	st;

		memset(&st, 0, sizeof st);
		st.st_nlink = 1;
		outfile(trailer, &st);
	}
	if (fmttype & TYP_TAR) {
		char b[512];

		memset(b, 0, sizeof b);
		bwrite(b, sizeof b);
		bwrite(b, sizeof b);
	}
	if (fmttype == FMT_ZIP)
		ziptrailer();
	bflush();
}

/*
 * Handle a single file for -o, do sanity checks and detect hard links.
 */
static int
outfile(const char *file, struct stat *st)
{
	uint32_t dev, ino;
	size_t pathsz;

	if ((st->st_mode&S_IFMT) == S_IFREG)
		if (mtst.st_dev == st->st_dev && mtst.st_ino == st->st_ino)
			return 0;
	if (st->st_size > maxsize) {
		msg(2, 0, "Size of %c%s%c >%lluGB. Not dumped\n",
				sysv3 ? '<' : '"',
				file,
				sysv3 ? '>' : '"',
				(maxsize+1) / (1024*1024*1024));
		return 1;
	}
	if (((st->st_mode&S_IFMT)==S_IFBLK||(st->st_mode&S_IFMT)==S_IFCHR) &&
		(maxrdev &&
			(unsigned long long)st->st_rdev > maxrdev ||
		maxmajor &&
			(unsigned long long)major(st->st_rdev) > maxmajor ||
		maxminor &&
			(unsigned long long)minor(st->st_rdev) > maxminor)) {
		cantsup(1, file);
		return 1;
	}
	if ((unsigned long long)st->st_uid > maxuid) {
		if (cray_eflag) {
			cantsup(1, file);
			return 1;
		}
		cantsup(0, file);
		st->st_uid = 60001;
		if ((st->st_mode&S_IFMT) == S_IFREG && st->st_mode & 0111)
			st->st_mode &= ~(mode_t)S_ISUID;
		if ((unsigned long long)st->st_gid > maxgid) {
			st->st_gid = 60001;
			if ((st->st_mode&S_IFMT)==S_IFREG && st->st_mode&0010)
				st->st_mode &= ~(mode_t)S_ISGID;
		}
	} else if ((unsigned long long)st->st_gid > maxgid) {
		if (cray_eflag) {
			cantsup(1, file);
			return 1;
		}
		cantsup(0, file);
		st->st_gid = 60001;
		if ((st->st_mode&S_IFMT) == S_IFREG && st->st_mode & 0010)
			st->st_mode &= ~(mode_t)S_ISGID;
	}
	if ((pathsz = strlen(file)) > maxpath) {
		msg(2, 0, "%s: file name too long\n", file);
		return 1;
	}
	/*
	 * Detect hard links and compute fake inode counts. The mechanism
	 * is as follows: If a file has more than one link, a fake device
	 * number starting at one is used for its device, and a fake inode
	 * number is used starting at one too.
	 *
	 * The information on links of directories is useless, so it is
	 * dropped and handled like a file with a single link only: Fake
	 * devices are allocated just below the format's limit, fake
	 * i-nodes the same.
	 *
	 * This way even the binary cpio format can have up to ~4G files.
	 */
	if (maxino && st->st_nlink > 1 && (st->st_mode&S_IFMT) != S_IFDIR) {
		struct dslot *ds, *dp;
		struct islot *ip;

		dev = 1;
		ds = devices;
		dp = NULL;
nextdev:
		for (; ds; dp = ds, ds = ds->d_nxt, dev++ /* see below! */)
			if (ds->d_dev == st->st_dev)
				break;
		if (markeddevs && marked(dev)) {
			dev++;
			goto nextdev;
		}
		if (dev >= fakedev)
			msg(4, 1, "Too many devices in archive, exiting\n");
		if (ds == NULL) {
			ds = scalloc(1, sizeof *ds);
			ds->d_dev = st->st_dev;
			ds->d_fake = dev;
			if (devices == NULL)
				devices = ds;
			else
				dp->d_nxt = ds;
		}
		harddev = dev;
		if ((ip = ifind(st->st_ino, &ds->d_isl)) == NULL) {
			if (ds->d_cnt >= maxino) {
				/* corresponds to for loop above */
				dev++, dp = ds, ds = ds->d_nxt;
				goto nextdev;
			}
			ip = scalloc(1, sizeof *ip);
			ip->i_ino = st->st_ino;
			ip->i_fino = ++ds->d_cnt;
			ip->i_nlk = st->st_nlink;
			if (fmttype & TYP_TAR)
				ip->i_name = sstrdup(file);
			if (fmttype & TYP_NCPIO) {
				ip->i_st = smalloc(sizeof *ip->i_st);
				*ip->i_st = *st;
			}
			iput(ip, &ds->d_isl);
		}
		ino = ip->i_fino;
		if (fmttype & TYP_NCPIO) {
			/*
			 * In SVR4 ascii cpio format, files with multiple
			 * links are stored with a zero size except for the
			 * last link, which contains the actual file content.
			 * As one cannot know which is the last link in
			 * advance since some links may be outside the
			 * archive content, all links have to be collected
			 * and written out at once.
			 */
			struct ilink *il, *ik;

			switch (ip->i_nlk) {
			case 1:
				/*
				 * This was the last link to a file. Write
				 * all previous links and break to write
				 * the actual file content. Free the pointers
				 * in islot; islot remains within the tree
				 * with a remaining link count of zero.
				 */
				ip->i_nlk--;
				free(ip->i_st);
				ip->i_st = NULL;
				for (il = ip->i_lnk, ik = NULL; il;
						ik = il, il = il->l_nxt,
						ik ? free(ik), 0 : 0) {
					errcnt += addfile(il->l_nam, st,
							dev, ino, 1, 0);
					free(il->l_nam);
				}
				break;
			case 0:
				/*
				 * This file got a link during operation, or
				 * -L was specified and we encountered a link
				 * more than once. Start with a fresh link
				 * count again.
				 */
				ip->i_nlk = st->st_nlink;
				ip->i_lnk = NULL;
				ip->i_st = smalloc(sizeof *ip->i_st);
				*ip->i_st = *st;
				/*FALLTHRU*/
			default:
				/*
				 * There are more links to this file. Store
				 * only the name and return.
				 */
				ip->i_nlk--;
				if (ip->i_lnk) {
					for (il = ip->i_lnk; il->l_nxt;
							il = il->l_nxt);
					il->l_nxt = scalloc(1,sizeof*il->l_nxt);
					il = il->l_nxt;
				} else {
					ip->i_lnk = scalloc(1,sizeof*ip->i_lnk);
					il = ip->i_lnk;
				}
				il->l_nam = smalloc(pathsz + 1);
				strcpy(il->l_nam, file);
				return 0;
			}
		} else if (fmttype & TYP_TAR) {
			if (strcmp(ip->i_name, file))
				return addfile(file, st, dev, ino, 1,
						ip->i_name);
		}
	} else {	/* single-linked or directory */
		dev = fakedev;
		while (markeddevs && marked(dev))
			dev--;
		if ((ino = fakeino--) == 0) {
			if (--dev <= harddev)
				msg(4, 1, "Too many devices in archive, "
						"exiting\n");
			fakedev = dev;
			ino = maxino;
			fakeino = ino - 1;
		}
	}
	return addfile(file, st, dev, ino, 0, 0);
}

/*
 * Add a single file to the archive with -o.
 */
static int
addfile(const char *realfile, struct stat *st,
		uint32_t dev, uint32_t ino, int zerolink, const char *linkname)
{
	union bincpio bc;
	int fd = -1;
	long long size;
	int pad, i;
	ssize_t rsz = 0, wsz = 0, hsz, fsz, psz;
	long long remsz, relative, nlink;
	long long Kbase = 0, Krest = 0, Ksize = 0;
	struct hdr_cpio	K2hdr;
	uint32_t	crc = 0;
	long long	csize = 0;
	char	*file;
	char	*symblink = NULL;
	int	failure = 1;

	file = sstrdup(realfile);
	if (pax != PAX_TYPE_CPIO && strcmp(file, trailer)) {
		size_t	junk = 0;
		if (pax_sflag && pax_sname(&file, &junk) == 0)
			goto cleanup;
		if (rflag && rname(&file, &junk) == 0)
			goto cleanup;
	}
	fsz = strlen(file);
	relative = nwritten;
	memset(bc.data, 0, sizeof bc.data);
	if (fmttype == FMT_PAX && pax_oflag & PO_LINKDATA &&
			(st->st_mode&S_IFMT) == S_IFREG)
		size = st->st_size;
	else if (zerolink)
		size = 0;
	else if ((st->st_mode&S_IFMT) == S_IFREG)
		size = st->st_size;
	else if ((st->st_mode&S_IFMT) == S_IFLNK) {
		i = st->st_size ? st->st_size : PATH_MAX;
		symblink = smalloc(i+1);
		if ((size = readlink(realfile, symblink, i)) < 0) {
			emsg(3, "Cannot read symbolic link \"%s\"", realfile);
			goto cleanup;
		}
		symblink[size] = '\0';
	} else
		size = 0;
	nlink = ((unsigned long long)st->st_nlink>maxnlink ?
			maxnlink : st->st_nlink);
	if (fmttype & TYP_NCPIO) {
		long	size1;
		if (fmttype & TYP_SCO && size >= mag_sco) {
			char	*ofile = file;
			size1 = mag_sco;
			fsz += 22;
			file = smalloc(fsz + 1);
			snprintf(file, fsz + 1, "%s%csize=%016llx",
					ofile, 0, size);
			free(ofile);
		} else
			size1 = size;
		pad = 4;
		sprintf(bc.data, "%*.*s%08lx%08lx%08lx%08lx%08lx%08lx"
				"%08lx%08lx%08lx%08lx%08lx%08lx%08lx",
			(int)(fmttype&TYP_CRC? sizeof mag_crc:sizeof mag_asc),
			(int)(fmttype&TYP_CRC? sizeof mag_crc:sizeof mag_asc),
			fmttype & TYP_CRC ? mag_crc : mag_asc,
			(long)ino & ECUT,
			(long)st->st_mode & ECUT,
			(long)st->st_uid & ECUT,
			(long)st->st_gid & ECUT,
			(long)nlink & ECUT,
			(long)st->st_mtime & ECUT,
			(long)size1 & ECUT,
			(long)major(dev) & ECUT,
			(long)minor(dev) & ECUT,
			(long)major(st->st_rdev) & ECUT,
			(long)minor(st->st_rdev) & ECUT,
			(long)++fsz,
			0L);
		hsz = SIZEOF_Exp_cpio_hdr;
		if ((psz = (hsz + fsz) % pad) != 0)
			psz = pad - psz;
	} else if (fmttype == FMT_ODC) {
		pad = 1;
		sprintf(bc.data, "%*.*s%06lo%06lo%06lo%06lo%06lo%06lo%06lo"
				"%011lo%06lo%011lo",
			(int)sizeof mag_odc, (int)sizeof mag_odc, mag_odc,
			(long)dev & OCUT,
			(long)ino & OCUT,
			(long)st->st_mode & OCUT,
			(long)st->st_uid & OCUT,
			(long)st->st_gid & OCUT,
			(long)nlink & OCUT,
			(long)st->st_rdev & OCUT,
			(long)st->st_mtime,
			(long)++fsz,
			(long)size);
		hsz = SIZEOF_c_hdr;
		if ((psz = (hsz + fsz) % pad) != 0)
			psz = pad - psz;
	} else if (fmttype == FMT_DEC) {
		pad = 1;
		sprintf(bc.data, "%*.*s%06lo%06lo%06lo%06lo%06lo%06lo"
				"%08lo%08lo%011lo%06lo%011lo",
			(int)sizeof mag_odc, (int)sizeof mag_odc, mag_odc,
			(long)dev & OCUT,
			(long)ino & OCUT,
			(long)st->st_mode & OCUT,
			(long)st->st_uid & OCUT,
			(long)st->st_gid & OCUT,
			(long)nlink & OCUT,
			(long)major(st->st_rdev) & 077777777,
			(long)minor(st->st_rdev) & 077777777,
			(long)st->st_mtime,
			(long)++fsz,
			(long)size);
		hsz = SIZEOF_d_hdr;
		if ((psz = (hsz + fsz) % pad) != 0)
			psz = pad - psz;
	} else if (fmttype & TYP_BINARY) {
		/*
		 * To avoid gcc's stupid 'comparison is always false due to
		 * limited range of data type' warning.
		 */
		unsigned long long	gcccrap;
		pad = 2;
		if (fmttype & TYP_BE) {
			be16p(mag_bin, bc.Hdr.c_magic);
			be16p(dev, bc.Hdr.c_dev);
			be16p(ino, bc.Hdr.c_ino);
			be16p(st->st_mode, bc.Hdr.c_mode);
			be16p(st->st_uid, bc.Hdr.c_uid);
			be16p(st->st_gid, bc.Hdr.c_gid);
			be16p(nlink, bc.Hdr.c_nlink);
			be16p(st->st_rdev, bc.Hdr.c_rdev);
			be32p(st->st_mtime, bc.Hdr.c_mtime);
			be16p(++fsz, bc.Hdr.c_namesize);
		} else {
			le16p(mag_bin, bc.Hdr.c_magic);
			le16p(dev, bc.Hdr.c_dev);
			le16p(ino, bc.Hdr.c_ino);
			le16p(st->st_mode, bc.Hdr.c_mode);
			le16p(st->st_uid, bc.Hdr.c_uid);
			le16p(st->st_gid, bc.Hdr.c_gid);
			le16p(nlink, bc.Hdr.c_nlink);
			le16p(st->st_rdev, bc.Hdr.c_rdev);
			me32p(st->st_mtime, bc.Hdr.c_mtime);
			le16p(++fsz, bc.Hdr.c_namesize);
		}
		if (fmttype & TYP_SGI && size > 0x7FFFFFFFLL) {
			Krest = size & 0x7FFFFFFFLL;
			Kbase = size - Krest;
			Ksize = 0x100000000LL - (Kbase >> 31);
			if (fmttype & TYP_BE)
				be32p(Ksize, bc.Hdr.c_filesize);
			else
				me32p(Ksize, bc.Hdr.c_filesize);
			K2hdr = bc.Hdr;
			if (fmttype & TYP_BE)
				be32p(Krest, K2hdr.c_filesize);
			else
				me32p(Krest, K2hdr.c_filesize);
		} else {
			if (fmttype & TYP_BE)
				be32p(size, bc.Hdr.c_filesize);
			else
				me32p(size, bc.Hdr.c_filesize);
		}
		if (fmttype & TYP_SGI &&
				(((st->st_mode&S_IFMT) == S_IFBLK ||
				 (st->st_mode&S_IFMT) == S_IFCHR) &&
				((unsigned long long)major(st->st_rdev)>0xFF ||
				 (unsigned long long)minor(st->st_rdev)>0xFF) ||
				(gcccrap = st->st_rdev) > 0177777)) {
			uint32_t	rdev;
			rdev = (minor(st->st_rdev) & 0x0003FFFF) +
				((major(st->st_rdev)<<18) & 0xFFFC0000);
			if (fmttype & TYP_BE) {
				be16p(0xFFFF, bc.Hdr.c_rdev);
				be32p(rdev, bc.Hdr.c_filesize);
			} else {
				le16p(0xFFFF, bc.Hdr.c_rdev);
				me32p(rdev, bc.Hdr.c_filesize);
			}
		}
		hsz = SIZEOF_hdr_cpio;
		psz = (hsz + fsz) % 2;
	} else if (fmttype & TYP_CRAY) {
		int	diff5 = fmttype==FMT_CRAY5 ? CRAY_PARAMSZ : 0;
		mode_t	mo;
		pad = 1;
		be64p(mag_bin, bc.Crayhdr.C_magic);
		be64p(dev, bc.Crayhdr.C_dev);
		be64p(ino, bc.Crayhdr.C_ino);
		if ((st->st_mode&S_IFMT) == S_IFLNK)	/* non-standard */
			mo = st->st_mode&07777|0130000;	/* S_IFLNK on Cray */
		else
			mo = st->st_mode;
		be64p(mo, bc.Crayhdr.C_mode);
		be64p(st->st_uid, bc.Crayhdr.C_uid);
		be64p(st->st_gid, bc.Crayhdr.C_gid);
		be64p(nlink, bc.Crayhdr.C_nlink);
		be64p(st->st_rdev, bc.Crayhdr.C_rdev);
		be64p(st->st_mtime, bc.Crayhdr.C_mtime - diff5);
		be64p(++fsz, bc.Crayhdr.C_namesize - diff5);
		be64p(size, bc.Crayhdr.C_filesize - diff5);
		hsz = SIZEOF_cray_hdr - diff5;
		psz = 0;
	} else if (fmttype & TYP_BAR) {
		int	c, n = 0;
		pad = 512;
		sprintf(bc.Bdr.b_mode, "%7.7o",(int)st->st_mode&(07777|S_IFMT));
		sprintf(bc.Bdr.b_uid, "%7.7lo", (long)st->st_uid);
		sprintf(bc.Bdr.b_gid, "%7.7lo", (long)st->st_gid);
		sprintf(bc.Bdr.b_size, "%11.11llo",
				(st->st_mode&S_IFMT) == S_IFREG && !zerolink ?
				(long long)st->st_size&077777777777LL : 0LL);
		sprintf(bc.Bdr.b_mtime, "%11.11lo", (long)st->st_mtime);
		sprintf(bc.Bdr.b_rdev, "%7.7lo", (long)st->st_rdev);
		strcpy(&bc.data[SIZEOF_bar_header], file);
		c = tlflag(st);
		if (zerolink == 0) {
			bc.Bdr.b_linkflag = c;
			if (c == '2') {
				strncpy(&bc.data[SIZEOF_bar_header+fsz+1],
						symblink,
						512-SIZEOF_bar_header-fsz);
				n = size;
			}
		} else {
			bc.Bdr.b_linkflag = '1';
			strncpy(&bc.data[SIZEOF_bar_header+fsz+1], linkname,
					512-SIZEOF_bar_header-fsz-1);
			n = strlen(linkname);
		}
		if (n > 512-SIZEOF_bar_header-fsz-1) {
			msg(3, 0, "%s: linked name too long\n", realfile);
			goto cleanup;
		}
		bchksum(&bc);
		hsz = 512;
		psz = 0;
		fsz = 0;
	} else if (fmttype & TYP_TAR) {
		const char	*cp;
		int	c;
		/*
		 * Many SVR4 cpio derivatives expect the mode field
		 * to contain S_IFMT bits. The meaning of these bits
		 * in the mode field of the ustar header is left
		 * unspecified by IEEE Std 1003.1, 1996, 10.1.1.
		 */
		int	mmask = fmttype == FMT_USTAR || fmttype == FMT_PAX ?
				07777 : 07777|S_IFMT;

		paxrec = globrec;
		pad = 512;
		if (tmkname(&bc.Tdr, file) != 0)
			goto cleanup;
		sprintf(bc.Tdr.t_mode, "%7.7o", (int)st->st_mode & mmask);
		if (fmttype == FMT_GNUTAR && st->st_uid > 07777777) {
			be64p(st->st_uid, bc.Tdr.t_uid);
			bc.Tdr.t_uid[0] |= 0200;
		} else {
			sprintf(bc.Tdr.t_uid, "%7.7lo",
					(long)st->st_uid&07777777);
			if (fmttype & TYP_PAX && st->st_uid > 07777777)
				paxrec |= PR_UID;
		}
		if (fmttype == FMT_GNUTAR && st->st_gid > 07777777) {
			be64p(st->st_gid, bc.Tdr.t_gid);
			bc.Tdr.t_gid[0] |= 0200;
		} else {
			sprintf(bc.Tdr.t_gid, "%7.7lo",
					(long)st->st_gid&07777777);
			if (fmttype & TYP_PAX && st->st_gid > 07777777)
				paxrec |= PR_GID;
		}
		if (fmttype == FMT_GNUTAR && (st->st_mode&S_IFMT) == S_IFREG &&
				st->st_size > 077777777777LL && !zerolink) {
			bc.Tdr.t_size[0] = '\200';
			be64p(st->st_size, &bc.Tdr.t_size[4]);
		} else {
			sprintf(bc.Tdr.t_size, "%11.11llo",
				(st->st_mode&S_IFMT) == S_IFREG &&
				(!zerolink || fmttype == FMT_PAX &&
				 	pax_oflag & PO_LINKDATA) ?
				(long long)st->st_size&077777777777LL : 0LL);
			if (fmttype & TYP_PAX &&
					(st->st_mode&S_IFMT) == S_IFREG &&
					st->st_size > 077777777777LL &&
					(!zerolink || fmttype == FMT_PAX &&
					 	pax_oflag & PO_LINKDATA))
				paxrec |= PR_SIZE;
		}
		sprintf(bc.Tdr.t_mtime, "%11.11lo", (long)st->st_mtime);
		if ((c = tlflag(st)) < 0) {
			if ((st->st_mode&S_IFMT) != S_IFDIR) {
				msg(2, 0, "%s is not a file. Not dumped\n",
						realfile);
				errcnt++;
			} else
				failure = 0;
			goto cleanup;
		}
		if (zerolink == 0) {
			bc.Tdr.t_linkflag = c;
			if (c == '2') {
				if (tmklink(&bc.Tdr, symblink) != 0)
					goto cleanup;
			}
		} else {
			bc.Tdr.t_linkflag = '1';
			if (tmklink(&bc.Tdr, linkname) != 0)
				goto cleanup;
		}
		if (fmttype & TYP_USTAR) {
			if (fmttype == FMT_GNUTAR)
				strcpy(bc.Tdr.t_magic, mag_gnutar);
			else {
				strcpy(bc.Tdr.t_magic, mag_ustar);
				bc.Tdr.t_version[0] = bc.Tdr.t_version[1] = '0';
			}
			if ((cp = getuser(st->st_uid)) != NULL)
				sprintf(bc.Tdr.t_uname, "%.31s", cp);
			else
				msg(1, 0, "could not get passwd information "
						"for %s\n", realfile);
			if ((cp = getgroup(st->st_gid)) != NULL)
				sprintf(bc.Tdr.t_gname, "%.31s", cp);
			else
				msg(1, 0, "could not get group information "
						"for %s\n", realfile);
			if (fmttype == FMT_GNUTAR &&
					(unsigned long long)major(st->st_rdev)
					> 077777777) {
				be64p(major(st->st_rdev), bc.Tdr.t_devmajor);
				bc.Tdr.t_devmajor[0] |= 0200;
			} else {
				if (fmttype == FMT_SUN &&
					(unsigned long long)major(st->st_rdev)
						> 077777777 &&
						((st->st_mode&S_IFMT)==S_IFBLK||
						 (st->st_mode&S_IFMT)==S_IFCHR))
					paxrec |= PR_SUN_DEVMAJOR;
				sprintf(bc.Tdr.t_devmajor, "%7.7o",
					(int)major(st->st_rdev)&07777777);
			}
			if (fmttype == FMT_GNUTAR &&
					(unsigned long long)minor(st->st_rdev)
					> 077777777) {
				be64p(minor(st->st_rdev), bc.Tdr.t_devminor);
				bc.Tdr.t_devminor[0] |= 0200;
			} else {
				if (fmttype == FMT_SUN &&
					(unsigned long long)minor(st->st_rdev)
						> 077777777 &&
						((st->st_mode&S_IFMT)==S_IFBLK||
						 (st->st_mode&S_IFMT)==S_IFCHR))
					paxrec |= PR_SUN_DEVMINOR;
				sprintf(bc.Tdr.t_devminor, "%7.7o",
					(int)minor(st->st_rdev)&07777777);
			}
		}
		tchksum(&bc);
		hsz = 512;
		psz = 0;
		fsz = 0;
	} else if (fmttype == FMT_ZIP) {
		pad = 1;
		memcpy(bc.Zdr.z_signature, mag_zipsig, sizeof mag_zipsig);
		bc.Zdr.z_version[0] = 10;
		mkdostime(st->st_mtime, bc.Zdr.z_modtime, bc.Zdr.z_moddate);
		if ((st->st_mode&S_IFMT) == S_IFREG ||
				(st->st_mode&S_IFMT) == S_IFLNK) {
			le32p(size, bc.Zdr.z_csize);
			le32p(size, bc.Zdr.z_nsize);
			csize = size;
		}
		le16p(fsz, bc.Zdr.z_namelen);
		le16p(SIZEOF_zextra_cp, bc.Zdr.z_extralen);
		hsz = SIZEOF_zip_header;
		psz = 0;
	} else
		abort();
	/*
	 * Start writing the file to the archive.
	 */
	if ((st->st_mode&S_IFMT) == S_IFREG && st->st_size != 0 &&
			(zerolink == 0 || fmttype == FMT_PAX &&
			 	pax_oflag & PO_LINKDATA)) {
		char	*buf;
		size_t	bufsize;
		int	readerr = 0;

		if ((fd = open(realfile, O_RDONLY)) < 0) {
			if (sysv3 < 0)
				msg(0, 0, "< %s > ?\n", realfile);
			else if (sysv3 > 0)
				fprintf(stderr, "<%s> ?\n", realfile);
			else
				msg(0, 0, "\"%s\" ?\n", realfile);
			goto cleanup;
		}
		if (fmttype == FMT_ZIP) {
			if (zipwrite(fd, file, st, &bc, fsz, dev, ino,
						&crc, &csize) < 0)
				goto cleanup2;
			goto done;
		}
		if (fmttype & TYP_CRC)
			if (sum(fd, realfile, st, bc.Edr.E_chksum) < 0)
				goto cleanup2;
		if (fmttype & TYP_PAX && paxrec != PR_NONE)
			wrpax(file, symblink?symblink:linkname, st);
		bwrite(bc.data, hsz);
		if (fsz)
			bwrite(file, fsz);
		if (psz)
			bwrite(&bc.data[hsz], psz);
		if (Kbase)
			remsz = Kbase;
		else
			remsz = st->st_size;
		getbuf(&buf, &bufsize, st->st_blksize);
	again:	while (remsz > 0) {
			if (fd < 0 || (rsz = read(fd, &buf[wsz],
							bufsize - wsz)) < 0) {
				if (readerr == 0) {
					emsg(3, "Cannot read \"%s\"", realfile);
					if (fd >= 0)
						errcnt++;
					readerr = 1;
				}
				if (fd >= 0 && lseek(fd, bufsize - wsz,
							SEEK_CUR) < 0) {
					close(fd);
					fd = -1;
				}
				rsz = bufsize - wsz;
				if (rsz > remsz)
					rsz = remsz;
				memset(&buf[wsz], 0, rsz);
			}
			if (rsz > remsz)
				rsz = remsz;
			wsz += rsz;
			remsz -= rsz;
			bwrite(buf, wsz);
			memset(buf, 0, wsz);
			size = wsz;
			wsz = 0;
		}
		wsz = size;
		if (Kbase) {
			if ((i = Ksize % pad) != 0)
				bwrite(&bc.data[hsz], i);
			bwrite((char *)&K2hdr, hsz);
			if (fsz)
				bwrite(file, fsz);
			if (psz)
				bwrite(&bc.data[hsz], psz);
			remsz = Krest;
			Kbase = 0;
			wsz = 0;
			goto again;
		} else if (Ksize)
			wsz = Krest;
	} else if ((fmttype == FMT_ZIP || fmttype & TYP_CPIO) &&
			(st->st_mode&S_IFMT) == S_IFLNK) {
		wsz = size;
		if (fmttype == FMT_ZIP) {
			crc = zipcrc(0, (unsigned char *)symblink, wsz);
			le32p(crc, bc.Zdr.z_crc32);
			bwrite(bc.data, SIZEOF_zip_header);
			bwrite(file, fsz);
			zipwxtra(file, st, dev, ino);
			bwrite(symblink, wsz);
		} else {
			bwrite(bc.data, hsz);
			if (fsz)
				bwrite(file, fsz);
			if (psz)
				bwrite(&bc.data[hsz], psz);
			bwrite(symblink, wsz);
		}
	} else {
		if (fmttype & TYP_PAX && paxrec != PR_NONE)
			wrpax(file, symblink?symblink:linkname, st);
		bwrite(bc.data, hsz);
		if (fsz)
			bwrite(file, fsz);
		if (psz)
			bwrite(&bc.data[hsz], psz);
		if (fmttype == FMT_ZIP)
			zipwxtra(file, st, dev, ino);
	}
done:	if (fmttype == FMT_ZIP) {
		zipdefer(file, st, relative, crc, csize, &bc.Zdr);
	}
	if ((i = wsz % pad) != 0)
		bwrite(&bc.data[hsz], pad - i);
	if (vflag && strcmp(file, trailer))
		fprintf(stderr, "%s\n", file);
	else if (Vflag)
		prdot(0);
	failure = 0;
cleanup2:
	if ((st->st_mode&S_IFMT) == S_IFREG) {
		if (fd >= 0)
			close(fd);
		if (aflag)
			errcnt += rstime(realfile, st, "access");
	}
cleanup:
	free(file);
	free(symblink);
	return failure;
}

/*
 * Flush a SVR4 cpio format inode tree for -o.
 */
static void
iflush(struct islot *ip, uint32_t dev)
{
	if (ip == inull)
		return;
	iflush(ip->i_lln, dev);
	iflush(ip->i_rln, dev);
	if (ip->i_nlk > 0 && ip->i_st) {
		struct ilink *il;

		for (il = ip->i_lnk; il->l_nxt; il = il->l_nxt)
			errcnt += addfile(il->l_nam, ip->i_st,
					dev, ip->i_fino, 1, 0);
		errcnt += addfile(il->l_nam, ip->i_st, dev, ip->i_fino, 0, 0);
	}
}

/*
 * Flush the SVR4 cpio link forest for -o.
 */
static void
lflush(void)
{
	struct dslot *ds;

	for (ds = devices; ds; ds = ds->d_nxt)
		iflush(ds->d_isl, ds->d_fake);
}

int
setfmt(char *s)
{
	int	i, j;

	struct {
		const char	*ucs;
		const char	*lcs;
		int	type;
		int	bits;
	} fs[] = {
		{ "NEWC",	"newc",		FMT_ASC,	00 },
		{ "SCO",	"sco",		FMT_SCOASC,	00 },
		{ "CRC",	"crc",		FMT_CRC,	00 },
		{ "SCOCRC",	"scocrc",	FMT_SCOCRC,	00 },
		{ "ODC",	"odc",		FMT_ODC,	00 },
		{ "DEC",	"dec",		FMT_DEC,	00 },
		{ "BIN",	"bin",		FMT_NONE,	00 },
		{ "BBS",	"bbs",		TYP_BE,		00 },
		{ "SGI",	"sgi",		FMT_SGIBE,	00 },
		{ "CRAY",	"cray",		FMT_CRAY,	00 },
		{ "CRAY5",	"cray5",	FMT_CRAY5,	00 },
		{ "TAR",	"tar",		FMT_TAR,	00 },
		{ "USTAR",	"ustar",	FMT_USTAR,	00 },
		{ "PAX:",	"pax:",		FMT_PAX,	00 },
		{ "SUN",	"sun",		FMT_SUN,	00 },
		{ "GNU",	"gnu",		FMT_GNUTAR,	00 },
		{ "OTAR",	"otar",		FMT_OTAR,	00 },
		{ "BAR",	"bar",		FMT_BAR,	00 },
		{ "ZIP:",	"zip:",		FMT_ZIP,	00 },
		{ NULL,		NULL,		0,		00 }
	};
	for (i = 0; fs[i].ucs; i++) {
		for (j = 0; s[j] &&
				(s[j] == fs[i].ucs[j] || s[j] == fs[i].lcs[j]);
				j++)
			if (fs[i].ucs[j] == ':')
				break;
		if (s[j] == '\0' &&
				(fs[i].ucs[j] == '\0' || fs[i].ucs[j] == ':') ||
				s[j] == ':' && fs[i].ucs[j] == ':') {
			fmttype = fs[i].type;
			if (fmttype == FMT_ZIP && s[j] == ':') {
#if	USE_ZLIB
				if (strcmp(&s[j+1], "en") == 0)
					zipclevel = 00;
				else if (strcmp(&s[j+1], "ex") == 0)
					zipclevel = 01;
				else if (strcmp(&s[j+1], "ef") == 0)
					zipclevel = 02;
				else if (strcmp(&s[j+1], "es") == 0)
					zipclevel = 03;
				else
#endif	/* USE_ZLIB */
				if (strcmp(&s[j+1], "e0") == 0)
					zipclevel = 04;
				else
#if	USE_BZLIB
				if (strcmp(&s[j+1], "bz2") == 0)
					zipclevel = 07;
				else
#endif	/* USE_BZLIB */
					continue;
			} else if (fmttype == FMT_NONE)
				fmttype = bigendian() ? FMT_BINBE : FMT_BINLE;
			else if (fmttype == TYP_BE)
				fmttype = bigendian() ? FMT_BINLE : FMT_BINBE;
			else if (fmttype == FMT_PAX && s[j] == ':') {
				if (pax_options(&s[j+1], 0) < 0)
					continue;
			}
			return 0;
		}
	}
	msg(3, 0, "Invalid header \"%s\" specified.\n", s);
	return -1;
}

static int
bigendian(void)
{
	union {
		char	u_c[2];
		int16_t	u_i;
	} u;
	u.u_i = 1;
	return u.u_c[1] == 1;
}

int
setreassign(const char *s)
{
	struct passwd	*pwd;
	int	val = 0;

	if (myuid != 0) {
		msg(3, 0, "R option only valid for super-user.\n");
		val = -1;
	}
	if ((pwd = getpwnam(s)) == NULL) {
		msg(3, 0, "\"%s\" is not a valid user id\n", s);
		val = -1;
	} else {
		Ruid = pwd->pw_uid;
		Rgid = pwd->pw_gid;
	}
	return val;
}

void *
srealloc(void *m, size_t n)
{
	if ((m = realloc(m, n)) == NULL) {
		write(2, "Out of memory.\n", 15);
		_exit(sysv3 ? 2 : 3);
	}
	return m;
}

void *
smalloc(size_t n)
{
	return srealloc(NULL, n);
}

void *
scalloc(size_t nmemb, size_t size)
{
	void	*vp;

	if ((vp = calloc(nmemb, size)) == NULL) {
		write(2, "Out of memory.\n", 15);
		_exit(sysv3 ? 2 : 3);
	}
	return vp;
}

void *
svalloc(size_t n, int force)
{
	static long	pagesize;
	void	*vp;

	if (pagesize == 0)
		if ((pagesize = sysconf(_SC_PAGESIZE)) < 0)
			pagesize = 4096;
	if ((vp = memalign(pagesize, n)) == NULL && force) {
		write(2, "Out of memory.\n", 15);
		_exit(sysv3 ? 2 : 3);
	}
	return vp;
}

/*
 * A single static buffer is used for intermediate copying from file
 * data to the tape buffer and vice versa, for creating checksums, and
 * for data transfer with -p.
 */
static void
getbuf(char **bufp, size_t *sizep, size_t best)
{
	static char	*buf;
	static size_t	size;

	if (size != best) {
		if (buf)
			free(buf);
		size = best;
		if (size == 0 || (buf = svalloc(size, 0)) == NULL)
			buf = svalloc(size = 512, 1);
	}
	*bufp = buf;
	*sizep = size;
}

static void
sevprnt(int sev)
{
	if (printsev) switch (sev) {
	case 1:
		fprintf(stderr, "INFORM: ");
		break;
	case 2:
		fprintf(stderr, "WARNING: ");
		break;
	case 3:
		fprintf(stderr, "ERROR: ");
		break;
	case 4:
		fprintf(stderr, "HALT: ");
		break;
	}
}

void
msg(int sev, int err, const char *fmt, ...)
{
	va_list	ap;

	/*
	 * The error message should appear near the file it refers to.
	 */
	if (tflag)
		fflush(stdout);
	else if (Vflag)
		prdot(1);
	if (sysv3 >= 0 && sev >= (printsev ? 0 : -1))
		fprintf(stderr, "%s: ", progname);
	sevprnt(sev);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	if (err > 0)
		done(err);
	else if (err == -2) {
		if (sysv3)
			done(1);
		usage();
	}
}

void
emsg(int sev, const char *fmt, ...)
{
	char	_fmt[60];
	int	i, fl = sev & 030, n;
	va_list	ap;

	sev &= ~030;
	i = errno;
	if (tflag)
		fflush(stdout);
	else if (Vflag)
		prdot(1);
	fprintf(stderr, "%s: ", progname);
	sevprnt(sev);
	va_start(ap, fmt);
	if (sysv3) {
		if (fmt[(n=strlen(fmt))-1] == '"' && fmt[n-2] == 's' &&
				fmt[n-3] == '%' && fmt[n-4] == '"' &&
				n < sizeof _fmt) {
			strcpy(_fmt, fmt);
			_fmt[n-1] = '>';
			_fmt[n-4] = '<';
			fmt = _fmt;
		}
	}
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	if (sysv3 > 0 && sev >= 0 && fl & 010)
		fprintf(stderr, "\n\t%s\n", strerror(i));
	else if (sysv3 < 0) {
		if (fl & 020)
			putc('\n', stderr);
		else
			fprintf(stderr, " (errno:%d)\n", i);
	} else
		fprintf(stderr, ", errno %d, %s\n", i, strerror(i));
}

static void
prdot(int flush)
{
	static int	column;

	if (flush && column != 0 || column >= 50) {
		write(action == 'o' && !Oflag ? 2 : 1, "\n", 1);
		column = 0;
	}
	if (!flush) {
		write(action == 'o' && !Oflag ? 2 : 1, ".", 1);
		column++;
	}
}

/*
 * Ask the user for new media if applicable, or exit.
 */
static void
newmedia(int err)
{
	static int	mediacnt = 1;
	static char answer[PATH_MAX+1];
	const char	*mesf = action == 'i' ?
		(Iflag && !sysv3 ? Iflag : "input") :
		(Oflag && !sysv3 ? Oflag : "output");
	char c;
	int i, j;

	if (mfl == 0 && close(mt) < 0) {
		emsg(3, "Close error on \"%s\"", mesf);
		errcnt++;
	}
	mfl = -1;
	if ((mtst.st_mode&S_IFMT)!=S_IFCHR && (mtst.st_mode&S_IFMT)!=S_IFBLK ||
			Dflag) {
		if (action == 'o') {
			switch (err) {
			case 0:
				break;
			case EFBIG:
				msg(3, 0, "ulimit reached for output file.\n");
				break;
			case ENOSPC:
				msg(3, 0, "No space left for output file.\n");
				break;
			default:
				msg(3, 0, "I/O error - cannot continue, "
						"errno %d, %s\n",
						err, strerror(err));
			}
		}
		return;
	}
	if (err == ENOSPC || err == ENXIO || err == 0)
		msg(-2, 0, sysv3 ? "Reached end of medium on %s.\a\n" :
				"End of medium on \"%s\".\a\n", mesf);
	else
		msg(3, 0, "I/O error on \"%s\", errno %d, %s\a\n", mesf,
				err, strerror(err));
	mediacnt++;
	while (mfl < 0) {
		if (Iflag || Oflag)
			msg(-2, 0, Mflag ? Mflag :
					"Change to part %d and press "
					"RETURN key. [q] ", mediacnt);
		else
			msg(-2, 0, sysv3 ? "If you want to go on, "
					"type device/file name when ready.\n" :
					"To continue, type device/file name "
					"when ready.\n");
		if (tty == 0)
			if ((tty = open("/dev/tty", O_RDWR)) < 0 ||
					fcntl(tty, F_SETFD, FD_CLOEXEC) < 0) {
			cantrt:	errcnt++;
				msg(4, 1, "Cannot read tty.\n");
			}
		i = 0;
		while ((j = read(tty, &c, 1)) == 1 && c != '\n')
			if (i < sizeof answer - 1)
				answer[i++] = c;
		if (j != 1)
			goto cantrt;
		answer[i] = 0;
		if (Iflag || Oflag) {
			if (answer[0] == '\0')
				snprintf(answer, sizeof answer, Iflag ? Iflag :
						Oflag);
			else if (answer[0] == 'q')
				exit(errcnt != 0 ? sysv3 ? 1 : 2 : 0);
			else if (Iflag)
				Iflag = sstrdup(answer);
			else if (Oflag)
				Oflag = sstrdup(answer);
		} else if (answer[0] == '\0')
			return;
		if ((mt = action == 'i' ? open(answer, O_RDONLY) :
					creat(answer, 0666)) < 0) {
			if (sysv3)
				msg(-2, 0, "That didn't work, "
						"cannot open \"%s\"\n%s\n",
						answer, strerror(errno));
			else
				emsg(3, "Cannot open \"%s\"", answer);
		}
		else
			mfl = 0;
	}
	mstat();
}

static void
mclose(void)
{
	if (action == 'o' && mt >= 0 && close(mt) < 0) {
		emsg(3, "Close error on \"%s\"",
				Oflag && !sysv3 ? Oflag : "output");
		errcnt++;
	}
}

/*
 * Write the archive buffer to tape.
 */
static ssize_t
mwrite(int max)
{
	ssize_t wo, wt = 0;

	do {
		if ((wo = write(mt, blkbuf + wt, (max?max:blksiz) - wt)) < 0) {
			if (errno == EINTR) {
				continue;
			} else {
				newmedia(errno);
				if (mfl == 0) {
					if (fmttype & TYP_BAR)
						dump_barhdr();
					continue;
				}
				else
					done(1);
			}
		}
		poffs += wo;
		wt += wo;
	} while (wt < (max?max:blksiz));
	blocks += wt >> 9;
	bytes += wt & 0777;
	return wt;
}

/*
 * Buffered writes to tape.
 */
static void
bwrite(const char *data, size_t sz)
{
	size_t di;

	nwritten += sz;
	while (curpos + sz > blksiz) {
		di = blksiz - curpos;
		sz -= di;
		memcpy(&blkbuf[curpos], data, di);
		mwrite(0);
		data += di;
		curpos = 0;
	}
	memcpy(&blkbuf[curpos], data, sz);
	curpos += sz;
}

/*
 * Flush the tape write buffer.
 */
static void
bflush(void)
{
	if (curpos) {
		memset(&blkbuf[curpos], 0, blksiz - curpos);
		mwrite(fmttype==FMT_ZIP && (mtst.st_mode&S_IFMT) == S_IFREG ?
			curpos : 0);
	}
	curpos = 0;
}

/*
 * CRC format checksum calculation with -i.
 */
static int
sum(int fd, const char *fn, struct stat *sp, char *tg)
{
	char	*buf;
	size_t	bufsize;
	uint32_t	size = sp->st_size, sum = 0;
	ssize_t	rd;
	char	c;

	getbuf(&buf, &bufsize, sp->st_blksize);
	/*
	 * Unfortunately, SVR4 cpio derivatives (as on Solaris 8 and
	 * UnixWare 2.1) compute the checksum of signed char values,
	 * whereas GNU cpio and the pax implementations of AT&T and
	 * BSD use unsigned chars. Since there is no 'open' standard
	 * for the SVR4 CRC format, the SVR4 implementation should be
	 * taken as a de facto reference and we thus create SVR4 type
	 * checksums.
	 */
	while ((rd = read(fd, buf, size>bufsize ? bufsize : size )) > 0) {
		size -= rd;
		do
			sum += ((signed char *)buf)[--rd];
		while (rd);
	}
	if (rd < 0) {
		msg(3, 0, "Error computing checksum\n");
		return 1;
	}
	if (lseek(fd, 0, SEEK_SET) == (off_t)-1) {
		emsg(3, "Cannot reset file after checksum");
		return 1;
	}
	c = tg[8];
	sprintf(tg, "%08lx", (long)sum);
	tg[8] = c;
	return 0;
}

static int
rstime(const char *fn, struct stat *st, const char *which)
{
	struct utimbuf	utb;

	utb.actime = st->st_atime;
	utb.modtime = st->st_mtime;
	if (pax != PAX_TYPE_CPIO &&
			(pax_preserve&(PAX_P_ATIME|PAX_P_MTIME)) != 0 &&
			(pax_preserve&PAX_P_EVERY) == 0) {
		struct stat	xst;
		if (stat(fn, &xst) < 0)
			goto fail;
		if (pax_preserve&PAX_P_ATIME)
			utb.actime = xst.st_atime;
		if (pax_preserve&PAX_P_MTIME)
			utb.modtime = xst.st_mtime;
	}
	if (utime(fn, &utb) < 0) {
	fail:	emsg(2, "Unable to reset %s time for \"%s\"", which, fn);
		return 1;
	}
	return 0;
}

/*
 * Top-down splay function for inode tree.
 */
static struct islot *
isplay(ino_t ino, struct islot *x)
{
	struct islot	hdr;
	struct islot	*leftmax, *rightmin;
	struct islot	*y;

	hdr.i_lln = hdr.i_rln = inull;
	leftmax = rightmin = &hdr;
	inull->i_ino = ino;
	while (ino != x->i_ino) {
		if (ino < x->i_ino) {
			if (ino < x->i_lln->i_ino) {
				y = x->i_lln;
				x->i_lln = y->i_rln;
				y->i_rln = x;
				x = y;
			}
			if (x->i_lln == inull)
				break;
			rightmin->i_lln = x;
			rightmin = x;
			x = x->i_lln;
		} else {
			if (ino > x->i_rln->i_ino) {
				y = x->i_rln;
				x->i_rln = y->i_lln;
				y->i_lln = x;
				x = y;
			}
			if (x->i_rln == inull)
				break;
			leftmax->i_rln = x;
			leftmax = x;
			x = x->i_rln;
		}
	}
	leftmax->i_rln = x->i_lln;
	rightmin->i_lln = x->i_rln;
	x->i_lln = hdr.i_rln;
	x->i_rln = hdr.i_lln;
	inull->i_ino = !ino;
	return x;
}

/*
 * Find the inode number ino.
 */
static struct islot *
ifind(ino_t ino, struct islot **it)
{
	if (*it == NULL)
		return NULL;
	*it = isplay(ino, *it);
	return (*it)->i_ino == ino ? *it : NULL;
}

/*
 * Put ik into the tree.
 */
static void
iput(struct islot *ik, struct islot **it)
{
	if ((*it) == NULL) {
		ik->i_lln = ik->i_rln = inull;
		(*it) = ik;
	} else {
		/* ifind() is always called before */
		/*(*it) = isplay(ik->i_ino, (*it));*/
		if (ik->i_ino < (*it)->i_ino) {
			ik->i_lln = (*it)->i_lln;
			ik->i_rln = (*it);
			(*it)->i_lln = inull;
			(*it) = ik;
		} else if ((*it)->i_ino < ik->i_ino) {
			ik->i_rln = (*it)->i_rln;
			ik->i_lln = (*it);
			(*it)->i_rln = inull;
			(*it) = ik;
		}
	}
}

/*
 * Find the device dev or add it to the device/inode forest if not
 * already present.
 */
static struct dslot *
dfind(struct dslot **root, dev_t dev)
{
	struct dslot	*ds, *dp;

	for (ds = *root, dp = NULL; ds; dp = ds, ds = ds->d_nxt)
		if (ds->d_dev == dev)
			break;
	if (ds == NULL) {
		ds = scalloc(1, sizeof *ds);
		ds->d_dev = dev;
		if (*root == NULL)
			*root = ds;
		else
			dp->d_nxt = ds;
	}
	return ds;
}

/*
 * Exit on fatal error conditions.
 */
static void
done(int i)
{
	if (tflag)
		fflush(stdout);
	errcnt += i;
	mclose();
	if (errcnt && !sysv3)
		fprintf(stderr, "%llu errors\n", errcnt);
	exit(sysv3 ? 2 : 3);
}

static char	*pcopy, *pcend;
static size_t	psz, pslen, pss;

/*
 * Execution for -p.
 */
static void
dopass(const char *target)
{
	struct stat	st;

	if (access(target, W_OK) < 0) {
		emsg(033, sysv3 ? "cannot write in <%s>" :
				"Error during access() of \"%s\"", target);
		if (sysv3)
			done(1);
		usage();
	}
	if (stat(target, &st) < 0) {
		emsg(023, "Error during stat() of \"%s\"", target);
		done(1);
	}
	if ((st.st_mode&S_IFMT) != S_IFDIR)
		msg(3, 1, sysv3 ? "<%s> not a directory.\n" :
				"\"%s\" is not a directory\n", target);
	getpath(target, &pcopy, &pcend, &psz, &pslen);
	copyout(passfile);
}

/*
 * Callback for sfile().
 */
/*ARGSUSED*/
void
writerr(void *vp, int count, int written)
{
}

/*
 * Copy file data of regular files with -p.
 */
static int
passdata(struct file *f, const char *tgt, int tfd)
{
	char	*buf;
	size_t	bufsize;
	ssize_t	rd = 0;

	if (f->f_fd < 0)	/* is a zero-sized unreadable file */
		return 0;
#ifdef	__linux__
	if (f->f_st.st_size > 0) {
		long long	sent;

		sent = sfile(tfd, f->f_fd, f->f_st.st_mode, f->f_st.st_size);
		blocks += (sent + 0777) >> 9;
		if (sent == f->f_st.st_size)
			return 0;
		if (sent < 0)
			goto rerr;
	}
#endif	/* __linux__ */
	getbuf(&buf, &bufsize, f->f_st.st_blksize);
	while ((rd = read(f->f_fd, buf, bufsize)) > 0) {
		blocks += (rd + 0777) >> 9;
		if (write(tfd, buf, rd) != rd) {
			emsg(3, "Cannot write \"%s\"", tgt);
			return -1;
		}
	}
	if (rd < 0) {
#ifdef	__linux__
	rerr:
#endif	/* __linux__ */
		emsg(3, "Cannot read \"%s\"", f->f_name);
		return -1;
	}
	return 0;
}

/*
 * Handle a single file for -p.
 */
static int
passfile(const char *fn, struct stat *st)
{
	struct file	f;
	ssize_t	sz;
	int	val;
	char	*newfn;
	size_t	newsz = 0;

	newfn = sstrdup(fn);
	if (pax != PAX_TYPE_CPIO) {
		if (pax_sflag && pax_sname(&newfn, &newsz) == 0)
			return 0;
		if (rflag && rname(&newfn, &newsz) == 0)
			return 0;
	}
	setpath(newfn, &pcopy, &pcend, pslen, &psz, &pss);
	free(newfn);
	memset(&f, 0, sizeof f);
	f.f_name = sstrdup(fn);
	f.f_nsiz = strlen(fn) + 1;
	f.f_st = *st;
	f.f_fd = -1;
	if ((st->st_mode&S_IFMT) == S_IFLNK) {
		sz = st->st_size ? st->st_size : PATH_MAX;
		f.f_lnam = smalloc(sz+1);
		if ((sz = readlink(fn, f.f_lnam, sz+1)) < 0) {
				emsg(3, "Cannot read symbolic link \"%s\"", fn);
				free(f.f_lnam);
				return 1;
		}
		f.f_lnam[sz] = '\0';
	} else if ((st->st_mode&S_IFMT) == S_IFREG) {
		if ((f.f_fd = open(fn, O_RDONLY)) < 0) {
			if (sysv3)
				msg(2, 0, "Cannot open file \"%s\".\n", fn);
			else
				emsg(2, "Cannot open \"%s\", skipped", fn);
			if (st->st_size != 0)
				return 1;
		}
	}
	val = filein(&f, passdata, pcopy);
	if (f.f_lnam)
		free(f.f_lnam);
	free(f.f_name);
	if (f.f_fd >= 0)
		close(f.f_fd);
	if (val <= 1 && aflag && (st->st_mode&S_IFMT) == S_IFREG)
		errcnt += rstime(fn, st, "access");
	return val != 0;
}

/*
 * Processing of a single file common to -i and -p. Return value: 0 if
 * successful, 1 at failure if data was read, 2 at failure if no data
 * was read.
 */
static int
filein(struct file *f, int (*copydata)(struct file *, const char *, int),
	char *tgt)
{
	struct stat	nst;
	char	*temp = NULL;
	size_t	len;
	int	fd, i, j, new;
	int	failure = 2;

	if (fmttype == FMT_ZIP && (f->f_st.st_mode&S_IFMT) != S_IFREG &&
			(f->f_st.st_mode&S_IFMT) != S_IFLNK &&
			(f->f_csize > 0 || f->f_gflag & FG_DESC))
		skipfile(f);
	if ((new = lstat(tgt, &nst)) == 0) {
		if (action == 'p' && f->f_st.st_dev == nst.st_dev &&
				f->f_st.st_ino == nst.st_ino) {
			msg(3, 0, sysv3 ?
				"Attempt to pass file to self!\n" :
				"Attempt to pass a file to itself.\n");
			return 1;
		}
		if ((f->f_st.st_mode&S_IFMT) == S_IFDIR) {
			if ((nst.st_mode&S_IFMT) == S_IFDIR)
				return setattr(tgt, &f->f_st);
			rmdir(tgt);
		} else {
			if (pax_kflag) {
				failure = 0;
				goto skip;
			}
			if (uflag == 0 && f->f_st.st_mtime <= nst.st_mtime) {
				if (pax == PAX_TYPE_CPIO)
					msg(-1, 0, sysv3 ?
					"current <%s> newer or same age\n" :
					"Existing \"%s\" same age or newer\n",
						tgt);
				else
					failure = 0;
				goto skip;
			}
		}
	} else {
		if (imdir(tgt) < 0)
			goto skip;
	}
	if (Vflag && !vflag)
		prdot(0);
	if ((f->f_st.st_mode&S_IFMT) != S_IFDIR && lflag) {
		if (Lflag) {
			char	*symblink, *name;
			struct stat	xst;
			name = f->f_name;
			for (;;) {
				if (lstat(name, &xst) < 0) {
					emsg(3, "Cannot lstat \"%s\"", name);
					if (name != f->f_name)
						free(name);
					goto cantlink;
				}
				if ((xst.st_mode&S_IFMT) != S_IFLNK)
					break;
				i = xst.st_size ? xst.st_size : PATH_MAX;
				symblink = smalloc(i+1);
				if ((j = readlink(name, symblink, i)) < 0) {
					emsg(3, "Cannot read symbolic link "
						"\"%s\"", name);
					free(symblink);
					if (name != f->f_name)
						free(name);
					goto cantlink;
				}
				symblink[j] = '\0';
				symblink = joinpath(name, symblink);
				if (name != f->f_name)
					free(name);
				name = symblink;
			}
			if (linkunlink(name, tgt) == 0) {
				if (vflag)
					fprintf(stderr, "%s\n", tgt);
				if (name != f->f_name)
					free(name);
				return 0;
			}
			if (name != f->f_name)
				free(name);
		} else if (linkunlink(f->f_name, tgt) == 0) {
			if (vflag)
				fprintf(stderr, "%s\n", tgt);
			return 0;
		}
cantlink:	errcnt += 1;
	}
	if ((f->f_st.st_mode&S_IFMT) != S_IFDIR && f->f_st.st_nlink > 1 &&
			(fmttype & TYP_CPIO || fmttype == FMT_ZIP
			 || action == 'p') &&
			(i = canlink(tgt, &f->f_st, 1)) != 0) {
		if (i < 0)
			goto skip;
		/*
		 * At this point, hard links in SVR4 cpio format have
		 * been reordered and data is associated with the first
		 * link; remaining links have st_size == 0 so don't
		 * overwrite the data here.
		 */
		if (fmttype & TYP_NCPIO && f->f_st.st_size == 0 ||
				(f->f_st.st_mode&S_IFMT) != S_IFREG) {
			if (vflag)
				fprintf(stderr, "%s\n", f->f_name);
			return 0;
		}
		/*
		 * Make sure we can creat() this file later.
		 */
		chmod(tgt, 0600);
	} else if (fmttype & TYP_TAR && f->f_st.st_nlink > 1) {
		if (linkunlink(f->f_lnam, f->f_name) == 0) {
			if (fmttype & TYP_USTAR && f->f_st.st_size > 0)
				chmod(tgt, 0600);
			else {
				if (vflag)
					fprintf(stderr, "%s\n", f->f_name);
				return 0;
			}
		} else {
			goto restore;
		}
	} else if (new == 0 && (f->f_st.st_mode&S_IFMT) != S_IFDIR) {
		len = strlen(tgt);
		temp = smalloc(len + 7);
		strcpy(temp, tgt);
		strcpy(&temp[len], "XXXXXX");
		if ((fd = mkstemp(temp)) < 0 || close(fd) < 0) {
			emsg(3, "Cannot create temporary file");
			if (fd < 0) {
				free(temp);
				temp = NULL;
			}
			goto skip;
		}
		cur_ofile = tgt;
		cur_tfile = temp;
		if (rename(tgt, temp) < 0) {
			emsg(3, "Cannot rename current \"%s\"", tgt);
			tunlink(&temp);
			goto skip;
		}
	}
	switch (f->f_st.st_mode & S_IFMT) {
	case S_IFDIR:
		if (!dflag) {
			if (action == 'p')
				msg(-1, 0, "Use -d option to copy \"%s\"\n",
						f->f_name);
			goto restore;
		}
		if (mkdir(tgt, 0777) < 0 && errno != EEXIST) {
			emsg(-1, "Cannot create directory \"%s\"", tgt);
			goto restore;
		}
		break;
	case S_IFLNK:
		if (symlink(f->f_lnam, tgt) < 0) {
			emsg(3, "Cannot create \"%s\"", tgt);
			goto restore;
		}
		break;
	case S_IFREG:
		if (temp && f->f_fd < 0)
			goto restore;
		cur_ofile = tgt;
		if ((fd = (compressed_bar ? zcreat : creat)(tgt,
						f->f_st.st_mode & 0777)) < 0) {
			emsg(3, "Cannot create \"%s\"", tgt);
			goto skip;
		}
		failure = 1;
		if (copydata(f, tgt, fd) != 0) {
			close(fd);
			goto restore;
		}
		if ((compressed_bar ? zclose : close)(fd) < 0) {
			emsg(3, "Close error on \"%s\"", tgt);
			goto restore;
		}
		break;
	case S_IFBLK:
	case S_IFCHR:
	case S_IFIFO:
	case S_IFNAM:
	case S_IFNWK:
		if (mknod(tgt, f->f_st.st_mode&(S_IFMT|0777),
					f->f_st.st_rdev) < 0) {
			emsg(3, "Cannot mknod() \"%s\"", tgt);
			goto restore;
		}
		break;
	default:
		msg(-1, 0, "Impossible file type\n");
		goto skip;
	}
	if (vflag)
		fprintf(stderr, "%s\n", f->f_name);
	tunlink(&temp);
	return setattr(tgt, &f->f_st);
skip:	if (copydata == indata)
		skipdata(f, copydata);
restore:
	if (temp) {
		if (rename(temp, tgt) < 0) {
			emsg(3, "Cannot recover original version of \"%s\"",
					tgt);
			tunlink(&temp);
		} else
			fprintf(stderr, "Restoring existing \"%s\"\n", tgt);
		cur_tfile = cur_ofile = NULL;
	}
	return failure;
}

static int
linkunlink(const char *path1, const char *path2)
{
	int	twice = 0;

	do {
		if (link(path1, path2) == 0) {
			if (vflag && pax == PAX_TYPE_CPIO)
				printf("%s linked to %s\n", path1, path2);
			return 0;
		}
		if (errno == EEXIST && unlink(path2) < 0)
			emsg(3, sysv3 ? "cannot unlink <%s>" :
					"Error cannot unlink \"%s\"", path2);
	} while (twice++ == 0);
	emsg(023, sysv3 ? "Cannot link <%s> & <%s>" :
			"Cannot link \"%s\" and \"%s\"", path1, path2);
	return -1;
}

static void
tunlink(char **fn)
{
	cur_tfile = cur_ofile = NULL;
	if (*fn) {
		if (unlink(*fn) < 0)
			emsg(3, "Cannot unlink() temp file \"%s\"", *fn);
		free(*fn);
		*fn = NULL;
	}
}

/*
 * For -it[v] option.
 */
static int
filet(struct file *f, int (*copydata)(struct file *, const char *, int))
{
	if (vflag && (
		(fmttype == FMT_ZIP && f->f_gflag & FG_DESC &&
		(f->f_cmethod == C_DEFLATED || f->f_cmethod == C_ENHDEFLD) &&
				(f->f_gflag & FG_CRYPT) == 0) ||
		f->f_Kbase)) {
		/*
		 * Need to read zip data descriptor located after the
		 * file contents in order to know the file size. Or
		 * need to read second header of -K archive.
		 */
		int	i;
		i = skipdata(f, copydata);
		filev(f);
		return i;
	}
	if (vflag)
		filev(f);
	else
		puts(f->f_name);
	return skipdata(f, copydata);
}

static void
filev(struct file *f)
{
	const char	*cp;
	long	c;

	if (pax == PAX_TYPE_CPIO && fmttype & TYP_TAR && f->f_st.st_nlink > 1)
		printf("%s linked to %s\n", f->f_lnam, f->f_name);
	if (sysv3)
		printf("%-6o", (int)f->f_st.st_mode&(07777|S_IFMT));
	else {
		c = typec(&f->f_st);
		putchar(c);
		permbits(f->f_st.st_mode);
	}
	if (fmttype == FMT_ZIP && vflag > 1)
		zipinfo(f);
	else {
		if (sysv3 == 0)
			printf(" %3d", fmttype&TYP_TAR &&
					(f->f_st.st_mode&S_IFMT) == S_IFLNK ?
					2 : (int)f->f_st.st_nlink);
		if ((cp = getuser(f->f_st.st_uid)) != NULL)
			printf(sysv3 ? " %-6s" : " %-8s", cp);
		else
			printf(sysv3 ? " %-6lu" : " %-8lu",
					(long)f->f_st.st_uid);
		if (sysv3 == 0) {
			if ((cp = getgroup(f->f_st.st_gid)) != NULL)
				printf(" %-8s", cp);
			else
				printf(" %-8lu", (long)f->f_st.st_gid);
		}
	}
	if (sysv3 || (f->f_st.st_mode&S_IFMT)!=S_IFCHR &&
			(f->f_st.st_mode&S_IFMT)!=S_IFBLK &&
			(f->f_st.st_mode&S_IFMT)!=S_IFNAM &&
			(f->f_st.st_mode&S_IFMT)!=S_IFNWK)
		printf(pax != PAX_TYPE_CPIO ? "%8llu" :
				sysv3 ? "%7llu" : " %-7llu", f->f_dsize);
	else
		printf(" %3lu,%3lu", (long)f->f_rmajor, (long)f->f_rminor);
	prtime(f->f_st.st_mtime);
	printf("%s", f->f_name);
	if ((f->f_st.st_mode&S_IFMT) == S_IFLNK)
		printf(" -> %s", f->f_lnam);
	if (pax != PAX_TYPE_CPIO && (f->f_st.st_mode&S_IFMT) != S_IFDIR &&
			f->f_st.st_nlink>1) {
		if (fmttype & TYP_TAR)
			printf(" == %s", f->f_lnam);
		else
			canlink(f->f_name, &f->f_st, 0);
	}
	putchar('\n');
}

static int
typec(struct stat *st)
{
	switch (st->st_mode&S_IFMT) {
	case S_IFREG:
		return '-';
	case S_IFDIR:
		return 'd';
	case S_IFLNK:
		return 'l';
	case S_IFCHR:
		return 'c';
	case S_IFBLK:
		return 'b';
	case S_IFIFO:
		return 'p';
	case S_IFNWK:
		return 'n';
	case S_IFNAM:
		switch (st->st_rdev) {
		case S_INSEM:
			return 's';
		case S_INSHD:
			return 'm';
		}
		/*FALLTHRU*/
	default:
		msg(-1, 0, "Impossible file type\n");
		errcnt++;
		return '-';
	}
}

static void
permbits(mode_t mode)
{
	mode_t	mask = 0700, shft = 6;

	while (mask) {
		if (((mode & mask) >> shft) & 04)
			putchar('r');
		else
			putchar('-');
		if (((mode & mask) >> shft) & 02)
			putchar('w');
		else
			putchar('-');
		if (mask == 0700 && mode & 04000)
			putchar('s');
		else if (mask == 0070 && (mode & 02010) == 02010)
			putchar('s');
		else if (mask == 0070 && mode & 02000)
			putchar('l');
		else if (mask == 0007 && mode & 01000)
			putchar('t');
		else if (((mode & mask) >> shft) & 01)
			putchar('x');
		else
			putchar('-');
		mask >>= 3;
		shft -= 3;
	}
}

static void
prtime_cpio(time_t t)
{
	char	b[30];

	strftime(b, sizeof b, sysv3 ? "%b %e %H:%M:%S %Y" : "%b %e %H:%M %Y",
			localtime(&t));
	printf(sysv3 ? "  %s  " : " %s, ", b);
}

/*
 * Prepare path to add names of files below it later.
 */
static void
getpath(const char *path, char **file, char **filend, size_t *sz, size_t *slen)
{
	*sz = 14 + strlen(path) + 2;
	*file = smalloc(*sz);
	*filend = *file;
	if (path[0] == '/' && path[1] == '\0')
		*(*filend)++ = '/';
	else {
		const char	*cp = path;
		while ((*(*filend)++ = *cp++) != '\0');
		(*filend)[-1] = '/';
	}
	*slen = *filend - *file;
}

/*
 * Concatenate base (prepared with getpath()) and file.
 */
static void
setpath(const char *base, char **file, char **filend,
		size_t slen, size_t *sz, size_t *ss)
{
	if (slen + (*ss = strlen(base)) >= *sz) {
		*sz += slen + *ss + 15;
		*file = srealloc(*file, *sz);
		*filend = &(*file)[slen];
	}
	strcpy(*filend, base);
}

/*
 * Create intermediate directories with -m.
 */
static int
imdir(char *fn)
{
	struct stat	st;
	char	*cp;
	int	dfl = dflag != 0;

	for (cp = fn; *cp == '/'; cp++);
	do {
		while (*cp && *cp != '/')
			cp++;
		if (*cp == '/') {
			*cp = '\0';
			if (stat(fn, &st) < 0) {
				if (dfl) {
					if (mkdir(fn, 0777) < 0) {
						*cp = '/';
						emsg(-1, "Cannot create "
							"directory for \"%s\"",
							fn);
						return -1;
					}
				} else {
					msg(-1, 0, sysv3 ?
						"missing 'd' option\n" :
						"Missing -d option\n");
					dfl = 2;
				}
			}
			*cp = '/';
			while (*cp == '/')
				cp++;
		}
	} while (*cp);
	return 0;
}

/*
 * Set file attributes and make sure that suid/sgid bits are only restored
 * if the owner is the same as on the tape.
 */
static int
setattr(const char *fn, struct stat *st)
{
	mode_t	mode = st->st_mode & 07777;
	uid_t	uid = Rflag ? Ruid : myuid;
	gid_t	gid = Rflag ? Rgid : mygid;

	if ((pax != PAX_TYPE_CPIO || myuid == 0) &&
			(pax == PAX_TYPE_CPIO ||
			 pax_preserve&(PAX_P_OWNER|PAX_P_EVERY))) {
		if (setowner(fn, st) != 0)
			return 1;
	}
	if (pax != PAX_TYPE_CPIO &&
			(pax_preserve&(PAX_P_OWNER|PAX_P_EVERY)) == 0)
		mode &= ~(mode_t)(S_ISUID|S_ISGID);
	if (myuid != 0 || Rflag) {
		if (st->st_uid != uid || st->st_gid != gid) {
			mode &= ~(mode_t)S_ISUID;
			if ((st->st_mode&S_IFMT) != S_IFDIR && mode & 0010)
				mode &= ~(mode_t)S_ISGID;
			if ((st->st_mode&S_IFMT) == S_IFDIR && st->st_gid!=gid)
				mode &= ~(mode_t)S_ISGID;
		}
	}
	if ((st->st_mode&S_IFMT) == S_IFLNK)
		return 0;
	if (hp_Uflag)
		mode &= ~umsk|S_IFMT;
	if (pax != PAX_TYPE_CPIO &&
			(pax_preserve&(PAX_P_MODE|PAX_P_OWNER|PAX_P_EVERY))==0)
		mode &= 01777|S_IFMT;
	if ((pax == PAX_TYPE_CPIO || pax_preserve&(PAX_P_MODE|PAX_P_EVERY)) &&
			chmod(fn, mode) < 0) {
		emsg(2, "Cannot chmod() \"%s\"", fn);
		return 1;
	}
	if (pax != PAX_TYPE_CPIO ?
			(pax_preserve&(PAX_P_ATIME|PAX_P_MTIME|PAX_P_EVERY)) !=
			(PAX_P_ATIME|PAX_P_MTIME) : mflag)
		return rstime(fn, st, "modification");
	else
		return 0;
}

static int
setowner(const char *fn, struct stat *st)
{
	uid_t	uid = Rflag ? Ruid : myuid ? myuid : st->st_uid;
	gid_t	gid = Rflag ? Rgid : st->st_gid;

	if (((st->st_mode&S_IFMT)==S_IFLNK?lchown:chown)(fn, uid, gid) < 0) {
		emsg(2, "Cannot chown() \"%s\"", fn);
		return 1;
	}
	if (pax >= PAX_TYPE_PAX2001 && myuid && myuid != st->st_uid &&
			pax_preserve & (PAX_P_OWNER|PAX_P_EVERY)) {
		/*
		 * Do not even try to preserve user ownership in this case.
		 * It would either fail, or, without _POSIX_CHOWN_RESTRICTED,
		 * leave us with a file we do not own and which we thus could
		 * not chmod() later.
		 */
		errno = EPERM;
		emsg(2, "Cannot chown() \"%s\"", fn);
		return 1;
	}
	return 0;
}

/*
 * For -i and -p: Check if device/inode have been created already and
 * if so, link path (or print the link name).
 */
static int
canlink(const char *path, struct stat *st, int really)
{
	struct dslot	*ds;
	struct islot	*ip;

	ds = dfind(&devices, st->st_dev);
	if ((ip = ifind(st->st_ino, &ds->d_isl)) == NULL ||
			ip->i_name == NULL) {
		if (ip == NULL) {
			ip = scalloc(1, sizeof *ip);
			ip->i_ino = st->st_ino;
			if ((fmttype&(TYP_NCPIO|TYP_TAR)) == 0)
				ip->i_nlk = st->st_nlink;
			iput(ip, &ds->d_isl);
		}
		ip->i_name = sstrdup(path);
	} else {
		if ((fmttype&(TYP_NCPIO|TYP_TAR)) == 0) {
			/*
			 * If an archive inode has more links than given in
			 * st_nlink, write a new disk inode. See the comments
			 * to storelink() for the rationale.
			 */
			if (--ip->i_nlk == 0) {
				free(ip->i_name);
				ip->i_name = sstrdup(path);
				ip->i_nlk = st->st_nlink;
				return 0;
			}
		}
		if (really) {
			/*
			 * If there was file data before the last link with
			 * SVR4 cpio formats, do not create a hard link.
			 */
			if (fmttype & TYP_NCPIO && ip->i_nlk == 0)
				return 0;
			if (linkunlink(ip->i_name, path) == 0)
				return 1;
			else
				return -1;
		} else {
			printf(" == %s", ip->i_name);
			return 1;
		}
	}
	return 0;
}

/*
 * Execution for -i.
 */
static void
doinp(void)
{
	union bincpio	bc;
	struct file	f;
	int	n;

	memset(&f, 0, sizeof f);
	if (Eflag)
		patfile();
	if (Iflag) {
		if ((mt = open(Iflag, O_RDONLY)) < 0) {
			if (sysv3) {
				emsg(3, "Cannot open <%s> for input.", Iflag);
				done(1);
			}
			else
				msg(3, -2, "Cannot open \"%s\" for input\n",
						Iflag);
		}
	} else
		mt = dup(0);
	mstat();
	blkbuf = svalloc(blksiz, 1);
	if ((n = mread()) == 0)
		unexeoa();
	else if (n < 0) {
		emsg(3, "Read error on \"%s\"",
			Iflag && !sysv3 ? Iflag : "input");
		done(1);
	}
	if (kflag == 0 && sixflag == 0)
		fmttype = FMT_NONE;
	if (fmttype == FMT_NONE)
		whathdr();
	else
		formatforced = 1;
	while (readhdr(&f, &bc) == 0) {
		if (fmttype & TYP_NCPIO && f.f_st.st_nlink > 1 &&
				(f.f_st.st_mode&S_IFMT) != S_IFDIR) {
			if (f.f_st.st_size != 0)
				flushlinks(&f);
			else
				storelink(&f);
		} else
			inpone(&f, 1);
		f.f_Kbase = f.f_Krest = f.f_Ksize = 0;
		f.f_oflag = 0;
	}
	if (fmttype & TYP_NCPIO)
		flushrest(f.f_pad);
}

/*
 * SVR4 cpio link handling with -i; called if we assume that more links
 * to this file will follow.
 */
static void
storelink(struct file *f)
{
	struct dslot	*ds;
	struct islot	*ip;
	struct ilink	*il, *iz;

	ds = dfind(&devices, f->f_st.st_dev);
	if ((ip = ifind(f->f_st.st_ino, &ds->d_isl)) == NULL) {
		ip = scalloc(1, sizeof *ip);
		ip->i_ino = f->f_st.st_ino;
		ip->i_nlk = f->f_st.st_nlink;
		ip->i_st = smalloc(sizeof *ip->i_st);
		*ip->i_st = f->f_st;
		iput(ip, &ds->d_isl);
	} else {
		if (ip->i_nlk == 0) {
			/*
			 * More links to an inode than given in the first
			 * st_nlink occurence are found within this archive.
			 * This happens if -L was specified and soft links
			 * point to a file with multiple hard links. We do
			 * the same as SVR4 cpio implementations here and
			 * associate these links with a new inode, since it
			 * is related to the way a file with a single hard
			 * link but referenced by soft links is unpacked.
			 */
			ip->i_nlk = f->f_st.st_nlink;
			if (ip->i_name)
				free(ip->i_name);
			ip->i_name = NULL;
			ip->i_st = smalloc(sizeof *ip->i_st);
			*ip->i_st = f->f_st;
		} else if (ip->i_nlk <= 2) {
			/*
			 * We get here if
			 * - a file with multiple links is empty;
			 * - a broken implementation has stored file data
			 *   before the last link;
			 * - a linked file has been added to the archive later
			 *   again (does not happen with our implementation).
			 */
			flushnode(ip, f);
			return;
		} else
			ip->i_nlk--;
	}
	for (il = ip->i_lnk, iz = NULL; il; il = il->l_nxt)
		iz = il;
	il = scalloc(1, sizeof *il);
	il->l_siz = strlen(f->f_name) + 1;
	il->l_nam = smalloc(il->l_siz);
	strcpy(il->l_nam, f->f_name);
	if (iz)
		iz->l_nxt = il;
	else
		ip->i_lnk = il;
}

/*
 * Flush all links of a file with SVR4 cpio format and -i.
 */
static void
flushlinks(struct file *f)
{
	struct dslot	*ds;
	struct islot	*ip;

	ds = dfind(&devices, f->f_st.st_dev);
	ip = ifind(f->f_st.st_ino, &ds->d_isl);
	flushnode(ip, f);
}

/*
 * Data of a multi-linked file shall be transferred now for SVR4 cpio
 * format and -i.
 */
static void
flushnode(struct islot *ip, struct file *f)
{
	struct file	nf;
	struct ilink	*il, *iz;

	f->f_dsize = f->f_st.st_size;
	if (ip && ip->i_nlk > 0) {
		/*
		 * Write out the file data with the first link the user
		 * wants to be extracted, but show the same display size
		 * for all links.
		 */
		for (il = ip->i_lnk, iz = NULL; il;
				iz = il, il = il->l_nxt, iz ? free(iz), 0 : 0) {
			memset(&nf, 0, sizeof nf);
			nf.f_name = il->l_nam;
			nf.f_nsiz = il->l_siz;
			nf.f_st = f->f_st;
			nf.f_chksum = f->f_chksum;
			nf.f_pad = f->f_pad;
			nf.f_dsize = f->f_dsize;
			if (inpone(&nf, 0) == 0) {
				f->f_chksum = 0;
				f->f_st.st_size = 0;
			}
			free(il->l_nam);
		}
		ip->i_lnk = NULL;
		if (ip->i_st)
			free(ip->i_st);
		ip->i_st = NULL;
	}
	if (f->f_name)
		inpone(f, 1);
	if (ip) {
		if (ip->i_nlk <= 2 && ip->i_name) {
			free(ip->i_name);
			ip->i_name = NULL;
		}
		ip->i_nlk = 0;
	}
}

/*
 * This writes all remaining multiply linked files, i. e. those with
 * st_size == 0 and st_nlink > number of links within the archive.
 */
static void
flushrest(int pad)
{
	struct dslot	*ds;

	for (ds = devices; ds; ds = ds->d_nxt)
		if (ds->d_isl != NULL)
			flushtree(ds->d_isl, pad);
}

static void
flushtree(struct islot *ip, int pad)
{
	struct file	f;

	if (ip == inull)
		return;
	flushtree(ip->i_lln, pad);
	flushtree(ip->i_rln, pad);
	if (ip->i_nlk > 0 && ip->i_st) {
		memset(&f, 0, sizeof f);
		f.f_st = *ip->i_st;
		f.f_pad = pad;
		flushnode(ip, &f);
	}
}

/*
 * See if the user wants to have this file with -i, and extract it or
 * skip its data on the tape.
 */
static int
inpone(struct file *f, int shallskip)
{
	struct glist	*gp = NULL, *gb = NULL;
	int	val = -1, selected = 0;

	if ((patterns == NULL || (gp = want(f, &gb)) != NULL ^ fflag) &&
			pax_track(f->f_name, f->f_st.st_mtime)) {
		selected = 1;
		if ((pax_sflag == 0 || pax_sname(&f->f_name, &f->f_nsiz)) &&
				(rflag == 0 || rname(&f->f_name, &f->f_nsiz))) {
			errcnt += infile(f);
			val = 0;
		}
	} else if (shallskip)
		errcnt += skipfile(f);
	if (gp != NULL && selected) {
		gp->g_gotcha = 1;
		if (gp->g_nxt && gp->g_nxt->g_art)
			gp->g_nxt->g_gotcha = 1;
		else if (gp->g_art && gb)
			gb->g_gotcha = 1;
	}
	return val;
}

/*
 * Read the header for a file with -i. Return values: 0 okay, 1 end of
 * archive, -1 failure.
 */
static int
readhdr(struct file *f, union bincpio *bp)
{
	long	namlen, l1, l2, rd, hsz;
	static long	attempts;
	long long	skipped = 0;

	if (fmttype & TYP_TAR) {
		if (f->f_name)
			f->f_name[0] = '\0';
		if (f->f_lnam)
			f->f_lnam[0] = '\0';
	}
	paxrec = globrec;
retry:	f->f_st = globst;
retry2:	if (fmttype & TYP_BINARY) {
		hsz = SIZEOF_hdr_cpio;
		if (attempts == 0 && bread(bp->data, hsz) != hsz)
			unexeoa();
		if ((fmttype&TYP_BE ? pbe16(bp->Hdr.c_magic) :
				ple16(bp->Hdr.c_magic)) != mag_bin)
			goto badhdr;
		if (fmttype & TYP_BE) {
			f->f_st.st_dev = pbe16(bp->Hdr.c_dev)&0177777;
			f->f_st.st_ino = pbe16(bp->Hdr.c_ino)&0177777;
			f->f_st.st_mode = pbe16(bp->Hdr.c_mode);
		} else {
			f->f_st.st_dev = ple16(bp->Hdr.c_dev)&0177777;
			f->f_st.st_ino = ple16(bp->Hdr.c_ino)&0177777;
			f->f_st.st_mode = ple16(bp->Hdr.c_mode);
		}
		if (sixflag) {
			/*
			 * relevant Unix 6th Edition style mode bits
			 * (according to /usr/sys/inode.h, untested:)
			 *
			 * 060000	IFMT	type of file
			 * 040000	IFDIR	directory
			 * 020000	IFCHR	character special
			 * 060000	IFBLK	block special, 0 is regular
			 * 007777		permission bits as today
			 */
			f->f_st.st_mode &= 067777;
			if ((f->f_st.st_mode & 060000) == 0)
				f->f_st.st_mode |= S_IFREG;
		}
		if (fmttype & TYP_BE) {
			f->f_st.st_uid = pbe16(bp->Hdr.c_uid)&0177777;
			f->f_st.st_gid = pbe16(bp->Hdr.c_gid)&0177777;
			f->f_st.st_nlink = pbe16(bp->Hdr.c_nlink)&0177777;
			f->f_st.st_mtime = pbe32(bp->Hdr.c_mtime);
			f->f_st.st_rdev = pbe16(bp->Hdr.c_rdev);
			namlen = pbe16(bp->Hdr.c_namesize);
			f->f_st.st_size = pbe32(bp->Hdr.c_filesize)&0xFFFFFFFF;
		} else {
			f->f_st.st_uid = ple16(bp->Hdr.c_uid)&0177777;
			f->f_st.st_gid = ple16(bp->Hdr.c_gid)&0177777;
			f->f_st.st_nlink = ple16(bp->Hdr.c_nlink)&0177777;
			f->f_st.st_mtime = pme32(bp->Hdr.c_mtime);
			f->f_st.st_rdev = ple16(bp->Hdr.c_rdev);
			namlen = ple16(bp->Hdr.c_namesize);
			f->f_st.st_size = pme32(bp->Hdr.c_filesize)&0xFFFFFFFF;
		}
		f->f_rmajor = major(f->f_st.st_rdev);
		f->f_rminor = minor(f->f_st.st_rdev);
		if ((f->f_st.st_rdev&0xFFFF) == 0xFFFF &&
				f->f_st.st_size > 0 &&
				((f->f_st.st_mode&S_IFMT) == S_IFBLK ||
				 (f->f_st.st_mode&S_IFMT) == S_IFCHR &&
				 (!formatforced || fmttype & TYP_SGI))) {
			fmttype |= TYP_SGI;
			f->f_rmajor = (f->f_st.st_size&0xFFFC0000)>>18;
			f->f_rminor = f->f_st.st_size&0x0003FFFF;
			f->f_st.st_rdev = makedev(f->f_rmajor, f->f_rminor);
			f->f_st.st_size = 0;
		}
		if ((f->f_st.st_mode&S_IFMT) == S_IFREG &&
				f->f_st.st_size&0x80000000 &&
				(!formatforced || fmttype & TYP_SGI)) {
			fmttype |= TYP_SGI;
			f->f_Ksize = f->f_st.st_size&0xFFFFFFFF;
			f->f_Kbase = (0x100000000LL-f->f_st.st_size) *
				0x80000000;
			f->f_st.st_size = 0;
		}
		f->f_pad = 2;
		rd = SIZEOF_hdr_cpio;
	} else if (fmttype == FMT_ODC) {
		hsz = SIZEOF_c_hdr;
		if (attempts == 0 && bread(bp->data, hsz) != hsz)
			unexeoa();
		if(memcmp(bp->Cdr.c_magic, mag_odc, sizeof mag_odc))
			goto badhdr;
		f->f_st.st_dev = rdoct(bp->Cdr.c_dev, 6);
		f->f_st.st_ino = rdoct(bp->Cdr.c_ino, 6);
		f->f_st.st_mode = rdoct(bp->Cdr.c_mode, 6);
		f->f_st.st_uid = rdoct(bp->Cdr.c_uid, 6);
		f->f_st.st_gid = rdoct(bp->Cdr.c_gid, 6);
		f->f_st.st_nlink = rdoct(bp->Cdr.c_nlink, 6);
		f->f_st.st_rdev = rdoct(bp->Cdr.c_rdev, 6);
		f->f_rmajor = major(f->f_st.st_rdev);
		f->f_rminor = minor(f->f_st.st_rdev);
		f->f_st.st_mtime = rdoct(bp->Cdr.c_mtime, 11);
		namlen = rdoct(bp->Cdr.c_namesz, 6);
		f->f_st.st_size = rdoct(bp->Cdr.c_filesz, 11);
		f->f_pad = 1;
		rd = SIZEOF_c_hdr;
	} else if (fmttype == FMT_DEC) {
		hsz = SIZEOF_d_hdr;
		if (attempts == 0 && bread(bp->data, hsz) != hsz)
			unexeoa();
		if(memcmp(bp->Ddr.d_magic, mag_odc, sizeof mag_odc))
			goto badhdr;
		f->f_st.st_dev = rdoct(bp->Ddr.d_dev, 6);
		f->f_st.st_ino = rdoct(bp->Ddr.d_ino, 6);
		f->f_st.st_mode = rdoct(bp->Ddr.d_mode, 6);
		f->f_st.st_uid = rdoct(bp->Ddr.d_uid, 6);
		f->f_st.st_gid = rdoct(bp->Ddr.d_gid, 6);
		f->f_st.st_nlink = rdoct(bp->Ddr.d_nlink, 6);
		f->f_rmajor = rdoct(bp->Ddr.d_rmaj, 8);
		f->f_rminor = rdoct(bp->Ddr.d_rmin, 8);
		f->f_st.st_rdev = makedev(f->f_rmajor, f->f_rminor);
		f->f_st.st_mtime = rdoct(bp->Ddr.d_mtime, 11);
		namlen = rdoct(bp->Ddr.d_namesz, 6);
		f->f_st.st_size = rdoct(bp->Ddr.d_filesz, 11);
		f->f_pad = 1;
		rd = SIZEOF_d_hdr;
	} else if (fmttype & TYP_NCPIO) {
		hsz = SIZEOF_Exp_cpio_hdr;
		if (attempts == 0 && bread(bp->data, hsz) != hsz)
			unexeoa();
		if (memcmp(bp->Edr.E_magic, mag_asc, sizeof mag_asc) &&
		    memcmp(bp->Edr.E_magic, mag_crc, sizeof mag_crc))
			goto badhdr;
		f->f_st.st_ino = rdhex(bp->Edr.E_ino, 8);
		f->f_st.st_mode = rdhex(bp->Edr.E_mode, 8);
		f->f_st.st_uid = rdhex(bp->Edr.E_uid, 8);
		f->f_st.st_gid = rdhex(bp->Edr.E_gid, 8);
		f->f_st.st_nlink = rdhex(bp->Edr.E_nlink, 8);
		f->f_st.st_mtime = rdhex(bp->Edr.E_mtime, 8);
		f->f_st.st_size = rdhex(bp->Edr.E_filesize, 8);
		l1 = rdhex(bp->Edr.E_maj, 8);
		l2 = rdhex(bp->Edr.E_min, 8);
		f->f_st.st_dev = makedev(l1, l2);
		f->f_rmajor = rdhex(bp->Edr.E_rmaj, 8);
		f->f_rminor = rdhex(bp->Edr.E_rmin, 8);
		f->f_st.st_rdev = makedev(f->f_rmajor, f->f_rminor);
		namlen = rdhex(bp->Edr.E_namesize, 8);
		f->f_chksum = rdhex(bp->Edr.E_chksum, 8);
		f->f_pad = 4;
		rd = SIZEOF_Exp_cpio_hdr;
	} else if (fmttype & TYP_CRAY) {
		int	diff5 = fmttype==FMT_CRAY5 ? CRAY_PARAMSZ : 0;
		hsz = SIZEOF_cray_hdr - diff5;
		if (attempts == 0 && bread(bp->data, hsz) != hsz)
			unexeoa();
		if (pbe64(bp->Crayhdr.C_magic) != mag_bin)
			goto badhdr;
		f->f_st.st_dev = pbe64(bp->Crayhdr.C_dev);
		f->f_st.st_ino = pbe64(bp->Crayhdr.C_ino);
		f->f_st.st_mode = pbe64(bp->Crayhdr.C_mode);
		/*
		 * Some of the S_IFMT bits on Cray UNICOS 9 differ from
		 * the common Unix values, namely:
		 *
		 * S_IFLNK	0130000		symbolic link
		 * S_IFOFD	0110000		off line, with data
		 * S_IFOFL	0120000		off line, with no data
		 *
		 * We treat the latter ones as regular files.
		 */
		if ((f->f_st.st_mode&S_IFMT) == 0130000)
			f->f_st.st_mode = f->f_st.st_mode&07777|S_IFLNK;
		else if ((f->f_st.st_mode&S_IFMT) == 0110000 ||
				(f->f_st.st_mode&S_IFMT) == 0120000)
			f->f_st.st_mode = f->f_st.st_mode&07777|S_IFREG;
		f->f_st.st_uid = pbe64(bp->Crayhdr.C_uid);
		f->f_st.st_gid = pbe64(bp->Crayhdr.C_gid);
		f->f_st.st_nlink = pbe64(bp->Crayhdr.C_nlink);
		f->f_st.st_mtime = pbe64(bp->Crayhdr.C_mtime - diff5);
		f->f_st.st_rdev = pbe64(bp->Crayhdr.C_rdev);
		f->f_rmajor = major(f->f_st.st_rdev);
		f->f_rminor = minor(f->f_st.st_rdev);
		f->f_st.st_size = pbe64(bp->Crayhdr.C_filesize - diff5);
		namlen = pbe64(bp->Crayhdr.C_namesize - diff5);
		f->f_pad = 1;
		rd = SIZEOF_cray_hdr - diff5;
	} else if (fmttype & TYP_BAR) {
		hsz = 512;
		if (attempts == 0 && bread(bp->data, hsz) != hsz)
			unexeoa();
		if (bp->data[SIZEOF_bar_header] == '\0') {
			if (kflag && !allzero(bp->data, hsz))
				goto badhdr;
			return 1;
		}
		if (trdsum(bp) != 0 && kflag == 0)
			goto badhdr;
		bp->data[512] = bp->data[513] = '\0';
		if (f->f_name == NULL || f->f_name[0] == '\0') {
			if (f->f_nsiz < 512) {
				free(f->f_name);
				f->f_name = smalloc(f->f_nsiz = 512);
			}
			strcpy(f->f_name, &bp->data[SIZEOF_bar_header]);
		}
		namlen = strlen(f->f_name) + 1;
		f->f_st.st_mode = rdoct(bp->Bdr.b_mode, 8);
		f->f_st.st_uid = rdoct(bp->Bdr.b_uid, 8);
		f->f_st.st_gid = rdoct(bp->Bdr.b_gid, 8);
		f->f_st.st_size = rdoct(bp->Bdr.b_size, 12);
		f->f_st.st_mtime = rdoct(bp->Bdr.b_mtime, 12);
		f->f_st.st_rdev = rdoct(bp->Bdr.b_rdev, 8);
		f->f_rmajor = major(f->f_st.st_rdev);
		f->f_rminor = minor(f->f_st.st_rdev);
		switch (bp->Bdr.b_linkflag) {
		case '0':
			f->f_st.st_mode &= 07777;
			if (f->f_name[namlen-2] == '/')
				f->f_st.st_mode |= S_IFDIR;
			else
				f->f_st.st_mode |= S_IFREG;
			break;
		case '1':
			if (f->f_lnam == NULL || f->f_lnam[0] == '\0')
				blinkof(bp->data, f, namlen);
			f->f_st.st_nlink = 2;
			if ((f->f_st.st_mode&S_IFMT) == 0)
				f->f_st.st_mode |= S_IFREG;
			break;
		case '2':
			if (f->f_lnam == NULL || f->f_lnam[0] == '\0')
				blinkof(bp->data, f, namlen);
			f->f_st.st_mode &= 07777;
			f->f_st.st_mode |= S_IFLNK;
		}
		f->f_pad = 512;
		rd = 512;
	} else if (fmttype & TYP_TAR) {
		hsz = 512;
		if (attempts == 0 && bread(bp->data, hsz) != hsz)
			unexeoa();
		if (bp->Tdr.t_name[0] == '\0') {
			if (kflag && !allzero(bp->data, hsz))
				goto badhdr;
			return 1;
		}
		if (fmttype == FMT_GNUTAR) {
			if (memcmp(bp->Tdr.t_magic, mag_gnutar, 8))
				goto badhdr;
		} else if (fmttype & TYP_USTAR) {
			if (memcmp(bp->Tdr.t_magic, mag_ustar, 6))
				goto badhdr;
		}
		if (trdsum(bp) != 0 && kflag == 0)
			goto badhdr;
		if ((fmttype & TYP_PAX) == 0 && (fmttype == FMT_GNUTAR ||
				strcmp(bp->Tdr.t_name, "././@LongLink") == 0)) {
			switch (bp->Tdr.t_linkflag) {
			case 'L':
				if (readgnuname(&f->f_name, &f->f_nsiz,
						rdoct(bp->Tdr.t_size, 12)) < 0)
					goto badhdr;
				goto retry;
			case 'K':
				if (readgnuname(&f->f_lnam, &f->f_lsiz,
						rdoct(bp->Tdr.t_size, 12)) < 0)
					goto badhdr;
				goto retry;
			}
		}
		if (fmttype & TYP_USTAR && fmttype != FMT_GNUTAR) {
			switch (bp->Tdr.t_linkflag) {
			case 'g':
			case 'x':
				if (formatforced && fmttype != FMT_PAX)
					break;
				fmttype = FMT_PAX;
				tgetpax(&bp->Tdr, f);
				goto retry2;
			case 'X':
				if (formatforced && fmttype != FMT_SUN)
					break;
				fmttype = FMT_SUN;
				tgetpax(&bp->Tdr, f);
				goto retry2;
			}
		}
		if (f->f_name == NULL || f->f_name[0] == '\0') {
			if (f->f_nsiz < TNAMSIZ+TPFXSIZ+2) {
				free(f->f_name);
				f->f_name= smalloc(f->f_nsiz=TNAMSIZ+TPFXSIZ+2);
			}
			tnameof(&bp->Tdr, f->f_name);
		}
		namlen = strlen(f->f_name) + 1;
		f->f_st.st_mode = rdoct(bp->Tdr.t_mode, 8) & 07777;
		if (paxrec & PR_UID)
			/*EMPTY*/;
		else if (fmttype == FMT_GNUTAR && bp->Tdr.t_uid[0] & 0200)
			f->f_st.st_uid = pbe64(bp->Tdr.t_uid) &
				0x7FFFFFFFFFFFFFFFLL;
		else
			f->f_st.st_uid = rdoct(bp->Tdr.t_uid, 8);
		if (paxrec & PR_GID)
			/*EMPTY*/;
		else if (fmttype == FMT_GNUTAR && bp->Tdr.t_gid[0] & 0200)
			f->f_st.st_gid = pbe64(bp->Tdr.t_gid) &
				0x7FFFFFFFFFFFFFFFLL;
		else
			f->f_st.st_gid = rdoct(bp->Tdr.t_gid, 8);
		if (paxrec & PR_SIZE)
			/*EMPTY*/;
		else if (fmttype == FMT_GNUTAR && bp->Tdr.t_size[0] == '\200')
			f->f_st.st_size = pbe64(&bp->Tdr.t_size[4]);
		else
			f->f_st.st_size = rdoct(bp->Tdr.t_size, 12);
		if ((paxrec & PR_MTIME) == 0)
			f->f_st.st_mtime = rdoct(bp->Tdr.t_mtime, TTIMSIZ);
		if (bp->Tdr.t_linkflag == '1') {
			if (f->f_lnam == NULL || f->f_lnam[0] == '\0')
				tlinkof(&bp->Tdr, f);
			f->f_st.st_mode |= S_IFREG;	/* for -tv */
			f->f_st.st_nlink = 2;
		} else if (bp->Tdr.t_linkflag == '2') {
			if (f->f_lnam == NULL || f->f_lnam[0] == '\0')
				tlinkof(&bp->Tdr, f);
			f->f_st.st_mode |= S_IFLNK;
		} else if (tflag == 0 && (f->f_name)[namlen-2] == '/')
			f->f_st.st_mode |= S_IFDIR;
		else
			f->f_st.st_mode |= tifmt(bp->Tdr.t_linkflag);
		if (paxrec & PR_SUN_DEVMAJOR)
			/*EMPTY*/;
		else if (fmttype == FMT_GNUTAR && bp->Tdr.t_devmajor[0] & 0200)
			f->f_rmajor = pbe64(bp->Tdr.t_devmajor) &
				0x7FFFFFFFFFFFFFFFLL;
		else
			f->f_rmajor = rdoct(bp->Tdr.t_devmajor, 8);
		if (paxrec & PR_SUN_DEVMINOR)
			/*EMPTY*/;
		else if (fmttype == FMT_GNUTAR && bp->Tdr.t_devminor[0] & 0200)
			f->f_rminor = pbe64(bp->Tdr.t_devminor) &
				0x7FFFFFFFFFFFFFFFLL;
		else
			f->f_rminor = rdoct(bp->Tdr.t_devminor, 8);
		f->f_st.st_rdev = makedev(f->f_rmajor, f->f_rminor);
		f->f_pad = 512;
		rd = 512;
	} else if (fmttype == FMT_ZIP) {
		hsz = SIZEOF_zip_header;
		if (attempts == 0 && bread(bp->data, hsz) != hsz)
			unexeoa();
		if (memcmp(bp->Zdr.z_signature, mag_zipctr,
					sizeof mag_zipctr) == 0 ||
				memcmp(bp->Zdr.z_signature, mag_zipend,
					sizeof mag_zipend) == 0 ||
				memcmp(bp->Zdr.z_signature, mag_zip64e,
					sizeof mag_zip64e) == 0 ||
				memcmp(bp->Zdr.z_signature, mag_zip64l,
					sizeof mag_zip64l) == 0)
			return 1;
		if (memcmp(bp->Zdr.z_signature, mag_zipsig, sizeof mag_zipsig))
			goto badhdr;
		f->f_st.st_uid = myuid;
		f->f_st.st_gid = mygid;
		f->f_st.st_nlink = 1;
		f->f_st.st_mtime =
			gdostime(bp->Zdr.z_modtime, bp->Zdr.z_moddate);
		f->f_st.st_size = ple32(bp->Zdr.z_nsize);
		f->f_csize = ple32(bp->Zdr.z_csize);
		f->f_cmethod = ple16(bp->Zdr.z_cmethod);
		f->f_chksum = ple32(bp->Zdr.z_crc32);
		f->f_gflag = ple16(bp->Zdr.z_gflag);
		rd = SIZEOF_zip_header;
		namlen = ple16(bp->Zdr.z_namelen)&0177777;
		if (f->f_nsiz < namlen+1) {
			free(f->f_name);
			f->f_name = smalloc(f->f_nsiz = namlen+1);
		}
		if (bread(f->f_name, namlen) != namlen)
			goto corrupt;
		(f->f_name)[namlen] = '\0';
		if (f->f_name[namlen-1] == '/')
			f->f_st.st_mode = 0777&~(mode_t)umsk|S_IFDIR;
		else
			f->f_st.st_mode = 0666&~(mode_t)umsk|S_IFREG;
		rd += namlen;
		if ((l1 = ziprxtra(f, &bp->Zdr)) < 0)
			goto corrupt;
		rd += l1;
		f->f_pad = 1;
	} else
		abort();
	if (fmttype & TYP_CPIO) {
		if (f->f_st.st_nlink <= 0 || namlen <= 0 || namlen >= SANELIMIT)
			goto corrupt;
		if (namlen > f->f_nsiz) {
			if (f->f_name)
				free(f->f_name);
			f->f_name = smalloc(f->f_nsiz = namlen);
		}
		if (bread(f->f_name, namlen) != namlen)
			unexeoa();
		if (f->f_name[namlen-1] != '\0')
			goto corrupt;
		rd += namlen;
		skippad(rd, f->f_pad);
		if (fmttype & TYP_NCPIO &&
				(!formatforced || fmttype & TYP_SCO)) {
			/*
			 * The UnixWare format is a hack, but not a bad
			 * one. It is backwards compatible as far as
			 * possible; old implementations can use the -k
			 * option and will then get only the beginning
			 * of a large file, but all other files in the
			 * archive.
			 */
			if (namlen >= 24 && memcmp(&f->f_name[namlen-23],
						"\0size=", 6) == 0) {
				fmttype |= TYP_SCO;
				f->f_st.st_size = rdhex(&f->f_name[namlen-17],
						16);
			}
		}
	}
	if (attempts) {
		if (tflag)
			fflush(stdout);
		msg(0, 0, sysv3 < 0 ?
			"Re-synced after skipping %lld bytes.\n" :
			"Re-synchronized on magic number/header.\n",
			skipped);
	}
	attempts = 0;
	if (f->f_st.st_atime == 0 || (pax_preserve&(PAX_P_ATIME|PAX_P_EVERY)) ==
			PAX_P_ATIME)
		f->f_st.st_atime = f->f_st.st_mtime;
	if ((f->f_dsize = f->f_st.st_size) < 0)
		goto badhdr;
	if (fmttype & TYP_CPIO && strcmp(f->f_name, trailer) == 0)
		return 1;
	return 0;
corrupt:
	if (kflag) {
		if (tflag)
			fflush(stdout);
		msg(3, 0, "Corrupt header, file(s) may be lost.\n");
	}
badhdr:
	if (kflag) {
		int	next = fmttype & TYP_TAR ? 512 : 1;
		if (attempts++ == 0) {
			if (tflag)
				fflush(stdout);
			msg(1, 0, sysv3 < 0 ? "Out of phase; resyncing.\n" :
				"Searching for magic number/header.\n");
			errcnt++;
		}
		for (l1 = next; l1 < hsz; l1++)
			bp->data[l1-next] = bp->data[l1];
		if (bread(&bp->data[l1-next], next) != next)
			unexeoa();
		skipped++;
		goto retry;
	}
	if (tflag)
		fflush(stdout);
	msg(3, 1, sysv3 < 0 ? "Out of phase--get help\n" :
			sysv3 > 0 ? "Out of sync, bad magic number/header.\n" :
			"Bad magic number/header.\n");
	/*NOTREACHED*/
	return -1;
}

/*
 * Determine the kind of archive on tape.
 */
static void
whathdr(void)
{
again:	if (blktop >= SIZEOF_hdr_cpio &&
			ple16(blkbuf) == mag_bin) {
		fmttype = FMT_BINLE;
	} else if (blktop >= SIZEOF_hdr_cpio &&
			pbe16(blkbuf) == mag_bin) {
		fmttype = FMT_BINBE;
	} else if (blktop >= SIZEOF_c_hdr &&
			memcmp(blkbuf, mag_odc, sizeof mag_odc) == 0) {
		/*
		 * The DEC format is rubbish. Instead of introducing a new
		 * archive magic, its engineers reused the POSIX/odc magic
		 * and changed the fields. But there's a workaround: For a
		 * real odc archive, the byte following the file name is a
		 * \0 byte (unless the archive is damaged, of course). If
		 * it is not a \0 byte, but a \0 byte appears at the
		 * corresponding location for the DEC format, this is
		 * treated as a DEC archive. It must be noted that the
		 * Tru64 UNIX 5.1 cpio command is too stupid even for
		 * doing that - it will spill out a list of complaints
		 * if an extended archive is read but -e is not given.
		 */
		int	ns1, ns2;
		ns1 = rdoct(((struct c_hdr *)blkbuf)->c_namesz, 6);
		ns2 = rdoct(((struct d_hdr *)blkbuf)->d_namesz, 6);
		if (blktop >= SIZEOF_d_hdr &&
				(ns1 >= blktop - SIZEOF_c_hdr ||
				 blkbuf[SIZEOF_c_hdr+ns1-1] != '\0') &&
				ns2 <= blktop - SIZEOF_d_hdr &&
				blkbuf[SIZEOF_d_hdr+ns2-1] == '\0')
			fmttype = FMT_DEC;
		else
			fmttype = FMT_ODC;
	} else if (blktop >= SIZEOF_Exp_cpio_hdr &&
			memcmp(blkbuf, mag_asc, sizeof mag_asc) == 0) {
		fmttype = FMT_ASC;
	} else if (blktop >= SIZEOF_Exp_cpio_hdr &&
			memcmp(blkbuf, mag_crc, sizeof mag_crc) == 0) {
		fmttype = FMT_CRC;
	} else if (blktop >= SIZEOF_cray_hdr &&
			pbe64(blkbuf) == mag_bin) {
		/*
		 * Old and new Cray headers are identical in the first
		 * 64 header bytes (including the magic). cpio(5) on
		 * UNICOS does not describe what the new param field
		 * is for.
		 *
		 * An archive is treated as old format if the mtime
		 * and namesize fields make sense and all characters
		 * of the name are non-null.
		 */
		struct cray_hdr	*Cp = (struct cray_hdr *)blkbuf;
		long long	mtime, namesize;
		fmttype = FMT_CRAY;
		mtime = pbe64(Cp->C_mtime - CRAY_PARAMSZ);
		namesize = pbe64(Cp->C_namesize - CRAY_PARAMSZ);
		if (mtime > 0 && mtime < (1LL<<31) &&
				namesize > 0 && namesize < 2048 &&
				blktop > SIZEOF_cray_hdr-
					CRAY_PARAMSZ+namesize+1) {
			int	i;
			for (i = 0; i < namesize; i++)
				if (blkbuf[SIZEOF_cray_hdr-
						CRAY_PARAMSZ+i] == '\0')
					break;
			if (i == namesize-1)
				fmttype = FMT_CRAY5;
		}
	} else if (blktop >= 512 &&
			memcmp(&blkbuf[257], mag_ustar, 6) == 0 &&
			tcssum((union bincpio *)blkbuf, 1) == 0) {
		fmttype = FMT_USTAR;
	} else if (blktop >= 512 &&
			memcmp(&blkbuf[257], mag_gnutar, 8) == 0 &&
			tcssum((union bincpio *)blkbuf, 1) == 0) {
		fmttype = FMT_GNUTAR;
	} else if (blktop >= 512 && blkbuf[0] &&	/* require filename
							   to avoid match on
							   /dev/zero etc. */
			memcmp(&blkbuf[257], "\0\0\0\0\0", 5) == 0 &&
			tcssum((union bincpio *)blkbuf, 0) == 0) {
		fmttype = FMT_OTAR;
	} else if (blktop >= SIZEOF_zip_header &&
			(memcmp(blkbuf, mag_zipctr, sizeof mag_zipctr) == 0 ||
			 memcmp(blkbuf, mag_zipsig, sizeof mag_zipsig) == 0 ||
			 memcmp(blkbuf, mag_zipend, sizeof mag_zipend) == 0 ||
			 memcmp(blkbuf, mag_zip64e, sizeof mag_zip64e) == 0 ||
			 memcmp(blkbuf, mag_zip64l, sizeof mag_zip64l) == 0)) {
		fmttype = FMT_ZIP;
	} else if (blktop >= 512 && memcmp(blkbuf,"\0\0\0\0\0\0\0\0",8) == 0 &&
			memcmp(&blkbuf[65], mag_bar, sizeof mag_bar) == 0 &&
			bcssum((union bincpio *)blkbuf) == 0) {
		fmttype = FMT_BAR;
		compressed_bar = blkbuf[71] == '1';
		curpos = 512;
	} else if (!Aflag && blktop > 3 && memcmp(blkbuf, "BZh", 3) == 0 &&
			redirect("bzip2", "-cd") == 0) {
		goto zip;
	} else if (!Aflag && blktop > 2 && memcmp(blkbuf, "\37\235", 2) == 0 &&
			redirect("zcat", NULL) == 0) {
		goto zip;
	} else if (!Aflag && blktop > 2 && memcmp(blkbuf, "\37\213", 2) == 0 &&
			redirect("gzip", "-cd") == 0) {
		goto zip;
	} else if (!Aflag && blktop > 4 &&
			memcmp(blkbuf, "\355\253\356\333", 4) == 0 &&
			redirect("rpm2cpio", "-") == 0) {
		goto zip;
	} else {
		msg(3, 0, sysv3 ? "This is not a cpio file, bad header.\n" :
				"Not a cpio file, bad header.\n");
		done(1);
	}
	return;
zip:
	blktop = curpos = 0;
	blocks = 0;
	bytes = 0;
	mfl = 0;
	mstat();
	if (mread() == 0)
		unexeoa();
	goto again;
}

/*
 * Processing of a single file with -i.
 */
static int
infile(struct file *f)
{
	int	val;

	if ((fmttype & TYP_CPIO || fmttype == FMT_ZIP && f->f_st.st_size) &&
			(f->f_st.st_mode&S_IFMT) == S_IFLNK) {
		if (f->f_lsiz < f->f_st.st_size+1)
			f->f_lnam = srealloc(f->f_lnam,
					f->f_lsiz = f->f_st.st_size+1);
		if (bread(f->f_lnam, f->f_st.st_size) != f->f_st.st_size)
			unexeoa();
		f->f_lnam[f->f_st.st_size] = '\0';
		skippad(f->f_st.st_size, f->f_pad);
	}
	if (tflag)
		val = filet(f, indata);
	else
		val = filein(f, indata, f->f_name);
	return val != 0;
}

/*
 * Fetch the data of regular files from tape and write it to tfd or, if
 * tfd < 0, discard it.
 */
static int
indata(struct file *f, const char *tgt, int tfd)
{
	char	*buf;
	size_t	bufsize;
	struct stat	ts;
	long long	size;
	ssize_t	rd;
	uint32_t	ssum = 0, usum = 0;
	int	val = 0;
	int	doswap = 0;

	size = fmttype == FMT_ZIP ? f->f_csize :
		f->f_Kbase ? f->f_Kbase : f->f_st.st_size;
	doswap = ((bflag|sflag) == 0 ||
			ckodd(size, 2, "bytes", f->f_name) == 0) &
		((bflag|Sflag) == 0 ||
			 ckodd(size, 4, "halfwords", f->f_name) == 0) &
		(bflag|sflag|Sflag);
	if (fmttype == FMT_ZIP && f->f_cmethod != C_STORED)
		return zipread(f, tgt, tfd, doswap);
	if (tfd < 0 || fstat(tfd, &ts) < 0)
		ts.st_blksize = 4096;
	getbuf(&buf, &bufsize, ts.st_blksize);
again:	while (size) {
		if ((rd = bread(buf, size > bufsize ? bufsize : size)) <= 0)
			unexeoa();
		if (doswap)
			swap(buf, rd, bflag || sflag, bflag || Sflag);
		if (tfd >= 0 && write(tfd, buf, rd) != rd) {
			emsg(3, "Cannot write \"%s\"", tgt);
			tfd = -1;
			val = -1;
		}
		size -= rd;
		if (fmttype & TYP_CRC)
			do {
				rd--;
				ssum += ((signed char *)buf)[rd];
				usum += ((unsigned char *)buf)[rd];
			} while (rd);
	}
	if (f->f_Kbase) {
		skippad(f->f_Ksize, f->f_pad);
		readK2hdr(f);
		size = f->f_Krest;
		goto again;
	}
	skippad(fmttype==FMT_ZIP ? f->f_csize :
			f->f_Ksize ? f->f_Krest : f->f_st.st_size, f->f_pad);
	if (fmttype & TYP_CRC) {
		if (f->f_chksum != ssum && f->f_chksum != usum) {
			msg(3, 0, "\"%s\" - checksum error\n", f->f_name);
			if (kflag)
				errcnt++;
			else
				val = -1;
		}
	}
	return val;
}

/*
 * Skip the data for a file on tape.
 */
static int
skipfile(struct file *f)
{
	char	b[4096];
	long long	size;
	ssize_t	rd;

	if (fmttype & TYP_TAR && ((f->f_st.st_mode&S_IFMT) == S_IFLNK ||
			f->f_st.st_nlink > 1)) {
		if (fmttype & TYP_USTAR && (!formatforced ||
				fmttype == FMT_PAX) &&
				f->f_st.st_nlink > 1 &&
				f->f_st.st_size > 0)
			/*EMPTY*/;
		else
			return 0;
	}
	/*
	 * SVR4 cpio derivatives always ignore the size of these,
	 * even if not reading tar formats. We do the same for now,
	 * although POSIX (for -H odc format) says the contrary.
	 */
	if (fmttype != FMT_ZIP && ((f->f_st.st_mode&S_IFMT) == S_IFDIR ||
			(f->f_st.st_mode&S_IFMT) == S_IFCHR ||
			(f->f_st.st_mode&S_IFMT) == S_IFBLK ||
			(f->f_st.st_mode&S_IFMT) == S_IFIFO ||
			(f->f_st.st_mode&S_IFMT) == S_IFNAM ||
			(f->f_st.st_mode&S_IFMT) == S_IFNWK))
		return 0;
	if (fmttype == FMT_ZIP && f->f_gflag & FG_DESC)
		return zipread(f, f->f_name, -1, 0);
	size = fmttype == FMT_ZIP ? f->f_csize :
		f->f_Kbase ? f->f_Kbase : f->f_st.st_size;
again:	while (size) {
		if ((rd = bread(b, size > sizeof b ? sizeof b : size)) <= 0)
			unexeoa();
		size -= rd;
	}
	if (f->f_Kbase) {
		skippad(f->f_Ksize, f->f_pad);
		readK2hdr(f);
		size = f->f_Krest;
		goto again;
	}
	skippad(fmttype==FMT_ZIP ? f->f_csize :
			f->f_Ksize ? f->f_Krest : f->f_st.st_size, f->f_pad);
	return 0;
}

/*
 * Skip data also, but perform checks as if copying.
 */
static int
skipdata(struct file *f, int (*copydata)(struct file *, const char *, int))
{
	switch (f->f_st.st_mode&S_IFMT) {
	case S_IFLNK:
		break;
	case S_IFDIR:
	case S_IFBLK:
	case S_IFCHR:
	case S_IFIFO:
	case S_IFNAM:
	case S_IFNWK:
		if (fmttype != FMT_ZIP)
			break;
		/*FALLTHRU*/
	case S_IFREG:
	default:
		if (fmttype & TYP_TAR && f->f_st.st_nlink > 1 &&
				((fmttype & TYP_USTAR) == 0 ||
				 formatforced && fmttype != FMT_PAX))
			break;
		if (copydata(f, f->f_name, -1) != 0)
			return 1;
	}
	return 0;
}

/*
 * Seek to a position in the archive measured from the begin of
 * operation. Handle media-dependent seeks. n must be a multiple
 * of the tape device's physical block size.
 */
static int
tseek(off_t n)
{
	int	fault;

	if (tapeblock > 0) {
		int	i = (n - poffs) / tapeblock;
#if defined (__linux__) || defined (__sun) || defined (__FreeBSD__) || \
	defined (__hpux) || defined (_AIX) || defined (__NetBSD__) || \
	defined (__OpenBSD__) || defined (__DragonFly__) || defined (__APPLE__)
		struct mtop	mo;
		mo.mt_op = i > 0 ? MTFSR : MTBSR;
		mo.mt_count = i > 0 ? i : -i;
		fault = ioctl(mt, MTIOCTOP, &mo);
#else	/* SVR4.2MP */
		int	t, a;
		t = i > 0 ? T_SBF : T_SBB;
		a = i > 0 ? i : -i;
		fault = ioctl(mt, t, a);
#endif	/* SVR4.2MP */
	} else
		fault = lseek(mt, n - poffs, SEEK_CUR) == (off_t)-1 ? -1 : 0;
	if (fault == 0)
		poffs = n;
	return fault;
}

/*
 * Advance to the trailer on the tape (for -A).
 */
static int
totrailer(void)
{
	union bincpio	bc;
	struct file	f;
	off_t	ooffs, noffs, diff;
	int	i;

	if (mread() == 0)
		unexeoa();
	if (fmttype == FMT_NONE)
		whathdr();
	memset(&f, 0, sizeof f);
	for (;;) {
		ooffs = aoffs;
		if ((i = readhdr(&f, &bc)) < 0)
			goto fail;
		if (i > 0)
			break;
		markdev(f.f_st.st_dev);
		if (skipfile(&f) != 0)
			goto fail;
		if (fmttype == FMT_ZIP)
			zipdefer(f.f_name, &f.f_st, ooffs,
					f.f_chksum, f.f_csize, &bc.Zdr);
		pax_track(f.f_name, f.f_st.st_mtime);
	}
	/*
	 * Now seek to the position of the trailer, but retain the
	 * block alignment.
	 */
	diff = ooffs % blksiz;
	noffs = ooffs - diff;
	if (diff ? tseek(noffs) == 0 && mread() >= diff && tseek(noffs) == 0
			: tseek(ooffs) == 0) {
		free(f.f_name);
		free(f.f_lnam);
		nwritten = ooffs;
		curpos = diff;
		return 0;
	}
fail:	msg(4, 1, "Unable to append to this archive\n");
	/*NOTREACHED*/
	return 1;
}

static long long
rdoct(const char *data, int len)
{
	int	i;
	long long	val = 0;

	for (i = 0; i < len && data[i] == ' '; i++);
	for ( ; i < len && data[i] && data[i] != ' '; i++) {
		val <<= 3;
		val += data[i] - '0';
	}
	return val;
}

static long long
rdhex(const char *data, int len)
{
	int	i;
	long long	val = 0;

	for (i = 0; i < len && data[i] == ' '; i++);
	for ( ; i < len && data[i] && data[i] != ' '; i++) {
		val <<= 4;
		val += data[i] > '9' ? data[i] > 'F' ? data[i] - 'a' + 10 :
			data[i] - 'A' + 10 : data[i] - '0';
	}
	return val;
}

void
unexeoa(void)
{
	if (sysv3) {
		fprintf(stderr,
		"Can't read input:  end of file encountered "
			"prior to expected end of archive.\n");
		done(1);
	}
	else
		msg(3, 1, "Unexpected end-of-archive encountered.\n");
}

static char	*peekdata;
static size_t	peekbot, peektop, peeksize;

void
bunread(const char *data, size_t sz)
{
	peekdata = srealloc(peekdata, peeksize += sz);
	memcpy(&peekdata[peekbot], data, sz);
	peektop += sz;
	aoffs -= sz;
}

/*
 * Buffered read of data from tape. sz is the amount of data required
 * by the archive format; if it cannot be retrieved, processing fails.
 */
ssize_t
bread(char *data, size_t sz)
{
	ssize_t	rd = 0;

	if (peekdata) {
		rd = sz>peektop-peekbot ? peektop-peekbot : sz;
		memcpy(&data[0], &peekdata[peekbot], rd);
		sz -= rd;
		peekbot += rd;
		if (peekbot == peektop) {
			free(peekdata);
			peekdata = 0;
			peeksize = 0;
			peekbot = peektop = 0;
		}
	}
	while (sz) {
		if (blktop - curpos >= sz) {
			memcpy(&data[rd], &blkbuf[curpos], sz);
			curpos += sz;
			rd += sz;
			aoffs += rd;
			return rd;
		}
		if (blktop > curpos) {
			memcpy(&data[rd], &blkbuf[curpos], blktop - curpos);
			rd += blktop - curpos;
			sz -= blktop - curpos;
			curpos = blktop;
		}
		if (mfl < 0) {
			if (mfl == -1) {
				emsg(3, "I/O error on \"%s\"",
						Iflag ? Iflag : "input");
				if (kflag == 0)
					break;
				if (tapeblock < 0 && (
						(mtst.st_mode&S_IFMT)==S_IFBLK||
						(mtst.st_mode&S_IFMT)==S_IFCHR||
						(mtst.st_mode&S_IFMT)==S_IFREG
						) && lseek(mt, blksiz, SEEK_CUR)
							== (off_t)-1) {
					emsg(3, "Cannot lseek()");
					done(1);
				}
			}
			if (kflag == 0 || mfl == -2) {
				if ((mtst.st_mode&S_IFMT)!=S_IFCHR &&
						(mtst.st_mode&S_IFMT)!=S_IFBLK)
					break;
				newmedia(mfl == -1 ? errno : 0);
				if (mfl == -1) {
					mfl = -2;
					break;
				}
				if (fmttype & TYP_BAR)
					curpos = 512;
			}
		}
		mread();
	}
	aoffs += rd;
	return rd;
}

/*
 * Read a block of data from tape.
 */
static ssize_t
mread(void)
{
	ssize_t	ro, rt = 0;

	do {
		if ((ro = read(mt, blkbuf + rt, blksiz - rt)) <= 0) {
			if (ro < 0) {
				if (errno == EINTR)
					continue;
				mfl = -1;
			} else
				mfl = -2;
			if (rt > 0) {
				rt += ro;
				break;
			}
			curpos = blktop = 0;
			return ro;
		}
		rt += ro;
		poffs += ro;
		if (tapeblock == 0) {
			tapeblock = ro;
			if (!Bflag && !Cflag)
				blksiz = ro;
		}
	} while (rt < blksiz);
	curpos = 0;
	blocks += rt >> 9;
	bytes += rt & 0777;
	blktop = rt;
	return rt;
}

/*
 * Look what kind of tape or other archive media we are working on and
 * set the buffer size appropriately (if not specified by the user).
 */
static void
mstat(void)
{
	if (fstat(mt, &mtst) < 0) {
		emsg(3, "Error during stat() of archive");
		done(1);
	}
#if defined (__linux__)
	if ((mtst.st_mode&S_IFMT) == S_IFCHR) {
		struct mtget	mg;
		if (ioctl(mt, MTIOCGET, &mg) == 0)
			tapeblock = (mg.mt_dsreg&MT_ST_BLKSIZE_MASK) >>
					MT_ST_BLKSIZE_SHIFT;
	} else if ((mtst.st_mode&S_IFMT) == S_IFBLK) {
		/*
		 * If using a block device, write blocks of the floppy
		 * disk sector with direct i/o. This enables signals
		 * after each block is written instead of being ~40
		 * seconds in uninterruptible sleep when calling close()
		 * later. For block devices other than floppies, use the 
		 * kernel defined i/o block size. For floppies, use direct
		 * i/o even when reading since it is faster.
		 */
		struct floppy_struct	fs;
		int	floppy = -1;
		int	blkbsz;

		if (blksiz == 0) {
			if ((floppy = ioctl(mt, FDGETPRM, &fs)) == 0)
				blksiz = fs.sect * FD_SECTSIZE(&fs);
#ifdef	BLKBSZGET
			else if (ioctl(mt, BLKBSZGET, &blkbsz) == 0)
				blksiz = blkbsz;
#endif	/* BLKBSZGET */
		}
#ifdef	O_DIRECT
		if ((action == 'o' || floppy == 0) && blksiz != 0) {
			int	flags;
			if ((flags = fcntl(mt, F_GETFL)) != -1)
				fcntl(mt, F_SETFL, flags | O_DIRECT);
		}
	}
#endif	/* O_DIRECT */
#elif defined (__sun)
	if ((mtst.st_mode&S_IFMT) == S_IFCHR) {
		struct mtdrivetype_request	mr;
		static struct mtdrivetype	md;
		mr.size = sizeof md;
		mr.mtdtp = &md;
		if (ioctl(mt,  MTIOCGETDRIVETYPE, &mr) == 0)
			tapeblock = md.bsize;
	}
#elif defined (__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) \
		|| defined (__DragonFly__) || defined (__APPLE__)
	if ((mtst.st_mode&S_IFMT) == S_IFCHR) {
		struct mtget	mg;
		if (ioctl(mt, MTIOCGET, &mg) == 0)
			tapeblock = mg.mt_blksiz;
	}
#elif defined (__hpux) || defined (_AIX)
#else	/* SVR4.2MP */
	if ((mtst.st_mode&S_IFMT) == S_IFCHR) {
		struct blklen	bl;
		if (ioctl(mt, T_RDBLKLEN, &bl) == 0)
			/*
			 * These are not the values we're interested in
			 * (always 1 and 16M-1 for DAT/DDS tape drives).
			 */
			tapeblock = 0;
	}
#endif	/* SVR4.2MP */
	if (blksiz == 0)
		switch (mtst.st_mode&S_IFMT) {
		case S_IFREG:
		case S_IFBLK:
			blksiz = 4096;
			break;
		case S_IFCHR:
			if (action == 'o' && !Aflag) {
				if (pax != PAX_TYPE_CPIO) {
					if (fmttype & TYP_PAX)
						blksiz = 5120;
					else if (fmttype & TYP_TAR)
						blksiz = 10240;
					else if (fmttype & TYP_CPIO)
						blksiz = 5120;
					else
						blksiz = 512;
				} else
					blksiz = tapeblock>0 ? tapeblock : 512;
			} else
				blksiz = tapeblock > 0 && tapeblock % 1024 ?
					tapeblock : tapeblock > 10240 ?
					tapeblock : 10240;
			break;
		default:
			blksiz = 512;
		}
}

/*
 * Skip tape data such that size becomes aligned to pad.
 */
static int
skippad(unsigned long long size, int pad)
{
	char	b[512];
	int	to;

	if ((to = size % pad) != 0) {
		if (bread(b, pad - to) != pad - to)
			unexeoa();
	}
	return 0;
}

static int
allzero(const char *bp, int n)
{
	int	i;

	for (i = 0; i < n; i++)
		if (bp[i] != '\0')
			return 0;
	return 1;
}

#define	CACHESIZE	16

static const char *
getuser(uid_t uid)
{
	static struct {
		char	*name;
		uid_t	uid;
	} cache[CACHESIZE];
	static int	last;
	int	i;
	struct passwd	*pwd;
	const char	*name;

	for (i = 0; i < CACHESIZE && cache[i].name; i++)
		if (cache[i].uid == uid)
			goto found;
	if ((pwd = getpwuid(uid)) != NULL)
		name = pwd->pw_name;
	else
		name = "";
	if (i >= CACHESIZE) {
		if (last >= CACHESIZE)
			last = 0;
		i = last++;
	}
	if (cache[i].name)
		free(cache[i].name);
	cache[i].name = sstrdup(name);
	cache[i].uid = uid;
found:	return cache[i].name[0] ? cache[i].name : NULL;
}

static const char *
getgroup(gid_t gid)
{
	static struct {
		char	*name;
		gid_t	gid;
	} cache[CACHESIZE];
	static int	last;
	int	i;
	struct group	*grp;
	const char	*name;

	for (i = 0; i < CACHESIZE && cache[i].name; i++)
		if (cache[i].gid == gid)
			goto found;
	if ((grp = getgrgid(gid)) != NULL)
		name = grp->gr_name;
	else
		name = "";
	if (i >= CACHESIZE) {
		if (last >= CACHESIZE)
			last = 0;
		i = last++;
	}
	if (cache[i].name)
		free(cache[i].name);
	cache[i].name = sstrdup(name);
	cache[i].gid = gid;
found:	return cache[i].name[0] ? cache[i].name : NULL;
}

/*
 * Return a version of the passed string that contains at most one '%d'
 * and no other printf formats.
 */
char *
oneintfmt(const char *op)
{
	char	*new, *np;
	int	no = 0;

	np = new = smalloc(2 * strlen(op) + 1);
	do {
		if (*op == '%') {
			*np++ = *op++;
			if (*op != '%')
				if (*op != 'd' || no++)
					*np++ = '%';
		}
		*np++ = *op;
	} while (*op++);
	return new;
}

char *
sstrdup(const char *op)
{
	char	*np;

	np = smalloc(strlen(op) + 1);
	strcpy(np, op);
	return np;
}

/*
 * Add this pattern to the extraction list with -i.
 */
void
addg(const char *pattern, int art)
{
	struct glist	*gp;

	gp = scalloc(1, sizeof *gp);
	if (pax == PAX_TYPE_CPIO && pattern[0] == '!') {
		gp->g_not = 1;
		pattern++;
	}
	gp->g_pat = sstrdup(pattern);
	gp->g_art = art;
	if (pax != PAX_TYPE_CPIO) {
		struct glist	*gb = NULL, *gc;
		for (gc = patterns; gc; gc = gc->g_nxt)
			gb = gc;
		if (gb)
			gb->g_nxt = gp;
		else
			patterns = gp;
	} else {
		gp->g_nxt = patterns;
		patterns = gp;
	}
}

/*
 * Check if the file name s matches any of the given patterns.
 */
static struct glist *
want(struct file *f, struct glist **gb)
{
	extern int	gmatch(const char *, const char *);
	struct glist	*gp;

	for (gp = patterns; gp; gp = gp->g_nxt) {
		if ((gmatch(f->f_name, gp->g_pat) != 0) ^ gp->g_not &&
				(pax_nflag == 0 || gp->g_gotcha == 0)) {
			return gp;
		}
		*gb = gp;
	}
	return NULL;
}

static void
patfile(void)
{
	struct iblok	*ip;
	char	*name = NULL;
	size_t	namsiz = 0, namlen;

	if ((ip = ib_open(Eflag, 0)) == NULL)
		msg(3, -2, "Cannot open \"%s\" to read patterns\n", Eflag);
	while ((namlen = ib_getlin(ip, &name, &namsiz, srealloc)) != 0) {
		if (name[namlen-1] == '\n')
			name[--namlen] = '\0';
		addg(name, 0);
	}
	ib_close(ip);
}

void
swap(char *b, size_t sz, int s8, int s16)
{
	uint8_t	u8;
	uint16_t	u16;
	union types2	*t2;
	union types4	*t4;
	int	i;

	if (s8) {
		for (i = 0; i < (sz >> 1); i++) {
			t2 = &((union types2 *)b)[i];
			u8 = t2->byte[0];
			t2->byte[0] = t2->byte[1];
			t2->byte[1] = u8;
		}
	}
	if (s16) {
		for (i = 0; i < (sz >> 2); i++) {
			t4 = &((union types4 *)b)[i];
			u16 = t4->sword[0];
			t4->sword[0] = t4->sword[1];
			t4->sword[1] = u16;
		}
	}
}

static int
ckodd(long long size, int mod, const char *str, const char *fn)
{
	if (size % mod) {
		msg(3, 0, "Cannot swap %s of \"%s\", odd number of %s\n",
				str, fn, str);
		errcnt++;
		return 1;
	}
	return 0;
}

/*
 * Interactive rename (-r option).
 */
static int
rname(char **oldp, size_t *olds)
{
	char	*new = NULL;
	size_t	newsize = 0;
	int	i, r;
	char	c;

	fprintf(stderr, "Rename \"%s\"? ", *oldp);
	if (tty == 0)
		if ((tty = open("/dev/tty", O_RDWR)) < 0 ||
				fcntl(tty, F_SETFD, FD_CLOEXEC) < 0)
		err:	msg(3, 1, "Cannot read tty.\n");
	i = 0;
	while ((r = read(tty, &c, 1)) == 1 && c != '\n') {
		if (i+1 >= newsize)
			new = srealloc(new, newsize += 32);
		new[i++] = c;
	}
	if (r <= 0)
		goto err;
	if (new == NULL)
		return 0;
	new[i] = '\0';
	if (new[0] == '.' && new[1] == '\0') {
		free(new);
	} else {
		free(*oldp);
		*oldp = new;
		*olds = newsize;
	}
	return 1;
}

/*
 * Filter data from tape through the commands given in arg?.
 */
static int
redirect(const char *arg0, const char *arg1)
{
	int	pd[2];

	if (pipe(pd) < 0)
		return -1;
	switch (fork()) {
	case 0:
		if (tapeblock>=0 || lseek(mt, -blktop, SEEK_CUR) == (off_t)-1) {
			int	xpd[2];
			if (pipe(xpd) == 0 && fork() == 0) {
				ssize_t	rd, wo, wt;
				close(xpd[0]);
				do {
					wo = wt = 0;
					do {
						if ((wo = write(xpd[1],
								blkbuf + wt,
								blktop - wt))
									<= 0) {
							if (errno == EINTR)
								continue;
							_exit(0);
						}
						wt += wo;
					} while (wt < blktop);
				} while ((rd = mread()) >= 0);
				if (rd < 0) {
					emsg(3, "Read error on \"%s\"",
							Iflag && !sysv3 ?
							Iflag : "input");
				}
				_exit(0);
			} else {
				close(xpd[1]);
				dup2(xpd[0], 0);
				close(xpd[0]);
			}
		} else {
			dup2(mt, 0);
		}
		close(mt);
		dup2(pd[1], 1);
		close(pd[0]);
		close(pd[1]);
		execlp(arg0, arg0, arg1, NULL);
		fprintf(stderr, "%s: could not exec %s: %s\n",
				progname, arg0, strerror(errno));
		_exit(0177);
		/*NOTREACHED*/
	default:
		dup2(pd[0], mt);
		close(pd[0]);
		close(pd[1]);
		tapeblock = -1;
		break;
	case -1:
		return -1;
	}
	return 0;
}

/*
 * Get the name stored in a tar header. buf is expected to be at least
 * TPFXSIZ+TNAMSIZ+2 bytes.
 */
static char *
tnameof(struct tar_header *hp, char *buf)
{
	const char	*cp;
	char	*bp = buf;

	if (fmttype & TYP_USTAR && fmttype != FMT_GNUTAR &&
			hp->t_prefix[0] != '\0') {
		cp = hp->t_prefix;
		while (cp < &hp->t_prefix[TPFXSIZ] && *cp)
			*bp++ = *cp++;
		if (bp > buf)
			*bp++ = '/';
	}
	cp = hp->t_name;
	while (cp < &hp->t_name[TNAMSIZ] && *cp)
		*bp++ = *cp++;
	*bp = '\0';
	return buf;
}

/*
 * Store fn as file name in a tar header.
 */
static int
tmkname(struct tar_header *hp, const char *fn)
{
	const char	*cp, *cs = NULL;

	for (cp = fn; *cp; cp++) {
		if (fmttype & TYP_USTAR && *cp == '/' && cp[1] != '\0' &&
				cp > fn && cp-fn <= TPFXSIZ)
			cs = cp;
	}
	if (fmttype == FMT_GNUTAR && cp - fn > 99) {
		writegnuname(fn, cp - fn + 1, 'L');
		cp = &fn[99];
	} else if (cp - (cs ? &cs[1] : fn) > TNAMSIZ) {
		if (fmttype & TYP_PAX && utf8(fn)) {
			paxrec |= PR_PATH;
			strcpy(hp->t_name, sequence());
			return 0;
		}
		msg(3, 0, "%s: file name too long\n", fn);
		return -1;
	}
	if (cs && cp - fn > TNAMSIZ) {
		memcpy(hp->t_prefix, fn, cs - fn);
		if (cs - fn < TPFXSIZ)
			hp->t_prefix[cs - fn] = '\0';
		memcpy(hp->t_name, &cs[1], cp - &cs[1]);
		if (cp - &cs[1] < TNAMSIZ)
			hp->t_name[cp - &cs[1]] = '\0';
	} else {
		memcpy(hp->t_name, fn, cp - fn);
		if (cp - fn < TNAMSIZ)
			hp->t_name[cp - fn] = '\0';
	}
	return 0;
}

/*
 * Get the link name of a tar header.
 */
static void
tlinkof(struct tar_header *hp, struct file *f)
{
	const char	*cp;
	char	*bp;


	if (f->f_lsiz < TNAMSIZ+1)
		f->f_lnam = srealloc(f->f_lnam, f->f_lsiz = TNAMSIZ+1);
	cp = hp->t_linkname;
	bp = f->f_lnam;
	while (cp < &hp->t_linkname[TNAMSIZ] && *cp)
		*bp++ = *cp++;
	*bp = '\0';
}

/*
 * Create the link name in a tar header.
 */
static int
tmklink(struct tar_header *hp, const char *fn)
{
	const char	*cp;

	for (cp = fn; *cp; cp++);
	if (fmttype == FMT_GNUTAR && cp - fn > 99) {
		writegnuname(fn, cp - fn + 1, 'K');
		cp = &fn[99];
	} else if (cp - fn > TNAMSIZ) {
		if (fmttype & TYP_PAX && utf8(fn)) {
			paxrec |= PR_LINKPATH;
			strcpy(hp->t_linkname, sequence());
			return 0;
		}
		msg(3, 0, "%s: linked name too long\n", fn);
		return -1;
	}
	memcpy(hp->t_linkname, fn, cp - fn);
	if (cp - fn < TNAMSIZ)
		hp->t_linkname[cp - fn] = '\0';
	return 0;
}

static int
tlflag(struct stat *st)
{
	if (fmttype & TYP_BAR) {
		switch (st->st_mode & S_IFMT) {
		case S_IFREG:
		case S_IFDIR:
			return '0';
		case S_IFLNK:
			return '2';
		default:
			return '3';
		}
	} else if (fmttype & TYP_USTAR) {
		switch (st->st_mode & S_IFMT) {
		case S_IFREG:
			return '0';
		case S_IFLNK:
			return '2';
		case S_IFCHR:
			return '3';
		case S_IFBLK:
			return '4';
		case S_IFDIR:
			return '5';
		case S_IFIFO:
			return '6';
		default:
			return -1;
		}
	} else {
		switch (st->st_mode & S_IFMT) {
		case S_IFREG:
			return '\0';
		case S_IFLNK:
			return '2';
		default:
			return -1;
		}
	}
}

/*
 * Ustar checksums are created using unsigned chars, as specified by
 * POSIX. Traditional tar implementations use signed chars. Some
 * implementations (notably SVR4 cpio derivatives) use signed chars
 * even for ustar archives, but this is clearly an implementation bug.
 */
static void
tchksum(union bincpio *bp)
{
	uint32_t	sum;
	char	*cp;

	memset(bp->Tdr.t_chksum, ' ', sizeof bp->Tdr.t_chksum);
	sum = 0;
	if (fmttype & TYP_USTAR)
		for (cp = bp->data; cp < &bp->data[512]; cp++)
			sum += *((unsigned char *)cp);
	else
		for (cp = bp->data; cp < &bp->data[512]; cp++)
			sum += *((signed char *)cp);
	sprintf(bp->Tdr.t_chksum, "%7.7o", sum);
}

static int
tcssum(union bincpio *bp, int ustar)
{
	uint32_t	ssum = 0, usum = 0, osum;
	char	ochk[sizeof bp->Tdr.t_chksum];
	char	*cp;

	osum = rdoct(bp->Tdr.t_chksum, 8);
	memcpy(ochk, bp->Tdr.t_chksum, sizeof ochk);
	memset(bp->Tdr.t_chksum, ' ', sizeof bp->Tdr.t_chksum);
	for (cp = bp->data; cp < &bp->data[512]; cp++) {
		ssum += *((signed char *)cp);
		usum += *((unsigned char *)cp);
	}
	memcpy(bp->Tdr.t_chksum, ochk, sizeof bp->Tdr.t_chksum);
	return ssum != osum && usum != osum;
}

static int
trdsum(union bincpio *bp)
{
	int	i;

	if (fmttype & TYP_BAR)
		i = bcssum(bp);
	else
		i = tcssum(bp, fmttype & TYP_USTAR);
	if (i)
		msg(3, 0, "Bad header - checksum error.\n");
	return i;
}

static mode_t
tifmt(int c)
{
	switch (c) {
	default:
	case '\0':
	case '0':
		return S_IFREG;
	case '2':
		return S_IFLNK;
	case '3':
		return S_IFCHR;
	case '4':
		return S_IFBLK;
	case '5':
		return S_IFDIR;
	case '6':
		return S_IFIFO;
	}
}

/*
 * bar format support functions.
 */
static void
bchksum(union bincpio *bp)
{
	uint32_t	sum;
	char	*cp;

	memset(bp->Bdr.b_chksum, ' ', sizeof bp->Bdr.b_chksum);
	sum = 0;
	for (cp = bp->data; cp < &bp->data[512]; cp++)
		sum += *((signed char *)cp);
	sprintf(bp->Bdr.b_chksum, "%7.7o", sum);
}

static int
bcssum(union bincpio *bp)
{
	uint32_t	sum, osum;
	char	ochk[sizeof bp->Bdr.b_chksum];
	char	*cp;

	osum = rdoct(bp->Bdr.b_chksum, 8);
	memcpy(ochk, bp->Bdr.b_chksum, sizeof ochk);
	memset(bp->Bdr.b_chksum, ' ', sizeof bp->Bdr.b_chksum);
	sum = 0;
	for (cp = bp->data; cp < &bp->data[512]; cp++)
		sum += *((signed char *)cp);
	memcpy(bp->Bdr.b_chksum, ochk, sizeof bp->Bdr.b_chksum);
	return sum != osum;
}

static void
blinkof(const char *cp, struct file *f, int namlen)
{
	if (f->f_lsiz < 512)
		f->f_lnam = srealloc(f->f_lnam, f->f_lsiz = 512);
	strcpy(f->f_lnam, &cp[SIZEOF_bar_header + namlen]);
}

static void
dump_barhdr(void)
{
	union bincpio	bc;
	static int	volno;
	static time_t	now = -1;

	memset(&bc, 0, 512);
	sprintf(bc.Bdr.b_uid, "%d", myuid & 07777777);
	sprintf(bc.Bdr.b_gid, "%d", mygid & 07777777);
	bc.Bdr.b_size[0] = '0';
	memcpy(bc.Bdr.b_bar_magic, mag_bar, sizeof bc.Bdr.b_bar_magic);
	sprintf(bc.Bdr.b_volume_num, "%d", ++volno & 0777);
	bc.Bdr.b_compressed = '0';
	if (now == (time_t)-1)
		time(&now);
	sprintf(bc.Bdr.b_date, "%llo", now & 077777777777LL);
	bchksum(&bc);
	bwrite(bc.data, 512);
}

/*
 * Support for compressed bar format (any regular file is piped through zcat).
 */
static pid_t	zpid;

static int
zcreat(const char *name, mode_t mode)
{
	int	pd[2];
	int	fd;

	if (pipe(pd) < 0)
		return -1;
	if ((fd = creat(name, mode)) < 0) {
		fd = errno;
		close(pd[0]);
		close(pd[1]);
		errno = fd;
		return -1;
	}
	switch (zpid = fork()) {
	case -1:
		return -1;
	case 0:
		dup2(pd[0], 0);
		dup2(fd, 1);
		close(pd[0]);
		close(pd[1]);
		close(fd);
		execlp("zcat", "zcat", NULL);
		_exit(0177);
		/*NOTREACHED*/
	}
	close(pd[0]);
	close(fd);
	sigset(SIGPIPE, SIG_IGN);
	return pd[1];
}

static int
zclose(int fd)
{
	int	c, s;

	c = close(fd);
	while (waitpid(zpid, &s, 0) != zpid);
	return c != 0 || s != 0 ? -1 : 0;
}

/*
 * If using the -A option, device numbers that appear in the archive
 * are not reused for appended files. This avoids wrong hardlink
 * connections on extraction.
 *
 * In fact, this should be done even if we did not fake device and
 * inode numbers, since it is not guaranteed that the archive was
 * created on the same machine, or even on the same machine, inode
 * number could have changed after a file was unlinked.
 */
static void
markdev(dev_t dev)
{
	struct dslot	*dp, *dq = NULL;;

	for (dp = markeddevs; dp; dp = dp->d_nxt) {
		if (dp->d_dev == dev)
			return;
		dq = dp;
	}
	dp = scalloc(1, sizeof *dp);
	dp->d_dev = dev;
	if (markeddevs == NULL)
		markeddevs = dp;
	else
		dq->d_nxt = dp;
}

static int
marked(dev_t dev)
{
	struct dslot	*dp;

	for (dp = markeddevs; dp; dp = dp->d_nxt)
		if (dp->d_dev == dev)
			return 1;
	return 0;
}

static void
cantsup(int err, const char *file)
{
	if (sysv3)
		msg(err ? 3 : 2, 0,
			"format can't support expanded types on %s\n",
			file);
	else
		msg(0, 0, "%s format can't support expanded types on %s\n",
			err ? "Error" : "Warning", file);
	errcnt++;
}

static void
onint(int signo)
{
	if (cur_ofile && cur_tfile && rename(cur_tfile, cur_ofile) < 0)
		emsg(3, "Cannot recover original \"%s\"", cur_ofile);
	if (cur_ofile && cur_tfile == NULL && unlink(cur_ofile) < 0)
		emsg(3, "Cannot remove incomplete \"%s\"", cur_ofile);
	exit(signo | 0200);
}

/*
 * Read the compressed zip data part for a file.
 */
static int
zipread(struct file *f, const char *tgt, int tfd, int doswap)
{
	int	val = 0;
	uint32_t	crc = 0;

	if (f->f_gflag & FG_DESC) {
		if (f->f_cmethod != C_DEFLATED && f->f_cmethod != C_ENHDEFLD ||
				f->f_gflag & FG_CRYPT)
			msg(4, 1, "Cannot handle zip data descriptor\n");
		f->f_csize = 0x7FFFFFFFFFFFFFFFLL;
		f->f_st.st_size = 0x7FFFFFFFFFFFFFFFLL;
	} else if (tfd < 0)
		return skipfile(f);
	if (f->f_gflag & FG_CRYPT)
		return cantunzip(f, "encrypted");
	switch (f->f_cmethod) {
	case C_DEFLATED:
	case C_ENHDEFLD:
		val = zipinflate(f, tgt, tfd, doswap, &crc);
		break;
	case C_SHRUNK:
		val = zipunshrink(f, tgt, tfd, doswap, &crc);
		break;
	case C_IMPLODED:
		val = zipexplode(f, tgt, tfd, doswap, &crc);
		break;
	case C_REDUCED1:
	case C_REDUCED2:
	case C_REDUCED3:
	case C_REDUCED4:
		val = zipexpand(f, tgt, tfd, doswap, &crc);
		break;
	case C_TOKENIZED:
		return cantunzip(f, "tokenized");
	case C_DCLIMPLODED:
		val = zipblast(f, tgt, tfd, doswap, &crc);
		break;
	case C_BZIP2:
#if USE_BZLIB
		val = zipunbz2(f, tgt, tfd, doswap, &crc);
		break;
#else	/* !USE_BZLIB */
		return cantunzip(f, "bzip2 compressed");
#endif	/* !USE_BZLIB */
	default:
		return cantunzip(f, "compressed");
	}
	if (f->f_gflag & FG_DESC)
		zipreaddesc(f);
	if (val == 0 && crc != f->f_chksum) {
		msg(3, 0, "\"%s\" - checksum error\n", f->f_name);
		return -1;
	}
	return val;
}

/*
 * Read a zip data descriptor (i. e. a field after the compressed data
 * that contains the actual values for sizes and crc).
 */
static void
zipreaddesc(struct file *f)
{
	if (f->f_oflag & OF_ZIP64) {
		struct	zipddesc64	zd64;
		bread((char *)&zd64, SIZEOF_zipddesc64);
		if (memcmp(zd64.zd_signature, mag_zipdds, sizeof mag_zipdds))
			msg(4, 1, "Invalid zip data descriptor\n");
		f->f_chksum = ple32(zd64.zd_crc32);
		f->f_dsize = f->f_st.st_size = ple64(zd64.zd_nsize) &
			0xFFFFFFFFFFFFFFFFULL;
		f->f_csize = ple64(zd64.zd_csize) &
			0xFFFFFFFFFFFFFFFFULL;
	} else {
		struct	zipddesc zd;
		bread((char *)&zd, SIZEOF_zipddesc);
		if (memcmp(zd.zd_signature, mag_zipdds, sizeof mag_zipdds))
			msg(4, 1, "Invalid zip data descriptor\n");
		f->f_chksum = ple32(zd.zd_crc32);
		f->f_dsize = f->f_st.st_size = ple32(zd.zd_nsize)&0xFFFFFFFFUL;
		f->f_csize = ple32(zd.zd_csize)&0xFFFFFFFFUL;
	}
}

static int
cantunzip(struct file *f, const char *method)
{
	msg(3, 0, "Cannot unzip %s file \"%s\"\n", method, f->f_name);
	errcnt++;
	return skipfile(f);
}

/*
 * PC-DOS time format:
 *
 * 31            24      20        15        10           4       0
 * | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
 * |               |               |               |               |
 * |year         |months |days     |hours    |minutes    |biseconds|
 *
 * refers to local time on the machine it was created on.
 */

static time_t
gdostime(const char *tp, const char *dp)
{
	uint32_t	v;
	struct tm	tm;

	v = (int)(tp[0]&0377) +
		((int)(tp[1]&0377) << 8) +
		((int)(dp[0]&0377) << 16) +
		((int)(dp[1]&0377) << 24);
	memset(&tm, 0, sizeof tm);
	tm.tm_sec = (v&0x1F) << 1;
	tm.tm_min = (v&0x7E0) >> 5;
	tm.tm_hour = (v&0xF800) >> 11;
	tm.tm_mday = ((v&0x1F0000) >> 16);
	tm.tm_mon = ((v&0x1E00000) >> 21) - 1;
	tm.tm_year = ((v&0xFE000000) >> 25) + 80;
	tm.tm_isdst = -1;
	return mktime(&tm);
}

static void
mkdostime(time_t t, char *tp, char *dp)
{
	uint32_t	v;
	struct tm	*tm;

	tm = localtime(&t);
	v = (tm->tm_sec >> 1) + (tm->tm_sec&1) + (tm->tm_min << 5) +
		(tm->tm_hour << 11) + (tm->tm_mday << 16) +
		((tm->tm_mon+1) << 21) + ((tm->tm_year - 80) << 25);
	le16p(v&0x0000ffff, tp);
	le16p((v&0xffff0000) >> 16, dp);
}

/*
 * Read and interpret the zip extra field for a file.
 */
static ssize_t
ziprxtra(struct file *f, struct zip_header *z)
{
	union zextra	*x, *xp;
	short	tag, size;
	ssize_t	len;

	len = ple16(z->z_extralen)&0177777;
	if (len > 0) {
		x = smalloc(len);
		if (bread((char *)x, len) != len)
			return -1;
		xp = x;
		while (len > 0) {
			if (len < 4)
				return -1;
			tag = ple16(xp->Ze_gn.ze_gn_tag);
			size = (ple16(xp->Ze_gn.ze_gn_tsize)&0177777) + 4;
			switch (tag) {
			case mag_zip64f:	/* ZIP64 extended information */
				if (size != SIZEOF_zextra_64 &&
						size != SIZEOF_zextra_64_a &&
						size != SIZEOF_zextra_64_b)
					break;
				if (f->f_st.st_size == 0xffffffff)
					f->f_st.st_size =
						ple64(xp->Ze_64.ze_64_nsize);
				if (f->f_csize == 0xffffffff)
					f->f_csize =
						ple64(xp->Ze_64.ze_64_csize);
				f->f_oflag |= OF_ZIP64;
				break;
			case 0x000d:	/* PKWARE Unix Extra Field */
				if (size != SIZEOF_zextra_pk)
					break;
				f->f_st.st_atime = ple32(xp->Ze_pk.ze_pk_atime);
				f->f_st.st_mtime = ple32(xp->Ze_pk.ze_pk_mtime);
				f->f_st.st_uid = ple16(xp->Ze_pk.ze_pk_uid) &
					0177777;
				f->f_st.st_gid = ple16(xp->Ze_pk.ze_pk_gid) &
					0177777;
				break;
			case 0x5455:	/* Extended Timestamp Extra Field */
				if (xp->Ze_et.ze_et_flags[0] & 1)
					f->f_st.st_atime =
						ple32(xp->Ze_et.ze_et_atime);
				if (xp->Ze_et.ze_et_flags[0] & 2)
					f->f_st.st_mtime =
						ple32(xp->Ze_et.ze_et_mtime);
				if (xp->Ze_et.ze_et_flags[0] & 3)
					f->f_st.st_ctime =
						ple32(xp->Ze_et.ze_et_ctime);
				break;
			case 0x5855:	/* Info-ZIP Unix Extra Field #1 */
				if (size != SIZEOF_zextra_i1)
					break;
				f->f_st.st_atime = ple32(xp->Ze_i1.ze_i1_atime);
				f->f_st.st_mtime = ple32(xp->Ze_i1.ze_i1_mtime);
				f->f_st.st_uid = ple16(xp->Ze_i1.ze_i1_uid) &
					0177777;
				f->f_st.st_gid = ple16(xp->Ze_i1.ze_i1_gid) &
					0177777;
				break;
			case 0x7855:	/* Info-ZIP Unix Extra Field #2 */
				if (size != SIZEOF_zextra_i2)
					break;
				f->f_st.st_uid = ple16(xp->Ze_i2.ze_i2_uid) &
					0177777;
				f->f_st.st_gid = ple16(xp->Ze_i2.ze_i2_gid) &
					0177777;
				break;
			case 0x756e:	/* ASi Unix Extra Field */
				if (size < SIZEOF_zextra_as)
					break;
				f->f_st.st_mode = ple16(xp->Ze_as.ze_as_mode);
				f->f_st.st_uid = ple16(xp->Ze_as.ze_as_uid) &
					0177777;
				f->f_st.st_gid = ple16(xp->Ze_as.ze_as_gid) &
					0177777;
				if ((f->f_st.st_mode&S_IFMT) == S_IFLNK) {
					if (f->f_lsiz < size-14+1)
						f->f_lnam = srealloc(f->f_lnam,
								f->f_lsiz =
								size-18+1);
					memcpy(f->f_lnam, &((char *)xp)[18],
							size-18);
					f->f_lnam[size-18] = '\0';
					f->f_st.st_size = size-18;
				} else {
					f->f_st.st_rdev =
						ple32(xp->Ze_as.ze_as_sizdev);
					f->f_rmajor = major(f->f_st.st_rdev);
					f->f_rminor = minor(f->f_st.st_rdev);
				}
				break;
			case mag_zipcpio:
				if (size != SIZEOF_zextra_cp)
					break;
				f->f_st.st_dev = ple32(xp->Ze_cp.ze_cp_dev);
				f->f_st.st_ino = ple32(xp->Ze_cp.ze_cp_ino);
				f->f_st.st_mode = ple32(xp->Ze_cp.ze_cp_mode);
				f->f_st.st_uid = ple32(xp->Ze_cp.ze_cp_uid) &
					0xFFFFFFFFUL;
				f->f_st.st_gid = ple32(xp->Ze_cp.ze_cp_gid) &
					0xFFFFFFFFUL;
				f->f_st.st_nlink = ple32(xp->Ze_cp.ze_cp_nlink)&
					0xFFFFFFFFUL;
				f->f_st.st_rdev = ple32(xp->Ze_cp.ze_cp_rdev);
				f->f_rmajor = major(f->f_st.st_rdev);
				f->f_rminor = minor(f->f_st.st_rdev);
				f->f_st.st_mtime = ple32(xp->Ze_cp.ze_cp_mtime);
				f->f_st.st_atime = ple32(xp->Ze_cp.ze_cp_atime);
				break;
			}
			xp = (union zextra *)&((char *)xp)[size];
			len -= size;
		}
		free(x);
	}
	return len;
}

/*
 * Write the central directory and the end of a zip file.
 */
static void
ziptrailer(void)
{
	struct zipstuff	*zs;
	struct zipcentral	zc;
	struct zipend	ze;
	long long	cpstart, cpend, entries = 0;
	size_t	sz;

	cpstart = nwritten;
	for (zs = zipbulk; zs; zs = zs->zs_next) {
		entries++;
		memset(&zc, 0, SIZEOF_zipcentral);
		memcpy(zc.zc_signature, mag_zipctr, 4);
		zc.zc_versionmade[0] = 20;
		zc.zc_versionextr[0] = zs->zs_cmethod == 8 ? 20 : 10;
		mkdostime(zs->zs_mtime, zc.zc_modtime, zc.zc_moddate);
		le32p(zs->zs_crc32, zc.zc_crc32);
		le16p(zs->zs_cmethod, zc.zc_cmethod);
		le16p(zs->zs_gflag, zc.zc_gflag);
		/*
		 * We flag files as created on PC-DOS / FAT filesystem
		 * and thus set PC-DOS attributes here.
		 */
		if ((zs->zs_mode&0222) == 0)
			zc.zc_external[0] |= 0x01;	/* readonly attribute */
		if ((zs->zs_mode&S_IFMT) == S_IFDIR)
			zc.zc_external[0] |= 0x10;	/* directory attr. */
		sz = strlen(zs->zs_name);
		le16p(sz, zc.zc_namelen);
		if (zs->zs_size >= 0xffffffff || zs->zs_csize >= 0xffffffff ||
				zs->zs_relative >= 0xffffffff) {
			struct zextra_64	zf;

			memset(&zf, 0, SIZEOF_zextra_64);
			le16p(mag_zip64f, zf.ze_64_tag);
			le16p(SIZEOF_zextra_64 - 4, zf.ze_64_tsize);
			if ((zs->zs_mode&S_IFMT) == S_IFREG ||
					(zs->zs_mode&S_IFMT) == S_IFLNK) {
				le32p(0xffffffff, zc.zc_csize);
				le32p(0xffffffff, zc.zc_nsize);
				le64p(zs->zs_csize, zf.ze_64_csize);
				le64p(zs->zs_size, zf.ze_64_nsize);
			}
			le64p(zs->zs_relative, zf.ze_64_reloff);
			le32p(0xffffffff, zc.zc_relative);
			le16p(SIZEOF_zextra_64, zc.zc_extralen);
			bwrite((char *)&zc, SIZEOF_zipcentral);
			bwrite(zs->zs_name, sz);
			bwrite((char *)&zf, SIZEOF_zextra_64);
		} else {
			if ((zs->zs_mode&S_IFMT) == S_IFREG ||
					(zs->zs_mode&S_IFMT) == S_IFLNK) {
				le32p(zs->zs_csize, zc.zc_csize);
				le32p(zs->zs_size, zc.zc_nsize);
			}
			le32p(zs->zs_relative, zc.zc_relative);
			bwrite((char *)&zc, SIZEOF_zipcentral);
			bwrite(zs->zs_name, sz);
		}
	}
	cpend = nwritten;
	memset(&ze, 0, SIZEOF_zipend);
	memcpy(ze.ze_signature, mag_zipend, 4);
	if (cpend >= 0xffffffff || entries >= 0xffff) {
		struct zip64end	z6;
		struct zip64loc	z4;

		memset(&z6, 0, SIZEOF_zip64end);
		memcpy(z6.z6_signature, mag_zip64e, 4);
		le64p(SIZEOF_zip64end - 12, z6.z6_recsize);
		z6.z6_versionmade[0] = 20;
		z6.z6_versionextr[0] = 20;
		le64p(entries, z6.z6_thisentries);
		le64p(entries, z6.z6_allentries);
		le64p(cpend - cpstart, z6.z6_dirsize);
		le64p(cpstart, z6.z6_startsize);
		bwrite((char *)&z6, SIZEOF_zip64end);
		memset(&z4, 0, SIZEOF_zip64loc);
		memcpy(z4.z4_signature, mag_zip64l, 4);
		le64p(cpend, z4.z4_reloff);
		le32p(1, z4.z4_alldiskn);
		bwrite((char *)&z4, SIZEOF_zip64loc);
		le16p(0xffff, ze.ze_thisentries);
		le16p(0xffff, ze.ze_allentries);
		le32p(0xffffffff, ze.ze_dirsize);
		le32p(0xffffffff, ze.ze_startsize);
	} else {
		le16p(entries, ze.ze_thisentries);
		le16p(entries, ze.ze_allentries);
		le32p(cpend - cpstart, ze.ze_dirsize);
		le32p(cpstart, ze.ze_startsize);
	}
	bwrite((char *)&ze, SIZEOF_zipend);
}

/*
 * Store the data later needed for the central directory.
 */
static void
zipdefer(const char *fn, struct stat *st, long long relative,
		uint32_t crc, long long csize, const struct zip_header *zh)
{
	struct zipstuff	*zp;

	zp = scalloc(1, sizeof *zp);
	zp->zs_name = sstrdup(fn);
	zp->zs_size = st->st_size;
	zp->zs_mtime = st->st_mtime;
	zp->zs_mode = st->st_mode;
	zp->zs_relative = relative;
	zp->zs_cmethod = ple16(zh->z_cmethod);
	zp->zs_gflag = ple16(zh->z_gflag);
	zp->zs_csize = csize;
	zp->zs_crc32 = crc;
	zp->zs_next = zipbulk;
	zipbulk = zp;
}

#define	ziptrlevel()	( \
	zipclevel == 01 ? 9 : 	/* maximum */	\
	zipclevel == 02 ? 3 : 	/* fast */	\
	zipclevel == 03 ? 1 :	/* super fast */\
	/*zipclevel==00*/ 6  	/* normal */	\
)

/*
 * Write (and compress) data for a regular file to a zip archive.
 */
static int
zipwrite(int fd, const char *fn, struct stat *st, union bincpio *bp, size_t sz,
		uint32_t dev, uint32_t ino, uint32_t *crc, long long *csize)
{
#if USE_ZLIB
	struct z_stream_s	z;
	int	i;
	size_t	osize = 0;
#endif	/* USE_ZLIB */
	char	*ibuf, *obuf = 0;

	if (st->st_size > 196608 || (ibuf = malloc(st->st_size)) == 0) {
#if USE_ZLIB
		if (zipclevel < 04)
			return zipwdesc(fd, fn, st, bp, sz, dev, ino,
					crc, csize);
#endif	/* USE_ZLIB */
		return zipwtemp(fd, fn, st, bp, sz, dev, ino, crc, csize);
	}
	*csize = 0;
	if (read(fd, ibuf, st->st_size) != st->st_size) {
		free(ibuf);
		emsg(3, "Cannot read \"%s\"", fn);
		close(fd);
		return -1;
	}
	*crc = zipcrc(0, (unsigned char *)ibuf, st->st_size);
#if USE_BZLIB
	if (zipclevel == 07) {
		unsigned int	sb;
		if ((obuf = malloc(sb = st->st_size)) == 0)
			goto store;
		if (BZ2_bzBuffToBuffCompress(obuf, &sb, ibuf, st->st_size,
					9, 0, 0) != BZ_OK)
			goto store;
		*csize = sb;
		bp->Zdr.z_cmethod[0] = C_BZIP2;
		bp->Zdr.z_version[0] = 0x2e;
		goto out;
	}
#endif /* USE_BZLIB */
	if (zipclevel > 03)
		goto store;
#if USE_ZLIB
	memset(&z, 0, sizeof z);
	if (deflateInit2(&z, ziptrlevel(), Z_DEFLATED, -15,
			8, Z_DEFAULT_STRATEGY) < 0)
		goto store;
	z.next_in = (unsigned char *)ibuf;
	z.avail_in = z.total_in = st->st_size;
	do {
		if (z.avail_out == 0) {
			if ((obuf = realloc(obuf, osize += 4096)) == 0) {
				deflateEnd(&z);
				goto store;
			}
			z.next_out = (unsigned char *)&obuf[*csize];
			z.avail_out = osize - *csize;
		}
		if ((i = deflate(&z, z.avail_in ? Z_NO_FLUSH : Z_FINISH)) < 0) {
			deflateEnd(&z);
			goto store;
		}
		*csize = osize - z.avail_out;
	} while (z.avail_in || i != Z_STREAM_END);
	deflateEnd(&z);
	if (*csize < st->st_size) {
		bp->Zdr.z_cmethod[0] = C_DEFLATED;
		bp->Zdr.z_gflag[0] |= zipclevel << 1;
		bp->Zdr.z_version[0] = 20;
	} else
#endif	/* USE_ZLIB */
	store:	*csize = st->st_size;
#if USE_BZLIB
out:
#endif /* USE_BZLIB */
	le32p(*crc, bp->Zdr.z_crc32);
	le32p(*csize, bp->Zdr.z_csize);
	bwrite((char *)bp, SIZEOF_zip_header);
	bwrite(fn, sz);
	zipwxtra(fn, st, dev, ino);
	switch (bp->Zdr.z_cmethod[0]) {
	case C_DEFLATED:
	case C_BZIP2:
		bwrite(obuf, *csize);
		break;
	default:
		bwrite(ibuf, *csize);
	}
	free(ibuf);
	free(obuf);
	close(fd);
	return 0;
}

/*
 * Write and compress data to a zip archive for a file that is to large
 * too be kept in memory. If there is an error with the temporary file
 * (e. g. no space left on device), the file is stored in uncompressed
 * form.
 */
static int
zipwtemp(int fd, const char *fn, struct stat *st, union bincpio *bp, size_t sz,
		uint32_t dev, uint32_t ino, uint32_t *crc, long long *csize)
{
	static int	tf = -1;
	static char	tlate[] = "/var/tmp/cpioXXXXXX";
	char	ibuf[32768];
#if USE_ZLIB || USE_BZLIB
	char	obuf[32768];
#endif	/* USE_ZLIB || USE_BZLIB */
	struct zextra_64	zf;
	struct zextra_64	*zfp = 0;
	long long	size = st->st_size;
	const char	*sname;
	int	cuse, sf;
	ssize_t	rd;

	*csize = 0;
	*crc = 0;
#if USE_ZLIB || USE_BZLIB
	if (tf < 0) {
		if ((tf = mkstemp(tlate)) >= 0)
			unlink(tlate);
	} else if (lseek(tf, 0, SEEK_SET) != 0) {
		close(tf);
		tf = -1;
	}
#endif	/* USE_ZLIB || USE_BZLIB */
#if USE_ZLIB
	if (zipclevel < 04) {
		struct z_stream_s	z;
		memset(&z, 0, sizeof z);
		if ((cuse = deflateInit2(&z, ziptrlevel(), Z_DEFLATED,
				-15, 8, Z_DEFAULT_STRATEGY)) < 0)
			goto store;
		do {
			if (z.avail_in == 0 && size > 0) {
				if ((rd = read(fd, ibuf, sizeof ibuf)) <= 0) {
					emsg(3, "Cannot read \"%s\"", fn);
					close(fd);
					ftruncate(tf, 0);
					return -1;
				}
				z.next_in = (unsigned char *)ibuf;
				z.avail_in = z.total_in = rd;
				size -= rd;
				*crc = zipcrc(*crc, (unsigned char *)ibuf, rd);
			}
			if (z.next_out == NULL || (char *)z.next_out > obuf) {
				if (z.next_out && tf >= 0) {
					if (write(tf, obuf,
					    (char *)z.next_out-obuf) !=
					    (char *)z.next_out-obuf) {
						close(tf);
						tf = -1;
					}
					*csize += (char *)z.next_out - obuf;
				}
				z.next_out = (unsigned char *)obuf;
				z.avail_out = sizeof obuf;
			}
			if (cuse >= 0 && cuse != Z_STREAM_END)
				cuse = deflate(&z,
					z.avail_in?Z_NO_FLUSH:Z_FINISH);
			else
				z.avail_in = 0;
		} while (size>0 || (char *)z.next_out>obuf ||
				cuse>=0 && cuse!=Z_STREAM_END);
		deflateEnd(&z);
		goto out;
	}
#endif	/* USE_ZLIB */
#if USE_BZLIB
	if (zipclevel == 07) {
		bz_stream	bs;
		int	ok, on;
		memset(&bs, sizeof bs, 0);
		if ((ok = BZ2_bzCompressInit(&bs, 9, 0, 0)) != BZ_OK)
			goto store;
		cuse = 1;
		do {
			if (bs.avail_in == 0 && size > 0) {
				if ((rd = read(fd, ibuf, sizeof ibuf)) <= 0) {
					emsg(3, "Cannot read \"%s\"", fn);
					close(fd);
					ftruncate(tf, 0);
					return -1;
				}
				bs.next_in = ibuf;
				bs.avail_in = rd;
				size -= rd;
				*crc = zipcrc(*crc, (unsigned char *)ibuf, rd);
			}
			if (bs.next_out == NULL || bs.next_out > obuf) {
				if (bs.next_out && tf >= 0) {
					on = bs.next_out - obuf;
					if (write(tf, obuf, on) != on) {
						close(tf);
						tf = -1;
					}
					*csize += on;
				}
				bs.next_out = obuf;
				bs.avail_out = sizeof obuf;
			}
			if (ok != BZ_STREAM_END) {
				switch (ok = BZ2_bzCompress(&bs,
					bs.avail_in?BZ_RUN:BZ_FINISH)) {
				case BZ_RUN_OK:
				case BZ_FINISH_OK:
				case BZ_STREAM_END:
					break;
				default:
					msg(3, 1, "Compression error %d "
							"on \"%s\"\n", ok, fn);
					close(fd);
					return -1;
				}
			}
		} while (size > 0 || bs.next_out > obuf || ok != BZ_STREAM_END);
		BZ2_bzCompressEnd(&bs);
		goto out;
	}
#endif	/* USE_BZLIB */
store:	cuse = -1;
	while (size > 0) {
		if ((rd = read(fd, ibuf, sizeof ibuf)) <= 0) {
			emsg(3, "Cannot read \"%s\"", fn);
			close(fd);
			return -1;
		}
		size -= rd;
		*crc = zipcrc(*crc, (unsigned char *)ibuf, rd);
	}
out:	if (tf >= 0 && cuse >= 0 && *csize < st->st_size) {
		if (zipclevel == 07) {
			bp->Zdr.z_cmethod[0] = C_BZIP2;
			bp->Zdr.z_version[0] = 0x2e;
		} else {
			bp->Zdr.z_cmethod[0] = C_DEFLATED;
			bp->Zdr.z_gflag[0] |= zipclevel << 1;
			bp->Zdr.z_version[0] = 20;
		}
		sf = tf;
		sname = tlate;
	} else {
		*csize = st->st_size;
		sf = fd;
		sname = fn;
	}
	if ((lseek(sf, 0, SEEK_SET)) != 0) {
		emsg(3, "Cannot rewind \"%s\"", sname);
		errcnt++;
		close(fd);
		ftruncate(tf, 0);
		return -1;
	}
	le32p(*crc, bp->Zdr.z_crc32);
	if (st->st_size >= 0xffffffff || *csize >= 0xffffffff) {
		int	n;
		zfp = &zf;
		memset(&zf, 0, SIZEOF_zextra_64);
		le16p(mag_zip64f, zf.ze_64_tag);
		le16p(SIZEOF_zextra_64 - 4, zf.ze_64_tsize);
		le64p(st->st_size, zf.ze_64_nsize);
		le64p(*csize, zf.ze_64_csize);
		le32p(0xffffffff, bp->Zdr.z_csize);
		le32p(0xffffffff, bp->Zdr.z_nsize);
		n = (ple16(bp->Zdr.z_extralen)&0177777) + SIZEOF_zextra_64;
		le16p(n, bp->Zdr.z_extralen);
	} else
		le32p(*csize, bp->Zdr.z_csize);
	bwrite((char *)bp, SIZEOF_zip_header);
	bwrite(fn, sz);
	if (zfp)
		bwrite((char *)zfp, SIZEOF_zextra_64);
	zipwxtra(fn, st, dev, ino);
	size = *csize;
	while (size) {
		if ((rd=read(sf, ibuf, size>sizeof ibuf?sizeof ibuf:size)) <= 0)
			msg(3, 1, "Cannot read \"%s\"\n", sname);
		bwrite(ibuf, rd);
		size -= rd;
	}
	ftruncate(tf, 0);
	close(fd);
	return 0;
}

#if USE_ZLIB
/*
 * Write a zip archive entry using the data descriptor structure.
 */
static int
zipwdesc(int fd, const char *fn, struct stat *st, union bincpio *bp, size_t sz,
		uint32_t dev, uint32_t ino, uint32_t *crc, long long *csize)
{
	struct zextra_64	zf;
	struct zextra_64	*zfp = 0;
	char	ibuf[32768], obuf[32768];
	long long	size = st->st_size;
	ssize_t	rd;
	struct z_stream_s	z;
	int	cuse;

	*csize = 0;
	*crc = 0;
	memset(&z, 0, sizeof z);
	if ((cuse = deflateInit2(&z, ziptrlevel(), Z_DEFLATED,
					-15, 8, Z_DEFAULT_STRATEGY)) < 0)
		return zipwtemp(fd, fn, st, bp, sz, dev, ino, crc, csize);
	bp->Zdr.z_cmethod[0] = C_DEFLATED;
	bp->Zdr.z_gflag[0] |= zipclevel << 1 | FG_DESC;
	bp->Zdr.z_version[0] = 20;
	/*
	 * RFC 1951 states that deflate compression needs 5 bytes additional
	 * space per 32k block in the worst case. Thus a compressed size
	 * greater than 4G-1 can be reached if at least 131052 blocks are
	 * used.
	 */
	if (st->st_size >= 131052LL*32768) {
		int	n;
		zfp = &zf;
		memset(&zf, 0, SIZEOF_zextra_64);
		le16p(mag_zip64f, zf.ze_64_tag);
		le16p(SIZEOF_zextra_64 - 4, zf.ze_64_tsize);
		le32p(0xffffffff, bp->Zdr.z_csize);
		le32p(0xffffffff, bp->Zdr.z_nsize);
		n = (ple16(bp->Zdr.z_extralen)&0177777) + SIZEOF_zextra_64;
		le16p(n, bp->Zdr.z_extralen);
	}
	bwrite((char *)bp, SIZEOF_zip_header);
	bwrite(fn, sz);
	if (zfp)
		bwrite((char *)zfp, SIZEOF_zextra_64);
	zipwxtra(fn, st, dev, ino);
	do {
		if (z.avail_in == 0 && size > 0) {
			if ((rd = read(fd, ibuf, sizeof ibuf)) <= 0) {
				emsg(3, "Cannot read \"%s\"", fn);
				st->st_size -= size;
				size = 0;	/* can't simply stop here */
				rd = 0;		/* no data */
			}
			z.next_in = (unsigned char *)ibuf;
			z.avail_in = z.total_in = rd;
			size -= rd;
			*crc = zipcrc(*crc, (unsigned char *)ibuf, rd);
		}
		if (z.next_out == NULL || (char *)z.next_out > obuf) {
			if (z.next_out) {
				bwrite(obuf, (char *)z.next_out - obuf);
				*csize += (char *)z.next_out - obuf;
			}
			z.next_out = (unsigned char *)obuf;
			z.avail_out = sizeof obuf;
		}
		if (cuse >= 0 && cuse != Z_STREAM_END)
			cuse = deflate(&z, z.avail_in?Z_NO_FLUSH:Z_FINISH);
		else
			z.avail_in = 0;
	} while (size > 0 || (char *)z.next_out > obuf ||
			cuse >= 0 && cuse != Z_STREAM_END);
	deflateEnd(&z);
	if (zfp) {
		struct zipddesc64	zd64;
		memcpy(zd64.zd_signature, mag_zipdds, sizeof mag_zipdds);
		le32p(*crc, zd64.zd_crc32);
		le64p(st->st_size, zd64.zd_nsize);
		le64p(*csize, zd64.zd_csize);
		bwrite((char *)&zd64, SIZEOF_zipddesc64);
	} else {
		struct zipddesc	zd;
		memcpy(zd.zd_signature, mag_zipdds, sizeof mag_zipdds);
		le32p(*crc, zd.zd_crc32);
		le32p(st->st_size, zd.zd_nsize);
		le32p(*csize, zd.zd_csize);
		bwrite((char *)&zd, SIZEOF_zipddesc);
	}
	close(fd);
	return 0;
}
#endif	/* USE_ZLIB */

/*
 * Write the extra fields for a file to a zip archive (currently
 * our own field type). Note that the z_extralen field in the file
 * header must correspond to the size of the data written here.
 */
static int
zipwxtra(const char *fn, struct stat *st, uint32_t dev, uint32_t ino)
{
	struct zextra_cp	z;

	memset(&z, 0, SIZEOF_zextra_cp);
	le16p(mag_zipcpio, z.ze_cp_tag);
	le16p(SIZEOF_zextra_cp - 4, z.ze_cp_tsize);
	le32p(dev, z.ze_cp_mode);
	le32p(ino, z.ze_cp_ino);
	le32p(st->st_mode&(S_IFMT|07777), z.ze_cp_mode);
	le32p(st->st_uid, z.ze_cp_uid);
	le32p(st->st_gid, z.ze_cp_gid);
	le32p(st->st_nlink, z.ze_cp_nlink);
	le32p(st->st_rdev, z.ze_cp_rdev);
	le32p(st->st_mtime, z.ze_cp_mtime);
	le32p(st->st_atime, z.ze_cp_atime);
	bwrite((char *)&z, SIZEOF_zextra_cp);
	return SIZEOF_zextra_cp;
}

static void
zipinfo(struct file *f)
{
	const char	*cp;
	char	b[5];
	int	i;

	printf(" %7llu", f->f_csize);
	if (f->f_dsize) {
		i = f->f_csize*100 / f->f_dsize;
		i += f->f_csize*200 / f->f_dsize & 1;
		i = 100 - i;
	} else
		i = 0;
	printf(" %3d%%", i);
	switch (f->f_cmethod) {
	case C_STORED:
		cp = "stor";
		break;
	case C_SHRUNK:
		cp = "shrk";
		break;
	case C_REDUCED1:
		cp = "re:1";
		break;
	case C_REDUCED2:
		cp = "re:2";
		break;
	case C_REDUCED3:
		cp = "re:3";
		break;
	case C_REDUCED4:
		cp = "re:4";
		break;
	case C_IMPLODED:
		b[0] = 'i';
		b[1] = f->f_gflag & FG_BIT1 ? '8' : '4';
		b[2] = ':';
		b[3] = f->f_gflag & FG_BIT2 ? '3' : '2';
		b[4] = '\0';
		cp = b;
		break;
	case C_TOKENIZED:
		cp = "tokn";
		break;
	case C_DEFLATED:
		b[0] = 'd', b[1] = 'e', b[2] = 'f', b[4] = '\0';
		if (f->f_gflag & FG_BIT2)
			b[3] = f->f_gflag & FG_BIT1 ? 'S' : 'F';
		else
			b[3] = f->f_gflag & FG_BIT1 ? 'X' : 'N';
		cp = b;
		break;
	case C_ENHDEFLD:
		cp = "edef";
		break;
	case C_DCLIMPLODED:
		cp = "dcli";
		break;
	case C_BZIP2:
		cp = "bz2 ";
		break;
	default:
		snprintf(b, sizeof b, "%4.4X", f->f_cmethod);
		cp = b;
	}
	printf(" %s", cp);
}

#if USE_BZLIB
int
zipunbz2(struct file *f, const char *tgt, int tfd, int doswap, uint32_t *crc)
{
	bz_stream	bs;
	long long	isize = f->f_csize;
	char	ibuf[4096], obuf[8192];
	int	in, on, val = 0, ok;

	memset(&bs, 0, sizeof bs);

	if ((ok = BZ2_bzDecompressInit(&bs, 0, 0)) != BZ_OK) {
		msg(3, 0, "bzip2 initialization error %d on \"%s\"\n",
				ok, f->f_name);
		errcnt++;
		return skipfile(f);
	}
	while (isize > 0 || ok == BZ_OK) {
		if (bs.avail_in == 0 && isize > 0) {
			in = sizeof ibuf < isize ? sizeof ibuf : isize;
			isize -= in;
			if (bread(ibuf, in) != in)
				unexeoa();
			if (doswap)
				swap(ibuf, in, bflag || sflag, bflag || Sflag);
			bs.next_in = ibuf;
			bs.avail_in = in;
		}
		if (ok == BZ_OK) {
			bs.next_out = obuf;
			bs.avail_out = sizeof obuf;
			switch (ok = BZ2_bzDecompress(&bs)) {
			case BZ_OK:
			case BZ_STREAM_END:
				on = sizeof obuf - bs.avail_out;
				if (tfd >= 0 && write(tfd, obuf, on) != on) {
					emsg(3, "Cannot write \"%s\"", tgt);
					tfd = -1;
					val = 1;
				}
				*crc = zipcrc(*crc, (unsigned char *)obuf, on);
				break;
			default:
				msg(3, 0, "compression error %d on \"%s\"\n",
					ok, f->f_name);
				errcnt++;
				val = 1;
			}
		}
	}
	BZ2_bzDecompressEnd(&bs);
	return val;
}
#endif	/* USE_BZLIB */

struct	blasthow {
	struct file	*bh_f;
	const char	*bh_tgt;
	long long	bh_isize;
	uint32_t	*bh_crc;
	int		bh_tfd;
	int		bh_doswap;
	int		bh_val;
};

static unsigned
blastin(void *how, unsigned char **buf)
{
	const int	chunk = 16384;
	static unsigned char	*hold;
	struct blasthow	*bp = how;
	unsigned	sz;

	if (bp->bh_isize <= 0)
		return 0;
	if (hold == NULL)
		hold = smalloc(chunk);
	sz = bp->bh_isize > chunk ? chunk : bp->bh_isize;
	bp->bh_isize -= sz;
	if (bread((char *)hold, sz) != sz)
		unexeoa();
	if (bp->bh_doswap)
		swap((char *)hold, sz, bflag || sflag, bflag || Sflag);
	*buf = hold;
	return sz;
}

static int
blastout(void *how, unsigned char *buf, unsigned len)
{
	struct blasthow	*bp = how;

	if (bp->bh_tfd >= 0 && write(bp->bh_tfd, buf, len) != len) {
		emsg(3, "Cannot write \"%s\"", bp->bh_tgt);
		bp->bh_tfd = -1;
		bp->bh_val = 1;
	}
	*bp->bh_crc = zipcrc(*bp->bh_crc, buf, len);
	return 0;
}

int
zipblast(struct file *f, const char *tgt, int tfd, int doswap, uint32_t *crc)
{
	struct blasthow	bh;
	int	n;

	bh.bh_f = f;
	bh.bh_tgt = tgt;
	bh.bh_isize = f->f_csize;
	bh.bh_crc = crc;
	bh.bh_tfd = tfd;
	bh.bh_doswap = doswap;
	bh.bh_val = 0;
	switch (n = blast(blastin, &bh, blastout, &bh)) {
	case 0:
		break;
	default:
		msg(3, 0, "compression error %d on \"%s\"\n", n, f->f_name);
		errcnt++;
		bh.bh_val = 1;
	}
	while (bh.bh_isize) {
		char	buf[4096];
		unsigned	n;
		n = bh.bh_isize > sizeof buf ? sizeof buf : bh.bh_isize;
		if (bread(buf, n) != n)
			unexeoa();
		bh.bh_isize -= n;
	}
	return bh.bh_val;
}

/*
 * The SGI -K format was introduced with SGI's IRIX 5.X. It is essentially
 * a slightly extended binary format. The known additions are:
 *
 * - If the major or minor st_rdev device number exceeds the limit of
 *   255 imposed by the 16-bit c_rdev field, this field is set to 0xFFFF
 *   and st_rdev is stored in the c_filesize 32-bit field. The first 14
 *   bits of this field compose the major device number, the minor is
 *   stored in the remaining 18 bits. This enables implementations to
 *   read the modified format without special support if they ignore
 *   the size of device files; otherwise they will try to read a lot
 *   of archive data and fail.
 *
 * - If the file is larger than 2 GB - 1 byte, two nearly identical
 *   archive headers are stored for it. The only difference is in
 *   the c_filesize field:
 *
 *   [first header: file size X times 2 GB]
 *    [first data part: X times 2 GB]
 *   [second header: file size modulo 2 GB]
 *    [second data part: rest of data]
 *
 *   The first header can be recognized by a negative c_filesize. A
 *   value of 0xFFFFFFFF means that 2 GB follow, 0xFFFFFFFE -> 4 GB
 *   0xFFFFFFFD -> 6 GB, 0xFFFFFFFC -> 8 GB, and so forth. The second
 *   is a standard binary cpio header, although the following data is
 *   meant to be appended to the preceding file.
 *
 *   It is important to note that padding follows the number in
 *   c_filesize, not the amount of data written; thus all data parts
 *   with odd c_filesize fields (0xFFFFFFFF = 2+ GB, 0xFFFFFFFD = 6+ GB
 *   etc.) cause the following archive entries to be aligned on an odd
 *   offset. This seems to be an implementation artifact but has to be
 *   followed for compatibility.
 *
 *   This extension seems a bit weird since no known cpio implementation
 *   is able to read these archive entries without special support. Thus
 *   a more straightforward extension (such as storing the size just past
 *   the file name) would well have had the same effect. Nevertheless,
 *   the cpio -K format is useful, so it is implemented here.
 *
 *   --Note that IRIX 6.X tar also has a -K option. This option extends
 *   the tar format in essentially the same way as the second extension
 *   to cpio described above. Unfortunately, the result is definitively
 *   broken. Contrasting to the binary cpio format, the standard POSIX
 *   tar format is well able to hold files of size 0xFFFFFFFF and below
 *   in a regular manner. Thus, a tar -K archive entry is _exactly_ the
 *   same as two regular POSIX tar entries for the same file. And this
 *   situation even occurs on a regular basis with tar -r! For this
 *   reason, we do not implement the IRIX tar -K format and will probably
 *   never do so unless it is changed (the tar format really has a lot
 *   of options to indicate extensions that might be used for this).
 *
 *   Many thanks to Sven Mascheck who made archiving tests on the IRIX
 *   machine.
 */
/*
 * This function reads the second header of a SGI -K format archive.
 */
static void
readK2hdr(struct file *f)
{
	struct file	n;
	union bincpio	bc;

	n.f_name = n.f_lnam = NULL;
	n.f_nsiz = n.f_lsiz = 0;
	readhdr(&n, &bc);
	f->f_Krest = n.f_st.st_size;
	f->f_dsize = f->f_st.st_size = n.f_st.st_size + f->f_Kbase;
	f->f_Kbase = 0;
	free(n.f_name);
	free(n.f_lnam);
}

/*
 * Read the data of a GNU filename extra header.
 */
static int
readgnuname(char **np, size_t *sp, long length)
{
	if (length > SANELIMIT)
		return -1;
	if (*sp == 0 || *sp <= length)
		*np = srealloc(*np, *sp = length+1);
	bread(*np, length);
	(*np)[length] = '\0';
	skippad(length, 512);
	return 0;
}

/*
 * Write a GNU filename extra header and its data.
 */
static void
writegnuname(const char *fn, long length, int flag)
{
	union bincpio	bc;

	memset(bc.data, 0, 512);
	strcpy(bc.Tdr.t_name, "././@LongLink");
	sprintf(bc.Tdr.t_mode, "%7.7o", 0);
	sprintf(bc.Tdr.t_uid, "%7.7o", 0);
	sprintf(bc.Tdr.t_gid, "%7.7o", 0);
	sprintf(bc.Tdr.t_size, "%11.11lo", length);
	sprintf(bc.Tdr.t_mtime, "%11.11lo", 0L);
	bc.Tdr.t_linkflag = flag;
	memcpy(bc.Tdr.t_magic, mag_gnutar, 8);
	strcpy(bc.Tdr.t_uname, "root");
	strcpy(bc.Tdr.t_gname, "root");
	tchksum(&bc);
	bwrite(bc.data, 512);
	bwrite(fn, length);
	length %= 512;
	memset(bc.data, 0, 512 - length);
	bwrite(bc.data, 512 - length);
}

/*
 * POSIX.1-2001 pax format support.
 */
static void
tgetpax(struct tar_header *tp, struct file *f)
{
	char	*keyword, *value;
	char	*block, *bp;
	long long	n;
	enum paxrec	pr;

	n = rdoct(tp->t_size, 12);
	bp = block = smalloc(n+1);
	bread(block, n);
	skippad(n, 512);
	block[n] = '\0';
	while (bp < &block[n]) {
		int	c;
		pr = tgetrec(&bp, &keyword, &value);
		switch (pr) {
		case PR_ATIME:
			f->f_st.st_atime = strtoll(value, NULL, 10);
			break;
		case PR_GID:
			f->f_st.st_gid = strtoll(value, NULL, 10);
			break;
		case PR_LINKPATH:
			c = strlen(value);
			if (f->f_lnam == NULL || f->f_lsiz < c+1) {
				f->f_lsiz = c+1;
				f->f_lnam = srealloc(f->f_lnam, c+1);
			}
			strcpy(f->f_lnam, value);
			break;
		case PR_MTIME:
			f->f_st.st_mtime = strtoll(value, NULL, 10);
			break;
		case PR_PATH:
			c = strlen(value);
			if (f->f_name == NULL || f->f_nsiz < c+1) {
				f->f_nsiz = c+1;
				f->f_name = srealloc(f->f_name, c+1);
			}
			strcpy(f->f_name, value);
			break;
		case PR_SIZE:
			f->f_st.st_size = strtoll(value, NULL, 10);
			break;
		case PR_UID:
			f->f_st.st_uid = strtoll(value, NULL, 10);
			break;
		case PR_SUN_DEVMAJOR:
			f->f_rmajor = strtoll(value, NULL, 10);
			break;
		case PR_SUN_DEVMINOR:
			f->f_rminor = strtoll(value, NULL, 10);
			break;
		}
		paxrec |= pr;
	}
	if (tp->t_linkflag == 'g') {
		globrec = paxrec & ~(PR_LINKPATH|PR_PATH|PR_SIZE);
		globst = f->f_st;
	}
	free(block);
}

static enum paxrec
tgetrec(char **bp, char **keyword, char **value)
{
	char	*x;
	long	n = 0;
	enum paxrec	pr;

	*keyword = "";
	*value = "";
	while (**bp && (n = strtol(*bp, &x, 10)) <= 0 && (*x!=' ' || *x!='\t'))
		do
			(*bp)++;
		while (**bp && **bp != '\n');
	if (*x == '\0' || **bp == '\0') {
		(*bp)++;
		return PR_NONE;
	}
	while (x < &(*bp)[n] && (*x == ' ' || *x == '\t'))
		x++;
	if (x == &(*bp)[n] || *x == '=')
		goto out;
	*keyword = x;
	while (x < &(*bp)[n] && *x != '=')
		x++;
	if (x == &(*bp)[n])
		goto out;
	*x = '\0';
	if (&x[1] < &(*bp)[n])
		*value = &x[1];
	(*bp)[n-1] = '\0';
out:	*bp = &(*bp)[n];
	if (strcmp(*keyword, "atime") == 0)
		pr = PR_ATIME;
	else if (strcmp(*keyword, "gid") == 0)
		pr = PR_GID;
	else if (strcmp(*keyword, "linkpath") == 0)
		pr = PR_LINKPATH;
	else if (strcmp(*keyword, "mtime") == 0)
		pr = PR_MTIME;
	else if (strcmp(*keyword, "path") == 0)
		pr = PR_PATH;
	else if (strcmp(*keyword, "size") == 0)
		pr = PR_SIZE;
	else if (strcmp(*keyword, "uid") == 0)
		pr = PR_UID;
	else if (strcmp(*keyword, "SUN.devmajor") == 0)
		pr = PR_SUN_DEVMAJOR;
	else if (strcmp(*keyword, "SUN.devminor") == 0)
		pr = PR_SUN_DEVMINOR;
	else
		pr = PR_NONE;
	return pr;
}

static void
wrpax(const char *longname, const char *linkname, struct stat *sp)
{
	union bincpio	bc;
	char	*pdata = NULL;
	long	psize = 0, pcur = 0;
	long long	blocks;

	memset(bc.data, 0, 512);
	if (paxrec & PR_ATIME)
		addrec(&pdata, &psize, &pcur, "atime", NULL, sp->st_atime);
	if (paxrec & PR_GID)
		addrec(&pdata, &psize, &pcur, "gid", NULL, sp->st_gid);
	if (paxrec & PR_LINKPATH)
		addrec(&pdata, &psize, &pcur, "linkpath", linkname, 0);
	if (paxrec & PR_MTIME)
		addrec(&pdata, &psize, &pcur, "mtime", NULL, sp->st_mtime);
	if (paxrec & PR_PATH)
		addrec(&pdata, &psize, &pcur, "path", longname, 0);
	if (paxrec & PR_SIZE)
		addrec(&pdata, &psize, &pcur, "size", NULL, sp->st_size);
	if (paxrec & PR_UID)
		addrec(&pdata, &psize, &pcur, "uid", NULL, sp->st_uid);
	if (paxrec & PR_SUN_DEVMAJOR)
		addrec(&pdata, &psize, &pcur, "SUN.devmajor", NULL,
				major(sp->st_rdev));
	if (paxrec & PR_SUN_DEVMINOR)
		addrec(&pdata, &psize, &pcur, "SUN.devminor", NULL,
				minor(sp->st_rdev));
	paxnam(&bc.Tdr, longname);
	sprintf(bc.Tdr.t_mode, "%7.7o", fmttype==FMT_SUN ? 0444|S_IFREG : 0444);
	sprintf(bc.Tdr.t_uid, "%7.7o", 0);
	sprintf(bc.Tdr.t_gid, "%7.7o", 0);
	sprintf(bc.Tdr.t_size, "%11.11lo", pcur);
	sprintf(bc.Tdr.t_mtime, "%11.11o", 0);
	strcpy(bc.Tdr.t_magic, "ustar");
	bc.Tdr.t_version[0] = bc.Tdr.t_version[1] = '0';
	strcpy(bc.Tdr.t_uname, "root");
	strcpy(bc.Tdr.t_gname, "root");
	bc.Tdr.t_linkflag = fmttype==FMT_SUN ? 'X' : 'x';
	tchksum(&bc);
	bwrite(bc.data, 512);
	memset(&pdata[pcur], 0, psize - pcur);
	blocks = (pcur + (512-1)) / 512;
	bwrite(pdata, blocks * 512);
	free(pdata);
}

static void
addrec(char **pdata, long *psize, long *pcur,
		const char *keyword, const char *sval, long long lval)
{
	char	dval[25], xval[25];
	long	od, d, r;

	if (sval == 0) {
		sprintf(xval, "%lld", lval);
		sval = xval;
	}
	r = strlen(keyword) + strlen(sval) + 3;
	d = 0;
	do {
		od = d;
		d = sprintf(dval, "%ld", od + r);
	} while (d != od);
	*psize += d + r + 1 + 512;
	*pdata = srealloc(*pdata, *psize);
	sprintf(&(*pdata)[*pcur], "%s %s=%s\n", dval, keyword, sval);
	*pcur += d + r;
}

static void
paxnam(struct tar_header *hp, const char *name)
{
	char	buf[257], *bp;
	const char	*cp, *np;
	int	bl = 0;
	static int	pid;

	if (pid == 0)
		pid = getpid();
	for (np = name; *np; np++);
	while (np > name && *np != '/') {
		np--;
		bl++;
	}
	if ((np > name || *name == '/') && np-name <= 120)
		for (bp = buf, cp = name; cp < np; bp++, cp++)
			*bp = *cp;
	else {
		*buf = '.';
		bp = &buf[1];
	}
	snprintf(bp, sizeof buf - (bp - buf), "/PaxHeaders.%d/%s",
			pid, bl < 100 ? np>name?&np[1]:name : sequence());
	tmkname(hp, buf);
}

static char *
sequence(void)
{
	static char	buf[25];
	static long long	d;

	sprintf(buf, "%10.10lld", ++d);
	return buf;
}

static int
pax_oneopt(const char *s, int warn)
{
	if (strcmp(s, "linkdata") == 0)
		pax_oflag |= PO_LINKDATA;
	else if (strcmp(s, "times") == 0)
		pax_oflag |= PO_TIMES;
	else {
		if (warn)
			msg(2, 0, "Unknown flag \"-o %s\"\n", s);
		return -1;
	}
	return 0;
}

int
pax_options(char *s, int warn)
{
	char	*o = s, c;
	int	val = 0, word = 0;

	do {
		if (word == 0) {
			if (isspace(*s&0377))
				o = &s[1];
			else
				word = 1;
		}
		if (*s == ',' || *s == '\0') {
			c = *s;
			*s = '\0';
			val |= pax_oneopt(o, warn);
			*s = c;
			o = &s[1];
			word = 0;
		}
	} while (*s++);
	return val;
}

/*
 * Given a symbolic link "base" and the result of readlink "name", form
 * a valid path name for the link target.
 */
static char *
joinpath(const char *base, char *name)
{
	const char	*bp = NULL, *cp;
	char	*new, *np;

	if (*name == '/')
		return name;
	for (cp = base; *cp; cp++)
		if (*cp == '/')
			bp = cp;
	if (bp == NULL)
		return name;
	np = new = smalloc(bp - base + strlen(name) + 2);
	for (cp = base; cp < bp; cp++)
		*np++ = *cp;
	*np++ = '/';
	for (cp = name; *cp; cp++)
		*np++ = *cp;
	*np = '\0';
	free(name);
	return new;
}

static int
utf8(const char *cp)
{
	int	c, n;

	while (*cp) if ((c = *cp++ & 0377) & 0200) {
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
		while (n--) {
			c = *cp++ & 0377;
			if (c != (c & 077 | 0200))
				return 0;
		}
	}
	return 1;
}

static time_t
fetchtime(const char *cp)
{
	struct tm	tm;
	time_t	t;
	char	*xp;
	int	n;

	t = strtoll(cp, &xp, 10);
	if (*xp == '\0')
		return t;
	memset(&tm, 0, sizeof tm);
	n = sscanf(cp, "%4d%2d%2dT%2d%2d%2d",
			&tm.tm_year, &tm.tm_mon, &tm.tm_mday,
			&tm.tm_hour, &tm.tm_min, &tm.tm_sec);
	tm.tm_year -= 1900;
	tm.tm_mon--;
	tm.tm_isdst = -1;
	t = mktime(&tm);
	if (n < 3 || t == (time_t)-1)
		msg(4, 1, "line %lld: illegal time \"%s\"\n",
				lineno, cp);
	return t;
}

static char *
nextfield(char *cp, const char *fieldname)
{
	while (*cp && *cp != ':')
		cp++;
	if (*cp == 0)
		msg(4, 1, "line %lld: unterminated \"%s\" field\n",
				lineno, fieldname);
	*cp++ = 0;
	return cp;
}

static char *
getproto(char *np, struct prototype *pp)
{
	char	*tp, *xp;
	long long	t, u;

	memset(pp, 0, sizeof *pp);
	if (*np == ':')
		np++;
	else {
		tp = nextfield(np, "type");
		if (np[1]) 
			goto notype;
		switch (np[0]) {
		case 'b':
			pp->pt_mode |= S_IFBLK;
			break;
		case 'c':
			pp->pt_mode |= S_IFCHR;
			break;
		case 'd':
			pp->pt_mode |= S_IFDIR;
			break;
		case 'f':
			pp->pt_mode |= S_IFREG;
			break;
		case 'p':
			pp->pt_mode |= S_IFIFO;
			break;
		case 's':
			pp->pt_mode |= S_IFLNK;
			break;
		default:
		notype:
			msg(4, 1, "line %lld: unknown type \"%s\"\n",
					lineno, np);
		}
		pp->pt_spec |= PT_TYPE;
		np = tp;
	}
	if (*np == ':')
		np++;
	else {
		struct passwd	*pwd;
		tp = nextfield(np, "owner");
		t = strtoll(np, &xp, 10);
		if (*xp == '\0')
			pp->pt_uid = t;
		else {
			if ((pwd = getpwnam(np)) == NULL)
				msg(4, 1, "line %lld: unknown user \"%s\"\n",
						lineno, np);
			pp->pt_uid = pwd->pw_uid;
		}
		pp->pt_spec |= PT_OWNER;
		np = tp;
	}
	if (*np == ':')
		np++;
	else {
		struct group	*grp;
		tp = nextfield(np, "group");
		t = strtoll(np, &xp, 10);
		if (*xp == '\0')
			pp->pt_gid = t;
		else {
			if ((grp = getgrnam(np)) == NULL)
				msg(4, 1, "line %lld: unknown group \"%s\"\n",
						lineno, np);
			pp->pt_gid = grp->gr_gid;
		}
		pp->pt_spec |= PT_GROUP;
		np = tp;
	}
	if (*np == ':')
		np++;
	else {
		tp = nextfield(np, "mode");
		t = strtol(np, &xp, 8);
		if (t & ~07777 || *xp)
			msg(4, 1, "line %lld: illegal mode \"%s\"\n",
					lineno, np);
		pp->pt_mode |= t;
		pp->pt_spec |= PT_MODE;
		np = tp;
	}
	if (*np == ':')
		np++;
	else {
		tp = nextfield(np, "access time");
		pp->pt_atime = fetchtime(np);
		pp->pt_spec |= PT_ATIME;
		np = tp;
	}
	if (*np == ':')
		np++;
	else {
		tp = nextfield(np, "modification time");
		pp->pt_mtime = fetchtime(np);
		pp->pt_spec |= PT_MTIME;
		np = tp;
	}
	if (*np == ':') {
		np++;
		if (*np++ != ':')
		majmin:	msg(4, 1, "line %lld: need either both major and "
				"minor or none\n",
				lineno);
	} else {
		tp = nextfield(np, "major");
		t = strtoll(np, &xp, 10);
		if (*xp)
			msg(4, 1, "line %lld: illegal major \"%s\"\n",
					lineno, np);
		np = tp;
		if (*np == ':')
			goto majmin;
		tp = nextfield(np, "minor");
		u = strtoll(np, &xp, 10);
		if (*xp)
			msg(4, 1, "line %lld: illegal minor \"%s\"\n",
					lineno, np);
		np = tp;
		pp->pt_rdev = makedev(t, u);
		pp->pt_spec |= PT_RDEV;
	}
	return np;
}
