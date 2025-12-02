/*
 * apply.c - apply a command to a group of arguments
 */
/*
 * Copyright (C) 2024-2025: Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 */

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
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
#define EOUTRANGE      -1
#define ENOTNO         -2

static char *progname;
static bool enamo = false;
static size_t cmdlen = 0;
static int8_t mstep = 0,
	      *magias = NULL;
static char magia = '%';

void main(int argc, char *argv[]);
int8_t crargs(char *s);
uint8_t magiac(char cmd[]);
char *buildcmd(char cmd[], char *arg[], int carg);
int eXec(const char command[]);
void usage(void);

void main(int argc, char *argv[]) {
	progname = argv[0];
	shift(argv, argc);

	uint8_t maxmstep = 0;
	unsigned int opt = 0,
		     c = 0,
		     i = 0;
	int cmdc = 0,
	    eoargs = 0,
	    estatus = 0;
	char **arg,
	     *cmd = NULL,
	     *cmdl = NULL;
	bool fVerbose = false,
	     fDry = false,
	     fExitOnErr = false,
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
					case 'e':
						fExitOnErr = true;
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
	cmdlen = strlen(cmd);

	/*
	 * Initialize the magias[] array
	 * with invalid magic numbers, so
	 * we will avoid false-positives
	 * such as c=0.
	 */
	magias = malloc(cmdlen);
	if (magias == NULL) {
		fprintf(stderr,
			"%s: failed to allocate %lu bytes on memory: %s\n",
			progname, cmdlen, strerror(errno));
		exit(1);
	}
	for (size_t i = 0; i < cmdlen; i++)
		magias[i] = -1;

	maxmstep = magiac(cmd);
	/* If nothing defined a magic
	 * number, set it as one. */
	mstep = (maxmstep == 0)?
		(mstep == 0 && !fMagia)?
			1
			: mstep
		: maxmstep;
	/*
	 * Case in which there's not a
	 * magical character on the string.
	 * Just a clearer expression than
	 * checking if maxmstep is
	 * different from zero.
	 */
	enamo = (maxmstep != 0)
		? false
		: true;

	for (i=0; i < cmdc; i += ((mstep == 0) ? 1 : mstep)) {
		if ((cmdc - i) < mstep) {
			fprintf(stderr, "%s: expecting %d argument(s) after `%s'\n",
					progname, (mstep - (cmdc - i)), arg[cmdc - 1]);
			exit(1);
		}

		/* Set command to be run. */
		cmdl = buildcmd(cmd, arg, i);
		if (fDry || fVerbose) puts(cmdl);
		if (!fDry)
			estatus = eXec(cmdl);
		if ((!fDry && fExitOnErr) && estatus != 0)
			i = cmdc;
		free(cmdl);
	}
	free(cmd);
	free(magias);

	exit(estatus);
}

/* Parses -# into #, with # being an integer. */
int8_t crargs(char *s) {
	long int n = 0;
	char *r = NULL;

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

	return (int8_t)n;
}

/*
 * Count the number of magic
 * characters on the command
 * string.
 */
uint8_t magiac(char cmd[]) {
	/*
	 * 'm' is the magic number found,
	 * 'maxms' is the largest magic
	 * number on the string.
	 */
	uint8_t m = 0,
	 	maxms = 0;
	unsigned int c = 0;
	char ch = '\0';

	for (c = 0; cmd[c] != '\0'; c++) {
		ch = cmd[c];
		if (ch == magia) {
			switch (isalpha(cmd[(c + 1)])) {
				case 0: /* Integer */
					m = (cmd[(c + 1)] - '0');

					/* Store magic character location. */
					if (m == 0 || m > 9) break;
					magias[c] = m;

					/* Set largest argument. */
					if (m > maxms) maxms = m;
				default:
					break;
			}
			c++;
		}
	}

	return maxms;
}

char *buildcmd(char cmd[], char *arg[], int carg) {
	uint8_t i = 0,
	       m = 0,
	       n = 0;
	unsigned int arglen = 0,
		     cmdbuflen = 0,
		     c = 0,
		     d = 0,
		     l = 0;
	char ch = '\0',
	     *cmdbuf = NULL,
	     *cmdbufp = NULL;

	/*
	 * Count the actual size needed to make the command
	 * string. Now we will do things more efficiently,
	 * in other words, optmize (a little) the use of
	 * memory to use precisely just the necessary number
	 * of characters.
	 */
	cmdbuflen = cmdlen;
	for (l = (enamo? mstep : cmdlen); l--;) {
		m = (enamo? l : (magias[l] - 1));
		n = (carg + m);
		if (!enamo && magias[l] == -1)
			continue;
		arglen += strlen(arg[n]);
		n = 0;
		/*
		 * In case of using magic numbers,
		 * remove %# from the count. Otherwise,
		 * add one to the new buffer length since
		 * arguments are separed by spaces in this
		 * case (see below).
		 */
		if (enamo)
			cmdbuflen++;
		else
			cmdbuflen -= 2;
		l -= !enamo;
	}
	cmdbuflen += (arglen + 1);
	cmdbuflen *= sizeof(char);

	/* Allocate the command buffer. */
	cmdbuf = malloc(cmdbuflen);
	if (cmdbuf == NULL) {
		fprintf(stderr,
			"%s: failed to allocate %lu bytes on memory: %s\n",
			progname, cmdbuflen, strerror(errno));
		exit(1);
	}
	cmdbufp = cmdbuf;
	switch (enamo) {
		case true:
			/*
			 * Enamoured: payload for cases
			 * where a magic character in the
			 * string is not present.
			 * In this case, it will just copy
			 * the string verbatim and then
			 * amend arguments as per 'mstep'.
			 */
			cmdbufp = stpncpy(cmdbufp, cmd, (size_t)cmdlen);
			for (i = 0; i < mstep; i++) {
				n = (carg + i);
				sputchar(cmdbufp, ' ');
				for (d = 0; arg[n][d]; d++)
					sputchar(cmdbufp, arg[n][d]);
			}
			break;
		default:
			for (c = 0; cmd[c] != '\0'; c++) {
				ch = cmd[c];
				switch (magias[c]) {
					case -1:
						sputchar(cmdbufp, ch);
						break;
					default: /* Magic! */
						m = (cmd[(c + 1)] - '0');
						n = (carg + (m - 1));
						c++;
						for (d = 0; arg[n][d]; d++)
							sputchar(cmdbufp, arg[n][d]);
						continue;
				}
			}
			break;
	}

	/* Close the string. */
	*cmdbufp = '\0';

	return cmdbuf;
}

/* What the name says: it executes a command. */
int eXec(const char command[]) {
	int st = 0;
	char *shell = NULL,
	     *shpath = NULL,
	     *name = NULL;
	pid_t pid = 0;

	shell = (getenv("SHELL") != NULL)?
		getenv("SHELL")
		: SHELL;
	shpath = strdup(shell);
	name = basename(shpath);
	free(shpath); /* free() before this function can fail. */

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
