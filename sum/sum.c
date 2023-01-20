/*
 * sum - print checksum and block count of a file
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, January 2003.
 *
 * Parts taken from /usr/src/cmd/sum.c, Unix 7th Edition:
 *
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * SPDX-Licence-Identifier: Caldera
 */

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
#ifdef	UCB
static const char sccsid[] USED = "@(#)/usr/ucb/sum.sl	1.9 (gritter) 5/29/05";
#else
static const char sccsid[] USED = "@(#)sum.sl	1.9 (gritter) 5/29/05";
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <inttypes.h>
#include <libgen.h>

#ifdef	UCB
#define	UNIT	1024
#else
#define	UNIT	512
#endif

static char	*progname;
static int	rflag;

static int
sum(const char *name)
{
	uint16_t	sum;
	uint32_t	s;
	int	err = 0;
	int	fd;
	ssize_t	sz, i;
	char	buf[4096];
	unsigned long long	nbytes;

	if (name) {
		if ((fd = open(name, O_RDONLY)) < 0) {
			fprintf(stderr, "%s: Can't open %s\n", progname, name);
			return 10;
		}
	} else
		fd = 0;
	sum = 0;
	nbytes = 0;
	s = 0;
	while ((sz = read(fd, buf, sizeof buf)) > 0) {
		nbytes += sz;
		if (rflag) {
			for (i = 0; i < sz; i++) {
				if (sum&01)
					sum = (sum>>1) + 0x8000;
				else
					sum >>= 1;
				sum += buf[i] & 0377;
				sum &= 0xFFFF;
			}
		} else {
			for (i = 0; i < sz; i++)
				s += buf[i] & 0377;
		}
	}
	if (sz < 0) {
		fprintf(stderr, "%s: read error on %s\n", progname,
				name ? name : "-");
		err = 1;
	}
	if (rflag)
		printf("%.5u %5llu", (unsigned)sum,
				(unsigned long long)(nbytes+UNIT-1)/UNIT);
	else {
		s = (s & 0xFFFF) + (s >> 16);
		s = (s & 0xFFFF) + (s >> 16);
		printf("%u %llu", (unsigned)s,
				(unsigned long long)(nbytes+UNIT-1)/UNIT);
	}
	if(name)
		printf(" %s", name);
	printf("\n");
	if (fd > 0)
		close(fd);
	return err;
}

int
main(int argc, char **argv)
{
	int	i, errflg = 0;

	progname = basename(argv[0]);
	i = 1;
#ifdef	UCB
	rflag = 1;
#endif
	while (i < argc && argv[i][0] == '-' && argv[i][1]) {
#ifndef	UCB
	nxt:
#endif
		switch (argv[i][1]) {
#ifndef	UCB
		case 'r':
			rflag = 1;
			if (argv[i][2] == 'r') {
				(argv[i])++;
				goto nxt;
			} else
				i++;
			break;
#endif
		case '-':
			if (argv[i][2] == '\0')
				i++;
			/*FALLTHRU*/
		default:
			goto opnd;
		}
	}
opnd:	if (argc > i) {
		do
			errflg |= sum(argv[i]);
		while (++i < argc);
	} else
		errflg |= sum(NULL);
	return errflg;
}
