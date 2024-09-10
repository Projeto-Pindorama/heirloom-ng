/*
 * apply.c - apply a command to a group of arguments
 */
/*
 * Copyright (C) 2024: Luiz Antônio Rangel (takusuman)
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
#define EOUTRANGE      (INT8_MIN >> 1)
#define ENOTNO         (INT8_MIN >> 2)

static char *progname;
static bool enamo = false;
static int8_t mstep = 0,
	      magias[10];
static char magia = '%';

void main(int argc, char *argv[]);
int8_t crargs(char *s);
bool ncontains(int8_t array[], uint8_t elem, int dsize);
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
	     *cmd = "",
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

	/*
	 * Initialize the magias[] array
	 * with invalid magic numbers, so
	 * we will avoid false-positives
	 * for c=0.
	 */
	memset(magias, -1, (10 * (sizeof(uint8_t))));

	maxmstep = magiac(cmd);
	/* If nothing defined a magic
	 * number, set it as one. */
	mstep = (maxmstep == 0)
		? (mstep == 0 && !fMagia)
			? 1
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
		if (!fDry) estatus = eXec(cmdl);
	}
	free(cmdl);

	exit(estatus);
}

/* Parses -# into #, with # being an integer. */
int8_t crargs(char *s) {
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

	return (int8_t)n;
}

/*
 * Check if an integer array
 * contains a value.
 */
bool ncontains(int8_t array[], uint8_t elem, int dsize) {
	for (; dsize--;) {
		if (array[dsize] == elem) return true;
	}
	return false;
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
					magias[m] = c;

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
	unsigned int c = 0,
		     l = 0;
	int cmdlen = 0;
	char ch = '\0',
	     *cmdbuf = "",
	     *cmdbufp = "";

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
	switch (enamo) {
		case true:
			/*
			 * Enamorated: payload for cases
			 * where a magic character in the
			 * string is not present.
			 * In this case, it will just copy
			 * the string verbatim and then
			 * amend arguments as per 'mstep'.
			 */
			cmdbufp = stpncpy(cmdbufp, cmd, (size_t)cmdlen);
			for (i = 0; i < mstep; i++) {
				n = (carg + i);
				cmdbufp += sprintf(cmdbufp, "%c%s",
						' ', arg[n]);
			}
			break;
		default:
			for (c = 0; cmd[c] != '\0'; c++) {
				ch = cmd[c];
				switch (ncontains(magias, c, 10)) {
					case true:
						m = (cmd[(c + 1)] - '0');
						n = (carg + (m - 1));
						c++;
						cmdbufp += sprintf(cmdbufp,
								"%s", arg[n]);
						continue;
					default:
						*cmdbufp++ = ch;
						break;
				}
			}
			break;
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
