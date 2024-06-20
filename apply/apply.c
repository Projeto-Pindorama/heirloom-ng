/*
 * apply.c - apply a command to a set of arguments
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

/* 
 * Boilerplate for 'pointer++; pointerc--;'
 * when shifting argv and friends.
 */
#define shift(p, d)	p++; d--
#define shiftn(p, d, t) \
	for (register int i=0; i < t; i++) shift(p, d);

/* Error codes for crargs(). */
#define EOUTRANGE	(SHRT_MIN >> 10)
#define ENOTNO		(SHRT_MIN >> 11)

/*
 * Devem funcionar:
 * apply echo * # equals ls
 * apply -2 cmp A1 B1 A2 B2   # compares A's with B's
 * apply "ln %1 /usr/fred/dir" #  duplicates a directory
 * apply -v ...	# print each command first
 */
/*
 * Apply runs the named command on each argument arg in turn.
 * Normally arguments are chosen singly; the optional number n
 * specifies the number of arguments to be passed to command.
 * If n is zero, command is run without arguments once for each
 * arg. Character sequences of the form %d in command, where d
 * is a digit from 1 to 9, are replaced by the d'th following
 * unused arg. If any such sequences occur, n is ignored, and
 * the number of arguments passed to command is the maximum
 * value of d in command. The character `%' may be changed by
 * the -a option.
 */

char *progname;

short int mstep = 0;
char magia = '%',
     *cmd = "";

void main(int argc, char *argv[]);
short int magiac();
char *buildcmd(char *arg[], int carg);
int execute(const char command[]);
short int crargs(char *s);
void usage(void);

void main(int argc, char *argv[]) {
	progname = argv[0];
	shift(argv, argc);

	register unsigned int opt = 0,
		 c = 0,
		 i = 0;
	short int maxmstep = 0;
	int cmdc = 0,
	    eoargs = 0;
	char **arg,
	     *toexec = "";
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
						/* Dry-run, do not execute
						 * the command. */
						fDry = true;
						break;
					case 'v':
						/* Verbose, sets dry-run as
						 * false. */
						fDry = false;
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
						/* Check for numeric value
						 * betwixt 0 and 9. */
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
	if (maxmstep == 0) {
		/* 
		 * If nothing defined a 
		 * magic number, set it
		 * as one.
		 */
		mstep = (mstep == 0 && !fMagia)
			? 1
			: mstep;
	} else {
		mstep = maxmstep;
	}

	/* Set command to be run. */
	for (i=0; i < cmdc; i += ((mstep == 0) ? 1 : mstep)) {
		if (cmdc < mstep) {
			fprintf(stderr, "%s: expecting %d arguments after `%s'\n",
					progname, (mstep - cmdc), arg[i]);
		}
		toexec = buildcmd(arg, i);
		puts(toexec);
	}

	/* Debug */
	printf("argc: %d\ncmdc: %d\ncommandl: %s\nvflag: %d\nmagia: %c\nnargs: %d\n",
		argc, cmdc, toexec, fVerbose, magia, mstep);


	free(toexec);
	exit(0);
}

/* 
 * Count the number of magic
 * characters on the command
 * string.
 */
short int magiac() {
	/* 
	 * 'm' is the magic number found,
	 * 'maxms' is the largest magic
	 * number on the string.
	 */
	register short int m = 0,
	 	maxms = 0;
	register unsigned int c = 0;
	register char ch = '\0';

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
	register unsigned int c = 0,
		 l = 0;
	register char ch = '\0';
	int m = 0,
	    n = 0,
	    cmdlen = 0;
	char *cmdbuf = "",
	     *cmdbufp = "";

	/* 
	 * Count the actual size needed
	 * to make the command string.
	 */
	for (cmdlen = strlen(cmd), l=0; l < mstep; l++,
		n = (carg + l), cmdlen += strlen(arg[n]));

	/* 
	 * Allocate the command buffer.
	 */
	cmdbuf = calloc((size_t)(l + 1), sizeof(char *));
	
	cmdbufp = cmdbuf;
	for (c = 0; cmd[c] != '\0'; c++) {
		ch = cmd[c];
		if (ch == magia) {
			m = (cmd[(c + 1)] - '0');
			n = (carg + m);
			c++;
			if (m == 0) continue;
			cmdbufp += sprintf(cmdbufp, "%s", arg[n]);
		} else if (mstep != 0 && n == 0) {
			register short int i = 0;
			for (i = 0; i < mstep; i++) {
				n = (carg + i);
				cmdbufp += sprintf(cmdbufp, "%s", arg[n]);
			}
			break;
		} else {
			*cmdbufp++ = ch;
		}
	}

	/* Close the string. */
	*cmdbufp = '\0';

	//sprintf(cmdbuf, "%s + %s", cmd, arg[carg]);
	return cmdbuf; 
}

/* Parses -# into #, with # being an integer. */
short int crargs(char *s) {
	long n = 0;
	char *r = "";

	/* 
	 * Shift the first character
	 * (expected to be '-').
	 */
	s++;

	n = strtol(s, &r, 10);
	if (n == 0 && r[0] != '\0') {
		n = ENOTNO;
	} else if (n < 0 || 9 < n) {
		n = EOUTRANGE;
	}

	return (short int)n;
}

int execute(const char command[]) {
	char *shell = "";
	shell = (getenv("SHELL") != NULL)
		? getenv("SHELL")
		: SHELL;

	printf("SHELL: %s\n", shell);
	return 0;
}

void usage(void) {
	fprintf(stderr, "%s: [-#] [-a magia] command args ...\n", progname);
	exit(1);
}
