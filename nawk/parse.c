/*
   Changes by Gunnar Ritter, Freiburg i. Br., Germany, December 2002.
  
   Sccsid @(#)parse.c	1.7 (gritter) 12/4/04>
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

/*	from unixsrc:usr/src/common/cmd/awk/parse.c /main/uw7_nj/1	*/
/*	from RCS Header: parse.c 1.2 91/06/25	*/

#define DEBUG
#include <stdio.h>
#include <string.h>
#include <pfmt.h>
#include "awk.h"
#include "y.tab.h"

Node *nodealloc(int n)
{
	register Node *x;
	x = (Node *) malloc(sizeof(Node) + (n-1)*sizeof(Node *));
	if (x == NULL)
		error(MM_ERROR, outofspace, "nodealloc");
	x->nnext = NULL;
	x->lineno = lineno;
	return(x);
}

Node *exptostat(Node *a)
{
	a->ntype = NSTAT;
	return(a);
}

Node *node1(int a, Node *b)
{
	register Node *x;
	x = nodealloc(1);
	x->nobj = a;
	x->narg[0]=b;
	return(x);
}

Node *node2(int a, Node *b, Node *c)
{
	register Node *x;
	x = nodealloc(2);
	x->nobj = a;
	x->narg[0] = b;
	x->narg[1] = c;
	return(x);
}

Node *node3(int a, Node *b, Node *c, Node *d)
{
	register Node *x;
	x = nodealloc(3);
	x->nobj = a;
	x->narg[0] = b;
	x->narg[1] = c;
	x->narg[2] = d;
	return(x);
}

Node *node4(int a, Node *b, Node *c, Node *d, Node *e)
{
	register Node *x;
	x = nodealloc(4);
	x->nobj = a;
	x->narg[0] = b;
	x->narg[1] = c;
	x->narg[2] = d;
	x->narg[3] = e;
	return(x);
}

Node *stat3(int a, Node *b, Node *c, Node *d)
{
	register Node *x;
	x = node3(a,b,c,d);
	x->ntype = NSTAT;
	return(x);
}

Node *op2(int a, Node *b, Node *c)
{
	register Node *x;
	x = node2(a,b,c);
	x->ntype = NEXPR;
	return(x);
}

Node *op1(int a, Node *b)
{
	register Node *x;
	x = node1(a,b);
	x->ntype = NEXPR;
	return(x);
}

Node *stat1(int a, Node *b)
{
	register Node *x;
	x = node1(a,b);
	x->ntype = NSTAT;
	return(x);
}

Node *op3(int a, Node *b, Node *c, Node *d)
{
	register Node *x;
	x = node3(a,b,c,d);
	x->ntype = NEXPR;
	return(x);
}

Node *op4(int a, Node *b, Node *c, Node *d, Node *e)
{
	register Node *x;
	x = node4(a,b,c,d,e);
	x->ntype = NEXPR;
	return(x);
}

Node *stat2(int a, Node *b, Node *c)
{
	register Node *x;
	x = node2(a,b,c);
	x->ntype = NSTAT;
	return(x);
}

Node *stat4(int a, Node *b, Node *c, Node *d, Node *e)
{
	register Node *x;
	x = node4(a,b,c,d,e);
	x->ntype = NSTAT;
	return(x);
}

Node *valtonode(Cell *a, int b)
{
	register Node *x;

	a->ctype = OCELL;
	a->csub = b;
	x = node1(0, (Node *) a);
	x->ntype = NVALUE;
	return(x);
}

Node *rectonode(void)
{
	/* return valtonode(lookup("$0", symtab), CFLD); */
	return valtonode(recloc, CFLD);
}

Node *makearr(Node *p)
{
	Cell *cp;

	if (isvalue(p)) {
		cp = (Cell *) (p->narg[0]);
		if (isfunc(cp))
			vyyerror(":38:%s is a function, not an array",
				cp->nval);
		else if (!isarr(cp)) {
			xfree(cp->sval);
			cp->sval = (unsigned char *) makesymtab(NSYMTAB);
			cp->tval = ARR;
		}
	}
	return p;
}

Node *pa2stat(Node *a,Node *b,Node *c)
{
	register Node *x;
	x = node4(PASTAT2, a, b, c, (Node *) paircnt);
	paircnt++;
	x->ntype = NSTAT;
	return(x);
}

Node *linkum(Node *a,Node *b)
{
	register Node *c;

	if (errorflag)	/* don't link things that are wrong */
		return a;
	if (a == NULL) return(b);
	else if (b == NULL) return(a);
	for (c = a; c->nnext != NULL; c = c->nnext)
		;
	c->nnext = b;
	return(a);
}

void defn(Cell *v, /* turn on FCN bit in definition */
		Node *vl, Node *st)	/* arglist, body of function */
{
	Node *p;
	int n;

	if (isarr(v)) {
		vyyerror(":39:`%s' is an array name and a function name",
			v->nval);
		return;
	}
	v->tval = FCN;
	v->sval = (unsigned char *) st;
	n = 0;	/* count arguments */
	for (p = vl; p; p = p->nnext)
		n++;
	v->fval = n;
	dprintf( ("defining func %s (%d args)\n", v->nval, n) );
}

int isarg(const char *s)	/* is s in argument list for current function? */
{
	extern Node *arglist;
	Node *p = arglist;
	int n;

	for (n = 0; p != 0; p = p->nnext, n++)
		if (strcmp((char *)((Cell *)(p->narg[0]))->nval, s) == 0)
			return n;
	return -1;
}
