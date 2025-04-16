/*-
 * This code contains changes by
 *      Gunnar Ritter, Freiburg i. Br., Germany, 2002. All rights reserved.
 *
 * Conditions 1, 2, and 4 and the no-warranty notice below apply
 * to these changes.
 *
 *
 * Copyright (c) 1991
 *      The Regents of the University of California.  All rights reserved.
 *
 * SPDX-Licence-Identifier: BSD-4-Clause-UC
 *
 *
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * SPDX-Licence-Identifier: Caldera
 */

#ifdef	notdef
char copyright[] =
"Copyright (c) 1991 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif

/*	from 4.4BSD /usr/src/old/awk/main.c	4.6 (Berkeley) 4/17/91	*/
/*	Sccsid @(#)main.c	1.11 (gritter) 12/12/04>	*/

#include "stdio.h"
#include "ctype.h"
#include "awk.def"
#include "awk.h"
#include <libgen.h>
#include <locale.h>
#include <langinfo.h>

int	dbg	= 0;
int	ldbg	= 0;
#ifdef	notdef
int	svflg	= 0;
#endif
int	rstflg	= 0;
int	svargc;
char	**svargv, **xargv;
extern FILE	*yyin;	/* lex input file */
char	*lexprog;	/* points to program argument if it exists */
extern	int errorflag;	/* non-zero if any syntax errors; set by yyerror */

int filefd, symnum, ansfd, mb_cur_max;
const char *radixchar = ".";
int radixcharlen = 1;
char *filelist;
char *progname;
extern int maxsym;
int
main(int argc, char **argv) {
	setlocale(LC_COLLATE, "");
	setlocale(LC_CTYPE, "");
	setlocale(LC_NUMERIC, "C");
	mb_cur_max = MB_CUR_MAX;
	progname = tostring(basename(argv[0]));
	if (argc == 1)
		error(FATAL, "Usage: %s [-f source | 'cmds'] [files]",
				progname);
	fldinit();
	syminit();
	growrec();
	while (argc > 1) {
		argc--;
		argv++;
		/* this nonsense is because gcos argument handling */
		/* folds -F into -f.  accordingly, one checks the next */
		/* character after f to see if it's -f file or -Fx.
		*/
		if (argv[0][0] == '-' && argv[0][1] == 'f' &&
				argv[0][2] == '\0') {
			if (argv[1] == NULL)
				error(FATAL, "no argument for -f");
			if (argv[1][0] == '-' && argv[1][1] == '\0')
				yyin = stdin;
			else {
				yyin = fopen(argv[1], "r");
				if (yyin == NULL)
					error(FATAL, "can't open %s", argv[1]);
			}
			argc--;
			argv++;
			break;
		} else if (argv[0][0] == '-' && argv[0][1] == 'F') {	/* set field sep */
			if (argv[0][2] == 't')	/* special case for tab */
				**FS = '\t';
			else
				*FS = tostring(&argv[0][2]);
			continue;
		} else if (argv[0][0] != '-') {
			dprintf("cmds=|%s|\n", argv[0]);
			yyin = NULL;
			lexprog = argv[0];
			argv[0] = argv[-1];	/* need this space */
			break;
		} else if (strcmp("-d", argv[0])==0) {
			dbg = 1;
#ifdef	notdef
		}
		else if (strcmp("-l", argv[0])==0) {
			ldbg = 1;
		}
		else if(strcmp("-S", argv[0]) == 0) {
			svflg = 1;
		}
		else if(strncmp("-R", argv[0], 2) == 0) {
			if(thaw(argv[0] + 2) == 0)
				rstflg = 1;
			else {
				fprintf(stderr, "not restored\n");
				exit(1);
			}
#endif	/* !notdef */
		}
	}
	if (argc <= 1) {
		argv[0][0] = '-';
		argv[0][1] = '\0';
		argc++;
		argv--;
	}
	svargc = --argc;
	svargv = ++argv;
	dprintf("svargc=%d svargv[0]=%s\n", svargc, svargv[0]);
	*FILENAME = *svargv;	/* initial file name */
	if(rstflg == 0)
		yyparse();
	dprintf("errorflag=%d\n", errorflag);
	if (errorflag)
		exit(errorflag);
#ifdef	notdef
	if(svflg) {
		svflg = 0;
		if(freeze("awk.out") != 0)
			fprintf(stderr, "not saved\n");
		exit(0);
	}
#endif
	setlocale(LC_NUMERIC, "");
	if (*(radixchar = nl_langinfo(RADIXCHAR)) != '\0')
		radixchar = tostring(radixchar);
	else
		radixchar = ".";
	radixcharlen = strlen(radixchar);
	run();
	exit(errorflag);
}

int
yywrap(void)
{
	return(1);
}
