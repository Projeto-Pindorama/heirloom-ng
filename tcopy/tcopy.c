/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
/*
 * This code contains changes by
 * Gunnar Ritter, Freiburg i. Br., Germany, October 2003. All rights reserved.
 *
 * Conditions 1, 2, and 4 and the no-warranty notice below apply
 * to these changes.
 *
 *
 * Copyright (c) 1991
 * 	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 	This product includes software developed by the University of
 * 	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*	from 4.3BSD tcopy.c	1.2 (Berkeley) 12/11/85	*/
#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)tcopy.sl	1.16 (gritter) 1/22/06";

#include <stdio.h>
#include <signal.h>
#include "sigset.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/ioctl.h>
#if defined (__linux__) || defined (__sun) || defined (__FreeBSD__) || \
	defined (__hpux) || defined (_AIX) || defined (__NetBSD__) || \
	defined (__OpenBSD__) || defined (__DragonFly__) || defined (__APPLE__)
#include <sys/mtio.h>
#else	/* SVR4.2MP */
#include <sys/tape.h>
#endif	/* SVR4.2MP */

#define SIZE	(256 * 1024)

static char buff[SIZE];
static int filen=1;
static long long count, lcount;
static int nfile;
static long long size, tsize;
static int ln;
static char *inf, *outf;
static int copy;
static const char *progname;
static int status;

static void	RUBOUT(int);

int
main(int argc, char **argv)
{
	register int n, nw, inp, outp = -1;

	progname = basename(argv[0]);
	if (argc <=1 || argc > 3) {
		fprintf(stderr, "Usage: %s src [dest]\n", progname);
		exit(1);
	}
	inf = argv[1];
	if (argc == 3) {
		outf = argv[2];
		copy = 1;
	}
	if ((inp=open(inf, O_RDONLY, 0666)) < 0) {
		fprintf(stderr,"Can't open %s\n", inf);
		exit(1);
	}
	if (copy) {
		if ((outp=open(outf, O_WRONLY, 0666)) < 0) {
			fprintf(stderr,"Can't open %s\n", outf);
			exit(3);
		}
	}
	if (sigset(SIGINT, SIG_IGN) != SIG_IGN)
		(void) sigset(SIGINT, RUBOUT);
	ln = -2;
	for (;;) {
		count++;
		n = read(inp, buff, SIZE);
		if (n > 0) {
		    if (copy) {
			    nw = write(outp, buff, n);
			    if (nw != n) {
				fprintf(stderr, "write (%d) != read (%d)\n",
					nw, n);
				fprintf(stderr, "COPY Aborted\n");
				exit(5);
			    }
		    }
		    size += n;
		    if (n != ln) {
			if (ln > 0)
			    if (count - lcount > 1)
				printf("file %d: records %lld to %lld: "
						"size %d\n",
					filen, lcount, count-1, ln);
			    else
				printf("file %d: record %lld: size %d\n",
					filen, lcount, ln);
			ln = n;
			lcount = count;
		    }
		}
		else if (n < 0) {
			fprintf(stderr, "file %d: read error "
				"after %lld records: %lld bytes\n",
				filen, count-1, size);
			status = 4;
		}
		else {
			if (ln <= 0 && ln != -2) {
				printf("eot\n");
				break;
			}
			if (ln > 0)
			    if (count - lcount > 1)
				printf("file %d: records %lld to %lld: "
						"size %d\n",
					filen, lcount, count-1, ln);
			    else
				printf("file %d: record %lld: size %d\n",
					filen, lcount, ln);
			printf("file %d: eof after %lld records: %lld bytes\n",
				filen, count-1, size);
			if (copy) {
#if defined (__linux__) || defined (__sun) || defined (__FreeBSD__) || \
	defined (__hpux) || defined (_AIX) || defined (__NetBSD__) || \
	defined (__OpenBSD__) || defined (__DragonFly__) || defined (__APPLE__)
				struct mtop op;
				op.mt_op = MTWEOF;
				op.mt_count = 1;
				if(ioctl(outp, MTIOCTOP, &op) < 0)
#else	/* SVR4.2MP */
				if (ioctl(outp, T_WRFILEM, 1) < 0)
#endif	/* SVR4.2MP */
				{
					perror("Write EOF");
					exit(6);
				}
			}
			filen++;
			count = 0;
			lcount = 0;
			tsize += size;
			size = 0;
			if (nfile && filen > nfile)
				break;
			ln = n;
		}
	}
	if (copy)
		(void) close(outp);
	printf("total length: %lld bytes\n", tsize);
	return status;
}

/*ARGSUSED*/
static void
RUBOUT(int signo)
{
	if (count > lcount)
		--count;
	if (count)
		if (count > lcount)
			printf("file %d: records %lld to %lld: size %d\n",
				filen, lcount, count, ln);
		else
			printf("file %d: record %lld: size %d\n",
				filen, lcount, ln);
	printf("rubout at file %d: record %lld\n", filen, count);
	printf("total length: %lld bytes\n", tsize+size);
	exit(1);
}
