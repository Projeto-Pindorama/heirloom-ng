/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, August 2003.
 */
/*	from Unix 32V /usr/src/cmd/tsort.c	*/
/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   Redistributions of source code and documentation must retain the
 *    above copyright notice, this list of conditions and the following
 *    disclaimer.
 *   Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *   All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed or owned by Caldera
 *      International, Inc.
 *   Neither the name of Caldera International, Inc. nor the names of
 *    other contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE
 * LIABLE FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)tsort.sl	1.6 (gritter) 5/29/05";

/*	topological sort
 *	input is sequence of pairs of items (blank-free strings)
 *	nonidentical pair is a directed edge in graph
 *	identical pair merely indicates presence of node
 *	output is ordered list of items consistent with
 *	the partial ordering specified by the graph
*/
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include "asciitype.h"

#if defined (__GLIBC__) && defined (_IO_getc_unlocked)
#undef	getc
#define	getc(f)		_IO_getc_unlocked(f)
#endif

/*	the nodelist always has an empty element at the end to
 *	make it easy to grow in natural order
 *	states of the "live" field:*/
#define DEAD 0	/* already printed*/
#define LIVE 1	/* not yet printed*/
#define VISITED 2	/*used only in findloop()*/

static struct nodelist {
	struct nodelist *nextnode;
	struct predlist *inedges;
	char *name;
	int live;
} firstnode = {NULL, NULL, NULL, DEAD};

/*	a predecessor list tells all the immediate
 *	predecessors of a given node
*/
struct predlist {
	struct predlist *nextpred;
	struct nodelist *pred;
};

static char *empty = "";
static char *progname;

static int present(struct nodelist *, struct nodelist *);
static int anypred(struct nodelist *);
static struct nodelist *idx(const char *);
static int cmp(const char *, const char *);
static void error(const char *, const char *);
static void note(const char *, const char *);
static struct nodelist *findloop(void);
static struct nodelist *mark(struct nodelist *);
static char *token(char **, int *, FILE *);

/*	the first for loop reads in the graph,
 *	the second prints out the ordering
*/
int
main(int argc,char **argv)
{
	register struct predlist *t;
	FILE *input = stdin;
	register struct nodelist *i, *j;
	int x;
	char *precedes = NULL, *follows = NULL;
	int presiz = 0, folsiz = 0;
	progname = basename(argv[0]);
	if (argc > 1 && argv[1][0] == '-' && argv[1][1] == '-'
			&& argv[1][2] == '\0')
		argv++, argc--;
	if(argc>1) {
		if (argc > 2) {
			fprintf(stderr, "Usage:  %s [ file ]\n", progname);
			exit(2);
		}
		input = fopen(argv[1],"r");
		if(input==NULL)
			error("cannot open ", argv[1]);
	}
	for(;;) {
		if (token(&precedes, &presiz, input) == NULL)
			break;
		if (token(&follows, &folsiz, input) == NULL)
			error("odd data",empty);
		i = idx(precedes);
		j = idx(follows);
		if(i==j||present(i,j)) 
			continue;
		t = malloc(sizeof(struct predlist));
		t->nextpred = j->inedges;
		t->pred = i;
		j->inedges = t;
	}
	for(;;) {
		x = 0;	/*anything LIVE on this sweep?*/
		for(i= &firstnode; i->nextnode!=NULL; i=i->nextnode) {
			if(i->live==LIVE) {
				x = 1;
				if(!anypred(i))
					break;
			}
		}
		if(x==0)
			break;
		if(i->nextnode==NULL)
			i = findloop();
		printf("%s\n",i->name);
		i->live = DEAD;
	}
	return 0;
}

/*	is i present on j's predecessor list?
*/
static int
present(struct nodelist *i,struct nodelist *j)
{
	register struct predlist *t;
	for(t=j->inedges; t!=NULL; t=t->nextpred)
		if(t->pred==i)
			return(1);
	return(0);
}

/*	is there any live predecessor for i?
*/
static int
anypred(struct nodelist *i)
{
	register struct predlist *t;
	for(t=i->inedges; t!=NULL; t=t->nextpred)
		if(t->pred->live==LIVE)
			return(1);
	return(0);
}

/*	turn a string into a node pointer
*/
static struct nodelist *
idx(register const char *s)
{
	register struct nodelist *i;
	register char *t;
	for(i= &firstnode; i->nextnode!=NULL; i=i->nextnode)
		if(cmp(s,i->name))
			return(i);
	for(t=(char *)s; *t; t++) ;
	t = malloc((unsigned)(t+1-s));
	i->nextnode = malloc(sizeof(struct nodelist));
	if(i->nextnode==NULL||t==NULL)
		error("too many items",empty);
	i->name = t;
	i->live = LIVE;
	i->nextnode->nextnode = NULL;
	i->nextnode->inedges = NULL;
	i->nextnode->live = DEAD;
	while(*t++ = *s++);
	return(i);
}

static int
cmp(register const char *s,register const char *t)
{
	while(*s==*t) {
		if(*s==0)
			return(1);
		s++;
		t++;
	}
	return(0);
}

static void
error(const char *s,const char *t)
{
	note(s,t);
	exit(1);
}

static void
note(const char *s,const char *t)
{
	fprintf(stderr,"%s: %s%s\n",progname,s,t);
}

/*	given that there is a cycle, find some
 *	node in it
*/
static struct nodelist *
findloop(void)
{
	register struct nodelist *i, *j;
	for(i= &firstnode; i->nextnode!=NULL; i=i->nextnode)
		if(i->live==LIVE)
			break;
	note("cycle in data",empty);
	i = mark(i);
	if(i==NULL)
		error("program error",empty);
	for(j= &firstnode; j->nextnode!=NULL; j=j->nextnode)
		if(j->live==VISITED)
			j->live = LIVE;
	return(i);
}

/*	depth-first search of LIVE predecessors
 *	to find some element of a cycle;
 *	VISITED is a temporary state recording the
 *	visits of the search
*/
static struct nodelist *
mark(register struct nodelist *i)
{
	register struct nodelist *j;
	register struct predlist *t;
	if(i->live==DEAD)
		return(NULL);
	if(i->live==VISITED)
		return(i);
	i->live = VISITED;
	for(t=i->inedges; t!=NULL; t=t->nextpred) {
		j = mark(t->pred);
		if(j!=NULL) {
			note(i->name,empty);
			return(j);
		}
	}
	return(NULL);
}

static char *
token(char **buf, int *sz, FILE *fp)
{
	char	*bp, *ob;
	int	c;

	bp = *buf;
	while ((c = getc(fp)) != EOF && whitechar(c));
	if (c == EOF)
		return NULL;
	do {
		if (bp - *buf >= *sz-1) {
			ob = *buf;
			if ((*buf = realloc(*buf, *sz += 50)) == NULL)
				error("out of memory", empty);
			bp += *buf - ob;
		}
		*bp++ = c;
	} while ((c = getc(fp)) != EOF && !whitechar(c));
	*bp = '\0';
	return *buf;
}
