/*
 * Aho-Corasick algorithm derived from Unix 32V /usr/src/cmd/fgrep.c,
 * additionally incorporating the fix from the v7 addenda tape.
 *
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, September 2002.
 */
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
static const char sccsid[] USED = "@(#)fgrep.sl	2.10 (gritter) 5/29/05";

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "grep.h"
#include "alloc.h"

#include <mbtowi.h>

#define	MAXSIZ	256
#define	QSIZE	128

struct words {
	struct words	*nst;
	struct words	*link;
	struct words	*fail;
	int 	inp;
	char	out;
};

static	struct words	*w, *wcur;
static	struct words	*smax;
static	struct words	*q;

static void	ac_build(void);
static int	ac_match(const char *, size_t);
static int	ac_matchw(const char *, size_t);
static int	ac_range(struct iblok *, char *);
static int	ac_rangew(struct iblok *, char *);
static void	cgotofn(void);
static void	check(int, int);
static void	woverflo(void);
static void	qoverflo(struct words ***queue, int *qsize);
static void	cfail(void);
static int	a0_match(const char *, size_t);
static int	a1_match(const char *, size_t);

void
ac_select(void)
{
	build = ac_build;
	match = mbcode ? ac_matchw : ac_match;
	matchflags &= ~MF_NULTERM;
	matchflags |= MF_LOCONV;
}

static void
ac_build(void)
{
	struct expr *e;

	if (e0->e_flg & E_NULL) {
		match = a0_match;
		return;
	}
	for (e = e0; e; e = e->e_nxt) {
		if (e->e_len == 0 && !xflag) {
			match = a1_match;
			return;
		}
	}
	cgotofn();
	cfail();
	if (!iflag)
		range = mbcode ? ac_rangew : ac_range;
}

static int
ac_match(const char *line, size_t sz)
{
	register const char *p;
	register int z;
	register struct words *c;
	int failed;

	p = line;
	failed = 0;
	c = w;
	if (p == &line[sz])
		z = '\n';
	else
		z = *p & 0377;
	for (;;) {
		nstate:
			if (c->inp == z) {
				c = c->nst;
			}
			else if (c->link != 0) {
				c = c->link;
				goto nstate;
			}
			else {
				c = c->fail;
				failed = 1;
				if (c==0) {
					c = w;
					istate:
					if (c->inp == z) {
						c = c->nst;
					}
					else if (c->link != 0) {
						c = c->link;
						goto istate;
					}
				}
				else goto nstate;
			}
		if (c->out) {
			if (xflag) {
				if (failed || p < &line[sz])
					return 0;
			}
			return 1;
		}
		if (++p >= &line[sz]) {
			if (z == '\n')
				return 0;
			else
				z = '\n';
		} else
			z = *p & 0377;
	}
}

static int
ac_range(struct iblok *ip, char *last)
{
	register char *p;
	register struct words *c;
	int failed;

	p = ip->ib_cur;
	lineno++;
	failed = 0;
	c = w;
	for (;;) {
		nstate:
			if (c->inp == (*p & 0377)) {
				c = c->nst;
			}
			else if (c->link != 0) {
				c = c->link;
				goto nstate;
			}
			else {
				c = c->fail;
				failed = 1;
				if (c==0) {
					c = w;
					istate:
					if (c->inp == (*p & 0377)) {
						c = c->nst;
					}
					else if (c->link != 0) {
						c = c->link;
						goto istate;
					}
				}
				else goto nstate;
			}
		if (c->out) {
			if (xflag) {
				register char *ep = p;
				while (*ep != '\n')
					ep++;
				if ((failed || ep > p) && vflag == 0) {
					ip->ib_cur = &ep[1];
					goto nogood;
				}
			}
			if (vflag == 0) {
		succeed:	outline(ip, last, p - ip->ib_cur);
				if (qflag || lflag)
					return 1;
			} else {
				ip->ib_cur = p;
				while (*ip->ib_cur++ != '\n');
			}
		nogood:	if ((p = ip->ib_cur) > last)
				return 0;
			lineno++;
			c = w;
			failed = 0;
			continue;
		}
		if (*p++ == '\n') {
			if (vflag) {
				p--;
				goto succeed;
			}
			if ((ip->ib_cur = p) > last)
				return 0;
			lineno++;
			c = w;
			failed = 0;
		}
	}
}

static int
ac_matchw(const char *line, size_t sz)
{
	register const char *p;
	wint_t z;
	register struct words *c;
	int failed, n = 0;

	p = line;
	failed = 0;
	c = w;
	if (p == &line[sz])
		z = '\n';
	else {
		if (*p & 0200) {
			if ((n = mbtowi(&z, p, &line[sz] - p)) < 0) {
				n = 1;
				z = WEOF;
			}
		} else {
			z = *p;
			n = 1;
		}
	}
	for (;;) {
		nstate:
			if (c->inp == z) {
				c = c->nst;
			}
			else if (c->link != 0) {
				c = c->link;
				goto nstate;
			}
			else {
				c = c->fail;
				failed = 1;
				if (c==0) {
					c = w;
					istate:
					if (c->inp == z) {
						c = c->nst;
					}
					else if (c->link != 0) {
						c = c->link;
						goto istate;
					}
				}
				else goto nstate;
			}
		if (c->out) {
			if (xflag) {
				if (failed || p < &line[sz])
					return 0;
			}
			return 1;
		}
		p += n;
		if (p >= &line[sz]) {
			if (z == '\n')
				return 0;
			else
				z = '\n';
		} else {
			if (*p & 0200) {
				if ((n = mbtowi(&z, p, &line[sz] - p)) < 0) {
					n = 1;
					z = WEOF;
				}
			} else {
				z = *p;
				n = 1;
			}
		}
	}
}

static int
ac_rangew(struct iblok *ip, char *last)
{
	register char *p;
	wint_t	z;
	register struct words *c;
	int failed, n = 0;

	p = ip->ib_cur;
	lineno++;
	failed = 0;
	c = w;
	for (;;) {
		nstate:
			if (*p & 0200) {
				if ((n = mbtowi(&z, p, last + 1 - p)) < 0) {
					n = 1;
					z = WEOF;
				}
			} else {
				z = *p;
				n = 1;
			}
			if (c->inp == z) {
				c = c->nst;
			}
			else if (c->link != 0) {
				c = c->link;
				goto nstate;
			}
			else {
				c = c->fail;
				failed = 1;
				if (c==0) {
					c = w;
					istate:
					if (c->inp == z) {
						c = c->nst;
					}
					else if (c->link != 0) {
						c = c->link;
						goto istate;
					}
				}
				else goto nstate;
			}
		if (c->out) {
			if (xflag) {
				register char *ep = p;
				while (*ep != '\n')
					ep++;
				if ((failed || ep > p) && vflag == 0) {
					ip->ib_cur = &ep[1];
					goto nogood;
				}
			}
			if (vflag == 0) {
		succeed:	outline(ip, last, p - ip->ib_cur);
				if (qflag || lflag)
					return 1;
			} else {
				ip->ib_cur = p;
				while (*ip->ib_cur++ != '\n');
			}
		nogood:	if ((p = ip->ib_cur) > last)
				return 0;
			lineno++;
			c = w;
			failed = 0;
			continue;
		}
		p += n;
		if (p[-n] == '\n') {
			if (vflag) {
				p--;
				goto succeed;
			}
			if ((ip->ib_cur = p) > last)
				return 0;
			lineno++;
			c = w;
			failed = 0;
		}
	}
}

static void
cgotofn(void)
{
	register int c;
	register struct words *s;

	woverflo();
	s = smax = w = wcur;
nword:	for(;;) {
		c = nextch();
		if (c==EOF)
			return;
		if (c == '\n') {
			if (xflag) {
				for(;;) {
					if (s->inp == c) {
						s = s->nst;
						break;
					}
					if (s->inp == 0) goto nenter;
					if (s->link == 0) {
						if (++smax >= &wcur[MAXSIZ])
							woverflo();
						s->link = smax;
						s = smax;
						goto nenter;
					}
					s = s->link;
				}
			}
			s->out = 1;
			s = w;
		} else {
		loop:	if (s->inp == c) {
				s = s->nst;
				continue;
			}
			if (s->inp == 0) goto enter;
			if (s->link == 0) {
				if (++smax >= &wcur[MAXSIZ])
					woverflo();
				s->link = smax;
				s = smax;
				goto enter;
			}
			s = s->link;
			goto loop;
		}
	}

	enter:
	do {
		s->inp = c;
		if (++smax >= &wcur[MAXSIZ])
			woverflo();
		s->nst = smax;
		s = smax;
	} while ((c = nextch()) != '\n' && c!=EOF);
	if (xflag) {
	nenter:	s->inp = '\n';
		if (++smax >= &wcur[MAXSIZ])
			woverflo();
		s->nst = smax;
	}
	smax->out = 1;
	s = w;
	if (c != EOF)
		goto nword;
}

static void
check(int val, int incr)
{
	if ((unsigned)(val + incr) >= INT_MAX) {
		fprintf(stderr, "%s: wordlist too large\n", progname);
		exit(2);
	}
}

static void
woverflo(void)
{
	wcur = smax = scalloc(MAXSIZ, sizeof *smax);
}

static void
qoverflo(struct words ***queue, int *qsize)
{
	check(*qsize, QSIZE);
	*queue = srealloc(*queue, (*qsize += QSIZE) * sizeof **queue);
}

static void
cfail(void)
{
	struct words **queue = NULL;
	int front, rear;
	int qsize = 0;
	struct words *state;
	int bstart;
	register char c;
	register struct words *s;
	qoverflo(&queue, &qsize);
	s = w;
	front = rear = 0;
init:	if ((s->inp) != 0) {
		queue[rear++] = s->nst;
		if (rear >= qsize - 1)
			qoverflo(&queue, &qsize);
	}
	if ((s = s->link) != 0) {
		goto init;
	}

	while (rear!=front) {
		s = queue[front];
		if (front == qsize-1)
			qoverflo(&queue, &qsize);
		front++;
	cloop:	if ((c = s->inp) != 0) {
			bstart = 0;
			q = s->nst;
			queue[rear] = q;
			if (front < rear) {
				if (rear >= qsize-1)
					qoverflo(&queue, &qsize);
				rear++;
			} else
				if (++rear == front)
					qoverflo(&queue, &qsize);
			state = s->fail;
		floop:	if (state == 0) {
				state = w;
				bstart = 1;
			}
			if (state->inp == c) {
			qloop:	q->fail = state->nst;
				if ((state->nst)->out == 1)
					q->out = 1;
				if ((q = q->link) != 0) goto qloop;
			}
			else if ((state = state->link) != 0)
				goto floop;
			else if (bstart == 0) {
				state = 0;
				goto floop;
			}
		}
		if ((s = s->link) != 0)
			goto cloop;
	}
	free(queue);
}

/*ARGSUSED*/
static int
a0_match(const char *str, size_t sz)
{
	return 0;
}

/*ARGSUSED*/
static int
a1_match(const char *str, size_t sz)
{
	return 1;
}
