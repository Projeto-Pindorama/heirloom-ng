/*
   Changes by Gunnar Ritter, Freiburg i. Br., Germany, December 2002.
  
   Sccsid @(#)maketab.c	1.11 (gritter) 12/4/04>
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
/*		copyright	"%c%" 	*/

/*	from unixsrc:usr/src/common/cmd/awk/maketab.c /main/uw7_nj/1	*/
/*	from RCS Header: maketab.c 1.2 91/06/25 	*/
static const char sccsid[] = "@(#)maketab.c	1.11 (gritter) 12/4/04";

#include <stdio.h>
#include <string.h>
#include "awk.h"
#include "y.tab.h"

struct xx
{	int token;
	char *name;
	char *pname;
} proc[] = {
	{ PROGRAM, "program", NULL },
	{ BOR, "boolop", " || " },
	{ AND, "boolop", " && " },
	{ NOT, "boolop", " !" },
	{ NE, "relop", " != " },
	{ EQ, "relop", " == " },
	{ LE, "relop", " <= " },
	{ LT, "relop", " < " },
	{ GE, "relop", " >= " },
	{ GT, "relop", " > " },
	{ ARRAY, "array", NULL },
	{ INDIRECT, "indirect", "$(" },
	{ SUBSTR, "substr", "substr" },
	{ SUB, "sub", "sub" },
	{ GSUB, "gsub", "gsub" },
	{ INDEX, "sindex", "sindex" },
	{ SPRINTF, "awsprintf", "sprintf " },
	{ ADD, "arith", " + " },
	{ MINUS, "arith", " - " },
	{ MULT, "arith", " * " },
	{ DIVIDE, "arith", " / " },
	{ MOD, "arith", " % " },
	{ UMINUS, "arith", " -" },
	{ POWER, "arith", " **" },
	{ PREINCR, "incrdecr", "++" },
	{ POSTINCR, "incrdecr", "++" },
	{ PREDECR, "incrdecr", "--" },
	{ POSTDECR, "incrdecr", "--" },
	{ CAT, "cat", " " },
	{ PASTAT, "pastat", NULL },
	{ PASTAT2, "dopa2", NULL },
	{ MATCH, "matchop", " ~ " },
	{ NOTMATCH, "matchop", " !~ " },
	{ MATCHFCN, "matchop", "matchop" },
	{ INTEST, "intest", "intest" },
	{ PRINTF, "aprintf", "printf" },
	{ PRINT, "print", "print" },
	{ DELETE, "delete", "delete" },
	{ SPLIT, "split", "split" },
	{ ASSIGN, "assign", " = " },
	{ ADDEQ, "assign", " += " },
	{ SUBEQ, "assign", " -= " },
	{ MULTEQ, "assign", " *= " },
	{ DIVEQ, "assign", " /= " },
	{ MODEQ, "assign", " %= " },
	{ POWEQ, "assign", " ^= " },
	{ CONDEXPR, "condexpr", " ?: " },
	{ IF, "ifstat", "if(" },
	{ WHILE, "whilestat", "while(" },
	{ FOR, "forstat", "for(" },
	{ DO, "dostat", "do" },
	{ IN, "instat", "instat" },
	{ NEXT, "jump", "next" },
	{ EXIT, "jump", "exit" },
	{ BREAK, "jump", "break" },
	{ CONTINUE, "jump", "continue" },
	{ RETURN, "jump", "ret" },
	{ BLTIN, "bltin", "bltin" },
	{ CALL, "call", "call" },
	{ ARG, "arg", "arg" },
	{ VARNF, "getnf", "NF" },
	{ GETLINE, "getline", "getline" },
	{ 0, "", "" },
};

#define SIZE	LASTTOKEN - FIRSTTOKEN + 1
char *table[SIZE];
char *names[SIZE];

int main(void)
{
	struct xx *p;
	int i, n, tok;
	char c;
	FILE *fp;
	char buf[100], name[100], def[100];

	printf("#include <stdio.h>\n");
	printf("#include \"awk.h\"\n");
	printf("#include \"y.tab.h\"\n\n");
/*	printf("Cell *nullproc();\n");
	for (i = SIZE; --i >= 0; )
		names[i] = "";
	for (p=proc; p->token!=0; p++)
		if (p == proc || strcmp(p->name, (p-1)->name))
			printf("extern Cell *%s();\n", p->name);*/

	if ((fp = fopen("y.tab.h", "r")) == NULL) {
		fprintf(stderr, "maketab can't open y.tab.h!\n");
		exit(1);
	}
	printf("static unsigned char *printname[%d] = {\n", SIZE);
	i = 0;
	while (fgets(buf, sizeof buf, fp) != NULL) {
		if (*buf == '\n')
			continue;
		n = sscanf(buf, "%1c %s %s %d", &c, def, name, &tok);
		if ((c != '#' || n != 4) && (strcmp(def,"define") != 0))	/* not a valid #define */
			continue;
		if (strncmp(name, "YY", 2) == 0 || strncmp(name, "yy", 2) == 0)
			continue;
		if (tok < FIRSTTOKEN || tok > LASTTOKEN) {
			continue;
			/*
			fprintf(stderr, "maketab funny token %d %s\n", tok, buf);
			exit(1);
			*/
		}
		names[tok-FIRSTTOKEN] = (char *) malloc(strlen(name)+1);
		strcpy(names[tok-FIRSTTOKEN], name);
		printf("\t(unsigned char *) \"%s\",\t/* %d */\n", name, tok);
		i++;
	}
	printf("};\n\n");

	for (p=proc; p->token!=0; p++)
		table[p->token-FIRSTTOKEN] = p->name;
	printf("\nCell *(*proctab[%d])(Node **, int) = {\n", SIZE);
	for (i=0; i<SIZE; i++)
		if (table[i]==0)
			printf("\tnullproc,\t/* %s */\n", names[i]);
		else
			printf("\t%s,\t/* %s */\n", table[i], names[i]);
	printf("};\n\n");

	printf("unsigned char *tokname(int n)\n");	/* print a tokname() function */

	printf("{\n");
	printf("	static unsigned char buf[100];\n\n");
	printf("	if (n < FIRSTTOKEN || n > LASTTOKEN) {\n");
	printf("		snprintf((char *)buf, sizeof buf, \"token %%d\", n);\n");
	printf("		return buf;\n");
	printf("	}\n");
	printf("	return printname[n-257];\n");
	printf("}\n");
	exit(0);
}
