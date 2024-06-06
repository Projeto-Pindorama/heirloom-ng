#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#define EOUTRANGE	-2
#define ENOTNO		-1

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
int crargs(char *s);
void usage(void);

void main(int argc, char *argv[]) {
	progname = argv[0];
	argv++;
	argc--;

	register unsigned int i = 0,
		 	j = 0,
			cmdc = 0;
	int nargs = 0;
	char magia = '\0';
	bool vflag = false;

	for (i = 0; argv[i]; i++) { 
		if (cmdc) goto cmd;
		switch (argv[i][0]) {
			case '-':
				switch (argv[i][1]) {
					case 'v':
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
						nargs = crargs(argv[i]);
						switch (nargs) {
							case EOUTRANGE:
							case ENOTNO:
								fprintf(stderr,
									(EOUTRANGE
									 ? "%s: number out of range: %s\n"
									 : "%s: unrecognized flag: %s\n"),
										progname, argv[i]);
								usage();
							default:
								break;
						}

				}
				argv[i] = NULL;
				argc--;
				break;
			default:
cmd:
				/* End of arguments for the program. */
				cmdc++;
				argc--;
				continue;
		}
	}

	if (cmdc < 1) {
		usage();
	}

	/* Debug */
	printf("argc: %d\ncmdc: %d\nvflag: %d\nmagia: %c\nnargs: %d\n",
			argc, cmdc, vflag, magia, nargs);
}

int crargs(char *s) {
	long n = 0;
	char *ss = NULL,
	     *r = NULL;

	/* 
	 * Shift the first character
	 * (expected to be '-').
	 */
	ss = s;
	ss++;

	n = strtol(ss, &r, 10);
	if (n == 0 && r[0] != '\0') {
		n = ENOTNO;
	} else if (n < 0 || 9 < n) {
		n = EOUTRANGE;
	}

	return (int)n;
}

void usage(void) {
	fprintf(stderr, "%s: [-#] [-a magia] command args ...\n", progname);
	exit(1);
}
