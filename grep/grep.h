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

/*	Sccsid @(#)grep.h	1.23 (gritter) 1/4/05>	*/

#include	<sys/types.h>
#include	<regex.h>

#include	"iblok.h"

#include	"config.h"

#define	BSZ		512		/* block size */

/*
 * Expression flags.
 */
enum eflags {
	E_NONE	= 0,		/* no flags set */
	E_NL	= 1,		/* pattern ends with newline */
	E_NULL	= 2		/* no pattern, not even an empty one */
};

/*
 * List of search expressions; not used for compile() matching.
 */
struct expr {
	struct expr	*e_nxt;		/* next item in list */
	char		*e_pat;		/* search pattern */
	regex_t		*e_exp;		/* compiled pattern from regcomp() */
	long		e_len;		/* pattern length */
	enum eflags	e_flg;		/* expression flags */
};

/*
 * Matcher flags.
 */
enum	matchflags {
	MF_NULTERM	= 01,	/* search string must be \0 terminated*/
	MF_LOCONV	= 02	/* lower-case search string if -i is set */
};

/*
 * Variables in grep.c.
 */
extern int		Eflag;		/* use EREs */
extern int		Fflag;		/* use fixed strings */
extern int		bflag;		/* print buffer count */
extern int		cflag;		/* print count only */
extern int		fflag;		/* had pattern file argument */
extern int		hflag;		/* do not print filenames */
extern int		iflag;		/* ignore case */
extern int		lflag;		/* print filenames only */
extern int		nflag;		/* print line numbers */
extern int		qflag;		/* no output at all */
extern int		sflag;		/* avoid error messages */
extern int		vflag;		/* inverse selection */
extern int		wflag;		/* search for words */
extern int		xflag;		/* match entire line */
extern int		mb_cur_max;	/* MB_CUR_MAX */
#define	mbcode		(mb_cur_max>1)	/* multibyte characters in use */
extern unsigned		status;		/* exit status */
extern off_t		lmatch;		/* count of matching lines */
extern off_t		lineno;		/* current line number */
extern char		*progname;	/* argv[0] to main() */
extern char		*filename;	/* name of current file */
extern void		(*build)(void);	/* compile function */
extern int		(*match)(const char *, size_t); /* comparison */
extern int		(*range)(struct iblok *, char *); /* grep range */
extern struct expr	*e0;		/* start of expression list */
extern enum matchflags	matchflags;	/* matcher flags */

/*
 * These differ amongst grep flavors.
 */
extern int		sus;		/* POSIX.2 command version in use */
extern char		*stdinmsg;	/* name for standard input */
extern char		*usagemsg;	/* usage string */
extern char		*options;	/* for getopt() */

/*
 * In grep.c.
 */
extern size_t		loconv(char *, char *, size_t);
extern void		wcomp(char **, long *);
extern void		report(const char *, size_t, off_t, int);

/*
 * Flavor dependent.
 */
extern void		usage(void);
extern void		misop(void);
extern void		rc_error(struct expr *, int);
extern void		init(void);

/*
 * Traditional egrep only.
 */
extern void		eg_select(void);

/*
 * Fgrep only.
 */
extern void		ac_select(void);

/*
 * compile()/step()-related.
 */
extern void		st_select(void);

/*
 * regcomp()/regexec()-related.
 */
extern void		rc_select(void);

/*
 * Not for SVID3 grep.
 */
extern void		patstring(char *);
extern void		patfile(char *);
extern int		nextch(void);
extern void		outline(struct iblok *, char *, size_t);
