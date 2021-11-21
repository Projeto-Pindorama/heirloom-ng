/*
 * psrinfo - displays information about processors
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, June 2001.
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

#ifdef	__linux__

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)psrinfo.sl	1.16 (gritter) 2/15/07";

#include	<sys/utsname.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>
#include	<time.h>
#include	<ctype.h>

struct cpu {
	time_t		c_now;		/* time of measurement */
	time_t		c_since;	/* on-line time */
	unsigned	c_MHz;		/* frequency in MHz */
	int		c_nofpu;	/* lacks an fpu */
};

unsigned	errcnt;			/* count of errors */
int		nflag;			/* display number of processors */
int		sflag;			/* be silent */
int		vflag;			/* be verbose */
char		*progname;		/* argv[0] to main() */
int		ncpu = -1;		/* number of cpus (starting at zero */
long		hz;			/* clock tick */
struct cpu	**chain;		/* chain of cpus */
struct utsname	un;			/* system information */
const char	*machine;		/* processor name */

void
pnerror(int eno, const char *string)
{
	fprintf(stderr, "%s: %s: %s\n", progname, string, strerror(eno));
	errcnt |= 1;
}

void *
srealloc(void *addr, size_t nbytes)
{
	void *p;

	if ((p = (void *)realloc(addr, nbytes)) == NULL) {
		write(2, "Out of memory\n", 14);
		exit(077);
	}
	return p;
}

/*
 * Output.
 */
void
display(const char *id)
{
	char tim[20];
	char *cp, *ep;
	int c, first, last, gotcha = 0;

	if (id) {
		if ((cp = strchr(id, '-')) != NULL) {
			if (sflag) {
				fprintf(stderr,
			"%s: must specify exactly one processor if -s used\n",
					progname);
				errcnt |= 1;
				return;
			}
			first = (int)strtol(id, &ep, 10);
			if (ep != cp) {
				fprintf(stderr,
					"%s: invalid processor range %s\n",
					progname, id);
				errcnt |= 1;
				return;
			}
			last = (int)strtol(&cp[1], &ep, 10);
			if ((ep != NULL && *ep != '\0') || last < first
					|| first < 0) {
				fprintf(stderr,
					"%s: invalid processor range %s\n",
					progname, id);
				errcnt |= 1;
				return;
			}
		} else {
			first = last = (int)strtol(id, &ep, 10);
			if ((ep && *ep != '\0') || first < 0) {
				fprintf(stderr,
					"%s: invalid processor range %s\n",
					progname, id);
				errcnt |= 1;
				return;
			}
		}
	} else {
		first = 0;
		last = ncpu;
	}
	for (c = first; c <= ncpu && c <= last; c++) {
		gotcha++;
		if (sflag) {
			/*EMPTY*/;
		} else if (vflag) {
			strftime(tim, sizeof tim, "%D %T",
					localtime(&chain[c]->c_now));
			printf("Status of processor %u as of %s\n", c, tim);
			strftime(tim, sizeof tim, "%D %T",
					localtime(&chain[c]->c_since));
			printf("  Processor has been on-line since %s.\n", tim);
			if (chain[c]->c_MHz)
			printf("  The %s processor operates at %u MHz,\n",
				machine, chain[c]->c_MHz);
			else
		printf("  The %s processor operates at an unknown frequency,\n",
				machine);
			if (chain[c]->c_nofpu)
		printf("        and has no floating point processor.\n\n");
			else
		printf("        and has a%s floating point processor.\n\n",
	un.machine[0] == 'i' && un.machine[1] && un.machine[2] == '8'
	&& un.machine[3] == '6' && un.machine[4] == '\0' ? "n i387 compatible"
		: "");
		} else {
			strftime(tim, sizeof tim, "%D %T",
					localtime(&chain[c]->c_since));
			printf("%u\t%-8s  since %s\n", c, "on-line", tim);
		}
	}
	if (gotcha == 0 && sflag == 0) {
		fprintf(stderr, "%s: no processors in range %s\n",
				progname, id);
		errcnt |= 1;
	}
}

static char *
pair(char *line)
{
	int	i;
	char	*value;

	for (i = 0; line[i]; i++)
		if (line[i] == ':') {
			value = &line[i+1];
			while (isblank(*value & 0377))
				value++;
			line[i--] = '\0';
			while (isblank(line[i] & 0377))
				line[i--] = '\0';
			return value;
		}
	return NULL;
}

/*
 * Get frequencies.
 */
void
freqs(void)
{
	char buf[2048];
	FILE *fp;
	char	*value;
	char *cp;
	float freq;
	int c = -1;
	int intel = 0, family = -1;

	if ((fp = fopen("/proc/cpuinfo", "r")) == NULL)
		return;
	while (fgets(buf, sizeof buf, fp) != NULL) {
		if ((cp = strchr(buf, '\n')) != NULL)
			cp[0] = '\0';
		value = pair(buf);
#ifndef	__R5900
		if (strcmp(buf, "processor") == 0) {
#else	/* __R5900 */
		if (strcmp(buf, "cpu") == 0) {
#endif	/* __R5900 */
			c = atoi(value);
			if (c < 0 || c > ncpu)
				c = -1;
		} else if (strcmp(buf, "cpu MHz") == 0) {
			freq = atof(value);
			if (c != -1) {
				chain[c]->c_MHz = (int)freq;
				if (freq - chain[c]->c_MHz >= .5)
					chain[c]->c_MHz++;
			}
		} else if (strcmp(buf, "fpu") == 0) {
			if (strcmp(value, "no") == 0 && c != -1)
				chain[c]->c_nofpu = 1;
		} else if (strcmp(buf, "vendor_id") == 0) {
			if (strcmp(value, "GenuineIntel") == 0)
				intel = 1;
		} else if (strcmp(buf, "cpu family") == 0) {
			family = atoi(value);
		} else if (strcmp(buf, "family") == 0) {
			machine = strdup(value);
		}
	}
	if (intel && family == 15)
		machine = "Pentium_4";
	fclose(fp);
}

/*
 * Read cpu time statistics.
 */
#ifndef	__R5900
void
stats(void)
{
	char line[512], *lp;
	FILE *fp;
	unsigned long long j, t;
	time_t now;

	if ((fp = fopen("/proc/stat", "r")) == NULL) {
		pnerror(errno, "cannot open /proc/stat");
		errcnt |= 1;
		return;
	}
	time(&now);
	while (fgets(line, sizeof line, fp) != NULL) {
		if (strncmp(line, "cpu", 3) || !isdigit(line[3]))
			continue;
		t = 0;
		lp = line;
		while (*lp != ' ')
			lp++;
		for (;;) {
			j = strtoull(lp, &lp, 10);
			if (*lp != ' ')
				break;
			t += j;
		}
		ncpu++;
		chain = srealloc(chain, sizeof *chain * (ncpu + 1));
		chain[ncpu] = srealloc(NULL, sizeof **chain);
		memset(chain[ncpu], 0, sizeof **chain);
		chain[ncpu]->c_now = now;
		chain[ncpu]->c_since = now - t / hz;
	}
	fclose(fp);
}
#else	/* __R5900 */
/*
 * No /proc/stats on PS2.
 */
void
stats(void)
{
	FILE	*fp;
	time_t	now;
	float	up, idle;

	if ((fp = fopen("/proc/uptime", "r")) == NULL) {
		pnerror(errno, "cannot open /proc/uptime");
		errcnt |= 1;
		return;
	}
	if (fscanf(fp, "%f %f", &up, &idle) != 2) {
		fprintf(stderr, "%s: unexpected format of /proc/uptime\n",
				progname);
		errcnt |= 1;
		return;
	}
	fclose(fp);
	time(&now);
	ncpu = 0;
	chain = srealloc(chain, sizeof *chain * (ncpu + 1));
	chain[ncpu] = srealloc(NULL, sizeof **chain);
	memset(chain[ncpu], 0, sizeof **chain);
	chain[ncpu]->c_now = now;
	chain[ncpu]->c_since = now - up;
}
#endif	/* __R5900 */

void
usage(void)
{
	fprintf(stderr,
		"usage: \t%s -v [ processor_id  [... ] ]\n\
\t%s -s  processor_id\n\
\t%s -n\n",
		progname, progname, progname);
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
	if (argc == 0)
		usage();
	while ((i = getopt(argc, argv, "nsv")) != EOF) {
		switch (i) {
		case 'n':
			nflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 'v':
			vflag = 1;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (sflag) {
		if (vflag) {
			fprintf(stderr,
		"%s: options -s and -v are mutually exclusive\n",
				progname);
			exit(2);
		}
		if (argc != 1) {
			fprintf(stderr,
		"%s: must specify exactly one processor if -s used\n",
				progname);
			exit(2);
		}
	}
	if (nflag)
		if (sflag || vflag || argv[0])
			usage();
	hz = sysconf(_SC_CLK_TCK);
	uname(&un);
	if (strcmp(un.machine, "i586") == 0)
		machine = "Pentium";
	else if (strcmp(un.machine, "i686") == 0)
		machine = "Pentium_Pro";
	else
		machine = un.machine;
	stats();
	if (vflag)
		freqs();
	if (ncpu >= 0) {
		if (nflag) {
			printf("%u\n", ncpu + 1);
		} else {
			if (argv[0]) {
				while (argv[0]) {
					display(argv[0]);
					argv++;
				}
			} else
				display(NULL);
		}
	}
	return errcnt;
}

#else	/* !__linux__ */

#include	<unistd.h>

int
main(int argc, char **argv)
{
	execv("/usr/sbin/psrinfo", argv);
	write(2, "psrinfo not available\n", 22);
	_exit(0177);
}

#endif	/* !__linux__ */
