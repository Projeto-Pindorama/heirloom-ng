/*
 * wc - word count
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
#if defined (S42)
static const char sccsid[] USED = "@(#)wc_s42.sl	1.42 (gritter) 5/29/05";
#elif defined (SUS)
static const char sccsid[] USED = "@(#)wc_sus.sl	1.42 (gritter) 5/29/05";
#else
static const char sccsid[] USED = "@(#)wc.sl	1.42 (gritter) 5/29/05";
#endif

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<wchar.h>
#include	<locale.h>
#include	<ctype.h>
#include	<wctype.h>
#include	<unistd.h>
#include	<libgen.h>
#include	<limits.h>

#include	<iblok.h>
#include	<blank.h>
#include	<mbtowi.h>

static unsigned		errcnt;		/* count of errors */
static int		cflag;		/* count bytes only */
static int		lflag;		/* count lines only */
static int		mflag;		/* count characters only */
static int		wflag;		/* count words only */
static int		illflag;	/* illegal flag given */
static long long	bytec;		/* byte count */
static long long	charc;		/* character count */
static long long	wordc;		/* word count */
static long long	linec;		/* line count */
static long long	chart;		/* total character count */
static long long	bytet;		/* total byte count */
static long long	wordt;		/* total word count */
static long long	linet;		/* total line count */
#if defined (S42)
static int		putspace;	/* wrote space to output line */
#endif
static char		*progname;	/* argv0 to main */
static int		mb_cur_max;	/* MB_CUR_MAX */

static void		usage(void);

/*
 * Format output.
 */
static void
report(unsigned long long count)
{
#if defined (S42)
	if (putspace++)
		printf(" ");
	printf("%llu", count);
#else	/* !S42 */
	printf("%7llu ", count);
#endif	/* !S42 */
}

#ifdef	SUS
#define	COUNT(c) \
			if (isspace(c)) { \
				if ((c) == '\n') \
					linec++; \
				hadspace++; \
			} else { \
				if (hadspace) { \
					hadspace = 0; \
					wordc++; \
				} \
			}
#else	/* !SUS */
#define	COUNT(c) \
			if ((c) == '\n') { \
				linec++; \
				hadspace++; \
			} else if (blank[c]) { \
				hadspace++; \
			} else { \
				if (hadspace && graph[c]) { \
					hadspace = 0; \
					wordc++; \
				} \
			}
#endif	/* !SUS */

static int
sbwc(struct iblok *ip)
{
	register long long hadspace = 1;
	register int c, i;
	size_t sz;
#ifndef	SUS
	/*
	 * If SUS is defined, this optimization brings no measurable
	 * performance gain.
	 */
	int	blank[256], graph[256];

	for (c = 0; c < 256; c++) {
		blank[c] = isblank(c);
		graph[c] = isgraph(c);
	}
#endif	/* !SUS */

	while (ib_read(ip) != EOF) {
		ip->ib_cur--;
		sz = ip->ib_end - ip->ib_cur;
		charc += sz;
		for (i = 0; i < sz; i++) {
			c = ip->ib_cur[i] & 0377;
			COUNT(c)
		}
	}
	bytec = charc;
	return 0;
}

static int
mbwc(struct iblok *ip)
{
	register long long hadspace = 1;
	char	mb[MB_LEN_MAX];
	wint_t	c;
	int	i, k, n;
	size_t sz;
	char	*curp;
	int	eof = 0;
#ifndef	SUS
	/*
	 * If SUS is defined, this optimization brings no measurable
	 * performance gain.
	 */
	int	blank[128], graph[128];

	for (c = 0; c < 128; c++) {
		blank[c] = isblank(c);
		graph[c] = isgraph(c);
	}
#endif	/* !SUS */

	while (eof == 0) {
		if (ip->ib_cur == ip->ib_end) {
			if (ib_read(ip) == EOF)
				break;
			curp = ip->ib_cur - 1;
		} else
			curp = ip->ib_cur;
		ip->ib_cur = ip->ib_end;
		sz = ip->ib_end - curp;
		bytec += sz;
		/*
		 * Incrementing charc here en bloc and decrementing it
		 * later for multibyte characters if necessary is
		 * considerably faster with ASCII files.
		 */
		charc += sz;
		for (i = 0; i < sz; i++) {
			if (curp[i] & 0200) {
				if (sz - i < mb_cur_max) {
					for (k = 0; k < sz - i; k++)
						mb[k] = curp[i+k];
					while (eof == 0 && k < mb_cur_max) {
						if ((c = ib_get(ip)) != EOF) {
							mb[k++] = c;
							bytec++;
							charc++;
						} else
							eof = 1;
					}
					curp = mb;
					sz = k;
					i = 0;
				}
				n = mbtowi(&c, &curp[i], sz - i);
				if (n < 0) {
					charc--;
					continue;
				}
				charc -= n - 1;
				i += n - 1;
#ifdef	SUS
				if (iswspace(c)) {
					if (c == '\n')
						linec++;
					hadspace++;
				} else {
					if (hadspace) {
						hadspace = 0;
						wordc++;
					}
				}
#else	/* !SUS */
				if (c == '\n') {
					linec++;
					hadspace++;
				} else if (iswblank(c)) {
					hadspace++;
				} else {
					if (hadspace && iswgraph(c)) {
						hadspace = 0;
						wordc++;
					}
				}
#endif	/* !SUS */
			} else /* isascii(c) */ {
				c = curp[i];
				COUNT(c)
			}
		}
	}
	return 0;
}

/*
 * Count a file.
 */
static int
fpwc(int fd)
{
	struct iblok	*ip;

	bytec = charc = wordc = linec = 0;
	ip = ib_alloc(fd, 0);
	if ((mb_cur_max > 1 && (wflag || mflag) ? mbwc : sbwc)(ip)) {
		ib_free(ip);
		return -1;
	}
	ib_free(ip);
	if (illflag)
		usage();
#if defined (S42)
	putspace = 0;
#endif
	if (lflag)
		report(linec);
	if (wflag)
		report(wordc);
	if (cflag)
		report(bytec);
	if (mflag)
		report(charc);
	linet += linec;
	wordt += wordc;
	chart += charc;
	bytet += bytec;
	return 0;
}

/*
 * Count a file given by its name.
 */
static int
filewc(const char *fn)
{
	int fd;
	int r;

	if ((fd = open(fn, O_RDONLY)) < 0) {
		fprintf(stderr, "%s: cannot open %s\n", progname, fn);
		errcnt |= 1;
		return 0;
	}
	if ((r = fpwc(fd)) >= 0)
		printf(" %s\n", fn);
	if (fd > 0)
		close(fd);
	return r >= 0 ? 1 : 0;
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-clw] [name ...]\n", progname);
	exit(2);
}

int
main(int argc, char **argv)
{
	int c;
	unsigned ac;

#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	progname = basename(argv[0]);
	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	ac = 0;
	while ((c = getopt(argc, argv, ":clmw")) != EOF) {
		ac++;
		switch (c) {
		case 'c':
			cflag = ac;
			break;
		case 'l':
			lflag = ac;
			break;
		case 'm':
			mflag = ac;
			break;
		case 'w':
			wflag = ac;
			break;
		default:
			illflag = 1;
		}
	}
	if (ac == 0) {
#if !defined (SUS) && !defined(S42)
		if (argv[1] && argv[1][0] == '-' && argv[1][1] == '\0')
			optind++;
		else
#endif	/* !SUS, !S42 */
			cflag = lflag = wflag = 1;
	}
	if (mflag)
		cflag = 0;
	if (optind < argc) {
		ac = 0;
		while (optind < argc)
			ac += filewc(argv[optind++]);
		if (ac > 1) {
#if defined (S42)
			putspace = 0;
#endif
			if (lflag)
				report(linet);
			if (wflag)
				report(wordt);
			if (cflag)
				report(bytet);
			if (mflag)
				report(chart);
			printf(" total\n");
		}
	} else {
		fpwc(0);
		printf("\n");
	}
	return errcnt;
}
