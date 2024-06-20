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

short int mn = 0;
/* 
 * Permit buildcmd() to memorize
 * 'cmd' making it public. 
 */
char magia = '%',
     *cmd = "";
bool fMagia = false;

void main(int argc, char *argv[]);
short int crargs(char *s);
char **buildcmd(char *arg[], int cmdc);
int execute(const char command[]);
void usage(void);

void main(int argc, char *argv[]) {
	progname = argv[0];
	argv++;
	argc--;

	register unsigned int opt = 0,
		 c = 0,
		 i = 0;
	int cmdc = 0,
	    eoargs = 0;
	char **commandv,
	     **toexec;
	bool fVerbose = false,
	     fDry = false;

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
						mn = crargs(argv[opt]);
						switch (mn) {
							case EOUTRANGE:
							case ENOTNO:
								fprintf(stderr,
									(mn == EOUTRANGE
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
	
	commandv = calloc((size_t)cmdc, sizeof(char *));
	if (commandv == NULL) {
		fprintf(stderr,
			"%s: failed to allocate %lu bytes on memory: %s\n",
			progname, (argc * sizeof(char *)), strerror(errno));
		exit(1);
	}

	for (c = 0; c < cmdc; c++) {
		/* Shift element from the end
		 * of command arguments. */
		commandv[c] = argv[(eoargs + c)];
	}

	/* Set command to be run. */
	for (i=1; i < cmdc; i++) {
		toexec = buildcmd(commandv, cmdc);
		printf("toexec: %s\nk: %d\n", toexec[0], i);
	}

	/* Debug */
	printf("argc: %d\ncmdc: %d\ncommandl: %s\nvflag: %d\nmagia: %c\nnargs: %d\n",
		argc, cmdc, toexec[1], fVerbose, magia, mn);


	free(commandv);
	free(toexec);
	exit(0);
}

char **buildcmd(char *arg[], int cmdc) {
	register unsigned int c = 0,
		 d = 0,
		 e = 0,
		 i = 0,
		 m = 0,
		 maxm = 0;
	char *cmd = "",
	     *cmdbuf = "",
	     **execbuf,
	     ch = '\0';
	
	/* 
	 * Has 'cmd' already been created
	 * and magic already computed?
	 * If not, do it.
	 */
	if(! *cmd) {
		/* 
		 * Count the number of magic characters
		 * on the command string.
		 */
		cmd = strdup(arg[0]);
		for (c = 0; cmd[c]; c++) {
			ch = cmd[c];
			if (ch == magia) {
				m = (cmd[(c + 1)] - '0');
				if (m > maxm) maxm = m;
			}
		}
		arg++;
		cmdc--;
	}

	/* 
	 * Is 'mn' not defined nor do we have
	 * a magic character on the command string?
	 * Default it to one.
	 * It will also prioritize magic characters
	 * over numbers toggled by the switch.
	 */
	if ((mn == 0 && !fMagia) && !maxm) {
		mn = 1;
	} else {
		mn = maxm;
	}

	/* Allocate the command buffer. */
	execbuf = calloc((size_t)cmdc, sizeof(char *));
	
/*	for (i=0; i < cmdc; i+=mn) {
 *		cmdbuf = malloc(sizeof(cmd) + sizeof(arg[i]) + 1);
 *		for (d = 0; cmd[d]; d++) {
 *			ch = cmd[d];
 * 			if (ch == magia) {
 * 				m = ((cmd[(d + 1)] - '0') - 1);
 *				for (e = 0; e < strlen(arg[m]); e++) {
 *					cmdbuf[(d + e)] = arg[m][e]; 
 *				}
 *				d += (e + 1);
 *			} else {
 *				cmdbuf[d]= ch;
 *			}
 *			
 *		}
 *		puts(cmdbuf);
 *		free(cmdbuf);
 *	}
 */
	/* 
	 * puts(cmd);
	 * for (i = 0; i < cmdc; i++) {
	 * puts(arg[i]);
	 * }
	 */
	

	return execbuf;
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
