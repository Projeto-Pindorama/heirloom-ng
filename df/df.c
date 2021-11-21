/*
 * df - disk free
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, December 2000.
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
#ifdef	UCB
static const char sccsid[] USED = "@(#)/usr/ucb/df.sl	1.66 (gritter) 5/8/06";
#else
static const char sccsid[] USED = "@(#)df.sl	1.66 (gritter) 5/8/06";
#endif

/*
 * This code is portable for SUSv2 systems except for
 * - the use of the "long long" integer type;
 * - the use of the appropriate "ll" printf() modifier;
 * - the use of getmntent(), which is Linux-specific.
 */

typedef		unsigned long long	ull;

#include	<pwd.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#if defined (__dietlibc__) || defined (__OpenBSD__)
#include	"statvfs.c"
#elif defined (__FreeBSD__) && (__FreeBSD__) < 5
#include	"statvfs.c"
#elif defined (__NetBSD__)
#include	<sys/param.h>
#if __NetBSD_Version__ < 300000000
#include	"statvfs.c"
#else	/* __ __NetBSD_Version__ >= 300000000 */
#include	<sys/statvfs.h>
#endif	/* __ __NetBSD_Version__ >= 300000000 */
#else
#include	<sys/statvfs.h>
#endif
#include	<fcntl.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<errno.h>
#include	<stdlib.h>
#if defined	(__linux__) || defined (__hpux) || defined (_AIX)
#include	<mntent.h>
#elif defined	(__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) \
	|| defined (__DragonFly__) || defined (__APPLE__)
#include	<sys/param.h>
#include	<sys/ucred.h>
#include	<sys/mount.h>
#else	/* SVR4 */
#include	<sys/mnttab.h>
#endif	/* SVR4 */
#include	<libgen.h>
#include	<limits.h>

#ifndef	__GLIBC_PREREQ
#define	__GLIBC_PREREQ(x, y)	0
#endif

#ifndef	MNTTYPE_IGNORE
#define	MNTTYPE_IGNORE	""
#endif

static int	errcnt;			/* count of errors */
static int	aflag = 1;		/* print all filesystems */
static int	Pflag;			/* POSIX-style */
#ifndef	UCB
static int	fflag;			/* omit inodes in traditional form */
static int	gflag;			/* print entire statvfs structure */
static int	tflag;			/* include totals */
static int	Vflag;			/* print command lines */
static ull	totspace;		/* total space */
static ull	totavail;		/* total available space */
extern int	sysv3;			/* SYSV3 variable set */
#endif	/* !UCB */
static int	hflag;			/* print human-readable units */
static int	kflag;			/* select 1024-blocks */
static int	lflag;			/* local file systems only */
static int	dfspace;		/* this is the dfspace command */
static int	format;			/* output format */
static char	*fstype;		/* restrict to filesystem type */
static char	*progname;		/* argv[0] to main() */
#if defined	(__linux__) || defined (_AIX)
static const char	*mtab = "/etc/mtab";	/* mount table */
#elif defined (__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) \
	|| defined (__DragonFly__) || defined (__APPLE__)
/* nothing */
#else	/* !__linux__, !_AIX, !__FreeBSD__, !__NetBSD__, !__OpenBSD__,
	!__DragonFly__, !__APPLE__ */
static const char	*mtab = "/etc/mnttab";	/* mount table */
#endif	/* !__linux__, !_AIX, !__FreeBSD__, !__NetBSD__, !__OpenBSD__,
	!__DragonFly__, !__APPLE__ */

/*
 * perror()-alike.
 */
static void
pnerror(int eno, const char *string)
{
	fprintf(stderr, "%s: %s: %s\n", progname, string, strerror(eno));
	errcnt |= 1;
}

static char *
hfmt(ull n)
{
	char	*cp;
	const char	units[] = "KMGTPE", *up = units;
	int	rest = 0;

	while (n > 1023) {
		rest = (n % 1024) / 128;
		n /= 1024;
		up++;
	}
	cp = malloc(10);
	if (n < 10 && rest)
		snprintf(cp, 10, "%2llu.%u%c", n, rest, *up);
	else
		snprintf(cp, 10, "%4llu%c", n, *up);
	return cp;
}

#ifndef	UCB
static const char *
files(void)
{
	return sysv3 ? "i-nodes" : "files";
}
#endif	/* !UCB */

/*
 * Prepend the format's header.
 */
static void
printhead(void)
{
	switch (format) {
	case 'h':
		printf(
	"Filesystem             size   used  avail capacity  Mounted on\n");
		break;
	case 'P':
		printf(
		"Filesystem%s       Used  Available Capacity Mounted on\n",
		kflag ? "         1024-blocks" : "          512-blocks");
		break;
	case 'k':
		printf(
"Filesystem              kbytes       used      avail capacity Mounted on\n");
		break;
	case 'i':
		printf(
"Filesystem              itotal      iused      ifree %%iused Mounted on\n");
		break;
#ifndef	UCB
	case 'b':
		printf("Filesystem              avail\n");
		break;
	case 'e':
		printf("Filesystem              ifree\n");
		break;
	case 'v':
		printf(
"Mount Dir  Filesystem             blocks       used       free  %%used\n");
		break;
#endif	/* !UCB */
	}
}

#ifdef	__linux__
/*
 * Get i-node (that is, threads) count for /proc.
 */
static void
procnodes(const char *dir, struct statvfs *sv)
{
	char fn[_POSIX_PATH_MAX];
	long threads_max;
	long a0, a1, b0, b1, c0, c1, nr_running, nr_threads, last_pid;
	FILE *fp;

	snprintf(fn, sizeof fn, "%s/sys/kernel/threads-max", dir);
	if ((fp = fopen(fn, "r")) == NULL)
		return;
	if (fscanf(fp, "%lu", &threads_max) != 1) {
		fclose(fp);
		return;
	}
	fclose(fp);
	snprintf(fn, sizeof fn, "%s/loadavg", dir);
	if ((fp = fopen(fn, "r")) == NULL)
		return;
	if (fscanf(fp, "%ld.%ld %ld.%ld %ld.%ld %ld/%ld %ld",
				&a0, &a1, &b0, &b1, &c0, &c1, &nr_running,
				&nr_threads, &last_pid) != 9) {
		fclose(fp);
		return;
	}
	fclose(fp);
	sv->f_files = threads_max;
	sv->f_ffree = sv->f_favail = threads_max - nr_threads;
}
#endif	/* __linux__ */

/*
 * Percentage computation according to POSIX.
 */
static int
getpct(ull m, ull n)
{
	int	c;
	double	d;

	d = n ? m * 100. / n : 0;
	c = d;
	if (d - (int)d > 0)
		c++;
	return c;
}

/*
 * Print per-file-system statistics. Mdir is the mount point or a file
 * contained, mdev is the file system's (block) device.
 */
static void
printfs(const char *mdir, const char *mdev, const char *mtype)
{
	const char	*cp;
	struct statvfs sv;
	ull total, avail, used, percent;
	int bsize = kflag ? 1024 : 512;

	if (mdev == NULL)
		mdev = "(unknown)";
#ifndef	UCB
	if (Vflag) {
		printf("%s -F %s ", progname, mtype);
		if (tflag)
			printf("-t ");
		switch (format) {
		case 'b':
		case 'e':
		case 'f':
		case 'h':
		case 'g':
		case 'i':
		case 'k':
		case 'n':
		case 't':
		case 'v':
			printf("-%c ", format);
			break;
		case 'E':
			printf("-be ");
			break;
		case 'P':
			if (kflag)
				printf("-Pk ");
			else
				printf("-P");
			break;
		}
		printf("%s\n", *mdev == '/' ? mdev : mdir);
		return;
	}
#endif	/* !UCB */
	if (statvfs(mdir, &sv) < 0) {
		pnerror(errno, mdir);
		return;
	}
	if (aflag == 0 && sv.f_blocks == 0)
		return;
	total = (ull)sv.f_blocks * sv.f_frsize / bsize;
	avail = (ull)sv.f_bavail * sv.f_frsize / bsize;
	used = total - (Pflag ? (ull)sv.f_bavail : (ull)sv.f_bfree)
		* sv.f_frsize / bsize;
#ifdef	__linux__
	/*
	 * Try to estimate i-nodes for reiserfs filesystems. As the number
	 * of used i-nodes cannot be determined, this is not done for -i.
	 *
	 * A reiserfs i-node structure consumes at least 48 bytes. In fact,
	 * more space is used; 167 was measured.
	 */
#define	REISERFS_ISIZE	167
	if (format != 'i' && format != 'g' &&
			((int)sv.f_files == -1 || sv.f_files == 0) &&
			strcmp(mtype, "reiserfs") == 0) {
		sv.f_files = (ull)sv.f_blocks * sv.f_frsize / REISERFS_ISIZE;
		sv.f_ffree = (ull)sv.f_bfree * sv.f_frsize / REISERFS_ISIZE;
		sv.f_favail = (ull)sv.f_bavail * sv.f_frsize / REISERFS_ISIZE;
	}
	if (sv.f_files == 0 && strcmp(mtype, "proc") == 0)
		procnodes(mdir, &sv);
#endif	/* __linux__ */
	switch (format) {
	case 'h':
		percent = getpct(used, total);
		if (strlen(mdev) > 20) {
			printf("%s\n", mdev);
			cp = "";
		} else
			cp = mdev;
		printf("%-20s  %s  %s  %s   %3llu%%    %s\n",
				cp, hfmt(total), hfmt(used), hfmt(avail),
				percent, mdir);
		break;
	case 'P':
	case 'k':
		percent = getpct(used, total);
		printf("%-18s  %10llu %10llu %10llu     %3llu%% %s\n",
				mdev, total, used, avail, percent, mdir);
		break;
	case 'i':
		used = sv.f_files - sv.f_ffree;
		percent = getpct(used, sv.f_files);
		printf("%-18s  %10llu %10llu %10llu   %3llu%% %s\n",
				mdev, (ull)sv.f_files, used, (ull)sv.f_favail,
				percent, mdir);
		break;
#ifndef	UCB
	case 'v':
		percent = getpct(used, total);
		printf("%-10.10s %-18.18s %10llu %10llu %10llu %4u%%\n",
				mdir, mdev, total, used, avail,
				(unsigned)percent);
		break;
	case 'b':
		printf("%-18s %10llu\n", mdir, avail);
		break;
	case 'e':
		printf("%-18s %10llu\n", mdir, (ull)sv.f_favail);
		break;
	case 'n':
		printf("%-18s : %s\n", mdir, mtype);
		break;
	case 'g':
		printf("%-18s (%-18s): %12lu block size  %12lu frag size\n\
%8llu total blocks %10llu free blocks %8llu available %14llu total files\n\
%8llu free files %10lu filesys id\n\
%8s fstype       0x%08lx flag          %6lu filename length\n\n",
	mdir, mdev, (long)sv.f_bsize, (long)sv.f_frsize,
	(ull)sv.f_blocks, (ull)sv.f_bfree, (ull)sv.f_bavail, (ull)sv.f_files,
	(ull)sv.f_ffree,
#if !defined (_AIX) && (!defined (__GLIBC__) || __GLIBC_PREREQ(2,3))
	(long)sv.f_fsid,
#else	/* old glibc with weird fsid_t type */
	0L,
#endif
	mtype, (long)sv.f_flag, (long)sv.f_namemax);
		break;
	case 04:	/* dfspace command */
		totspace += total;
		totavail += avail;
		percent = total ? avail * 100 / total : 0;
printf("%-19s: Disk space: %5llu.%02llu MB of %5llu.%02llu MB available (%2u.%02u%%).\n",
			mdir, (ull)avail / 1024, (ull)avail % 1000 / 10,
			(ull)total / 1024, (ull)total % 1000 / 10,
			(int)percent,
			(int)(percent ? (avail * 10000 / total) % percent : 0));
		break;
	case 'E':	/* both -b and -e */
		printf("%-18s (%-18s): %10llu kilobytes\n", mdir, mdev, avail);
		printf("%-18s (%-18s): %10llu %s\n", mdir, mdev,
				(ull)sv.f_favail, files());
		break;
	default:
		printf("%-18s (%-16s): %9llu blocks", mdir, mdev, avail);
		if (fflag)
			printf("\n");
		else
			printf(" %9llu %s\n", (ull)sv.f_favail, files());
		if (tflag) {
	printf("                                total: %9llu blocks", total);
			if (fflag)
				printf("\n");
			else
				printf(" %9llu %s\n", (ull)sv.f_files, files());
		}
#endif	/* !UCB */
	}
}

/*
 * Determine whether a filesystem is mounted locally.
 */
static int
remotefs(const char *fn)
{
	if (strcmp(fn, "nfs") == 0)
		return 1;
	if (strcmp(fn, "smbfs") == 0)
		return 1;
	return 0;
}

/*
 * Read the mount table and output statistics for each file system.
 */
static void
printmnt(void)
{
#if defined	(__linux__) || defined (__hpux) || defined (_AIX)
	FILE *fp;
	struct mntent *mp;

	if ((fp = setmntent(mtab, "r")) == NULL) {
		pnerror(errno, mtab);
		exit(errcnt);
	}
	while ((mp = getmntent(fp)) != NULL) {
		if (strcmp(mp->mnt_type, MNTTYPE_IGNORE) == 0)
			continue;
		if (lflag && remotefs(mp->mnt_type))
			continue;
		if (dfspace && (strcmp(mp->mnt_type, "proc") == 0 ||
				strcmp(mp->mnt_type, "devpts") == 0 ||
				strcmp(mp->mnt_type, "sysfs") == 0))
			continue;
		if (fstype != NULL && strcmp(mp->mnt_type, fstype))
			continue;
		printfs(mp->mnt_dir, mp->mnt_fsname, mp->mnt_type);
	}
	endmntent(fp);
#elif defined (__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) \
		|| defined (__DragonFly__) || defined (__APPLE__)
	struct statfs	*sp = NULL;
	int	cnt, i;

	if ((cnt = getmntinfo(&sp, MNT_WAIT)) <= 0) {
		pnerror(errno, "getmntinfo");
		exit(errcnt);
	}
	for (i = 0; i < cnt; i++) {
		if (lflag && remotefs(sp[i].f_fstypename))
			continue;
		if (dfspace && (strcmp(sp[i].f_fstypename, "devfs") == 0 ||
				strcmp(sp[i].f_fstypename, "procfs") == 0))
			continue;
		if (fstype != NULL && strcmp(sp[i].f_fstypename, fstype))
			continue;
		printfs(sp[i].f_mntonname, sp[i].f_mntfromname,
				sp[i].f_fstypename);
	}
#else	/* SVR4 */
	FILE *fp;
	struct mnttab	mt;

	if ((fp = fopen(mtab, "r")) == NULL) {
		pnerror(errno, mtab);
		exit(errcnt);
	}
	while (getmntent(fp, &mt) == 0) {
		if (lflag && remotefs(mt.mnt_fstype))
			continue;
		if (dfspace && (strcmp(mt.mnt_fstype, "proc") == 0))
			continue;
		if (fstype != NULL && strcmp(mt.mnt_fstype, fstype))
			continue;
		printfs(mt.mnt_mountp, mt.mnt_special, mt.mnt_fstype);
	}
	fclose(fp);
#endif	/* SVR4 */
}

/*
 * Find the file system that contains the file given in the argument.
 */
static void
findfs(const char *fn)
{
#if defined	(__linux__) || defined (__hpux) || defined (_AIX)
	struct mntent *mp;
#elif defined	(__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) \
		|| defined (__DragonFly__) || defined (__APPLE__)
	struct statfs	*sp = NULL;
	int	count, i;
#else	/* SVR4 */
	struct mnttab mt;
#endif	/* SVR4 */
	struct stat s1, s2;
#if !defined (__FreeBSD__) && !defined (__NetBSD__) && !defined (__OpenBSD__) \
		&& !defined (__DragonFly__) && !defined (__APPLE__)
	FILE *fp;
#endif	/* !__FreeBSD__, !__NetBSD__, !__OpenBSD__, !__DragonFly__,
	   !__APPLE__ */
	int gotcha = 0;
	const char	*m_special, *m_mountp, *m_fstype;

#if defined	(__linux__) || defined (__hpux) || defined (_AIX)
	if ((fp = setmntent(mtab, "r")) == NULL) {
		pnerror(errno, mtab);
		exit(errcnt);
	}
#elif defined	(__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) \
		|| defined (__DragonFly__) || defined (__APPLE__)
	if ((count = getmntinfo(&sp, MNT_WAIT)) <= 0) {
		pnerror(errno, "getmntinfo");
		exit(errcnt);
	}
#else	/* SVR4 */
	if ((fp = fopen(mtab, "r")) == NULL) {
		pnerror(errno, mtab);
		exit(errcnt);
	}
#endif	/* SVR4 */
	if (lstat(fn, &s1) < 0 || S_ISCHR(s1.st_mode)) {
#ifdef	UCB
		pnerror(errno, fn);
#else	/* !UCB */
		fprintf(stderr,
	"%s: (%-10s) not a block device, directory or mounted resource\n",
		progname, fn);
#endif	/* !UCB */
		errcnt |= 1;
		return;
	}
#if defined	(__linux__) || defined (__hpux) || defined (_AIX)
	while ((mp = getmntent(fp)) != NULL) {
		m_special = mp->mnt_fsname;
		m_mountp = mp->mnt_dir;
		m_fstype =mp->mnt_type;
		if (strcmp(m_fstype, MNTTYPE_IGNORE) == 0)
			continue;
#elif defined	(__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) \
		|| defined (__DragonFly__) || defined (__APPLE__)
	for (i = 0; i < count; i++) {
		m_special = sp[i].f_mntfromname;
		m_mountp = sp[i].f_mntonname;
		m_fstype = sp[i].f_fstypename;
#else	/* SVR4 */
	while (getmntent(fp, &mt) == 0) {
		m_special = mt.mnt_special;
		m_mountp = mt.mnt_mountp;
		m_fstype = mt.mnt_fstype;
#endif	/* SVR4 */
		if (S_ISBLK(s1.st_mode)) {
			if (lstat(m_special, &s2) < 0) {
				/*
				 * Ignore this silently as it is most likely
				 * something like "proc", "devpts".
				 */
				continue;
			}
			if (memcmp(&s1.st_rdev, &s2.st_rdev, sizeof s1.st_rdev)
					== 0)
				gotcha++;
		} else {
			if (lstat(m_mountp, &s2) < 0) {
				pnerror(errno, m_mountp);
				continue;
			}
			if (memcmp(&s1.st_dev, &s2.st_dev, sizeof s1.st_dev)
					== 0)
				gotcha++;
		}
		if (gotcha) {
			if (fstype != NULL && strcmp(m_fstype, fstype)) {
				fprintf(stderr,
			"%s: Warning: %s mounted as a %s file system\n",
				progname, m_mountp, m_fstype);
				errcnt |= 1;
			} else if (lflag && remotefs(m_fstype)) {
				fprintf(stderr,
			"%s: Warning: %s is not a local file system\n",
				progname, m_mountp);
				errcnt |= 1;
			} else
				printfs(m_mountp, m_special, m_fstype);
			break;
		}
	}
	if (gotcha == 0) {
		if (S_ISBLK(s1.st_mode))
#ifdef	UCB
			fprintf(stderr,
			"%s: %s: cannot handle unmounted filesystem\n",
			progname, fn);
#else
			fprintf(stderr,
			"%s: (%-10s) cannot handle unmounted filesystem\n",
			progname, fn);
#endif
		else
			fprintf(stderr,
				"%s: Could not find mount point for %s\n",
			progname, fn);
		errcnt |= 1;
	}
#if defined	(__linux__)
	endmntent(fp);
#elif defined	(__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) \
		|| defined (__DragonFly__) || defined (__APPLE__)
	/* nothing */
#else	/* SVR4 */
	fclose(fp);
#endif	/* SVR4 */
}

#ifdef	UCB
static void
usage(void)
{
	fprintf(stderr, "usage: %s [ -i ] [ -a ] [ -t type | file... ]\n",
			progname);
	exit(2);
}

int
main(int argc, char **argv)
{
	int i;

#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	progname = basename(argv[0]);
	aflag = 0;
	format = 'k';
	while ((i = getopt(argc, argv, "aihkM:Pt:")) != EOF) {
		switch (i) {
		case 'a':
			aflag = 1;
			break;
		case 'h':
			hflag = 1;
			break;
		case 'k':
			kflag = 1;
			break;
		case 'M':
#if !defined (__FreeBSD__) && !defined (__NetBSD__) && !defined (__OpenBSD__) \
	&& !defined (__DragonFly__) && !defined (__APPLE__)
			mtab = optarg;
#endif	/* !__FreeBSD__, !__NetBSD__, !__OpenBSD__, !__DragonFly__,
	   !__APPLE__ */
			break;
		case 'P':
			Pflag = 1;
			/*FALLTHRU*/
		case 'i':
			format = i;
			break;
		case 't':
			fstype = optarg;
			aflag = 1;
			break;
		default:
			usage();
		}
	}
	if (hflag) {
		format = 'h';
		kflag = 1;
	} else if (format == 'k')
		kflag = 1;
	if (fstype && optind != argc)
		usage();
	printhead();
	if (optind != argc)
		while (optind < argc)
			findfs(argv[optind++]);
	else
		printmnt();
	return errcnt;
}
#else	/* !UCB */
static void
usage(void)
{
	fprintf(stderr, dfspace ?  "Usage: %s [file ...]\n" : "Usage:\n\
%s [-F FSType] [-befgklntVv] [current_options] \
[-o specific_options] [directory | special ...]\n",
		progname);
	exit(2);
}

int
main(int argc, char **argv)
{
	int i;

	progname = basename(argv[0]);
	if (getenv("SYSV3") != NULL)
		sysv3 = 1;
	if (strcmp(progname, "dfspace") == 0)
		dfspace = 1;
#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	while ((i = getopt(argc, argv,
			dfspace ? "F:" : "befF:ghiklM:no:PtvV")) != EOF) {
		switch (i) {
		case 'f':
			fflag = 1;
			break;
		case 'F':
			fstype = optarg;
			break;
		case 'h':
			hflag = 1;
			break;
		case 'g':
			gflag = 1;
			break;
		case 'l':
			lflag = 1;
			break;
		case 'M':
#if !defined (__FreeBSD__) && !defined (__NetBSD__) && !defined (__OpenBSD__) \
	&& !defined (__DragonFly__) && !defined (__APPLE__)
			mtab = optarg;
#endif	/* !__FreeBSD__, !__NetBSD__, !__OpenBSD__, !__DragonFly__,
	   !__APPLE__ */
			break;
		case 't':
			tflag = 1;
			break;
		case 'k':
			kflag = 1;
			break;
		case 'b':
			if (format == 'e')
				format = 'E';
			else
				format = i;
			break;
		case 'e':
			if (format == 'b')
				format = 'E';
			else
				format = i;
			break;
		case 'P':
			Pflag = 1;
			/*FALLTHRU*/
		case 'i':
		case 'n':
		case 'v':
			format = i;
			break;
		case 'o':
			fprintf(stderr, "%s: invalid suboption: -o %s\n",
					progname, optarg);
			exit(2);
		case 'V':
			Vflag = 1;
			break;
		default:
			usage();
		}
	}
	/*
	 * Some options override others, System V-compatible.
	 */
	if (dfspace) {
		format = 04;
		kflag = 1;
	} else if (hflag) {
		format = 'h';
		kflag = 1;
		tflag = 0;
	} else if (gflag) {
		format = 'g';
		kflag = tflag = 0;
	} else if (tflag && !kflag)
		switch (format) {
		case 'b':
		case 'e':
		case 'n':
			format = 00;
		}
	else if (kflag)
		switch (format) {
		case 'b':
		case 'e':
		case 'f':
		case 'n':
		case 'v':
		case 00:
			format = 'k';
		}
	else if (format == 'b' || format == 'E')
		kflag = 1;
	printhead();
	if (optind != argc)
		while (optind < argc)
			findfs(argv[optind++]);
	else
		printmnt();
	if (dfspace) {
		int percent = totspace ? totavail * 100 / totspace : 0;
printf("\n\
%s: %5llu.%02llu MB of %5llu.%02llu MB available (%2u.%02u%%).\n",
		"Total Disk Space",
		(ull)totavail / 1024, (ull)totavail % 1000 / 10,
		(ull)totspace / 1024, (ull)totspace % 1000 / 10,
		percent,
		(unsigned)(percent ? (totavail*10000/totspace) % percent : 0));
	}
	return errcnt;
}
#endif	/* !UCB */
