/*
 * renice - set system scheduling priorities of running processes
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, August 2002.
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
static const char sccsid[] USED = "@(#)renice.sl	1.7 (gritter) 5/29/05";

#include	<sys/time.h>
#include	<sys/resource.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>
#include	<pwd.h>

#ifndef	PRIO_MIN
#define	PRIO_MIN	-20
#endif
#ifndef	PRIO_MAX
#define	PRIO_MAX	20
#endif

unsigned	errcnt;			/* count of errors */
char		*progname;		/* argv[0] to main() */
int		selection;		/* one of '\0' 'g' 'p' 'i' */

int	traditional_donice(int which, int who, int prio);
int	traditional_renice(int argc, char **argv);

void
usage(void)
{
	fprintf(stderr, "\
usage: %s priority [ [ -p ] pids ] [ [ -g ] pgrps ] [ [ -u ] users ]\n\
       %s [ -n increment ] [-g | -p | -u ] ID\n",
			progname, progname);
	exit(2);
}

int
main(int argc, char **argv)
{
	struct passwd	*pwd;
	char	*x;
	int	i, which = 0, who = 0, oldval;
	int	increment = 10;

#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	progname = basename(argv[0]);
	if (argc > 1) {
		i = strtol(argv[1], &x, 10);
		if (*x == '\0' && i >= -20 && i <= 20)
			return traditional_renice(argc, argv);
	}
	while ((i = getopt(argc, argv, "gn:pu")) != EOF) {
		switch (i) {
		case 'g':
		case 'p':
		case 'u':
			if (selection != '\0')
				usage();
			selection = i;
			break;
		case 'n':
			increment = strtol(optarg, &x, 10);
			if (*x != '\0')
				usage();
			break;
		default:
			usage();
		}
	}
	if (optind >= argc)
		usage();
	for ( ; optind < argc; optind++) {
		switch (selection) {
		case '\0':
		case 'g':
		case 'p':
			which = (selection == 'g' ? PRIO_PGRP : PRIO_PROCESS);
			who = strtol(argv[optind], &x, 10);
			if (*x != '\0') {
				fprintf(stderr, "%s: bad value: %s\n",
						progname, argv[optind]);
				errcnt |= 1;
				continue;
			}
			break;
		case 'u':
			which = PRIO_USER;
			if ((pwd = getpwnam(argv[optind])) != NULL) {
				who = pwd->pw_uid;
			} else {
				who = strtol(argv[optind], &x, 10);
				if (*x != '\0') {
					fprintf(stderr,
						"%s: unknown user: %s\n",
						progname, argv[optind]);
					continue;
				}
			}
			break;
		}
		errno = 0;
		if ((oldval = getpriority(which, who)) == -1 && errno != 0) {
			fprintf(stderr, "%s: %d:getpriority: %s\n", progname,
					who, strerror(errno));
			errcnt |= 1;
			continue;
		}
		if (setpriority(which, who, oldval + increment) < 0) {
			fprintf(stderr, "%s: %d:setpriority: %s\n", progname,
					who, strerror(errno));
			errcnt |= 1;
			continue;
		}
	}
	return errcnt;
}

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
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
 *
 * from 4.3BSD Tahoe renice.c	5.1 (Berkeley) 5/28/85
 */

/*
 * Change the priority (nice) of processes
 * or groups of processes which are already
 * running.
 */
int
traditional_renice(int argc, char **argv)
{
	int	which = PRIO_PROCESS;
	int	who = 0, prio, errs = 0;

	argc--, argv++;
	if (argc < 2)
		usage();
	prio = atoi(*argv);
	argc--, argv++;
	if (prio > PRIO_MAX)
		prio = PRIO_MAX;
	if (prio < PRIO_MIN)
		prio = PRIO_MIN;
	for (; argc > 0; argc--, argv++) {
		if (strcmp(*argv, "-g") == 0) {
			which = PRIO_PGRP;
			continue;
		}
		if (strcmp(*argv, "-u") == 0) {
			which = PRIO_USER;
			continue;
		}
		if (strcmp(*argv, "-p") == 0) {
			which = PRIO_PROCESS;
			continue;
		}
		if (which == PRIO_USER) {
			register struct passwd	*pwd = getpwnam(*argv);
			
			if (pwd == NULL) {
				fprintf(stderr, "renice: %s: unknown user\n",
					*argv);
				continue;
			}
			who = pwd->pw_uid;
		} else {
			who = atoi(*argv);
			if (who < 0) {
				fprintf(stderr, "renice: %s: bad value\n",
					*argv);
				continue;
			}
		}
		errs += traditional_donice(which, who, prio);
	}
	return errs != 0;
}

int
traditional_donice(int which, int who, int prio)
{
	int	oldprio;

	errno = 0, oldprio = getpriority(which, who);
	if (oldprio == -1 && errno) {
		fprintf(stderr, "renice: %d: ", who);
		perror("getpriority");
		return 1;
	}
	if (setpriority(which, who, prio) < 0) {
		fprintf(stderr, "renice: %d: ", who);
		perror("setpriority");
		return 1;
	}
	printf("%d: old priority %d, new priority %d\n", who, oldprio, prio);
	return 0;
}
