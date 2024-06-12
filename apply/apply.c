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
#define EOUTRANGE	(INT_MIN >> 10)
#define ENOTNO		(INT_MIN >> 11)

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
char magia = '%';
void main(int argc, char *argv[]);
int crargs(char *s);
char *buildcmd(char *arg[]);
int execute(const char command[]);
void usage(void);

void main(int argc, char *argv[]) {
	progname = argv[0];
	argv++;
	argc--;

	register unsigned int i = 0,
		 j = 0;
	int cmdc = 0,
	    eoargs = 0,
	    nargs = 0;
	char **commandv,
	     *commandl = "";
	bool vflag = false,
	     dflag = false;

	/* 
	 * This is an argument parser.
	 * taks note: It's awful. There's probably another way
	 * to do this and that doesn't sacrifice speed and/or
	 * readability.
	 */
	for (i = 0; argv[i]; i++) { 
		switch (argv[i][0]) {
			case '-':
				switch (argv[i][1]) {
					case 'd':
						/* Dry-run, do not execute
						 * the command. */
						dflag = true;
						break;
					case 'v':
						/* Verbose, sets dry-run as
						 * false. */
						dflag = false;
						vflag = true;
						break;
					case 'a':
						argv[i]++;
						magia = argv[i][1];
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
						nargs = crargs(argv[i]);
						switch (nargs) {
							case EOUTRANGE:
							case ENOTNO:
								fprintf(stderr,
									(nargs == EOUTRANGE
									 ? "%s: number out of range: %s\n"
									 : "%s: illegal option -- %s\n"),
									progname, (argv[i] + 1));
								usage();
							default:
								break;
						}
						break;
				}
				argv[i] = NULL;
				continue;
			default:
				/* Mark the end of program arguments and start
				 * of the command line to be executed. */
				eoargs = i;
				break;
		}
		break;
	}

	cmdc = (argc - eoargs);
	if (cmdc < 2) {
		usage();
	}

	if ((commandv = calloc((size_t)cmdc, sizeof(char *))) == NULL) {
		fprintf(stderr,
			"%s: failed to allocate %lu bytes on memory: %s\n",
			progname, (argc * sizeof(char *)), strerror(errno));
		exit(1);
	}

	for (j = 0; j < cmdc; j++) {
		/* Shift element from the end
		 * of command arguments. */
		commandv[j] = argv[(eoargs + j)];
	}

	/* Set command to be run. */
	commandl = buildcmd(commandv);

	/* Debug */
	printf("argc: %d\ncmdc: %d\ncommandl: %s\nvflag: %d\nmagia: %c\nnargs: %d\n",
		argc, cmdc, commandl, vflag, magia, nargs);


	free(commandv);
	exit(0);
}

/* Parses -# into #, with # being an integer. */
int crargs(char *s) {
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

	return (int)n;
}

char *buildcmd(char *arg[]) {
	register unsigned int c = 0;
	char *cmdl = "",
	     m = '\0';
	/* 
	 * Count the number of magic characters
	 * on the command string.
	 */
	for (c = 0; arg[0][c]; c++) {
		m = arg[0][c];
		if (m == magia) {
			puts("O belo pode ser simples, e o simples pode ser belo");
			printf("magia: %c\n", arg[0][(c + 1)]);
		}
		printf("%c\n", m);
	}

	return cmdl;
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
