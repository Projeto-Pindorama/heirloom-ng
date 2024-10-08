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

/*	from 4.4BSD /usr/src/old/awk/parse.c	4.3 (Berkeley) 4/17/91	*/
/*	Sccsid @(#)parse.c	1.5 (gritter) 4/21/04>	*/

#include "awk.def"
#include "awk.h"
#include "stdio.h"
node *ALLOC(int n)
{
	node *x;
	x = (node *) malloc(sizeof(node) + (n-1)*sizeof(node *));
	if (x == NULL)
		error(FATAL, "out of space in ALLOC");
	return(x);
}
node *exptostat(node *a)
{
	a->ntype = NSTAT;
	return(a);
}
node	*nullstat;
node *node0(intptr_t a)
{
	node *x;
	x=ALLOC(0);
	x->nnext = NULL;
	x->nobj=a;
	return(x);
}
node *node1(intptr_t a,node *b)
{
	node *x;
	x=ALLOC(1);
	x->nnext = NULL;
	x->nobj=a;
	x->narg[0]=b;
	return(x);
}
node *node2(intptr_t a,node *b,node *c)
{
	node *x;
	x = ALLOC(2);
	x->nnext = NULL;
	x->nobj = a;
	x->narg[0] = b;
	x->narg[1] = c;
	return(x);
}
node *node3(intptr_t a,node *b,node *c,node *d)
{
	node *x;
	x = ALLOC(3);
	x->nnext = NULL;
	x->nobj = a;
	x->narg[0] = b;
	x->narg[1] = c;
	x->narg[2] = d;
	return(x);
}
node *node4(intptr_t a,node *b,node *c,node *d,node *e)
{
	node *x;
	x = ALLOC(4);
	x->nnext = NULL;
	x->nobj = a;
	x->narg[0] = b;
	x->narg[1] = c;
	x->narg[2] = d;
	x->narg[3] = e;
	return(x);
}
node *stat3(intptr_t a,node *b,node *c,node *d)
{
	node *x;
	x = node3(a,b,c,d);
	x->ntype = NSTAT;
	return(x);
}
node *op2(intptr_t a,node *b,node *c)
{
	node *x;
	x = node2(a,b,c);
	x->ntype = NEXPR;
	return(x);
}
node *op1(intptr_t a,node *b)
{
	node *x;
	x = node1(a,b);
	x->ntype = NEXPR;
	return(x);
}
node *stat1(intptr_t a,node *b)
{
	node *x;
	x = node1(a,b);
	x->ntype = NSTAT;
	return(x);
}
node *op3(intptr_t a,node *b,node *c,node *d)
{
	node *x;
	x = node3(a,b,c,d);
	x->ntype = NEXPR;
	return(x);
}
node *stat2(intptr_t a,node *b,node *c)
{
	node *x;
	x = node2(a,b,c);
	x->ntype = NSTAT;
	return(x);
}
node *stat4(intptr_t a,node *b,node *c,node *d,node *e)
{
	node *x;
	x = node4(a,b,c,d,e);
	x->ntype = NSTAT;
	return(x);
}
node *valtonode(cell *a, int b)
{
	node *x;
	x = node0((intptr_t)a);
	x->ntype = NVALUE;
	x->subtype = b;
	return(x);
}
node *pa2stat(node *a,node *b,node *c)
{
	node *x;
	x = node3(paircnt++, a, b, c);
	x->ntype = NPA2;
	return(x);
}
node *linkum(node *a,node *b)
{
	node *c;
	if(a == NULL) return(b);
	else if(b == NULL) return(a);
	for(c=a; c->nnext != NULL; c=c->nnext);
	c->nnext = b;
	return(a);
}
node *genprint(void)
{
	node *x;
	x = stat2(PRINT,valtonode(lookup("$record", symtab, 0), CFLD), nullstat);
	return(x);
}
