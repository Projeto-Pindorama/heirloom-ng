/*
   Changes by Gunnar Ritter, Freiburg i. Br., Germany, December 2002.
  
   Sccsid @(#)main.c	1.14 (gritter) 12/19/04>
 */
/* UNIX(R) Regular Expression Tools

   Copyright (C) 2001 Caldera International, Inc.
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to:
       Free Software Foundation, Inc.
       59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/*	copyright	"%c%"	*/

/*	from unixsrc:usr/src/common/cmd/awk/main.c /main/uw7_nj/2	*/
/*	from RCS Header: main.c 1.3 91/08/12 	*/

#define DEBUG
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <pfmt.h>
#include <errno.h>
#include <string.h>
#include <locale.h>
#include <langinfo.h>
#include <libgen.h>

#define	CMDCLASS	""/*"UX:"*/	/* Command classification */

#include <locale.h>

#include "awk.h"
#include "y.tab.h"

int	dbg	= 0;
unsigned char	*cmdname;	/* gets argv[0] for error messages */
extern	FILE	*yyin;	/* lex input file */
static FILE	*awk_yyin;
extern  FILE 	*yyout;
unsigned char	*lexprog;	/* points to program argument if it exists */
unsigned char	**start_delayed; /* first name=val argument delayed for BEGIN code */
unsigned char	**after_delayed; /* first argument after the delayed name=val's */
extern	int errorflag;	/* non-zero if any syntax errors; set by yyerror */
int	compile_time = 2;	/* for error printing: */
				/* 2 = cmdline, 1 = compile, 0 = running */

#define MAXPFILE 100
unsigned char	*pfile[MAXPFILE]; /* program filenames from -f's */
int	npfile = 0;	/* number of filenames */
int	curpfile = 0;	/* current filename */

int	mb_cur_max;	/* MB_CUR_MAX, for acceleration */

extern const char badopen[];

int main(int argc, unsigned char *argv[], unsigned char *envp[])
{
	unsigned char *fs = NULL;
	char label[MAXLABEL+1];	/* Space for the catalogue label */

	(void)setlocale(LC_COLLATE, "");
	(void)setlocale(LC_CTYPE, "");
	/*(void)setlocale(LC_MESSAGES, "");*/
	(void)setlocale(LC_NUMERIC, "POSIX");	/* redundant */
	mb_cur_max = MB_CUR_MAX;
	cmdname = (unsigned char *)basename((char *)argv[0]);
	(void)strcpy(label, CMDCLASS);
	(void)strncat(label, (char*) cmdname, (MAXLABEL - sizeof(CMDCLASS) - 1));
	(void)setcat("uxawk");
	(void)setlabel(label);
	/*version = (char*) gettxt(":31", "version Oct 11, 1989");*/
 	if (argc == 1) {
		if (0 /* posix */)
			pfmt(stderr, MM_ERROR, ":32:Incorrect usage\n");
		pfmt(stderr, MM_ACTION | (0 /* posix */ ? 0 : MM_NOSTD),
			":210107:Usage: %s [-f programfile | 'program'] [-Ffieldsep] [-v var=value] [files]\n",
			cmdname);
		exit(1);
	}
	signal(SIGFPE, fpecatch);
	awk_yyin = NULL;
	yyout = stdout;
	fldinit();
	syminit();
	while (argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0') {
		if (strcmp((char*) argv[1], "--") == 0) {	/* explicit end of args */
			argc--;
			argv++;
			break;
		}
		switch (argv[1][1]) {
		case 'f':	/* next argument is program filename */
			if (npfile >= MAXPFILE)
				error(MM_ERROR, ":106:Too many program filenames");
			if (argv[1][2] != '\0') { /* arg is -fname */
				pfile[npfile++] = &argv[1][2];
			} else {
				argc--;
				argv++;
				if (argc <= 1)
					error(MM_ERROR, ":34:No program filename");
				pfile[npfile++] = argv[1];
			}
			break;
		case 'F':	/* set field separator */
			if (argv[1][2] != 0) {	/* arg is -Fsomething */
				if (argv[1][2] == 't' && argv[1][3] == 0)	/* wart: t=>\t */
					fs = (unsigned char *) "\t";
				else if (argv[1][2] != 0)
					fs = &argv[1][2];
			} else {		/* arg is -F something */
				argc--; argv++;
				if (argc > 1 && argv[1][0] == 't' && argv[1][1] == 0)	/* wart: t=>\t */
					fs = (unsigned char *) "\t";
				else if (argc > 1 && argv[1][0] != 0)
					fs = &argv[1][0];
			}
			if (fs == NULL || *fs == '\0')
				error(MM_WARNING, ":35:Field separator FS is empty");
			break;
		case 'v':	/* -v a=1 to be done NOW.  one -v for each */
			if (argv[1][2] != '\0') { /* arg is -va=1 */
				if (!isclvar(&argv[1][2]))
					error(MM_ERROR, ":105:malformed -v assignment");
				setclvar(&argv[1][2]);
			} else if (--argc > 1 && isclvar((++argv)[1])) {
				setclvar(argv[1]);
			} else {
				error(MM_ERROR, ":105:malformed -v assignment");
			}
			break;
		case 'd':
			dbg = atoi((char *)&argv[1][2]);
			if (dbg == 0)
				dbg = 1;
			pfmt(stdout, (MM_INFO | MM_NOGET), "%s %s\n",
				cmdname, version);
			break;
		default:
			pfmt(stderr, MM_WARNING,
				":36:Unknown option %s ignored\n", argv[1]);
			break;
		}
		argc--;
		argv++;
	}
	/* argv[1] is now the first argument */
	if (npfile == 0) {	/* no -f; first argument is program */
		if (argc <= 1)
			error(MM_ERROR, ":37:No program given");
		dprintf( ("program = |%s|\n", argv[1]) );
		lexprog = argv[1];
		argc--;
		argv++;
	}
	/* hold leading name=val arguments until just after BEGIN */
	if (posix && argc > 1 && isclvar(argv[1])) {
		start_delayed = &argv[0];
		do {
			argv[0] = argv[1];
			argv++;
		} while (--argc > 1 && isclvar(argv[1]));
		after_delayed = &argv[0];
	}
	compile_time = 1;
	argv[0] = cmdname;	/* put prog name at front of arglist */
	dprintf( ("argc=%d, argv[0]=%s\n", argc, argv[0]) );
	arginit(argc, argv);
	envinit(envp);
	yyparse();
	if (fs)
		*FS = tostring(qstring(fs, '\0'));
	dprintf( ("errorflag=%d\n", errorflag) );
	if (errorflag == 0) {
		compile_time = 0;
		(void)setlocale(LC_NUMERIC, "");
		run(winner);
	} else
		bracecheck();
	exit(errorflag);
}

int pgetc(void)		/* get program character */
{
	int c;

	for (;;) {
		if (awk_yyin == NULL) {
			if (curpfile >= npfile)
				return EOF;
			if (!strcmp((char *)pfile[curpfile], "-"))
				awk_yyin = stdin;
			else if ((awk_yyin = fopen((char *) pfile[curpfile], "r")) == NULL)
				error(MM_ERROR, badopen,
					pfile[curpfile], strerror(errno));
		}
		if ((c = getc(awk_yyin)) != EOF)
			return c;
		awk_yyin = NULL;
		curpfile++;
	}
}
