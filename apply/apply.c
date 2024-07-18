/*
 * apply.c - apply a command to a group of arguments
 */
/*
 * Copyright (C) 2024: Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 */

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strmenta.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* main() exit()s, does not return(). */
#pragma clang diagnostic ignored "-Wmain-return-type"

/* Error codes for crargs(). */
#define EOUTRANGE      (SHRT_MIN >> 10)
#define ENOTNO         (SHRT_MIN >> 11)

static char *progname;
static short int mstep = 0;
static char magia = '%',
	    *cmd = "";

void main(int argc, char *argv[]);
short int crargs(char *s);
short int magiac(void);
char *buildcmd(char *arg[], int carg);
int eXec(const char command[]);
void usage(void);

void main(int argc, char *argv[]) {
	progname = argv[0];
	shift(argv, argc);

	unsigned int opt = 0,
		     c = 0,
		     i = 0;
	short int maxmstep = 0;
	int cmdc = 0,
	    eoargs = 0,
	    estatus = 0;
	char **arg,
	     *cmdl = "";
	bool fVerbose = false,
	     fDry = false,
	     fMagia = false;

	/*
	 * This is an argument parser.
	 * taks note: It's awful. There's probably another way
	 * to do this and that doesn't sacrifice speed and/or
	 * readability.
	 */
	for (opt = 0; argv[opt]; opt++) {
		switch (argv[opt][0]) {
			case '-':
				switch (argv[opt][1]) {
					case 'd':
						/*
						 * Dry-run, do not execute the
						 * command.
						 * This also sets fVerbose as false
						 * instead of the contrary, since
						 * it will be opting out for the
						 * least "destructive"/modifying
						 * action in case of ambiguity.
						 */
						fDry = true;
						fVerbose = false;
						break;
					case 'v':
						fVerbose = true;
						break;
					case 'a':
						argv[opt]++;
						magia = argv[opt][1];
						switch (magia) {
							case '\0':
								usage();
							default:
								break;
						}
						break;
					default:
						mstep = crargs(argv[opt]);
						switch (mstep) {
							case EOUTRANGE:
							case ENOTNO:
								fprintf(stderr,
									(mstep == EOUTRANGE
									 ? "%s: number out of range: %s\n"
									 : "%s: illegal option -- %s\n"),
								       progname, (argv[opt] + 1));
								usage();
							default:
								/* So you've set
								 * a number? */
								fMagia = true;
								break;
						}
						break;
				}
				argv[opt] = NULL;
				continue;
			default:
				/* Mark the end of program arguments and start
				 * of the command line to be executed. */
				eoargs = opt;
				break;
		}
		break;
	}

	cmdc = (argc - eoargs);
	if (cmdc < 2) {
		usage();
	}

	arg = calloc((size_t)cmdc, sizeof(char *));
	if (arg == NULL) {
		fprintf(stderr,
			"%s: failed to allocate %lu bytes on memory: %s\n",
			progname, (argc * sizeof(char *)), strerror(errno));
		exit(1);
	}

	for (c = 0; c < cmdc; c++) {
		/* Shift element from the end
		 * of command arguments. */
		arg[c] = argv[(eoargs + c)];
	}

	/* Declare the command string. */
	cmd = strdup(arg[0]);
	shift(arg, cmdc);

	maxmstep = magiac();
	/* If nothing defined a magic
	 * number, set it as one. */
	mstep = (maxmstep == 0) 
		? (mstep == 0 && !fMagia)
			? 1
			: mstep
		: maxmstep;

	for (i=0; i < cmdc; i += ((mstep == 0) ? 1 : mstep)) {
		if ((cmdc - i) < mstep) {
			fprintf(stderr, "%s: expecting %d argument(s) after `%s'\n",
					progname, (mstep - (cmdc - i)), arg[cmdc - 1]);
			exit(1);
		}

		/* Set command to be run. */
		cmdl = buildcmd(arg, i);
		if (fDry || fVerbose) puts(cmdl);
		if (!fDry) estatus = eXec(cmdl);
	}
	free(cmdl);

	exit(estatus);
}

/* Parses -# into #, with # being an integer. */
short int crargs(char *s) {
	long int n = 0;
	char *r = "";

	/*
	 * Shift the first character
	 * (expected to be '-').
	 */
	s++;

	n = strtol(s, &r, 10);
	if (n == 0 && r[0] != '\0') {
		n = ENOTNO;
	} else if (n < 0) {
		n = EOUTRANGE;
	}

	return (short int)n;
}

/*
 * Count the number of magic
 * characters on the command
 * string.
 */
short int magiac(void) {
	/*
	 * 'm' is the magic number found,
	 * 'maxms' is the largest magic
	 * number on the string.
	 */
	short int m = 0,
	 	maxms = 0;
	unsigned int c = 0;
	char ch = '\0';

	/*
	 * Count the number of magic characters
	 * on the command string.
	 */
	for (c = 0; cmd[c]; c++) {
		ch = cmd[c];
		if (ch == magia) {
			m = (cmd[(c + 1)] - '0');
			if (m > maxms) maxms = m;
			c++;
		}
	}

	return maxms;
}

char *buildcmd(char *arg[], int carg) {
	unsigned int c = 0,
		 l = 0;
	int m = 0,
	    n = 0,
	    cmdlen = 0;
	char ch = '\0',
	     *cmdbuf = "",
	     *cmdbufp = "";
	bool enamo = false;

	/*
	 * Count the actual size needed
	 * to make the command string.
	 */
	for (cmdlen = strlen(cmd), l=0; l < mstep; l++) {
		n = (carg + l);
	       	cmdlen += strlen(arg[n]);
		n = 0;
	}

	/* Allocate the command buffer. */
	cmdbuf = calloc((size_t)(cmdlen + 1), sizeof(char *));
	cmdbufp = cmdbuf;
	for (c = 0; cmd[c] != '\0'; c++) {
		ch = cmd[c];
		if (ch == magia) {
			m = (cmd[(c + 1)] - '0');
			n = (carg + (m - 1));
			c++;
			if (m <= 0 || 9 < m) {
				/* It will be doing the reverse of
				 * 'm' per adding '0'. */
				cmdbufp += sprintf(cmdbufp, "%c%c",
						ch, (m + '0'));
				continue;
			} else {
				cmdbufp += sprintf(cmdbufp, "%s", arg[n]);
				enamo = true;
			}
		} else {
			*cmdbufp++ = ch;
		}
	}

	/*
	 * Payload for cases where a magic character
	 * in the string is not present. It can be
	 * checked if 'enamo' is false.
	 */
	if (mstep != 0 && !enamo) {
		short int i = 0;
		for (i = 0; i < mstep; i++) {
			n = (carg + i);
			cmdbufp += sprintf(cmdbufp, "%c%s",
					' ', arg[n]);
		}
	}
	/* Close the string. */
	*cmdbufp = '\0';

	return cmdbuf;
}

/* What tha name says: it executes a command. */
int eXec(const char command[]) {
	int st = 0;
	char *shell = "",
	     *shpath = "",
	     *name = "";
	pid_t pid = 0;

	shell = (getenv("SHELL") != NULL)
		? getenv("SHELL")
		: SHELL;
	shpath = strdup(shell);
	name = basename(shpath);

	pid = fork();
	switch (pid) {
		case 0:
			execl(shell, name, "-c", command, (char *)NULL);
			_exit(127);
		default:
			break;
	}

	switch (pid) {
		case -1:
			fprintf(stderr, "%s: failed to fork: %s\n",
					progname, strerror(errno));
			exit(-1);
		default:
			while (waitpid(pid, &st, 0) == -1) continue;
			break;
	}

	return st;
}

void usage(void) {
	fprintf(stderr, "%s: [-#] [-a magia] command args ...\n", progname);
	exit(1);
}
