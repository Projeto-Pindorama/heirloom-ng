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

/*	from 4.4BSD /usr/src/old/awk/proc.c	4.4 (Berkeley) 4/17/91	*/
/*	Sccsid @(#)proc.c	1.8 (gritter) 12/25/06>	*/

#include "awk.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
struct xx
{	int token;
	const char *name;
	const char *pname;
} proc[] = {
	{ PROGRAM, "program", NULL},
	{ BOR, "boolop", " || "},
	{ AND, "boolop", " && "},
	{ NOT, "boolop", " !"},
	{ NE, "relop", " != "},
	{ EQ, "relop", " == "},
	{ LE, "relop", " <= "},
	{ LT, "relop", " < "},
	{ GE, "relop", " >= "},
	{ GT, "relop", " > "},
	{ ARRAY, "array", NULL},
	{ INDIRECT, "indirect", "$("},
	{ SUBSTR, "substr", "substr"},
	{ INDEX, "sindex", "sindex"},
	{ SPRINTF, "awsprintf", "sprintf "},
	{ ADD, "arith", " + "},
	{ MINUS, "arith", " - "},
	{ MULT, "arith", " * "},
	{ DIVIDE, "arith", " / "},
	{ MOD, "arith", " % "},
	{ UMINUS, "arith", " -"},
	{ PREINCR, "incrdecr", "++"},
	{ POSTINCR, "incrdecr", "++"},
	{ PREDECR, "incrdecr", "--"},
	{ POSTDECR, "incrdecr", "--"},
	{ CAT, "cat", " "},
	{ PASTAT, "pastat", NULL},
	{ PASTAT2, "dopa2", NULL},
	{ MATCH, "matchop", " ~ "},
	{ NOTMATCH, "matchop", " !~ "},
	{ PRINTF, "aprintf", "printf"},
	{ PRINT, "print", "print"},
	{ SPLIT, "split", "split"},
	{ ASSIGN, "assign", " = "},
	{ ADDEQ, "assign", " += "},
	{ SUBEQ, "assign", " -= "},
	{ MULTEQ, "assign", " *= "},
	{ DIVEQ, "assign", " /= "},
	{ MODEQ, "assign", " %= "},
	{ IF, "ifstat", "if("},
	{ WHILE, "whilestat", "while("},
	{ FOR, "forstat", "for("},
	{ IN, "instat", "instat"},
	{ NEXT, "jump", "next"},
	{ EXIT, "jump", "exit"},
	{ BREAK, "jump", "break"},
	{ CONTINUE, "jump", "continue"},
	{ FNCN, "fncn", "fncn"},
	{ GETLINE, "getline", "getline"},
	{ 0, ""},
};
#define SIZE	LASTTOKEN - FIRSTTOKEN
const char *table[SIZE];
const char *names[SIZE];

extern char	*tokname(int);

int
main(void)
{	struct xx *p;
	int i;
	printf("#include \"awk.def\"\n");
	printf("obj nullproc(node **, int);\n");
	for(p=proc;p->token!=0;p++)
		if(p==proc || strcmp(p->name, (p-1)->name))
			printf("extern obj %s(node **, int);\n",p->name);
	for(p=proc;p->token!=0;p++)
		table[p->token-FIRSTTOKEN]=p->name;
	printf("obj (*proctab[%d])(node **, int) = {\n", SIZE);
	for(i=0;i<SIZE;i++)
		if(table[i]==0) printf("/*%s*/\tnullproc,\n",tokname(i+FIRSTTOKEN));
		else printf("/*%s*/\t%s,\n",tokname(i+FIRSTTOKEN),table[i]);
	printf("};\n");
	printf("char *printname[%d] = {\n", SIZE);
	for(p=proc; p->token!=0; p++)
		names[p->token-FIRSTTOKEN] = p->pname;
	for(i=0; i<SIZE; i++)
		printf("/*%s*/\t\"%s\",\n",tokname(i+FIRSTTOKEN),
						names[i]?names[i]:"");
	printf("};\n");
	exit(0);
}

#include "token.c"
