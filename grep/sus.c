/*
 * grep - search a file for a pattern
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, April 2001.
 */
/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */

/*	Sccsid @(#)sus.c	1.24 (gritter) 5/29/05>	*/

/*
 * Code for POSIX.2 command version only.
 */

#include	<sys/types.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	"alloc.h"
#include	"grep.h"

#if defined (SU3)
int		sus = 3;
#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char su3id[] USED = "@(#)grep_su3.sl	1.24 (gritter) 5/29/05";
#else
int		sus = 1;
#endif
char		*stdinmsg = "(standard input)";

/*
 * Usage message.
 */
void
usage(void)
{
	char *sEF, *sq, *ss;

	if (*progname == 'f') {
		sq = "";
		ss = "";
	} else {
		sq = "|-q";
		ss = "s";
	}
	if (*progname == 'f' || *progname == 'e')
		sEF = "";
	else
		sEF = "[-E|-F] ";
	fprintf(stderr, "%s: Usage:\n\
     %s[-c|-l%s] [-bhin%svx] pattern [file ...]\n\
     %s[-c|-l%s] [-bhin%svx] -e pattern ... [-f file ...] [file ...]\n\
     %s[-c|-l%s] [-bhin%svx] -f file ... [-e pattern ...] [file ...]\n",
 		progname,
		sEF, sq, ss,
		sEF, sq, ss,
		sEF, sq, ss);
	exit(2);
}

void
misop(void)
{
	usage();
}

void
rc_error(struct expr *e, int rerror)
{
	char *regerrs;
	size_t resz;

	resz = regerror(rerror, e->e_exp, NULL, 0) + 1;
	regerrs = smalloc(resz);
	regerror(rerror, e->e_exp, regerrs, resz);
	fprintf(stderr, "%s: RE error: %s\n", progname, regerrs);
	exit(2);
}

void
init(void)
{
	switch (*progname) {
	case 'e':
		Eflag = 2;
		rc_select();
		options = "EFbce:f:hilnqrRsvxyz";
		break;
	case 'f':
		Fflag = 2;
		ac_select();
		options = "Fbce:f:hilnqrRsvxyz";
		break;
	default:
		rc_select();
		options = "EFbce:f:hilnqrRsvwxyz";
	}
}

void
eg_select(void)
{
}

void
st_select(void)
{
}
