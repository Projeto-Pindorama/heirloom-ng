/*
 * grep - search a file for a pattern
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, April 2001.
 */
/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute
 * it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 */

/*	Sccsid @(#)ggrep.c	1.26 (gritter) 1/4/05>	*/

/*
 * Code for generic SVID3 grep only.
 */

#include	<sys/types.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	"grep.h"
#include	"alloc.h"

/*
 * Error callback for compile().
 */
void
comperr(int code)
{
	char *msg;

	switch (code) {
	case 11:
		msg = "Range endpoint too large.";
		break;
	case 16:
		msg = "Bad number.";
		break;
	case 25:
		msg = "``\\digit'' out of range.";
		break;
	case 36:
		msg = "Illegal or missing delimiter.";
		break;
	case 41:
		msg = "No remembered search string.";
		break;
	case 42:
		msg = "\\( \\) imbalance.";
		break;
	case 43:
		msg = "Too many \\(.";
		break;
	case 44:
		msg = "More than 2 numbers given in \\{ \\}.";
		break;
	case 45:
		msg = "} expected after \\.";
		break;
	case 46:
		msg = "First number exceeds second in \\{ \\}.";
		break;
	case 49:
		msg = "[ ] imbalance.";
		break;
	case 50:
		msg = "Regular expression overflow.";
		break;
	case 67:
		msg = "Illegal byte sequence.";
		break;
	default:
		msg = "Unknown regexp error code!!";
	}
	fprintf(stderr, "%s: RE error %d: %s\n", progname, code, msg);
	exit(2);
}

#include	"regexpr.h"

static char	*c_exp;

/*
 * Compile a pattern.
 */
static void
st_build(void)
{
	if (iflag)
		e0->e_len = loconv(e0->e_pat, e0->e_pat, e0->e_len + 1) - 1;
	if ((c_exp = compile(e0->e_pat, NULL, NULL)) == NULL)
		comperr(regerrno);
}

/*
 * Compare a line and the global pattern using step().
 */
/*ARGSUSED1*/
static int
st_match(const char *line, size_t sz)
{
	return step(line, c_exp);
}

void
st_select(void)
{
	build = st_build;
	match = st_match;
	matchflags |= MF_NULTERM|MF_LOCONV;
}

void
patstring(char *pat)
{
	long	len = strlen(pat);

	e0 = (struct expr *)smalloc(sizeof *e0);
	e0->e_nxt = NULL;
	if (wflag)
		wcomp(&pat, &len);
	e0->e_pat = pat;
	e0->e_len = len;
	e0->e_flg = E_NONE;
	if (strchr(e0->e_pat, '\n')) {
		fprintf(stderr, "%s: newline in RE not allowed\n", progname);
		exit(2);
	}
}

void
init(void)
{
	st_select();
	options = "bchilnrRsvwyz";
}

void
misop(void)
{
	usage();
}

/*ARGSUSED*/
void
patfile(char *a)
{
}

void
ac_select(void)
{
}

void
eg_select(void)
{
}

void
rc_select(void)
{
}

char *usagemsg = "Usage: %s -hblcnsvi pattern file . . .\n";
char		*stdinmsg = "<stdin>";
