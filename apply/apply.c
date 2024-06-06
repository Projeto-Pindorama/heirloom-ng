#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
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
void usage(void);

void main(int argc, char *argv[]) {
	progname = argv[0];
	argv++;
	argc--;

	register unsigned int i = 0,
		 	j = 0,
			cmdc = 0;
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
						magia = argv[i][2];
						switch (magia) {
							case '\0':
								usage();
							default:
								break;
						}
						break;
					default:
						fprintf(stderr,
							"%s: unrecognized flag: %s\n",
								progname, argv[i]);
						usage();
				}
				argv[i] = NULL;
				argc--;
				break;
			default:
cmd:
				/* End of arguments for the program. */
				cmdc++;
				continue;
		}
	}

	/* Debug */
	printf("argc: %d\ncmdc: %d\nvflag: %d\nmaxia: %c\n",
			argc, cmdc, vflag, magia);
}

void usage(void) {
	fprintf(stderr, "%s: [-#] [-a magia] command args ...\n", progname);
	exit(1);
}
