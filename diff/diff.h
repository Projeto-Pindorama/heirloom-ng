/*
 * This code contains changes by
 * Gunnar Ritter, Freiburg i. Br., Germany, March 2003. All rights reserved.
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

/*	Sccsid @(#)diff.h	1.15 (gritter) 3/26/05>	*/
/*	from 4.3BSD diff.h	4.7	85/08/16	*/

/*
 * diff - common declarations
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>
#include <errno.h>
#include <setjmp.h>

#if defined (__GLIBC__)
#if defined (_IO_getc_unlocked)
#undef	getc
#define	getc(f)		_IO_getc_unlocked(f)
#endif
#if defined (_IO_putc_unlocked)
#undef	putc
#define	putc(c, f)	_IO_putc_unlocked(c, f)
#undef	putchar
#define	putchar(c)	_IO_putc_unlocked(c, stdout)
#endif
#endif

/*
 * Output format options
 */
int	opt;

#define	D_NORMAL	0	/* Normal output */
#define	D_EDIT		-1	/* Editor script out */
#define	D_REVERSE	1	/* Reverse editor script */
#define	D_CONTEXT	2	/* Diff with context */
#define	D_IFDEF		3	/* Diff with merged #ifdef's */
#define	D_NREVERSE	4	/* Reverse ed script with numbered
				   lines and no trailing . */
#define	D_UNIFIED	5	/* Unified diff */

int	aflag;			/* diff binary files */
int	tflag;			/* expand tabs on output */
int	pflag;			/* show surrounding C function */

/*
 * Algorithm related options
 */
int	hflag;			/* -h, use halfhearted DIFFH */
int	bflag;			/* ignore blanks in comparisons */
int	wflag;			/* totally ignore blanks in comparisons */
int	iflag;			/* ignore case in comparisons */
int	Bflag;			/* ignore changes that consist of blank lines */

/*
 * Options on hierarchical diffs.
 */
int	lflag;			/* long output format with header */
int	rflag;			/* recursively trace directories */
int	sflag;			/* announce files which are same */
int	Nflag;			/* write text of nonexistant files */
const char	*start;		/* do file only if name >= this */

struct xclusion {
	struct xclusion	*x_nxt;
	const char	*x_pat;
}	*xflag;			/* patterns to exclude from comparison */

/*
 * Variables for -I D_IFDEF option.
 */
int	wantelses;		/* -E */
char	*ifdef1;		/* String for -1 */
char	*ifdef2;		/* String for -2 */
char	*endifname;		/* What we will print on next #endif */
int	inifdef;

/*
 * Variables for -c context option.
 */
int	context;		/* lines of context to be printed */

/*
 * State for exit status.
 */
int	status;
int	anychange;
char	*tempfile1;		/* used when comparing against std input */
char	*tempfile2;		/* used when comparing against std input */

/*
 * Variables for diffdir.
 */
char	**diffargv;		/* option list to pass to recursive diffs */
int	recdepth;		/* recursion depth */
jmp_buf	recenv;			/* jump stack on error */

struct stackblk {
	struct stackblk	*s_prev;
	struct stackblk	*s_next;
} *curstack;

/*
 * Input file names.
 * With diffdir, file1 and file2 are allocated BUFSIZ space,
 * and padded with a '/', and then efile0 and efile1 point after
 * the '/'.
 */
char	*file1, *file2, *efile1, *efile2;
struct	stat stb1, stb2;

extern	const char diffh[], diff[], pr[];
extern	const char *argv0;
extern	const char *progname;
int	mb_cur_max;
extern int	sysv3;

/* diff.c */
void	diffany(char **);
int	min(int, int);
int	max(int, int);
void	done(void);
void	*dalloc(size_t);
void	*talloc(size_t);
void	*ralloc(void *, size_t);
void	tfree(void *);
void	purgestack(void);
void	oomsg(const char *);
/* diffdir.c */
void	diffdir(char **);
int	ascii(int);
/* diffreg.c */
void	diffreg(void);
