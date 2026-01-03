/*
 * apply.c - apply a command to a group of arguments
 */
/*
 * Copyright (C) 2024-2026: Luiz Antônio Rangel (takusuman)
 *                          Arthur Bacci (arthurbacci)
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

/* Error codes for crargs(). */
#define EOUTRANGE      -1
#define ENOTNO         -2

static char *progname;
static size_t cmdlen = 0;
static uint8_t maxmstep = 0;
static int8_t mstep = 0;
static char magia = '%';

int8_t crargs(char *s);
uint8_t magiac(char cmd[]);
char *buildcmd(char cmd[], char *arg[], int carg);
int eXec(const char command[]);
void usage(void);

int main(int argc, char *argv[]) {
	progname = argv[0];
	shift(argv, argc);

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
								break;
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

	cmdc = argc - eoargs;
	if (cmdc < 2) {
		usage();
	}

	arg = calloc((size_t)cmdc, sizeof(char *));
	if (arg == NULL) {
		fprintf(stderr,
			"%s: failed to allocate %lu bytes on memory: %s\n",
			progname, (cmdc * sizeof(char *)), strerror(errno));
		return 1;
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

	maxmstep = magiac(cmd);
	/*
	 * If nothing defined a magic
	 * number, set it as one.
	 */
	if (!maxmstep) {
		if (mstep == 0 && !fMagia)
			mstep = 1;
	} else {
		mstep = maxmstep;
	}

	for (i=0; i < cmdc; i += ((mstep == 0) ? 1 : mstep)) {
		if ((cmdc - i) < mstep) {
			fprintf(stderr, "%s: expecting %d argument(s) after `%s'\n",
					progname, (mstep - (cmdc - i)), arg[cmdc - 1]);
			return 1;
		}

		cmdl = buildcmd(cmd, arg, i);

		/* In case of failuring build the command string. */
		if (!cmdl) return 255;

		if (fDry || fVerbose) puts(cmdl);
		if (!fDry)
			estatus = eXec(cmdl);
		if (fExitOnErr && estatus != 0)
			i = cmdc;
		free(cmdl);
	}
	free(cmd);

	return estatus;
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
	size_t c = 0;
	char ch = '\0';

	for (c = 0; cmd[c] != '\0'; c++) {
		ch = cmd[c];
		if (ch == magia) {
			if (!isalpha(cmd[c + 1])) {
				/* Integer */
				m = cmd[c + 1] - '0';

				/* Store magic character location. */
				if (m == 0 || m > 9)
					continue;
				cmd[c] = -1;

				/* Set largest argument. */
				if (m > maxms)
					maxms = m;
			}
			c++;
		}
	}

	return maxms;
}

char *buildcmd(char cmd[], char *arg[], int carg) {
	bool enamo = false;
	uint8_t i = 0,
	       number = 0,
	       index = 0;
	size_t arglen = 0,
	       cmdbuflen = 0,
	       c = 0,
	       d = 0,
	       l = 0;
	char ch = '\0',
	     *cmdbuf = NULL,
	     *cmdbufp = NULL;

	/*
	 * !maxmstep (enamo):
	 * Case in which there's not a
	 * magical character on the string.
	 * Just a clearer expression than
	 * checking if maxmstep is
	 * different from zero.
	 */
	enamo = (maxmstep == 0);

	/*
	 * Count the actual size needed to make the command
	 * string. Now we will do things more efficiently,
	 * in other words, optmize (a little) the use of
	 * memory to use precisely just the necessary number
	 * of characters. In case of using magic numbers,
	 * remove %# from the count. Otherwise, add one to
	 * the new buffer length since arguments are
	 * separated by spaces.
	 */
	cmdbuflen = cmdlen;
	l = enamo?
		(size_t)mstep
		: cmdlen;
	for (; l--;) {
		if (!enamo && cmd[l] != -1)
			continue;
		if (enamo)
			number = l + 1;
		else
			number = cmd[l + 1] - '0';
		index = carg + (number - 1);
		arglen += strlen(arg[index]);
		index = 0;

		if (enamo) {
			cmdbuflen++;
		} else {
			cmdbuflen -= 2;
			l -= 1;
		}
	}
	cmdbuflen += arglen + 1;

	/* Allocate the command buffer. */
	cmdbuf = malloc(cmdbuflen * sizeof(char));
	if (cmdbuf == NULL) {
		fprintf(stderr,
			"%s: failed to allocate %lu bytes on memory: %s\n",
			progname, cmdbuflen, strerror(errno));
		return NULL;
	}
	cmdbufp = cmdbuf;
	if (enamo) {
		/*
		 * Enamoured: payload for cases
		 * where a magic character in the
		 * string is not present.
		 * In this case, it will just copy
		 * the string verbatim and then
		 * amend arguments as per 'mstep'.
		 */
		cmdbufp = stpncpy(cmdbufp, cmd, cmdlen);
		for (i = 0; i < mstep; i++) {
			index = carg + i;
			sputchar(cmdbufp, ' ');
			for (d = 0; arg[index][d]; d++)
				sputchar(cmdbufp, arg[index][d]);
		}
	} else {
		for (c = 0; cmd[c] != '\0'; c++) {
			ch = cmd[c];
			if (ch != -1) {
				sputchar(cmdbufp, ch);
			} else { /* Magic! */
				number = cmd[c + 1] - '0';
				index = carg + (number - 1);
				c++;
				for (d = 0; arg[index][d]; d++)
					sputchar(cmdbufp, arg[index][d]);
			}
		}
	}

	/* Close the string. */
	*cmdbufp = '\0';

	return cmdbuf;
}

/* What the name says: it executes a command. */
int eXec(const char command[]) {
	int st = 0,
	    ec = 0;
	char *shell = NULL,
	     *shpath = NULL,
	     *name = NULL;
	pid_t pid = 0;

	if ((shell = getenv("SHELL")) == NULL)
	    shell = SHELL;
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
			ec = (WIFEXITED(st))?
				WEXITSTATUS(st)
				: -1;
			break;
	}

	return ec;
}

void usage(void) {
	fprintf(stderr, "%s: [-#] [-a magia] command args ...\n", progname);
	exit(1);
}
