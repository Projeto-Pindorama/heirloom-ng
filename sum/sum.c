/*
 * sum - print checksum and block count of a file
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, January 2003.
 *
 * Parts taken from /usr/src/cmd/sum.c, Unix 7th Edition:
 *
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
