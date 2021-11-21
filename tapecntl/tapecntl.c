/*
 * tapecntl - tape control for tape devices
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, December 2003.
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
static const char sccsid[] USED = "@(#)tapecntl.sl	1.38 (gritter) 1/22/06";

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <errno.h>
#if defined (__linux__) || defined (__sun) || defined (__FreeBSD__) \
	|| defined (__hpux) || defined (_AIX) || defined (__NetBSD__) \
	|| defined (__OpenBSD__) || defined (__DragonFly__) \
	|| defined (__APPLE__)
#include <sys/ioctl.h>
#include <sys/mtio.h>
#else	/* SVR4.2MP */
#include <sys/ioctl.h>
#include <sys/scsi.h>
#include <sys/st01.h>
#endif	/* SVR4.2MP */

#define	eq(a, b)	(strcmp(a, b) == 0)

static const char	*tape;
static const char	deftape[] = "/dev/rmt/c0s0n";
static const char	*progname;
static int		status;
static int		omode = O_RDONLY;

static int		mt;

static enum oper {
	OP_NONE		= 0,
	OP_OFFLINE	= 0000001,
	OP_UNLOAD	= 0000002,
	OP_LOAD		= 0000004,
	OP_RESET	= 0000010,
	OP_RETENSION	= 0000020,
	OP_REWIND	= 0000040,
	OP_COMPRESSION	= 0000100,
	OP_DENSITY	= 0000200,
	OP_FIXED	= 0000400,
	OP_ERASE	= 0001000,
	OP_POSITION	= 0002000,
	OP_BACKWARD	= 0004000,
	OP_FWBLOCKS	= 0010000,
	OP_BWBLOCKS	= 0020000,
	OP_ENDDATA	= 0040000,
	OP_WRITEMARK	= 0100000
} operations;

static int		verbose;
static const char	*stats_info;
static int		stats_flags_ok;
static long		stats_flags;
static long		stats_density = -1;
static long		stats_recsiz = -1;

static int		count;

static void
m_usage(void)
{
	fprintf(stderr, "Usage: %s [-v] [-f tapename] command [count]\n",
			progname);
	fprintf(stderr, "Default device is %s\n", deftape);
	fprintf(stderr, "Common Tape Commands:\n"
			"  erase         - erase entire tape\n"
			"  rewind        - rewind the tape\n"
			"  retension     - wind tape to end of reel "
				"and rewind it\n"
			"Other Tape Commands:\n"
			"  eof, wfm      - write file mark\n"
			"  fsf, sfm      - seek next file mark\n"
			"  bsf           - seek previous file mark\n"
			"  enddata       - seek end of data\n");
	exit(1);
}

static void
m_options(int argc, char **argv)
{
	const char	optstring[] = ":vf:";
	int	i;

	while ((i = getopt(argc, argv, optstring)) != EOF) {
		switch (i) {
		case 'v':
			verbose = 1;
			omode |= O_NONBLOCK;
			break;
		case 'f':
			tape = optarg;
			break;
		default:
			m_usage();
		}
	}
	if (tape == NULL) {
		if ((tape = getenv("TAPE")) != NULL)
			tape = strdup(tape);
		else
			tape = deftape;
	}
	if (optind >= argc) {
		if (verbose)
			return;
		m_usage();
	}
	if (eq(argv[optind], "erase")) {
		operations = OP_ERASE;
		omode = O_RDWR | omode&O_NONBLOCK;
	} else if (eq(argv[optind], "rewind"))
		operations = OP_REWIND;
	else if (eq(argv[optind], "retension") || eq(argv[optind], "ret"))
		operations = OP_RETENSION;
	else if (eq(argv[optind], "wfm") || eq(argv[optind], "eof") ||
			eq(argv[optind], "weof")) {
		operations = OP_WRITEMARK;
		omode = O_RDWR | omode&O_NONBLOCK;
		if (optind+1 < argc)
			count = atoi(argv[optind+1]);
		else
			count = 1;
	} else if (eq(argv[optind], "sfm") || eq(argv[optind], "fsf")) {
		operations = OP_POSITION;
		if (optind+1 < argc)
			count = atoi(argv[optind+1]);
		else
			count = 1;
	} else if (eq(argv[optind], "bsf")) {
		operations = OP_BACKWARD;
		if (optind+1 < argc)
			count = atoi(argv[optind+1]);
		else
			count = 1;
	} else if (eq(argv[optind], "fsr")) {
		operations = OP_FWBLOCKS;
		if (optind+1 < argc)
			count = atoi(argv[optind+1]);
		else
			count = 1;
	} else if (eq(argv[optind], "bsr")) {
		operations = OP_BWBLOCKS;
		if (optind+1 < argc)
			count = atoi(argv[optind+1]);
		else
			count = 1;
	} else if (eq(argv[optind], "compression")) {
		operations = OP_COMPRESSION;
		if (optind+1 < argc)
			count = atoi(argv[optind+1]);
		else
			count = 1;
	} else if (eq(argv[optind], "enddata") || eq(argv[optind], "append") ||
			eq(argv[optind], "feom"))
		operations = OP_ENDDATA;
	else if (eq(argv[optind], "offline") || eq(argv[optind], "rewoffl"))
		operations = OP_OFFLINE;
	else if (eq(argv[optind], "status")) {
		verbose = 1;
		omode |= O_NONBLOCK;
	} else
		m_usage();
}

static void
tape_usage(void)
{
	fprintf(stderr, "\
tape v%s usage:   %s [-<tape>] [-a arg] <command> [device]\n\
  SCSI tape commands:\n\
        status     - get the status of the tape\n\
        reten      - retension the tape\n\
        erase      - erase and retension tape\n\
        reset      - reset controller and driver\n\
        rewind     - rewind tape controller\n\
        rfm        - skip to next file\n\
        eod        - position to EOD\n\
        wfm        - write filemark\n\
        load       - load tape\n\
        unload     - unload tape\n\
        setblk     - set block size (in bytes) for device\n\
        setcomp    - set compression (0 disabled, 1 enabled)\n\
        setdensity - set density code (in hexadecimal)\n",
        "1.38",
        progname);
        exit(1);
}

static const char *
tape_default_device(void)
{
	FILE	*fp;
	int	c, t = 0;
	const char	sts[] = " device =";
	char	*device;

	if ((device = getenv("TAPE")) != NULL)
		device = strdup(device);
	else
		device = (char *)deftape;
	if ((fp = fopen(TAPEDFL, "r")) == NULL)
		return device;
	while ((c = getc(fp)) != EOF) {
		switch (c) {
		case '\n':
			t = 0;
			break;
		case ' ':
			if (c != ' ')
				t = -1;
			break;
		default:
			if (t >= 0) {
				if (sts[t] == ' ')
					t++;
				if (sts[t] == c)
					t++;
				else
					t = -1;
			}
		}
		printf("c = %c t = %d\n", c, t);
		if (t >= 0 && sts[t] == '\0') {
			while ((c = getc(fp)) == ' ');
			t = 0;
			device = malloc(1);
			while (c != EOF && c != '\n') {
				device[t++] = c;
				device = realloc(device, t + 1);
				c = getc(fp);
			}
			device[t] = '\0';
		}
	}
	fclose(fp);
	return device;
}

static void
tape_options(int argc, char **argv)
{
	const char	optstring[] = ":csf8ia:";
	const char	*aflag = 0;
	int	i;

	while ((i = getopt(argc, argv, optstring)) != EOF) {
		switch (i) {
		case 'a':
			aflag = optarg;
			break;
		case '?':
		case ':':
			fprintf(stderr, " %s: unknown option '-%c'\n",
					progname, i);
			tape_usage();
		}
	}
	if (optind == argc)
		tape_usage();
	if (eq(argv[optind], "status")) {
		verbose = 1;
		omode |= O_NONBLOCK;
	} else if (eq(argv[optind], "reten"))
		operations |= OP_RETENSION;
	else if (eq(argv[optind], "erase")) {
		operations |= OP_ERASE;
		omode = O_RDWR | omode&O_NONBLOCK;
	} else if (eq(argv[optind], "reset")) {
		operations |= OP_RESET;
		omode |= O_NONBLOCK;
	} else if (eq(argv[optind], "rewind"))
		operations |= OP_REWIND;
	else if (eq(argv[optind], "rfm")) {
		operations |= OP_POSITION;
		count = aflag ? atoi(aflag) : 1;
	} else if (eq(argv[optind], "eod")) {
		operations |= OP_ENDDATA;
	} else if (eq(argv[optind], "wfm")) {
		operations |= OP_WRITEMARK;
		omode = O_RDWR | omode&O_NONBLOCK;
		count = aflag ? atoi(aflag) : 1;
	} else if (eq(argv[optind], "load")) {
		operations |= OP_LOAD;
		omode |= O_NONBLOCK;
	} else if (eq(argv[optind], "unload"))
		operations |= OP_UNLOAD;
	else if (eq(argv[optind], "setblk")) {
		operations |= OP_FIXED;
		count = aflag ? atoi(aflag) : 0;
	} else if (eq(argv[optind], "setcomp")) {
		operations |= OP_COMPRESSION;
		count = aflag ? atoi(aflag) : 1;
	} else if (eq(argv[optind], "setdensity")) {
		operations |= OP_DENSITY;
		if (aflag && aflag[0] == '0' &&
				(aflag[1] == 'x' || aflag[1] == 'X'))
			aflag += 2;
		count = aflag ? strtol(aflag, NULL, 16) : 0;
	} else {
		fprintf(stderr, " %s: invalid command '%s'\n",
				progname, argv[optind]);
		tape_usage();
	}
	if (optind < argc-1)
		tape = argv[optind+1];
	else
		tape = tape_default_device();
}

static void
t_options(int argc, char **argv)
{
	const char	optstring[] = "etrwp:alud:f:c:v";
	int	i;

	while ((i = getopt(argc, argv, optstring)) != EOF) {
		switch (i) {
		case 'e':
			operations |= OP_ERASE;
			omode = O_RDWR | omode&O_NONBLOCK;
			break;
		case 't':
			operations |= OP_RETENSION;
			break;
		case 'r':
			operations |= OP_RESET;
			omode |= O_NONBLOCK;
			break;
		case 'w':
			operations |= OP_REWIND;
			break;
		case 'p':
			operations |= OP_POSITION;
			count = atoi(optarg);
			break;
		case 'a':
			operations |= OP_ENDDATA;
			break;
		case 'l':
			operations |= OP_LOAD;
			omode |= O_NONBLOCK;
			break;
		case 'u':
			operations |= OP_UNLOAD;
			break;
		case 'd':
			operations |= OP_DENSITY;
			count = atoi(optarg);
			break;
		case 'f':
			operations |= OP_FIXED;
			count = atoi(optarg);
			break;
		case 'v':
			operations |= OP_FIXED;
			count = 0;
			break;
		case 'c':
			operations |= OP_COMPRESSION;
			count = atoi(optarg);
			break;
		default:
		usage:	fprintf(stderr, "Usage: %s [ -etrw ] [ -p arg ]\n",
					progname);
			exit(1);
		}
	}
	if (operations == OP_NONE)
		goto usage;
	if (optind < argc)
		tape = argv[optind];
	else if ((tape = getenv("TAPE")) != NULL)
		tape = strdup(tape);
	else
		tape = deftape;
}

#if defined (__linux__)
static int
process(int fd, enum oper op)
{
	struct mtop	m;

	switch (op) {
	default:
		errno = ENOSYS;
		return -1;
	case OP_RESET:
		m.mt_op = MTRESET;
		break;
	case OP_RETENSION:
		m.mt_op = MTRETEN;
		break;
	case OP_REWIND:
		m.mt_op = MTREW;
		break;
	case OP_POSITION:
		m.mt_op = MTFSF;
		m.mt_count = count;
		break;
	case OP_BACKWARD:
		m.mt_op = MTBSF;
		m.mt_count = count;
		break;
	case OP_FWBLOCKS:
		m.mt_op = MTFSR;
		m.mt_count = count;
		break;
	case OP_BWBLOCKS:
		m.mt_op = MTBSR;
		m.mt_count = count;
		break;
	case OP_ERASE:
		m.mt_op = MTERASE;
		break;
	case OP_WRITEMARK:
		m.mt_op = MTWEOF;
		m.mt_count = count;
		break;
	case OP_ENDDATA:
		m.mt_op = MTEOM;
		break;
	case OP_OFFLINE:
		m.mt_op = MTOFFL;
		break;
	case OP_LOAD:
		m.mt_op = MTLOAD;
		break;
	case OP_UNLOAD:
		m.mt_op = MTUNLOAD;
		break;
	case OP_DENSITY:
		m.mt_op = MTSETDENSITY;
		m.mt_count = count;
		break;
	case OP_FIXED:
		m.mt_op = MTSETBLK;
		m.mt_count = count;
		break;
	case OP_COMPRESSION:
		m.mt_op = MTCOMPRESSION;
		m.mt_count = count;
		break;
	}
	return ioctl(fd, MTIOCTOP, &m);
}

static int
stats(int fd)
{
	struct mtget	mg;
	static struct {
		int	type;
		const char	*name;
	} ti[] = MT_TAPE_INFO;
	int	i;

	if (ioctl(fd, MTIOCGET, &mg) < 0)
		return -1;
	for (i = 0; ti[i].name; i++)
		if (ti[i].type == mg.mt_type) {
			stats_info = ti[i].name;
			break;
		}
	stats_flags_ok = 1;
	stats_flags = mg.mt_gstat;
	stats_density = (mg.mt_dsreg&MT_ST_DENSITY_MASK)>>MT_ST_DENSITY_SHIFT;
	stats_recsiz = (mg.mt_dsreg&MT_ST_BLKSIZE_MASK) >> MT_ST_BLKSIZE_SHIFT;
	return 0;
}
#elif defined (__sun)
static int
process(int fd, enum oper op)
{
	struct mtop	m;

	switch (op) {
	default:
		errno = ENOSYS;
		return -1;
	case OP_RETENSION:
		m.mt_op = MTRETEN;
		break;
	case OP_REWIND:
		m.mt_op = MTREW;
		break;
	case OP_POSITION:
		m.mt_op = MTFSF;
		m.mt_count = count;
		break;
	case OP_BACKWARD:
		m.mt_op = MTBSF;
		m.mt_count = count;
		break;
	case OP_FWBLOCKS:
		m.mt_op = MTFSR;
		m.mt_count = count;
		break;
	case OP_BWBLOCKS:
		m.mt_op = MTBSR;
		m.mt_count = count;
		break;
	case OP_WRITEMARK:
		m.mt_op = MTWEOF;
		m.mt_count = count;
		break;
	case OP_ENDDATA:
		m.mt_op = MTEOM;
		break;
	case OP_OFFLINE:
		m.mt_op = MTOFFL;
		break;
	case OP_LOAD:
		m.mt_op = MTLOAD;
		break;
	case OP_FIXED:
		m.mt_op = MTSRSZ;
		m.mt_count = count;
		break;
	case OP_ERASE:
		m.mt_op = MTERASE;
		break;
	}
	return ioctl(fd, MTIOCTOP, &m);
}

static int
stats(int fd)
{
	struct mtdrivetype_request	mr;
	static struct mtdrivetype	md;

	mr.size = sizeof md;
	mr.mtdtp = &md;
	if (ioctl(fd, MTIOCGETDRIVETYPE, &mr) < 0)
		return -1;
	stats_info = md.name;
	stats_recsiz = md.bsize;
	stats_flags_ok = 1;
	stats_flags = md.options;
	stats_density = md.default_density;
}
#elif defined (__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) \
	|| defined (__DragonFly__) || defined (__APPLE__)
static int
process(int fd, enum oper op)
{
	struct mtop	m;

	switch (op) {
	default:
		errno = ENOSYS;
		return -1;
#ifdef	MTRETENS
	case OP_RETENSION:
		m.mt_op = MTRETENS;
		break;
#endif
	case OP_REWIND:
		m.mt_op = MTREW;
		break;
	case OP_POSITION:
		m.mt_op = MTFSF;
		m.mt_count = count;
		break;
	case OP_BACKWARD:
		m.mt_op = MTBSF;
		m.mt_count = count;
		break;
	case OP_FWBLOCKS:
		m.mt_op = MTFSR;
		m.mt_count = count;
		break;
	case OP_BWBLOCKS:
		m.mt_op = MTBSR;
		m.mt_count = count;
		break;
	case OP_ERASE:
		m.mt_op = MTERASE;
		break;
	case OP_WRITEMARK:
		m.mt_op = MTWEOF;
		m.mt_count = count;
		break;
#ifdef	MTEOD
	case OP_ENDDATA:
		m.mt_op = MTEOD;
		break;
#endif
	case OP_OFFLINE:
		m.mt_op = MTOFFL;
		break;
	case OP_DENSITY:
		m.mt_op = MTSETDNSTY;
		m.mt_count = count;
		break;
		break;
	case OP_FIXED:
		m.mt_op = MTSETBSIZ;
		m.mt_count = count;
		break;
#ifdef	MTCOMP
	case OP_COMPRESSION:
		m.mt_op = MTCOMP;
		m.mt_count = count;
		break;
#endif
	}
	return ioctl(fd, MTIOCTOP, &m);
}

static int
stats(int fd)
{
	struct mtget	mg;

	if (ioctl(fd, MTIOCGET, &mg) < 0)
		return -1;
	stats_flags_ok = 1;
	stats_flags = mg.mt_dsreg;
	stats_density = mg.mt_density;
	stats_recsiz = mg.mt_blksiz;
	return 0;
}
#elif defined (__hpux) || defined (_AIX)
static int
process(int fd, enum oper op)
{
	struct mtop	m;

	switch (op) {
	default:
		errno = ENOSYS;
		return -1;
	case OP_REWIND:
		m.mt_op = MTREW;
		break;
	case OP_POSITION:
		m.mt_op = MTFSF;
		m.mt_count = count;
		break;
	case OP_BACKWARD:
		m.mt_op = MTBSF;
		m.mt_count = count;
		break;
	case OP_FWBLOCKS:
		m.mt_op = MTFSR;
		m.mt_count = count;
		break;
	case OP_BWBLOCKS:
		m.mt_op = MTBSR;
		m.mt_count = count;
		break;
	case OP_ERASE:
#ifdef	MTERASE
		m.mt_op = MTERASE;
#endif
#ifdef	STERASE
		m.mt_op = STERASE;
#endif
		break;
	case OP_WRITEMARK:
		m.mt_op = MTWEOF;
		m.mt_count = count;
		break;
#ifdef	MTEOD
	case OP_ENDDATA:
		m.mt_op = MTEOD;
		break;
#endif
	case OP_OFFLINE:
		m.mt_op = MTOFFL;
		break;
	}
	return ioctl(fd, MTIOCTOP, &m);
}

static int
stats(int fd)
{
	struct mtget	mg;

	if (ioctl(fd, MTIOCGET, &mg) < 0)
		return -1;
	stats_flags_ok = 1;
	return 0;
}
#else	/* SVR4.2MP */
static int
process(int fd, enum oper op)
{
	switch (op) {
	default:
		errno = ENOSYS;
		return -1;
	case OP_RESET:
		return ioctl(fd, T_RST);
	case OP_RETENSION:
		return ioctl(fd, T_RETENSION);
	case OP_REWIND:
		return ioctl(fd, T_RWD);
	case OP_POSITION:
		return ioctl(fd, T_SFF, count);
	case OP_BACKWARD:
		return ioctl(fd, T_SFB, count);
	case OP_FWBLOCKS:
		return ioctl(fd, T_SBF, count);
	case OP_BWBLOCKS:
		return ioctl(fd, T_SBB, count);
	case OP_WRITEMARK:
		return ioctl(fd, T_WRFILEM, 1);
	case OP_ENDDATA:
		return ioctl(fd, T_EOD);
	case OP_LOAD:
		return ioctl(fd, T_LOAD);
	case OP_UNLOAD:
		return ioctl(fd, T_UNLOAD);
	case OP_DENSITY:
		return ioctl(fd, T_STD, count);
	case OP_FIXED: {
		struct blklen	bl;
		bl.min_blen = bl.max_blen = count;
		return ioctl(fd, T_WRBLKLEN, &bl);
	}
	case OP_ERASE:
		return ioctl(fd, T_ERASE);
	case OP_COMPRESSION:
		return ioctl(fd, T_SETCOMP, count);
	}
}

static int
stats(int fd)
{
	return -1;
}
#endif	/* SVR4.2MP */

int
main(int argc, char **argv)
{
	int	fd;
	int	i;

	progname = basename(argv[0]);
	if (progname[0] == 'm' && progname[1] == 't') {
		mt = 1;
		m_options(argc, argv);
	} else if (strstr(progname, "cntl") == NULL) {
		tape_options(argc, argv);
	} else
		t_options(argc, argv);
	if (mt) {
		struct stat	st;

		if (stat(tape, &st) < 0) {
			fprintf(stderr, "%s: could not find %s\n"
					"%s: %s\n",
					progname, tape,
					progname, strerror(errno));
			exit(1);
		} else if ((st.st_mode&S_IFMT) != S_IFCHR) {
			fprintf(stderr, "%s: %s is not a character device.\n",
					progname, tape);
			exit(1);
		}
	}
	if ((fd = open(tape, omode)) < 0) {
		if (mt) {
			status |= 1;
			fprintf(stderr, "%s: could not open %s\n"
					"%s: %s\n",
					progname, tape,
					progname, strerror(errno));
		} else if (operations & OP_ERASE && errno == EROFS) {
			status |= 3;
			fprintf(stderr, "Write protected cartridge.\n");
		} else {
			status |= errno == EBUSY ? 4 : 1;
			fprintf(stderr, "Device open failure.\n");
			fprintf(stderr, "Please check that cartridge or device "
					"cables are inserted properly.\n");
		}
		exit(status);
	}
	if (verbose && stats(fd) == 0) {
		if (stats_info != NULL)
			printf("Found %s:\n", stats_info);
		if (stats_flags_ok)
			printf("flags   = 0x%lx\n", stats_flags);
		if (stats_density >= 0)
			printf(mt ? "density = %ld\n" : "density = 0x%lx\n",
					stats_density);
		if (stats_recsiz >= 0)
			printf("recsiz  = %ld\n", stats_recsiz);
	}
	if (operations == OP_NONE)
		exit(0);
	for (i = 1; i <= 0177777; i <<= 1) {
		if (operations & i && process(fd, i) < 0) {
			const char	*msg = "";

			switch (i) {
			case OP_RESET:
				msg = "Reset function failure.";
				break;
			case OP_RETENSION:
				msg = "Retension function failure.\n"
					"Please check equipment carefully "
					"before retry.";
				break;
			case OP_REWIND:
				msg = "Rewind function failure.";
				break;
			case OP_ENDDATA:
			case OP_POSITION:
			case OP_BACKWARD:
			case OP_FWBLOCKS:
			case OP_BWBLOCKS:
				msg = "Positioning function failure";
				break;
			case OP_ERASE:
				if (errno == EACCES)
					msg = "Erase function failure.\n"
						"Write protected cartridge.";
				else
					msg = "Erase function failure.\n"
						"Please check that cartridge "
						"or device cables are "
						"inserted properly";
			case OP_OFFLINE:
				msg = "Offline function failure.";
				break;
			case OP_UNLOAD:
				msg = "Unload function failure.";
				break;
			case OP_LOAD:
				msg = "Load function failure.";
				break;
			case OP_DENSITY:
				msg = "Set tape density function failure.";
				break;
			case OP_FIXED:
				msg = "Set block length function failure.";
				break;
			case OP_WRITEMARK:
				msg = "Write mark function failure.";
			default:
				msg = strerror(errno);
			}
			fprintf(stderr, "%s\n", msg);
			status |= errno == EACCES ? 3 : 2;
		}
	}
	return status;
}
