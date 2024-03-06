/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*
 * Copyright 2003 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * Copyright 1985, 1993 by Mortice Kern Systems Inc.  All rights reserved.
 *
 */

/*	from OpenSolaris "getconf.c	1.14	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 */
#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined(__GNUC__)
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
#if defined(SU3)
static const char sccsid[] USED = "@(#)getconf_su3.sl	1.14 (gritter) 01/27/07";
#else	/* !SU3 */
static const char sccsid[] USED = "@(#)getconf.sl	1.14 (gritter) 01/27/07";
#endif	/* !SU3 */

/*
 * getconf -- POSIX.2 compatible utility to query configuration specific
 *	      parameters.
 *         -- XPG4 support added June/93
 *
 *         -- XPG5 support added Dec/97
 *
 *         -- XPG6 support added May/2003
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <libgen.h>

/*
 * Values applicable for the Heirloom Toolchest that partially
 * override those of the host platform.
 */
#undef	POSIX2_VERSION
#undef	_XOPEN_XCU_VERSION
#if defined(SU3)
#define	HEIRLOOM_PATH	SU3BIN ":" DEFBIN ":" CCSBIN ":" UCBBIN
#define	POSIX2_VERSION	200112
#define	_XOPEN_XCU_VERSION	600
#else	/* !SU3 */
#define	HEIRLOOM_PATH	SUSBIN ":" DEFBIN ":" CCSBIN ":" UCBBIN
#define	POSIX2_VERSION	199209
#define	_XOPEN_XCU_VERSION	4
#endif	/* !SU3 */

#if !defined(BC_BASE_MAX)
#define	BC_BASE_MAX	99
#endif
#if !defined(BC_DIM_MAX)
#define	BC_DIM_MAX	2048
#endif
#undef	BC_SCALE_MAX
#define	BC_SCALE_MAX	99
#undef	BC_STRING_MAX
#define	BC_STRING_MAX	1000
#undef	EXPR_NEST_MAX
#define	EXPR_NEST_MAX	32
#if !defined(LINE_MAX)
#define	LINE_MAX	2048
#endif
#undef	RE_DUP_MAX
#define	RE_DUP_MAX	255
#undef	POSIX2_UPE
#define	POSIX2_UPE	1

#include "heirloom.h"

/*
 * POSIX invariants, defined in case the host system does not provide
 * them, or to override possible wrong definitions.
 */
#undef	_POSIX_AIO_LISTIO_MAX
#define	_POSIX_AIO_LISTIO_MAX		2
#undef	_POSIX_AIO_MAX
#define	_POSIX_AIO_MAX			1
#undef	_POSIX_ARG_MAX
#define	_POSIX_ARG_MAX			4096
#undef	_POSIX_CHILD_MAX
#if defined(SU3)
#define	_POSIX_CHILD_MAX		25
#else	/* !SU3 */
#define	_POSIX_CHILD_MAX		6
#endif	/* !SU3 */
#undef	_POSIX_CLOCKRES_MIN
#define	_POSIX_CLOCKRES_MIN		20000000
#undef	_POSIX_DELAYTIMER_MAX
#define	_POSIX_DELAYTIMER_MAX		32
#undef	_POSIX_HOST_NAME_MAX
#define	_POSIX_HOST_NAME_MAX		255
#undef	_POSIX_LINK_MAX
#define	_POSIX_LINK_MAX			8
#undef	_POSIX_LOGIN_NAME_MAX
#define	_POSIX_LOGIN_NAME_MAX		9
#undef	_POSIX_MAX_CANON
#define	_POSIX_MAX_CANON		255
#undef	_POSIX_MAX_INPUT
#define	_POSIX_MAX_INPUT		255
#undef	_POSIX_MQ_OPEN_MAX
#define	_POSIX_MQ_OPEN_MAX		8
#undef	_POSIX_MQ_PRIO_MAX
#define	_POSIX_MQ_PRIO_MAX		32
#undef	_POSIX_NAME_MAX
#define	_POSIX_NAME_MAX			14
#undef	_POSIX_NGROUPS_MAX
#if defined(SU3)
#define	_POSIX_NGROUPS_MAX		8
#else	/* !SU3 */
#define	_POSIX_NGROUPS_MAX		0
#endif
#undef	_POSIX_OPEN_MAX
#if defined(SU3)
#define	_POSIX_OPEN_MAX			20
#else	/* !SU3 */
#define	_POSIX_OPEN_MAX			16
#endif
#undef	_POSIX_PATH_MAX
#if defined(SU3)
#define	_POSIX_PATH_MAX			256
#else	/* !SU3 */
#define	_POSIX_PATH_MAX			255
#endif	/* !SU3 */
#undef	_POSIX_PIPE_BUF
#define	_POSIX_PIPE_BUF			512
#undef	_POSIX_RE_DUP_MAX
#define	_POSIX_RE_DUP_MAX		255
#undef	_POSIX_RTSIG_MAX
#define	_POSIX_RTSIG_MAX		8
#undef	_POSIX_SEM_NSEMS_MAX
#define	_POSIX_SEM_NSEMS_MAX		256
#undef	_POSIX_SEM_VALUE_MAX
#define	_POSIX_SEM_VALUE_MAX		32767
#undef	_POSIX_SIGQUEUE_MAX
#define	_POSIX_SIGQUEUE_MAX		32
#undef	_POSIX_SSIZE_MAX
#define	_POSIX_SSIZE_MAX		32767
#undef	_POSIX_STREAM_MAX
#define	_POSIX_STREAM_MAX		8
#undef	_POSIX_SYMLINK_MAX
#define	_POSIX_SYMLINK_MAX		255
#undef	_POSIX_SYMLOOP_MAX
#define	_POSIX_SYMLOOP_MAX		8
#undef	_POSIX_THREAD_DESTRUCTOR_ITERATIONS
#define	_POSIX_THREAD_DESTRUCTOR_ITERATIONS	4
#undef	_POSIX_THREAD_KEYS_MAX
#define	_POSIX_THREAD_KEYS_MAX		128
#undef	_POSIX_THREAD_THREADS_MAX
#define	_POSIX_THREAD_THREADS_MAX	64
#undef	_POSIX_TIMER_MAX
#define	_POSIX_TIMER_MAX		32
#undef	_POSIX_TTY_NAME_MAX
#define	_POSIX_TTY_NAME_MAX		9
#undef	_POSIX_TZNAME_MAX
#if defined(SU3)
#define	_POSIX_TZNAME_MAX		6
#else	/* !SU3 */
#define	_POSIX_TZNAME_MAX		3
#endif	/* !SU3 */
#undef	_POSIX2_BC_BASE_MAX
#define	_POSIX2_BC_BASE_MAX		99
#undef	_POSIX2_BC_DIM_MAX
#define	_POSIX2_BC_DIM_MAX		2048
#undef	_POSIX2_BC_SCALE_MAX
#define	_POSIX2_BC_SCALE_MAX		99
#undef	_POSIX2_BC_STRING_MAX
#define	_POSIX2_BC_STRING_MAX		1000
#undef	_POSIX2_CHARCLASS_NAME_MAX
#define	_POSIX2_CHARCLASS_NAME_MAX	14
#undef	_POSIX2_COLL_WEIGHTS_MAX
#define	_POSIX2_COLL_WEIGHTS_MAX	2
#undef	_POSIX2_EXPR_NEST_MAX
#define	_POSIX2_EXPR_NEST_MAX		32
#undef	_POSIX2_LINE_MAX
#define	_POSIX2_LINE_MAX		2048
#undef	_POSIX2_RE_DUP_MAX
#define	_POSIX2_RE_DUP_MAX		255
#undef	_XOPEN_IOV_MAX
#define	_XOPEN_IOV_MAX			16
#undef	_XOPEN_NAME_MAX
#define	_XOPEN_NAME_MAX			255
#undef	_XOPEN_PATH_MAX
#define	_XOPEN_PATH_MAX			1024

extern size_t confstr(int, char *, size_t);

static int aflag = 0;

static const char	*progname;

#define	INVAL_ARG	"%s: Invalid argument (%s)\n"
#define	INVAL_PATHARG	"%s: Invalid argument (%s or %s)\n"

/*
 *  Notes:
 *  The sctab.value field is defined to be a long.
 *  There are some values that are "unsigned long"; these values
 *  can be stored in a "long" field but when output, must be printed
 *  as an unsigned value. Thus, these values must have UNSIGNED_VALUE bit
 *  set in sctab.flag field.
 *
 *  There are 2 different ways to indicate a symbol is undefined:
 *     1) sctab.flag = UNDEFINED
 *     2) or sctab.value = -1 (and if !UNDEFINED and !UNSIGNED_VALUE)
 *
 *  There are a group of symbols (e.g CHAR_MIN, INT_MAX, INT_MIN, LONG_BIT ...)
 *  which we may set to -1 if they are not pre-defined in a system header file.
 *  This is used to indicate that these symbols are "undefined".
 *  We are assuming that these symbols cannot reasonably have a value of -1
 *  if they were defined in a system header file.
 *  (Unless of course -1 can be used to indicate "undefined" for that symbol)
 */

static struct sctab {
	long value;
	char *name;
	enum { SELFCONF, SYSCONF, PATHCONF, CONFSTR, PATHVAR } type;
	int flag;
	long dfl;
/* bit fields for sctab.flag member */
#define	NOFLAGS		0	/* no special indicators */
#define	UNDEFINED	1	/* value is known undefined at compile time */
#define	UNSIGNED_VALUE	2	/* value is an unsigned */
} sctab[] = {
/*
 * Some entries in this structure have both leading _ and non leading _
 * versions for compatibility with various versions of the specifications.
 * It is not expected that both forms will be required for new entries.
 */

	/* POSIX.2-1992 table 2-17 */
{
	_POSIX2_BC_BASE_MAX,	"POSIX2_BC_BASE_MAX",	SELFCONF,   NOFLAGS
},
{
	_POSIX2_BC_BASE_MAX,	"_POSIX2_BC_BASE_MAX",	SELFCONF,  NOFLAGS
},
{
	_POSIX2_BC_DIM_MAX,	"POSIX2_BC_DIM_MAX",	SELFCONF,   NOFLAGS
},
{
	_POSIX2_BC_DIM_MAX,	"_POSIX2_BC_DIM_MAX",	SELFCONF,  NOFLAGS
},
{
	_POSIX2_BC_SCALE_MAX,	"POSIX2_BC_SCALE_MAX",	SELFCONF,   NOFLAGS
},
{
	_POSIX2_BC_SCALE_MAX,	"_POSIX2_BC_SCALE_MAX",	SELFCONF,  NOFLAGS
},
{
	_POSIX2_BC_STRING_MAX,	"POSIX2_BC_STRING_MAX",	SELFCONF,   NOFLAGS
},
{
	_POSIX2_BC_STRING_MAX,	"_POSIX2_BC_STRING_MAX", SELFCONF, NOFLAGS
},
{
	_POSIX2_COLL_WEIGHTS_MAX, "POSIX2_COLL_WEIGHTS_MAX", SELFCONF,  NOFLAGS
},
{
	_POSIX2_COLL_WEIGHTS_MAX, "_POSIX2_COLL_WEIGHTS_MAX", SELFCONF, NOFLAGS
},
{
	_POSIX2_EXPR_NEST_MAX,	"POSIX2_EXPR_NEST_MAX",	SELFCONF,   NOFLAGS
},
{
	_POSIX2_EXPR_NEST_MAX,	"_POSIX2_EXPR_NEST_MAX", SELFCONF,   NOFLAGS
},
{
	_POSIX2_LINE_MAX,	"POSIX2_LINE_MAX",	SELFCONF,   NOFLAGS
},
{
	_POSIX2_LINE_MAX,	"_POSIX2_LINE_MAX",	SELFCONF,  NOFLAGS
},
{
	_POSIX2_RE_DUP_MAX,	"POSIX2_RE_DUP_MAX",	SELFCONF,   NOFLAGS
},
{
	_POSIX2_RE_DUP_MAX,	"_POSIX2_RE_DUP_MAX",	SELFCONF,   NOFLAGS
},
	/* See limits.h section below for _POSIX_RE_DUP_MAX */

	/* UNIX 98 */
{
	_POSIX_AIO_LISTIO_MAX,	"_POSIX_AIO_LISTIO_MAX", SELFCONF, 	NOFLAGS
},
{
	_POSIX_AIO_MAX,		"_POSIX_AIO_MAX",	 SELFCONF, 	NOFLAGS
},
{
	_POSIX_DELAYTIMER_MAX,	"_POSIX_DELAYTIMER_MAX", SELFCONF, 	NOFLAGS
},
{
	_POSIX_LOGIN_NAME_MAX,	"_POSIX_LOGIN_NAME_MAX", SELFCONF, 	NOFLAGS
},
{
	_POSIX_MQ_OPEN_MAX,	"_POSIX_MQ_OPEN_MAX",	 SELFCONF, 	NOFLAGS
},
{
	_POSIX_MQ_PRIO_MAX,	"_POSIX_MQ_PRIO_MAX",	 SELFCONF, 	NOFLAGS
},
{
	_POSIX_RTSIG_MAX,	"_POSIX_RTSIG_MAX",	 SELFCONF,	NOFLAGS
},
{
	_POSIX_SEM_NSEMS_MAX,	"_POSIX_SEM_NSEMS_MAX",	 SELFCONF,	NOFLAGS
},
{
	_POSIX_SEM_VALUE_MAX,	"_POSIX_SEM_VALUE_MAX",	 SELFCONF,	NOFLAGS
},
{
	_POSIX_SIGQUEUE_MAX,	"_POSIX_SIGQUEUE_MAX",	 SELFCONF,	NOFLAGS
},
{
	_POSIX_TTY_NAME_MAX,	"_POSIX_TTY_NAME_MAX",	 SELFCONF,	NOFLAGS
},

	/* UNIX 03 */
{
	_POSIX_HOST_NAME_MAX,	"_POSIX_HOST_NAME_MAX",	  SELFCONF,   NOFLAGS
},
{
	_POSIX_RE_DUP_MAX,	"_POSIX_RE_DUP_MAX",	  SELFCONF,   NOFLAGS
},
{
	_POSIX_CLOCKRES_MIN,	"_POSIX_CLOCKRES_MIN",	  SELFCONF,   NOFLAGS
},
{
	_POSIX_SYMLINK_MAX,	"_POSIX_SYMLINK_MAX",	  SELFCONF,   NOFLAGS
},
{
	_POSIX_SYMLOOP_MAX,	"_POSIX_SYMLOOP_MAX",	  SELFCONF,   NOFLAGS
},
{
	_POSIX2_CHARCLASS_NAME_MAX, "_POSIX2_CHARCLASS_NAME_MAX", SELFCONF,   NOFLAGS
},

	/* POSIX.2-1992 table 2-17 */
{
	POSIX2_VERSION,		"POSIX2_VERSION",	SELFCONF,   NOFLAGS
},
{
	POSIX2_VERSION,		"_POSIX2_VERSION",	SELFCONF,   NOFLAGS
},

	/* POSIX.2-1992 table 2-18 */
{
	BC_BASE_MAX,		"BC_BASE_MAX",		SELFCONF,   NOFLAGS
},
{
	BC_DIM_MAX,		"BC_DIM_MAX",		SELFCONF,   NOFLAGS
},
{
	BC_SCALE_MAX,		"BC_SCALE_MAX",		SELFCONF,   NOFLAGS
},
{
	BC_STRING_MAX,		"BC_STRING_MAX",	SELFCONF,   NOFLAGS
},
#if defined(_SC_COLL_WEIGHTS_MAX)
{
	_SC_COLL_WEIGHTS_MAX,	"COLL_WEIGHTS_MAX",	SYSCONF,   NOFLAGS
},
#endif	/* _SC_COLL_WEIGHTS_MAX */
{
	EXPR_NEST_MAX,		"EXPR_NEST_MAX",	SELFCONF,   NOFLAGS
},
{
	LINE_MAX,		"LINE_MAX",		SELFCONF,   NOFLAGS
},
{
	RE_DUP_MAX,		"RE_DUP_MAX",		SELFCONF,   NOFLAGS
},

	/* POSIX.2-1992 table 2-19 */
#if defined(_SC_2_C_BIND)
{
	_SC_2_C_BIND,		"POSIX2_C_BIND",	SYSCONF,   NOFLAGS
},
#endif	/* _SC_2_C_BIND */
#if defined(_SC_2_C_BIND)
{
	_SC_2_C_BIND,		"_POSIX2_C_BIND",	SYSCONF,   NOFLAGS
},
#endif	/* _SC_2_C_BIND */
#if defined(_SC_2_C_DEV)
{
	_SC_2_C_DEV,		"POSIX2_C_DEV",		SYSCONF,   NOFLAGS
},
#endif	/* _SC_2_C_DEV */
#if defined(_SC_2_C_DEV)
{
	_SC_2_C_DEV,		"_POSIX2_C_DEV",	SYSCONF,   NOFLAGS
},
#endif	/* _SC_2_C_DEV */
#if defined(_SC_2_CHAR_TERM)
{
	_SC_2_CHAR_TERM,	"POSIX2_CHAR_TERM",	SYSCONF,   NOFLAGS
},
#endif	/* _SC_2_CHAR_TERM */
#if defined(_SC_2_CHAR_TERM)
{
	_SC_2_CHAR_TERM,	"_POSIX2_CHAR_TERM",	SYSCONF,   NOFLAGS
},
#endif	/* _SC_2_CHAR_TERM */
#if defined(_SC_2_FORT_DEV)
{
	_SC_2_FORT_DEV,		"POSIX2_FORT_DEV",	SYSCONF,   NOFLAGS
},
#endif	/* _SC_2_FORT_DEV */
#if defined(_SC_2_FORT_DEV)
{
	_SC_2_FORT_DEV,		"_POSIX2_FORT_DEV",	SYSCONF,   NOFLAGS
},
#endif	/* _SC_2_FORT_DEV */
#if defined(_SC_2_FORT_RUN)
{
	_SC_2_FORT_RUN,		"POSIX2_FORT_RUN",	SYSCONF,   NOFLAGS
},
#endif	/* _SC_2_FORT_RUN */
#if defined(_SC_2_FORT_RUN)
{
	_SC_2_FORT_RUN,		"_POSIX2_FORT_RUN",	SYSCONF,   NOFLAGS
},
#endif	/* _SC_2_FORT_RUN */
#if defined(_SC_2_LOCALEDEF)
{
	_SC_2_LOCALEDEF,	"POSIX2_LOCALEDEF",	SYSCONF,   NOFLAGS
},
#endif	/* _SC_2_LOCALEDEF */
#if defined(_SC_2_LOCALEDEF)
{
	_SC_2_LOCALEDEF,	"_POSIX2_LOCALEDEF",	SYSCONF,   NOFLAGS
},
#endif	/* _SC_2_LOCALEDEF */
#if defined(_SC_2_SW_DEV)
{
	_SC_2_SW_DEV,		"POSIX2_SW_DEV",	SYSCONF,   NOFLAGS
},
#endif	/* _SC_2_SW_DEV */
#if defined(_SC_2_SW_DEV)
{
	_SC_2_SW_DEV,		"_POSIX2_SW_DEV",	SYSCONF,   NOFLAGS
},
#endif	/* _SC_2_SW_DEV */
{
	POSIX2_UPE,		"POSIX2_UPE",		SELFCONF,   NOFLAGS
},
{
	POSIX2_UPE,		"_POSIX2_UPE",		SELFCONF,   NOFLAGS
},

	/* POSIX.1-1990 table 2-3 */
{
	_POSIX_ARG_MAX,		"_POSIX_ARG_MAX",	SELFCONF,   NOFLAGS
},
{
	_POSIX_CHILD_MAX,	"_POSIX_CHILD_MAX",	SELFCONF,   NOFLAGS
},
{
	_POSIX_LINK_MAX,	"_POSIX_LINK_MAX",	SELFCONF,   NOFLAGS
},
{
	_POSIX_MAX_CANON,	"_POSIX_MAX_CANON",	SELFCONF,   NOFLAGS
},
{
	_POSIX_MAX_INPUT,	"_POSIX_MAX_INPUT",	SELFCONF,   NOFLAGS
},
{
	_POSIX_NAME_MAX,	"_POSIX_NAME_MAX",	SELFCONF,   NOFLAGS
},
{
	_POSIX_NGROUPS_MAX,	"_POSIX_NGROUPS_MAX",	SELFCONF,   NOFLAGS
},
{
	_POSIX_OPEN_MAX,	"_POSIX_OPEN_MAX",	SELFCONF,   NOFLAGS
},
{
	_POSIX_PATH_MAX,	"_POSIX_PATH_MAX",	SELFCONF,   NOFLAGS
},
{
	_POSIX_PIPE_BUF,	"_POSIX_PIPE_BUF",	SELFCONF,   NOFLAGS
},
{
	_POSIX_SSIZE_MAX,	"_POSIX_SSIZE_MAX",	SELFCONF,   NOFLAGS
},
{
	_POSIX_STREAM_MAX,	"_POSIX_STREAM_MAX",	SELFCONF,   NOFLAGS
},
{
	_POSIX_TIMER_MAX,	"_POSIX_TIMER_MAX",	SELFCONF,   NOFLAGS
},
{
	_POSIX_THREAD_DESTRUCTOR_ITERATIONS,
	"_POSIX_THREAD_DESTRUCTOR_ITERATIONS",	SELFCONF,   NOFLAGS
},
{
	_POSIX_THREAD_KEYS_MAX,
	"_POSIX_THREAD_KEYS_MAX",		SELFCONF,   NOFLAGS
},
{
	_POSIX_THREAD_THREADS_MAX,
	"_POSIX_THREAD_THREADS_MAX",		SELFCONF,   NOFLAGS
},
{
	_POSIX_TZNAME_MAX,	"_POSIX_TZNAME_MAX",	SELFCONF,   NOFLAGS
},

	/* POSIX.1-1990 table 4-2 */
#if defined(_SC_ARG_MAX)
{
	_SC_ARG_MAX,		"ARG_MAX",		SYSCONF,   NOFLAGS
},
#endif	/* _SC_ARG_MAX */
#if defined(_SC_CHILD_MAX)
{
	_SC_CHILD_MAX,		"CHILD_MAX",		SYSCONF,   NOFLAGS
},
#endif	/* _SC_CHILD_MAX */
#if defined(_SC_CLK_TCK)
{
	_SC_CLK_TCK,		"CLK_TCK",		SYSCONF,   NOFLAGS
},
#endif	/* _SC_CLK_TCK */
#if defined(_SC_JOB_CONTROL)
{
	_SC_JOB_CONTROL,	"_POSIX_JOB_CONTROL",	SYSCONF,   NOFLAGS
},
#endif	/* _SC_JOB_CONTROL */
#if defined(_SC_NGROUPS_MAX)
{
	_SC_NGROUPS_MAX,	"NGROUPS_MAX",		SYSCONF,   NOFLAGS
},
#endif	/* _SC_NGROUPS_MAX */
#if defined(_SC_OPEN_MAX)
{
	_SC_OPEN_MAX,		"OPEN_MAX",		SYSCONF,   NOFLAGS
},
#endif	/* _SC_OPEN_MAX */
#if defined(_SC_SAVED_IDS)
{
	_SC_SAVED_IDS,		"_POSIX_SAVED_IDS",	SYSCONF,   NOFLAGS
},
#endif	/* _SC_SAVED_IDS */
#if defined(_SC_STREAM_MAX)
{
	_SC_STREAM_MAX,		"STREAM_MAX",		SYSCONF,   NOFLAGS
},
#endif	/* _SC_STREAM_MAX */
#if defined(_SC_TZNAME_MAX)
{
	_SC_TZNAME_MAX,		"TZNAME_MAX",		SYSCONF,   NOFLAGS
},
#endif	/* _SC_TZNAME_MAX */
#if defined(_SC_VERSION)
{
	_SC_VERSION,		"_POSIX_VERSION",	SYSCONF,   NOFLAGS
},
#endif	/* _SC_VERSION */

	/* pathconf() variables */

	/* POSIX.1-1990 table 5-2 */

#if defined(_PC_LINK_MAX)
{
	_PC_LINK_MAX,		"LINK_MAX",		PATHCONF,  NOFLAGS
},
#endif	/* _PC_LINK_MAX */
#if defined(_PC_MAX_CANON)
{
	_PC_MAX_CANON,		"MAX_CANON",		PATHCONF,  NOFLAGS
},
#endif	/* _PC_MAX_CANON */
#if defined(_PC_MAX_INPUT)
{
	_PC_MAX_INPUT,		"MAX_INPUT",		PATHCONF,  NOFLAGS
},
#endif	/* _PC_MAX_INPUT */
#if defined(_PC_NAME_MAX)
{
	_PC_NAME_MAX,		"NAME_MAX",		PATHCONF,  NOFLAGS
},
#endif	/* _PC_NAME_MAX */
#if defined(_PC_PATH_MAX)
{
	_PC_PATH_MAX,		"PATH_MAX",		PATHCONF,  NOFLAGS
},
#endif	/* _PC_PATH_MAX */
#if defined(_PC_PIPE_BUF)
{
	_PC_PIPE_BUF,		"PIPE_BUF",		PATHCONF,  NOFLAGS
},
#endif	/* _PC_PIPE_BUF */
#if defined(_PC_CHOWN_RESTRICTED)
{
	_PC_CHOWN_RESTRICTED,	"_POSIX_CHOWN_RESTRICTED", PATHCONF,  NOFLAGS
},
#endif	/* _PC_CHOWN_RESTRICTED */
#if defined(_PC_NO_TRUNC)
{
	_PC_NO_TRUNC,		"_POSIX_NO_TRUNC",	PATHCONF,  NOFLAGS
},
#endif	/* _PC_NO_TRUNC */
#if defined(_PC_VDISABLE)
{
	_PC_VDISABLE,		"_POSIX_VDISABLE",	PATHCONF,  NOFLAGS
},
#endif	/* _PC_VDISABLE */

	/* UNIX 98 additions */

#if defined(_PC_ASYNC_IO)
{
	_PC_ASYNC_IO,		"_POSIX_ASYNC_IO",	PATHCONF,  NOFLAGS
},
#endif	/* _PC_ASYNC_IO */
#if defined(_PC_PRIO_IO)
{
	_PC_PRIO_IO,		"_POSIX_PRIO_IO",	PATHCONF,  NOFLAGS
},
#endif	/* _PC_PRIO_IO */
#if defined(_PC_SYNC_IO)
{
	_PC_SYNC_IO,		"_POSIX_SYNC_IO",	PATHCONF,  NOFLAGS
},
#endif	/* _PC_SYNC_IO */

	/* Large File Summit name (UNIX 98) */

#if defined(_PC_FILESIZEBITS)
{
	_PC_FILESIZEBITS,	"FILESIZEBITS",		PATHCONF,  NOFLAGS
},
#endif	/* _PC_FILESIZEBITS */

	/* UNIX 03 Additions */

	/* Not supporting the Advisory Information (ADV) at this time.  */
#if defined(_PC_ALLOC_SIZE_MIN)
{
	_PC_ALLOC_SIZE_MIN,	"POSIX_ALLOC_SIZE_MIN",	PATHCONF,  NOFLAGS
},
#endif	/* _PC_ALLOC_SIZE_MIN */
#if defined(_PC_REC_INCR_XFER_SIZE)
{
	_PC_REC_INCR_XFER_SIZE,	"POSIX_REC_INCR_XFER_SIZE", PATHCONF,  NOFLAGS
},
#endif	/* _PC_REC_INCR_XFER_SIZE */
#if defined(_PC_REC_MAX_XFER_SIZE)
{
	_PC_REC_MAX_XFER_SIZE,	"POSIX_REC_MAX_XFER_SIZE",  PATHCONF,  NOFLAGS
},
#endif	/* _PC_REC_MAX_XFER_SIZE */
#if defined(_PC_REC_MIN_XFER_SIZE)
{
	_PC_REC_MIN_XFER_SIZE,	"POSIX_REC_MIN_XFER_SIZE",  PATHCONF,  NOFLAGS
},
#endif	/* _PC_REC_MIN_XFER_SIZE */
#if defined(_PC_REC_XFER_ALIGN)
{
	_PC_REC_XFER_ALIGN,	"POSIX_REC_XFER_ALIGN",	PATHCONF,  NOFLAGS
},
#endif	/* _PC_REC_XFER_ALIGN */

#if defined(_PC_2_SYMLINKS)
{
	_PC_2_SYMLINKS,		"POSIX2_SYMLINKS",	PATHCONF,  NOFLAGS
},
#endif	/* _PC_2_SYMLINKS */
#if defined(_PC_SYMLINK_MAX)
{
	_PC_SYMLINK_MAX,	"SYMLINK_MAX",		PATHCONF,  NOFLAGS
},
#endif	/* _PC_SYMLINK_MAX */

	/* confstr() variables */

	/* POSIX.2-1992 description of getconf utility */
{
	0,			"PATH",			PATHVAR,   NOFLAGS
},

	/* POSIX.2-1992 table B-18 */
{
	0,			"CS_PATH",		PATHVAR,   NOFLAGS
},
{
	0,			"_CS_PATH",		PATHVAR,   NOFLAGS
},

	/* command names for large file configuration information */
	/* large file compilation environment configuration */

#if defined(_CS_LFS_CFLAGS)
{
	_CS_LFS_CFLAGS,		"LFS_CFLAGS",		CONFSTR,   NOFLAGS
},
#endif	/* _CS_LFS_CFLAGS */
#if defined(_CS_LFS_LDFLAGS)
{
	_CS_LFS_LDFLAGS,	"LFS_LDFLAGS",		CONFSTR,   NOFLAGS
},
#endif	/* _CS_LFS_LDFLAGS */
#if defined(_CS_LFS_LIBS)
{
	_CS_LFS_LIBS,		"LFS_LIBS",		CONFSTR,   NOFLAGS
},
#endif	/* _CS_LFS_LIBS */
#if defined(_CS_LFS_LINTFLAGS)
{
	_CS_LFS_LINTFLAGS,	"LFS_LINTFLAGS",	CONFSTR,   NOFLAGS
},
#endif	/* _CS_LFS_LINTFLAGS */

	/* transitional large file interface configuration */

#if defined(_CS_LFS64_CFLAGS)
{
	_CS_LFS64_CFLAGS,	"LFS64_CFLAGS",		CONFSTR,   NOFLAGS
},
#endif	/* _CS_LFS64_CFLAGS */
#if defined(_CS_LFS64_LDFLAGS)
{
	_CS_LFS64_LDFLAGS,	"LFS64_LDFLAGS",	CONFSTR,   NOFLAGS
},
#endif	/* _CS_LFS64_LDFLAGS */
#if defined(_CS_LFS64_LIBS)
{
	_CS_LFS64_LIBS,		"LFS64_LIBS",		CONFSTR,   NOFLAGS
},
#endif	/* _CS_LFS64_LIBS */
#if defined(_CS_LFS64_LINTFLAGS)
{
	_CS_LFS64_LINTFLAGS,	"LFS64_LINTFLAGS",	CONFSTR,   NOFLAGS
},
#endif	/* _CS_LFS64_LINTFLAGS */

	/*
	 *  UNIX 98:
	 *  confstr additions, these support
	 *  programming specifications/environments.
	 *
	 * Unfortunately there is some uncertainty with
	 * the standard at this point about the constant
	 * name, thus we need to recognize BOTH with and
	 * without the initial underscore.
	 */
#if defined(_CS_XBS5_ILP32_OFF32_CFLAGS)
{
	_CS_XBS5_ILP32_OFF32_CFLAGS,	"XBS5_ILP32_OFF32_CFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_ILP32_OFF32_CFLAGS */
#if defined(_CS_XBS5_ILP32_OFF32_CFLAGS)
{
	_CS_XBS5_ILP32_OFF32_CFLAGS,	"_XBS5_ILP32_OFF32_CFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_ILP32_OFF32_CFLAGS */

#if defined(_CS_XBS5_ILP32_OFF32_LDFLAGS)
{
	_CS_XBS5_ILP32_OFF32_LDFLAGS,	"XBS5_ILP32_OFF32_LDFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_ILP32_OFF32_LDFLAGS */
#if defined(_CS_XBS5_ILP32_OFF32_LDFLAGS)
{
	_CS_XBS5_ILP32_OFF32_LDFLAGS,	"_XBS5_ILP32_OFF32_LDFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_ILP32_OFF32_LDFLAGS */

#if defined(_CS_XBS5_ILP32_OFF32_LIBS)
{
	_CS_XBS5_ILP32_OFF32_LIBS,	"XBS5_ILP32_OFF32_LIBS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_ILP32_OFF32_LIBS */
#if defined(_CS_XBS5_ILP32_OFF32_LIBS)
{
	_CS_XBS5_ILP32_OFF32_LIBS,	"_XBS5_ILP32_OFF32_LIBS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_ILP32_OFF32_LIBS */

#if defined(_CS_XBS5_ILP32_OFF32_LINTFLAGS)
{
	_CS_XBS5_ILP32_OFF32_LINTFLAGS,	"XBS5_ILP32_OFF32_LINTFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_ILP32_OFF32_LINTFLAGS */
#if defined(_CS_XBS5_ILP32_OFF32_LINTFLAGS)
{
	_CS_XBS5_ILP32_OFF32_LINTFLAGS,	"_XBS5_ILP32_OFF32_LINTFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_ILP32_OFF32_LINTFLAGS */

#if defined(_CS_XBS5_ILP32_OFFBIG_CFLAGS)
{
	_CS_XBS5_ILP32_OFFBIG_CFLAGS,	"XBS5_ILP32_OFFBIG_CFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_ILP32_OFFBIG_CFLAGS */
#if defined(_CS_XBS5_ILP32_OFFBIG_CFLAGS)
{
	_CS_XBS5_ILP32_OFFBIG_CFLAGS,	"_XBS5_ILP32_OFFBIG_CFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_ILP32_OFFBIG_CFLAGS */

#if defined(_CS_XBS5_ILP32_OFFBIG_LDFLAGS)
{
	_CS_XBS5_ILP32_OFFBIG_LDFLAGS,	"XBS5_ILP32_OFFBIG_LDFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_ILP32_OFFBIG_LDFLAGS */
#if defined(_CS_XBS5_ILP32_OFFBIG_LDFLAGS)
{
	_CS_XBS5_ILP32_OFFBIG_LDFLAGS,	"_XBS5_ILP32_OFFBIG_LDFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_ILP32_OFFBIG_LDFLAGS */

#if defined(_CS_XBS5_ILP32_OFFBIG_LIBS)
{
	_CS_XBS5_ILP32_OFFBIG_LIBS,	"XBS5_ILP32_OFFBIG_LIBS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_ILP32_OFFBIG_LIBS */
#if defined(_CS_XBS5_ILP32_OFFBIG_LIBS)
{
	_CS_XBS5_ILP32_OFFBIG_LIBS,	"_XBS5_ILP32_OFFBIG_LIBS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_ILP32_OFFBIG_LIBS */

#if defined(_CS_XBS5_ILP32_OFFBIG_LINTFLAGS)
{
	_CS_XBS5_ILP32_OFFBIG_LINTFLAGS, "XBS5_ILP32_OFFBIG_LINTFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_ILP32_OFFBIG_LINTFLAGS */
#if defined(_CS_XBS5_ILP32_OFFBIG_LINTFLAGS)
{
	_CS_XBS5_ILP32_OFFBIG_LINTFLAGS, "_XBS5_ILP32_OFFBIG_LINTFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_ILP32_OFFBIG_LINTFLAGS */

#if defined(_CS_XBS5_LP64_OFF64_CFLAGS)
{
	_CS_XBS5_LP64_OFF64_CFLAGS,	"XBS5_LP64_OFF64_CFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_LP64_OFF64_CFLAGS */
#if defined(_CS_XBS5_LP64_OFF64_CFLAGS)
{
	_CS_XBS5_LP64_OFF64_CFLAGS,	"_XBS5_LP64_OFF64_CFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_LP64_OFF64_CFLAGS */

#if defined(_CS_XBS5_LP64_OFF64_LDFLAGS)
{
	_CS_XBS5_LP64_OFF64_LDFLAGS,	"XBS5_LP64_OFF64_LDFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_LP64_OFF64_LDFLAGS */
#if defined(_CS_XBS5_LP64_OFF64_LDFLAGS)
{
	_CS_XBS5_LP64_OFF64_LDFLAGS,	"_XBS5_LP64_OFF64_LDFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_LP64_OFF64_LDFLAGS */

#if defined(_CS_XBS5_LP64_OFF64_LIBS)
{
	_CS_XBS5_LP64_OFF64_LIBS,	"XBS5_LP64_OFF64_LIBS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_LP64_OFF64_LIBS */
#if defined(_CS_XBS5_LP64_OFF64_LIBS)
{
	_CS_XBS5_LP64_OFF64_LIBS,	"_XBS5_LP64_OFF64_LIBS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_LP64_OFF64_LIBS */

#if defined(_CS_XBS5_LP64_OFF64_LINTFLAGS)
{
	_CS_XBS5_LP64_OFF64_LINTFLAGS,	"XBS5_LP64_OFF64_LINTFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_LP64_OFF64_LINTFLAGS */
#if defined(_CS_XBS5_LP64_OFF64_LINTFLAGS)
{
	_CS_XBS5_LP64_OFF64_LINTFLAGS,	"_XBS5_LP64_OFF64_LINTFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_LP64_OFF64_LINTFLAGS */

#if defined(_CS_XBS5_LPBIG_OFFBIG_CFLAGS)
{
	_CS_XBS5_LPBIG_OFFBIG_CFLAGS,	"XBS5_LPBIG_OFFBIG_CFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_LPBIG_OFFBIG_CFLAGS */
#if defined(_CS_XBS5_LPBIG_OFFBIG_CFLAGS)
{
	_CS_XBS5_LPBIG_OFFBIG_CFLAGS,	"_XBS5_LPBIG_OFFBIG_CFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_LPBIG_OFFBIG_CFLAGS */

#if defined(_CS_XBS5_LPBIG_OFFBIG_LDFLAGS)
{
	_CS_XBS5_LPBIG_OFFBIG_LDFLAGS,	"XBS5_LPBIG_OFFBIG_LDFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_LPBIG_OFFBIG_LDFLAGS */
#if defined(_CS_XBS5_LPBIG_OFFBIG_LDFLAGS)
{
	_CS_XBS5_LPBIG_OFFBIG_LDFLAGS,	"_XBS5_LPBIG_OFFBIG_LDFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_LPBIG_OFFBIG_LDFLAGS */

#if defined(_CS_XBS5_LPBIG_OFFBIG_LIBS)
{
	_CS_XBS5_LPBIG_OFFBIG_LIBS,	"XBS5_LPBIG_OFFBIG_LIBS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_LPBIG_OFFBIG_LIBS */
#if defined(_CS_XBS5_LPBIG_OFFBIG_LIBS)
{
	_CS_XBS5_LPBIG_OFFBIG_LIBS,	"_XBS5_LPBIG_OFFBIG_LIBS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_LPBIG_OFFBIG_LIBS */

#if defined(_CS_XBS5_LPBIG_OFFBIG_LINTFLAGS)
{
	_CS_XBS5_LPBIG_OFFBIG_LINTFLAGS, "XBS5_LPBIG_OFFBIG_LINTFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_LPBIG_OFFBIG_LINTFLAGS */
#if defined(_CS_XBS5_LPBIG_OFFBIG_LINTFLAGS)
{
	_CS_XBS5_LPBIG_OFFBIG_LINTFLAGS, "_XBS5_LPBIG_OFFBIG_LINTFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_XBS5_LPBIG_OFFBIG_LINTFLAGS */

	/*
	 * UNIX 03 confstr() additions for compilation environments
	 */
#if defined(_CS_POSIX_V6_ILP32_OFF32_CFLAGS)
{
	_CS_POSIX_V6_ILP32_OFF32_CFLAGS, "POSIX_V6_ILP32_OFF32_CFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_POSIX_V6_ILP32_OFF32_CFLAGS */
#if defined(_CS_POSIX_V6_ILP32_OFF32_LDFLAGS)
{
	_CS_POSIX_V6_ILP32_OFF32_LDFLAGS, "POSIX_V6_ILP32_OFF32_LDFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_POSIX_V6_ILP32_OFF32_LDFLAGS */
#if defined(_CS_POSIX_V6_ILP32_OFF32_LIBS)
{
	_CS_POSIX_V6_ILP32_OFF32_LIBS, "POSIX_V6_ILP32_OFF32_LIBS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_POSIX_V6_ILP32_OFF32_LIBS */
#if defined(_CS_POSIX_V6_ILP32_OFFBIG_CFLAGS)
{
	_CS_POSIX_V6_ILP32_OFFBIG_CFLAGS, "POSIX_V6_ILP32_OFFBIG_CFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_POSIX_V6_ILP32_OFFBIG_CFLAGS */
#if defined(_CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS)
{
	_CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS, "POSIX_V6_ILP32_OFFBIG_LDFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS */
#if defined(_CS_POSIX_V6_ILP32_OFFBIG_LIBS)
{
	_CS_POSIX_V6_ILP32_OFFBIG_LIBS, "POSIX_V6_ILP32_OFFBIG_LIBS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_POSIX_V6_ILP32_OFFBIG_LIBS */
#if defined(_CS_POSIX_V6_LP64_OFF64_CFLAGS)
{
	_CS_POSIX_V6_LP64_OFF64_CFLAGS, "POSIX_V6_LP64_OFF64_CFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_POSIX_V6_LP64_OFF64_CFLAGS */
#if defined(_CS_POSIX_V6_LP64_OFF64_LDFLAGS)
{
	_CS_POSIX_V6_LP64_OFF64_LDFLAGS, "POSIX_V6_LP64_OFF64_LDFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_POSIX_V6_LP64_OFF64_LDFLAGS */
#if defined(_CS_POSIX_V6_LP64_OFF64_LIBS)
{
	_CS_POSIX_V6_LP64_OFF64_LIBS, "POSIX_V6_LP64_OFF64_LIBS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_POSIX_V6_LP64_OFF64_LIBS */
#if defined(_CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS)
{
	_CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS, "POSIX_V6_LPBIG_OFFBIG_CFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS */
#if defined(_CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS)
{
	_CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS, "POSIX_V6_LPBIG_OFFBIG_LDFLAGS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS */
#if defined(_CS_POSIX_V6_LPBIG_OFFBIG_LIBS)
{
	_CS_POSIX_V6_LPBIG_OFFBIG_LIBS, "POSIX_V6_LPBIG_OFFBIG_LIBS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_POSIX_V6_LPBIG_OFFBIG_LIBS */
#if defined(_CS_POSIX_V6_WIDTH_RESTRICTED_ENVS)
{
	_CS_POSIX_V6_WIDTH_RESTRICTED_ENVS, "POSIX_V6_WIDTH_RESTRICTED_ENVS",
	CONFSTR,	NOFLAGS
},
#endif	/* _CS_POSIX_V6_WIDTH_RESTRICTED_ENVS */
	/* End of UNIX 03 compilation environments */

#if defined(_SC_2_C_VERSION)
	/* POSIX.2 table B-4 */
{
	_SC_2_C_VERSION,	"_POSIX2_C_VERSION",	SYSCONF,   NOFLAGS
},
#elif defined(_POSIX2_C_VERSION)
{
	_POSIX2_C_VERSION,	"_POSIX2_C_VERSION",	SELFCONF,   NOFLAGS
},
#endif	/* _SC_2_C_VERSION, _POSIX2_C_VERSION */

/*
 * XPG4 support BEGINS
 */

#if !defined(CHARCLASS_NAME_MAX)
#define	CHARCLASS_NAME_MAX	-1
#endif
#if !defined(CHAR_BIT)
#define	CHAR_BIT	-1
#endif
#if !defined(CHAR_MAX)
#define	CHAR_MAX	-1
#endif
#if !defined(CHAR_MIN)
#define	CHAR_MIN	-1
#endif
#if !defined(INT_MAX)
#define	INT_MAX	-1
#endif
#if !defined(INT_MIN)
#define	INT_MIN	-1
#endif
#if !defined(LONG_BIT)
#define	LONG_BIT	-1
#endif
#if !defined(LONG_MAX)
#define	LONG_MAX	-1
#endif
#if !defined(LONG_MIN)
#define	LONG_MIN	-1
#endif
#if !defined(MB_LEN_MAX)
#define	MB_LEN_MAX	-1
#endif
#if !defined(NL_NMAX)
#define	NL_NMAX	-1
#endif
#if !defined(NL_ARGMAX)
#define	NL_ARGMAX	-1
#endif
#if !defined(NL_LANGMAX)
#define	NL_LANGMAX	-1
#endif
#if !defined(NL_MSGMAX)
#define	NL_MSGMAX	-1
#endif
#if !defined(NL_SETMAX)
#define	NL_SETMAX	-1
#endif
#if !defined(NL_TEXTMAX)
#define	NL_TEXTMAX	-1
#endif
#if !defined(NZERO)
#define	NZERO	-1
#endif
#if !defined(SCHAR_MAX)
#define	SCHAR_MAX	-1
#endif
#if !defined(SCHAR_MIN)
#define	SCHAR_MIN	-1
#endif
#if !defined(SHRT_MAX)
#define	SHRT_MAX	-1
#endif
#if !defined(SHRT_MIN)
#define	SHRT_MIN	-1
#endif
#if !defined(TMP_MAX)
#define	TMP_MAX	-1
#endif
#if !defined(WORD_BIT)
#define	WORD_BIT	-1
#endif


#if !defined(_XOPEN_XPG2)
#define	_XOPEN_XPG2	-1
#endif
#if !defined(_XOPEN_XPG3)
#define	_XOPEN_XPG3	-1
#endif
#if !defined(_XOPEN_XPG4)
#define	_XOPEN_XPG4	-1
#endif

	/*
	 * the following are values that we should find in <limits.h>
	 * so we use SELFCONF here.
	 *
	 */

	/* POSIX.1-1990 */
#if defined(CHARCLASS_NAME_MAX)
{
	CHARCLASS_NAME_MAX,	"CHARCLASS_NAME_MAX",	  SELFCONF,   NOFLAGS
},
#endif	/* CHARCLASS_NAME_MAX */
{
	_POSIX_SSIZE_MAX,	"SSIZE_MAX",	SELFCONF,   NOFLAGS
},

	/* POSIX.1-1988 */
#if defined(CHAR_BIT)
{
	CHAR_BIT,		"CHAR_BIT",	SELFCONF,   NOFLAGS
},
#endif	/* CHAR_BIT */
#if defined(CHAR_MAX)
{
	CHAR_MAX,		"CHAR_MAX",	SELFCONF,   NOFLAGS
},
#endif	/* CHAR_MAX */
#if defined(CHAR_MIN)
{
	CHAR_MIN,		"CHAR_MIN",	SELFCONF,   NOFLAGS
},
#endif	/* CHAR_MIN */
#if defined(INT_MAX)
{
	INT_MAX,		"INT_MAX",	SELFCONF,   NOFLAGS
},
#endif	/* INT_MAX */
#if defined(INT_MIN)
{
	INT_MIN,		"INT_MIN",	SELFCONF,   NOFLAGS
},
#endif	/* INT_MIN */
#if defined(LONG_BIT)
{
	LONG_BIT,		"LONG_BIT",	SELFCONF,   NOFLAGS
},
#endif	/* LONG_BIT */
#if defined(LONG_MAX)
{
	LONG_MAX,		"LONG_MAX",	SELFCONF,   NOFLAGS
},
#endif	/* LONG_MAX */
#if defined(LONG_MIN)
{
	LONG_MIN,		"LONG_MIN",	SELFCONF,   NOFLAGS
},
#endif	/* LONG_MIN */
#if defined(MB_LEN_MAX)
{
	MB_LEN_MAX,		"MB_LEN_MAX",	SELFCONF,   NOFLAGS
},
#endif	/* MB_LEN_MAX */
#if defined(NL_NMAX)
{
	NL_NMAX,		"NL_NMAX",	SELFCONF,   NOFLAGS
},
#endif	/* NL_NMAX */
#if defined(NL_ARGMAX)
{
	NL_ARGMAX,		"NL_ARGMAX",	SELFCONF,   NOFLAGS
},
#endif	/* NL_ARGMAX */
#if defined(NL_LANGMAX)
{
	NL_LANGMAX,		"NL_LANGMAX",	SELFCONF,   NOFLAGS
},
#endif	/* NL_LANGMAX */
#if defined(NL_MSGMAX)
{
	NL_MSGMAX,		"NL_MSGMAX",	SELFCONF,   NOFLAGS
},
#endif	/* NL_MSGMAX */
#if defined(NL_SETMAX)
{
	NL_SETMAX,		"NL_SETMAX",	SELFCONF,   NOFLAGS
},
#endif	/* NL_SETMAX */
#if defined(NL_TEXTMAX)
{
	NL_TEXTMAX,		"NL_TEXTMAX",	SELFCONF,   NOFLAGS
},
#endif	/* NL_TEXTMAX */
#if defined(NZERO)
{
	NZERO,			"NZERO",	SELFCONF,   NOFLAGS
},
#endif	/* NZERO */
#if defined(SCHAR_MAX)
{
	SCHAR_MAX,		"SCHAR_MAX",	SELFCONF,   NOFLAGS
},
#endif	/* SCHAR_MAX */
#if defined(SCHAR_MIN)
{
	SCHAR_MIN,		"SCHAR_MIN",	SELFCONF,   NOFLAGS
},
#endif	/* SCHAR_MIN */
#if defined(SHRT_MAX)
{
	SHRT_MAX,		"SHRT_MAX",	SELFCONF,   NOFLAGS
},
#endif	/* SHRT_MAX */
#if defined(SHRT_MIN)
{
	SHRT_MIN,		"SHRT_MIN",	SELFCONF,   NOFLAGS
},
#endif	/* SHRT_MIN */
#if defined(TMP_MAX)
{
	TMP_MAX,		"TMP_MAX",	SELFCONF,   NOFLAGS
},
#endif	/* TMP_MAX */

	/*
	 * for the unsigned maximums, we cannot rely on the value -1
	 * to indicate "undefined".
	 */
#if !defined(UCHAR_MAX)
#if defined(0)
{
	0,			"UCHAR_MAX",	SELFCONF,   UNDEFINED
},
#endif	/* 0 */
#else
#if defined(UCHAR_MAX)
{
	(long)UCHAR_MAX,	"UCHAR_MAX",	SELFCONF,   UNSIGNED_VALUE
},
#endif	/* UCHAR_MAX */
#endif /* UCHAR_MAX */
#if !defined(UINT_MAX)
#if defined(0)
{
	0,			"UINT_MAX",	SELFCONF,   UNDEFINED
},
#endif	/* 0 */
#else
#if defined(UINT_MAX)
{
	(long)UINT_MAX,		"UINT_MAX",	SELFCONF,   UNSIGNED_VALUE
},
#endif	/* UINT_MAX */
#endif /* UINT_MAX */
#if !defined(ULONG_MAX)
#if defined(0)
{
	0,			"ULONG_MAX",	SELFCONF,   UNDEFINED
},
#endif	/* 0 */
#else
#if defined(ULONG_MAX)
{
	(long)ULONG_MAX,	"ULONG_MAX",	SELFCONF,   UNSIGNED_VALUE
},
#endif	/* ULONG_MAX */
#endif /* ULONG_MAX */
#if !defined(USHRT_MAX)
#if defined(0)
{
	0,			"USHRT_MAX",	SELFCONF,   UNDEFINED
},
#endif	/* 0 */
#else
#if defined(USHRT_MAX)
{
	(long)USHRT_MAX,	"USHRT_MAX",	SELFCONF,   UNSIGNED_VALUE
},
#endif	/* USHRT_MAX */
#endif /* USHRT_MAX */
#if !defined(SIZE_MAX)
{
	0,			"SIZE_MAX",	SELFCONF,   UNDEFINED
},
#else
#if defined(SIZE_MAX)
{
	(long)SIZE_MAX,		"SIZE_MAX",	SELFCONF,   UNSIGNED_VALUE
},
#endif	/* SIZE_MAX */
#endif /* SIZE_MAX */

#if defined(WORD_BIT)
{
	WORD_BIT,		"WORD_BIT",	SELFCONF,   NOFLAGS
},
#endif	/* WORD_BIT */

	/* XPG4 Version 2 */
{
	_XOPEN_IOV_MAX,		"_XOPEN_IOV_MAX",  SELFCONF, NOFLAGS
},

	/* UNIX 03 */
{
	_XOPEN_NAME_MAX,	"_XOPEN_NAME_MAX", SELFCONF, NOFLAGS
},
{
	_XOPEN_PATH_MAX,	"_XOPEN_PATH_MAX", SELFCONF, NOFLAGS
},

	/*
	 * The following are defined via sysconf().  These are considered
	 * an extension to sysconf().
	 * The -0 is needed if _XOPEN_XPG? is defined to be an empty string.
	 */
#if defined(_XOPEN_XPG2)
{
	_XOPEN_XPG2-0,		"_XOPEN_XPG2",	SELFCONF,    NOFLAGS
},
#endif	/* _XOPEN_XPG2 */
#if defined(_XOPEN_XPG3)
{
	_XOPEN_XPG3-0,		"_XOPEN_XPG3",	SELFCONF,    NOFLAGS
},
#endif	/* _XOPEN_XPG3 */
#if defined(_XOPEN_XPG4)
{
	_XOPEN_XPG4-0,		"_XOPEN_XPG4",	SELFCONF,    NOFLAGS
},
#endif	/* _XOPEN_XPG4 */

	/*
	 * The following should be provided by sysconf() (e.g use SYSCONF),
	 * so we  look for the appropriate _SC_* symbol in <unistd.h>.
	 * If it is not defined, then we use SELFCONF with the value of -1.
	 */
#if defined(_SC_XOPEN_VERSION)
{
	_SC_XOPEN_VERSION,	"_XOPEN_VERSION",	SYSCONF,   NOFLAGS
},
#endif	/* _SC_XOPEN_VERSION */
{
	_XOPEN_XCU_VERSION,	"_XOPEN_XCU_VERSION",	SELFCONF,  NOFLAGS
},
#if defined(_SC_XOPEN_CRYPT)
{
	_SC_XOPEN_CRYPT,	"_XOPEN_CRYPT",		SYSCONF,   NOFLAGS
},
#endif	/* _SC_XOPEN_CRYPT */
#if defined(_SC_XOPEN_ENH_I18N)
{
	_SC_XOPEN_ENH_I18N,	"_XOPEN_ENH_I18N",	SYSCONF,   NOFLAGS
},
#endif	/* _SC_XOPEN_ENH_I18N */
#if defined(_SC_XOPEN_SHM)
{
	_SC_XOPEN_SHM,		"_XOPEN_SHM",		SYSCONF,   NOFLAGS
},
#endif	/* _SC_XOPEN_SHM */

/*
 * XPG4 support ends
 */

/* XPG4, Version 2 support */

#if defined(_SC_ATEXIT_MAX)
{
	_SC_ATEXIT_MAX, 	"ATEXIT_MAX",		SYSCONF,	NOFLAGS
},
#endif	/* _SC_ATEXIT_MAX */
#if defined(_SC_PAGESIZE)
{
	_SC_PAGESIZE,		"PAGESIZE",		SYSCONF,	NOFLAGS
},
#endif	/* _SC_PAGESIZE */
#if defined(_SC_PAGE_SIZE)
{
	_SC_PAGE_SIZE,		"PAGE_SIZE",		SYSCONF,	NOFLAGS
},
#endif	/* _SC_PAGE_SIZE */
#if defined(_SC_XOPEN_UNIX)
{
	_SC_XOPEN_UNIX,		"_XOPEN_UNIX",		SYSCONF,	NOFLAGS
},
#endif	/* _SC_XOPEN_UNIX */
#if defined(_SC_IOV_MAX)
{
	_SC_IOV_MAX,		"IOV_MAX",		SYSCONF, 	NOFLAGS
},
#endif	/* _SC_IOV_MAX */

/*
 * UNIX 98 - XPG5 additions
 */
#if defined(_SC_XOPEN_LEGACY)
{
	_SC_XOPEN_LEGACY,	"_XOPEN_LEGACY",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_XOPEN_LEGACY */
#if defined(_SC_PASS_MAX)
{
	_SC_PASS_MAX,		"PASS_MAX",		SYSCONF, 	NOFLAGS
},
#endif	/* _SC_PASS_MAX */
#if defined(_SC_AIO_LISTIO_MAX)
{
	_SC_AIO_LISTIO_MAX,	"AIO_LISTIO_MAX",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_AIO_LISTIO_MAX */
#if defined(_SC_AIO_MAX)
{
	_SC_AIO_MAX,		"AIO_MAX",		SYSCONF, 	NOFLAGS
},
#endif	/* _SC_AIO_MAX */
#if defined(_SC_AIO_PRIO_DELTA_MAX)
{
	_SC_AIO_PRIO_DELTA_MAX,	"AIO_PRIO_DELTA_MAX",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_AIO_PRIO_DELTA_MAX */
#if defined(_SC_DELAYTIMER_MAX)
{
	_SC_DELAYTIMER_MAX,	"DELAYTIMER_MAX",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_DELAYTIMER_MAX */
#if defined(_SC_MQ_OPEN_MAX)
{
	_SC_MQ_OPEN_MAX,	"MQ_OPEN_MAX",		SYSCONF, 	NOFLAGS
},
#endif	/* _SC_MQ_OPEN_MAX */
#if defined(_SC_MQ_PRIO_MAX)
{
	_SC_MQ_PRIO_MAX,	"MQ_PRIO_MAX",		SYSCONF, 	NOFLAGS
},
#endif	/* _SC_MQ_PRIO_MAX */
#if defined(_SC_RTSIG_MAX)
{
	_SC_RTSIG_MAX,		"RTSIG_MAX",		SYSCONF, 	NOFLAGS
},
#endif	/* _SC_RTSIG_MAX */
#if defined(_SC_SEM_NSEMS_MAX)
{
	_SC_SEM_NSEMS_MAX,	"SEM_NSEMS_MAX",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_SEM_NSEMS_MAX */
#if defined(_SC_SEM_VALUE_MAX)
{
	_SC_SEM_VALUE_MAX,	"SEM_VALUE_MAX",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_SEM_VALUE_MAX */
#if defined(_SC_SIGQUEUE_MAX)
{
	_SC_SIGQUEUE_MAX,	"SIGQUEUE_MAX",		SYSCONF, 	NOFLAGS
},
#endif	/* _SC_SIGQUEUE_MAX */
#if defined(_SC_TIMER_MAX)
{
	_SC_TIMER_MAX,		"TIMER_MAX",		SYSCONF, 	NOFLAGS
},
#endif	/* _SC_TIMER_MAX */

#if defined(_SC_ASYNCHRONOUS_IO)
{
	_SC_ASYNCHRONOUS_IO,	"_POSIX_ASYNCHRONOUS_IO", SYSCONF, 	NOFLAGS
},
#endif	/* _SC_ASYNCHRONOUS_IO */
#if defined(_SC_FSYNC)
{
	_SC_FSYNC,		"_POSIX_FSYNC",		SYSCONF,	NOFLAGS
},
#endif	/* _SC_FSYNC */
#if defined(_SC_MAPPED_FILES)
{
	_SC_MAPPED_FILES,	"_POSIX_MAPPED_FILES",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_MAPPED_FILES */
#if defined(_SC_MEMLOCK)
{
	_SC_MEMLOCK,		"_POSIX_MEMLOCK",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_MEMLOCK */
#if defined(_SC_MEMLOCK_RANGE)
{
	_SC_MEMLOCK_RANGE,	"_POSIX_MEMLOCK_RANGE",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_MEMLOCK_RANGE */
#if defined(_SC_MEMORY_PROTECTION)
{
	_SC_MEMORY_PROTECTION,	"_POSIX_MEMORY_PROTECTION", SYSCONF, 	NOFLAGS
},
#endif	/* _SC_MEMORY_PROTECTION */
#if defined(_SC_MESSAGE_PASSING)
{
	_SC_MESSAGE_PASSING,	"_POSIX_MESSAGE_PASSING", SYSCONF, 	NOFLAGS
},
#endif	/* _SC_MESSAGE_PASSING */
#if defined(_SC_PRIORITIZED_IO)
{
	_SC_PRIORITIZED_IO,	"_POSIX_PRIORITIZED_IO", SYSCONF, 	NOFLAGS
},
#endif	/* _SC_PRIORITIZED_IO */
#if defined(_SC_PRIORITY_SCHEDULING)
{
	_SC_PRIORITY_SCHEDULING, "_POSIX_PRIORITY_SCHEDULING", SYSCONF, NOFLAGS
},
#endif	/* _SC_PRIORITY_SCHEDULING */
#if defined(_SC_REALTIME_SIGNALS)
{
	_SC_REALTIME_SIGNALS,	"_POSIX_REALTIME_SIGNALS", SYSCONF, 	NOFLAGS
},
#endif	/* _SC_REALTIME_SIGNALS */
#if defined(_SC_SEMAPHORES)
{
	_SC_SEMAPHORES,		"_POSIX_SEMAPHORES",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_SEMAPHORES */

#if defined(_SC_SHARED_MEMORY_OBJECTS)
{
	_SC_SHARED_MEMORY_OBJECTS, "_POSIX_SHARED_MEMORY_OBJECTS",
	    SYSCONF,	NOFLAGS
},
#endif	/* _SC_SHARED_MEMORY_OBJECTS */

#if defined(_SC_SYNCHRONIZED_IO)
{
	_SC_SYNCHRONIZED_IO,	"_POSIX_SYNCHRONIZED_IO", SYSCONF, 	NOFLAGS
},
#endif	/* _SC_SYNCHRONIZED_IO */
#if defined(_SC_TIMERS)
{
	_SC_TIMERS,		"_POSIX_TIMERS",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_TIMERS */

#if defined(_SC_LOGIN_NAME_MAX)
{
	_SC_LOGIN_NAME_MAX,	"LOGIN_NAME_MAX",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_LOGIN_NAME_MAX */

#if defined(_SC_THREAD_DESTRUCTOR_ITERATIONS)
{
	_SC_THREAD_DESTRUCTOR_ITERATIONS, "PTHREAD_DESTRUCTOR_ITERATIONS",
	    SYSCONF, NOFLAGS
},
#endif	/* _SC_THREAD_DESTRUCTOR_ITERATIONS */

#if defined(_SC_THREAD_KEYS_MAX)
{
	_SC_THREAD_KEYS_MAX,	"PTHREAD_KEYS_MAX",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_THREAD_KEYS_MAX */
#if defined(_SC_THREAD_STACK_MIN)
{
	_SC_THREAD_STACK_MIN,	"PTHREAD_STACK_MIN",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_THREAD_STACK_MIN */
#if defined(_SC_THREAD_THREADS_MAX)
{
	_SC_THREAD_THREADS_MAX,	"PTHREAD_THREADS_MAX",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_THREAD_THREADS_MAX */

#if defined(_SC_THREADS)
{
	_SC_THREADS,		"_POSIX_THREADS",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_THREADS */

#if defined(_SC_THREAD_ATTR_STACKADDR)
{
	_SC_THREAD_ATTR_STACKADDR, "POSIX_THREAD_ATTR_STACKADDR",
	    SYSCONF, NOFLAGS
},
#endif	/* _SC_THREAD_ATTR_STACKADDR */
#if defined(_SC_THREAD_ATTR_STACKADDR)
{
	_SC_THREAD_ATTR_STACKADDR, "_POSIX_THREAD_ATTR_STACKADDR",
	    SYSCONF, NOFLAGS
},
#endif	/* _SC_THREAD_ATTR_STACKADDR */

#if defined(_SC_THREAD_ATTR_STACKSIZE)
{
	_SC_THREAD_ATTR_STACKSIZE, "POSIX_THREAD_ATTR_STACKSIZE",
	    SYSCONF, NOFLAGS
},
#endif	/* _SC_THREAD_ATTR_STACKSIZE */
#if defined(_SC_THREAD_ATTR_STACKSIZE)
{
	_SC_THREAD_ATTR_STACKSIZE, "_POSIX_THREAD_ATTR_STACKSIZE",
	    SYSCONF, NOFLAGS
},
#endif	/* _SC_THREAD_ATTR_STACKSIZE */

#if defined(_SC_THREAD_PRIORITY_SCHEDULING)
{
	_SC_THREAD_PRIORITY_SCHEDULING, "POSIX_THREAD_PRIORITY_SCHEDULING",
	    SYSCONF, NOFLAGS
},
#endif	/* _SC_THREAD_PRIORITY_SCHEDULING */
#if defined(_SC_THREAD_PRIORITY_SCHEDULING)
{
	_SC_THREAD_PRIORITY_SCHEDULING, "_POSIX_THREAD_PRIORITY_SCHEDULING",
	    SYSCONF, NOFLAGS
},
#endif	/* _SC_THREAD_PRIORITY_SCHEDULING */

#if defined(_SC_THREAD_PRIO_INHERIT)
{
	_SC_THREAD_PRIO_INHERIT, "POSIX_THREAD_PRIO_INHERIT",
	    SYSCONF, NOFLAGS
},
#endif	/* _SC_THREAD_PRIO_INHERIT */
#if defined(_SC_THREAD_PRIO_INHERIT)
{
	_SC_THREAD_PRIO_INHERIT, "_POSIX_THREAD_PRIO_INHERIT",
	    SYSCONF, NOFLAGS
},
#endif	/* _SC_THREAD_PRIO_INHERIT */

#if defined(_SC_THREAD_PRIO_PROTECT)
{
	_SC_THREAD_PRIO_PROTECT, "POSIX_THREAD_PRIO_PROTECT",
	    SYSCONF, NOFLAGS
},
#endif	/* _SC_THREAD_PRIO_PROTECT */
#if defined(_SC_THREAD_PRIO_PROTECT)
{
	_SC_THREAD_PRIO_PROTECT, "_POSIX_THREAD_PRIO_PROTECT",
	    SYSCONF, NOFLAGS
},
#endif	/* _SC_THREAD_PRIO_PROTECT */

#if defined(_SC_THREAD_PROCESS_SHARED)
{
	_SC_THREAD_PROCESS_SHARED, "POSIX_THREAD_PROCESS_SHARED",
	    SYSCONF, NOFLAGS
},
#endif	/* _SC_THREAD_PROCESS_SHARED */
#if defined(_SC_THREAD_PROCESS_SHARED)
{
	_SC_THREAD_PROCESS_SHARED, "_POSIX_THREAD_PROCESS_SHARED",
	    SYSCONF, NOFLAGS
},
#endif	/* _SC_THREAD_PROCESS_SHARED */

#if defined(_SC_THREAD_SAFE_FUNCTIONS)
{
	_SC_THREAD_SAFE_FUNCTIONS, "POSIX_THREAD_SAFE_FUNCTIONS",
	    SYSCONF, NOFLAGS
},
#endif	/* _SC_THREAD_SAFE_FUNCTIONS */
#if defined(_SC_THREAD_SAFE_FUNCTIONS)
{
	_SC_THREAD_SAFE_FUNCTIONS, "_POSIX_THREAD_SAFE_FUNCTIONS",
	    SYSCONF, NOFLAGS
},
#endif	/* _SC_THREAD_SAFE_FUNCTIONS */

#if defined(_SC_TTY_NAME_MAX)
{
	_SC_TTY_NAME_MAX,	"TTY_NAME_MAX",		SYSCONF,	NOFLAGS
},
#endif	/* _SC_TTY_NAME_MAX */
#if defined(_SC_XOPEN_REALTIME)
{
	_SC_XOPEN_REALTIME,	"_XOPEN_REALTIME",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_XOPEN_REALTIME */
#if defined(_SC_XOPEN_REALTIME_THREADS)
{
	_SC_XOPEN_REALTIME_THREADS, "_XOPEN_REALTIME_THREADS", SYSCONF, NOFLAGS
},
#endif	/* _SC_XOPEN_REALTIME_THREADS */

	/*
	 * Due to ambiguity in the standard we need
	 * to recognize both the underscored name
	 * and the non-underscored version in the
	 * following variables.
	 */
#if defined(_SC_XBS5_ILP32_OFF32)
{
	_SC_XBS5_ILP32_OFF32,	"XBS5_ILP32_OFF32",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_XBS5_ILP32_OFF32 */
#if defined(_SC_XBS5_ILP32_OFF32)
{
	_SC_XBS5_ILP32_OFF32,	"_XBS5_ILP32_OFF32",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_XBS5_ILP32_OFF32 */

#if defined(_SC_XBS5_ILP32_OFFBIG)
{
	_SC_XBS5_ILP32_OFFBIG,	"XBS5_ILP32_OFFBIG",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_XBS5_ILP32_OFFBIG */
#if defined(_SC_XBS5_ILP32_OFFBIG)
{
	_SC_XBS5_ILP32_OFFBIG,	"_XBS5_ILP32_OFFBIG",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_XBS5_ILP32_OFFBIG */

#if defined(_SC_XBS5_LP64_OFF64)
{
	_SC_XBS5_LP64_OFF64,	"XBS5_LP64_OFF64",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_XBS5_LP64_OFF64 */
#if defined(_SC_XBS5_LP64_OFF64)
{
	_SC_XBS5_LP64_OFF64,	"_XBS5_LP64_OFF64",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_XBS5_LP64_OFF64 */

#if defined(_SC_XBS5_LPBIG_OFFBIG)
{
	_SC_XBS5_LPBIG_OFFBIG,	"XBS5_LPBIG_OFFBIG",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_XBS5_LPBIG_OFFBIG */
#if defined(_SC_XBS5_LPBIG_OFFBIG)
{
	_SC_XBS5_LPBIG_OFFBIG,	"_XBS5_LPBIG_OFFBIG",	SYSCONF, 	NOFLAGS
},
#endif	/* _SC_XBS5_LPBIG_OFFBIG */

/*
 * UNIX 03 additions for sysconf() names, and names that are not in interfaces
 */
#if defined(_SC_REGEXP)
{
	_SC_REGEXP,		"_POSIX_REGEXP",	  SYSCONF,	NOFLAGS
},
#endif	/* _SC_REGEXP */
#if defined(_SC_SHELL)
{
	_SC_SHELL,		"_POSIX_SHELL",		  SYSCONF,	NOFLAGS
},
#endif	/* _SC_SHELL */
#if defined(_SC_HOST_NAME_MAX)
{
	_SC_HOST_NAME_MAX,	"HOST_NAME_MAX",	  SYSCONF,	NOFLAGS
},
#endif	/* _SC_HOST_NAME_MAX */
#if defined(_SC_READER_WRITER_LOCKS)
{
	_SC_READER_WRITER_LOCKS, "_POSIX_READER_WRITER_LOCKS", SYSCONF,	NOFLAGS
},
#endif	/* _SC_READER_WRITER_LOCKS */
#if defined(_SC_IPV6)
{
	_SC_IPV6,		"_POSIX_IPV6",		  SYSCONF,	NOFLAGS
},
#endif	/* _SC_IPV6 */
#if defined(_SC_RAW_SOCKETS)
{
	_SC_RAW_SOCKETS,	"_POSIX_RAW_SOCKETS",	  SYSCONF,	NOFLAGS
},
#endif	/* _SC_RAW_SOCKETS */
#if defined(_SC_XOPEN_STREAMS)
{
	_SC_XOPEN_STREAMS, 	"_XOPEN_STREAMS",	  SYSCONF,	NOFLAGS
},
#endif	/* _SC_XOPEN_STREAMS */
#if defined(_SC_SYMLOOP_MAX)
{
	_SC_SYMLOOP_MAX,	"SYMLOOP_MAX",		  SYSCONF,	NOFLAGS,
		20
},
#endif	/* _SC_SYMLOOP_MAX */
#if defined(_SC_2_PBS)
{
	_SC_2_PBS,		"_POSIX2_PBS",		  SYSCONF,	NOFLAGS
},
#endif	/* _SC_2_PBS */
#if defined(_SC_2_PBS_ACCOUNTING)
{
	_SC_2_PBS_ACCOUNTING,   "_POSIX2_PBS_ACCOUNTING", SYSCONF,	NOFLAGS
},
#endif	/* _SC_2_PBS_ACCOUNTING */
#if defined(_SC_2_PBS_CHECKPOINT)
{
	_SC_2_PBS_CHECKPOINT,   "_POSIX2_PBS_CHECKPOINT", SYSCONF,	NOFLAGS
},
#endif	/* _SC_2_PBS_CHECKPOINT */
#if defined(_SC_2_PBS_LOCATE)
{
	_SC_2_PBS_LOCATE,	"_POSIX2_PBS_LOCATE",	  SYSCONF,	NOFLAGS
},
#endif	/* _SC_2_PBS_LOCATE */
#if defined(_SC_2_PBS_MESSAGE)
{
	_SC_2_PBS_MESSAGE,	"_POSIX2_PBS_MESSAGE",	  SYSCONF,	NOFLAGS
},
#endif	/* _SC_2_PBS_MESSAGE */
#if defined(_SC_2_PBS_TRACK)
{
	_SC_2_PBS_TRACK,	"_POSIX2_PBS_TRACK",	  SYSCONF,	NOFLAGS
},
#endif	/* _SC_2_PBS_TRACK */

#if defined(_SC_ADVISORY_INFO)
{
	_SC_ADVISORY_INFO,	"_POSIX_ADVISORY_INFO",   SYSCONF,	NOFLAGS
},
#endif	/* _SC_ADVISORY_INFO */
#if defined(_SC_BARRIERS)
{
	_SC_BARRIERS,		"_POSIX_BARRIERS",	  SYSCONF,	NOFLAGS
},
#endif	/* _SC_BARRIERS */
#if defined(_SC_CLOCK_SELECTION)
{
	_SC_CLOCK_SELECTION,	"_POSIX_CLOCK_SELECTION", SYSCONF,	NOFLAGS
},
#endif	/* _SC_CLOCK_SELECTION */
#if defined(_SC_CPUTIME)
{
	_SC_CPUTIME,		"_POSIX_CPUTIME",	  SYSCONF,	NOFLAGS
},
#endif	/* _SC_CPUTIME */
#if defined(_SC_MONOTONIC_CLOCK)
{
	_SC_MONOTONIC_CLOCK,	"_POSIX_MONOTONIC_CLOCK", SYSCONF,	NOFLAGS
},
#endif	/* _SC_MONOTONIC_CLOCK */
#if defined(_SC_SPAWN)
{
	_SC_SPAWN,		"_POSIX_SPAWN",		  SYSCONF,	NOFLAGS
},
#endif	/* _SC_SPAWN */
#if defined(_SC_SPIN_LOCKS)
{
	_SC_SPIN_LOCKS,		"_POSIX_SPIN_LOCKS",	  SYSCONF,	NOFLAGS
},
#endif	/* _SC_SPIN_LOCKS */
#if defined(_SC_SPORADIC_SERVER)
{
	_SC_SPORADIC_SERVER,	"_POSIX_SPORADIC_SERVER", SYSCONF,	NOFLAGS
},
#endif	/* _SC_SPORADIC_SERVER */
#if defined(_SC_SS_REPL_MAX)
{
	_SC_SS_REPL_MAX,	"_POSIX_SS_REPL_MAX",	  SYSCONF,	NOFLAGS
},
#endif	/* _SC_SS_REPL_MAX */
#if defined(_SC_THREAD_CPUTIME)
{
	_SC_THREAD_CPUTIME,	"_POSIX_THREAD_CPUTIME",   SYSCONF,	NOFLAGS
},
#endif	/* _SC_THREAD_CPUTIME */
#if defined(_SC_THREAD_SPORADIC_SERVER)
{
	_SC_THREAD_SPORADIC_SERVER, "_POSIX_THREAD_SPORADIC_SERVER", 	SYSCONF,
	NOFLAGS
},
#endif	/* _SC_THREAD_SPORADIC_SERVER */
#if defined(_SC_TIMEOUTS)
{
	_SC_TIMEOUTS,		"_POSIX_TIMEOUTS",	  SYSCONF,	NOFLAGS
},
#endif	/* _SC_TIMEOUTS */
#if defined(_SC_TRACE)
{
	_SC_TRACE,		"_POSIX_TRACE",		  SYSCONF,	NOFLAGS
},
#endif	/* _SC_TRACE */
#if defined(_SC_TRACE_EVENT_FILTER)
{
	_SC_TRACE_EVENT_FILTER, "_POSIX_TRACE_EVENT_FILTER", SYSCONF, 	NOFLAGS
},
#endif	/* _SC_TRACE_EVENT_FILTER */
#if defined(_SC_TRACE_EVENT_NAME_MAX)
{
	_SC_TRACE_EVENT_NAME_MAX, "_POSIX_TRACE_EVENT_NAME_MAX", SYSCONF,
	NOFLAGS
},
#endif	/* _SC_TRACE_EVENT_NAME_MAX */
#if defined(_SC_TRACE_INHERIT)
{
	_SC_TRACE_INHERIT,	"_POSIX_TRACE_INHERIT",   SYSCONF,	NOFLAGS
},
#endif	/* _SC_TRACE_INHERIT */
#if defined(_SC_TRACE_LOG)
{
	_SC_TRACE_LOG,		"_POSIX_TRACE_LOG",	  SYSCONF,	NOFLAGS
},
#endif	/* _SC_TRACE_LOG */
#if defined(_SC_TRACE_NAME_MAX)
{
	_SC_TRACE_NAME_MAX,	"_POSIX_TRACE_NAME_MAX",  SYSCONF,	NOFLAGS
},
#endif	/* _SC_TRACE_NAME_MAX */
#if defined(_SC_TRACE_SYS_MAX)
{
	_SC_TRACE_SYS_MAX,	"_POSIX_TRACE_SYS_MAX",	  SYSCONF,	NOFLAGS
},
#endif	/* _SC_TRACE_SYS_MAX */
#if defined(_SC_TRACE_USER_EVENT_MAX)
{
	_SC_TRACE_USER_EVENT_MAX, "_POSIX_TRACE_USER_EVENT_MAX", SYSCONF,
	NOFLAGS
},
#endif	/* _SC_TRACE_USER_EVENT_MAX */
#if defined(_SC_TYPED_MEMORY_OBJECTS)
{
	_SC_TYPED_MEMORY_OBJECTS, "_POSIX_TYPED_MEMORY_OBJECTS",	SYSCONF,
	NOFLAGS
},
#endif	/* _SC_TYPED_MEMORY_OBJECTS */

	/*
	 * Due to an ambiguity in the specification, we need to recognize both
	 * the underscored POSIX name and the non-underscored name. In addition,
	 * we need to permit the use of the _V6_* environment names specified
	 * in unistd.h.
	 */
#if defined(_SC_V6_ILP32_OFF32)
{
	_SC_V6_ILP32_OFF32,	"_V6_ILP32_OFF32", 	SYSCONF,	NOFLAGS
},
#endif	/* _SC_V6_ILP32_OFF32 */
#if defined(_SC_V6_ILP32_OFF32)
{
	_SC_V6_ILP32_OFF32,	"_POSIX_V6_ILP32_OFF32", SYSCONF,	NOFLAGS
},
#endif	/* _SC_V6_ILP32_OFF32 */
#if defined(_SC_V6_ILP32_OFF32)
{
	_SC_V6_ILP32_OFF32,	"POSIX_V6_ILP32_OFF32", SYSCONF,	NOFLAGS
},
#endif	/* _SC_V6_ILP32_OFF32 */

#if defined(_SC_V6_ILP32_OFFBIG)
{
	_SC_V6_ILP32_OFFBIG,	"_V6_ILP32_OFFBIG", 	SYSCONF,	NOFLAGS
},
#endif	/* _SC_V6_ILP32_OFFBIG */
#if defined(_SC_V6_ILP32_OFFBIG)
{
	_SC_V6_ILP32_OFFBIG,	"_POSIX_V6_ILP32_OFFBIG", SYSCONF,	NOFLAGS
},
#endif	/* _SC_V6_ILP32_OFFBIG */
#if defined(_SC_V6_ILP32_OFFBIG)
{
	_SC_V6_ILP32_OFFBIG,	"POSIX_V6_ILP32_OFFBIG", SYSCONF,	NOFLAGS
},
#endif	/* _SC_V6_ILP32_OFFBIG */

#if defined(_SC_V6_LP64_OFF64)
{
	_SC_V6_LP64_OFF64,	"_V6_LP64_OFF64", 	SYSCONF,	NOFLAGS
},
#endif	/* _SC_V6_LP64_OFF64 */
#if defined(_SC_V6_LP64_OFF64)
{
	_SC_V6_LP64_OFF64,	"_POSIX_V6_LP64_OFF64",	SYSCONF,	NOFLAGS
},
#endif	/* _SC_V6_LP64_OFF64 */
#if defined(_SC_V6_LP64_OFF64)
{
	_SC_V6_LP64_OFF64,	"POSIX_V6_LP64_OFF64", 	SYSCONF,	NOFLAGS
},
#endif	/* _SC_V6_LP64_OFF64 */

#if defined(_SC_V6_LPBIG_OFFBIG)
{
	_SC_V6_LPBIG_OFFBIG,	"_V6_LPBIG_OFFBIG", 	SYSCONF,	NOFLAGS
},
#endif	/* _SC_V6_LPBIG_OFFBIG */
#if defined(_SC_V6_LPBIG_OFFBIG)
{
	_SC_V6_LPBIG_OFFBIG,	"_POSIX_V6_LPBIG_OFFBIG", SYSCONF,	NOFLAGS
},
#endif	/* _SC_V6_LPBIG_OFFBIG */
#if defined(_SC_V6_LPBIG_OFFBIG)
{
	_SC_V6_LPBIG_OFFBIG,	"POSIX_V6_LPBIG_OFFBIG",  SYSCONF,	NOFLAGS
},
#endif	/* _SC_V6_LPBIG_OFFBIG */
#if !defined(SU3) /* Extensions for the default getconf command. */
/* _NPROCESSORS_ONLN and _NPROCESSORS_CONF, which gives an integer that
 * correspond to the total number of processors on the machine. */
{
	_SC_NPROCESSORS_ONLN,	"_NPROCESSORS_ONLN",	SYSCONF,	NOFLAGS
},
{
	_SC_NPROCESSORS_CONF,	"_NPROCESSORS_CONF",	SYSCONF,	NOFLAGS
},
#endif /* Not SUSv3 */

	/* local values */
{
	HEIRLOOM_TOOLCHEST_VERSION,	"HEIRLOOM_TOOLCHEST_VERSION",
		SELFCONF,	NOFLAGS
},
{
	0,		NULL,	0,	0		/* end of table */
}
};


/*
 * Print usage message.
 */
static int
usage(void)
{

	fprintf(stderr,
		"usage:\t%s [-v specification] system_var\n"
		"\t%s [-v specification] path_var pathname\n"
		"\t%s -a\n", progname, progname, progname);
	return (2);
}


static int
namecmp(const void *a, const void *b)
{
	return (strcoll(((const struct sctab *)a)->name,
	    ((const struct sctab *)b)->name));
}


static int
getconf(struct sctab *scp, int argc, char *name, char *file)
{
	register size_t len;
	register char *buffer;
	long value;

	switch (scp->type) {
	case SELFCONF:
		if (argc > 2)
			return (usage());
		if (scp->flag & UNDEFINED ||
		    (!(scp->flag&UNSIGNED_VALUE) && scp->value == -1)) {
			 printf("undefined\n");
		} else
			if (scp->flag & UNSIGNED_VALUE)
				 printf("%lu\n", scp->value);
			else
				 printf("%ld\n", scp->value);
		break;

	case SYSCONF:
		if (argc > 2)
			return (usage());
		errno = 0;
		if (scp->flag & UNDEFINED ||
		    (value = sysconf((int)scp->value)) == -1) {
			if (scp->dfl) {
				printf("%ld\n", scp->dfl);
			} else if (errno == EINVAL) {
				fprintf(stderr, INVAL_ARG, progname, name);
				return (1);
			} else {
				printf("undefined\n");
				return (0);
			}
		} else
			 printf("%ld\n", value);
		break;

	case CONFSTR:
		if (argc > 2)
			return (usage());
		errno = 0;
		len = confstr((int)scp->value, NULL, (size_t)0);
		if (len == 0) {
			if (errno == EINVAL) {
				fprintf(stderr, INVAL_ARG,
				    progname, name);
				return (1);
			} else {
				printf("undefined\n");
				return (0);
			}
		}
		/*
		 * allocate space to store result of constr() into
		 */
		if ((buffer = malloc(len)) == NULL) {
			fprintf(stderr, "insufficient memory available\n");
			return (1);
		}

		if (confstr((int)scp->value, buffer, len) == 0) {
			fprintf(stderr, "confstr() behavior not consistent\n");
			return (1);
		}

		printf("%s\n", buffer);
		free(buffer);
		break;

	case PATHCONF:
		if (argc != 3)
			return (usage());
		errno = 0;
		if ((value = pathconf(file, (int)scp->value)) == -1) {
			if (errno == EINVAL || errno == 0) {
				/* Does not support the association */
				 printf("undefined\n");
				return (0);
			} else {
				/* Probably problems with the pathname itself */
				if (aflag) {
					printf("%s\n", file);
				} else {
					printf("%s", file);
				}
				return (1);
			}
		}
		printf("%ld\n", value);
		break;
	case PATHVAR:
		buffer = NULL;
#if defined(_CS_PATH)
		if ((len = confstr(_CS_PATH, NULL, 0)) == 0 ||
				(buffer = malloc(len)) == NULL ||
				confstr(_CS_PATH, buffer, len) == 0)
			buffer = NULL;
#endif	/* _CS_PATH */
		printf("%s:%s\n", HEIRLOOM_PATH,
				buffer ? buffer : "/bin:/usr/bin");
		break;
	}
	return (0);
}

int
main(int argc, char **argv)
{
	register struct sctab *scp;
	int c;
	int exstat = 0;

	progname = basename(argv[0]);
	while ((c = getopt(argc, argv, "av:")) != -1)
		switch (c) {
		case 'a':
			aflag = 1;
			break;
		case 'v':
			/*
			 * Unix98 adds the -v option to allow
			 * programming 'specifications' to be
			 * indicated, for present purposes
			 * the specification isn't really
			 * doing anything of value, so for
			 * the moment getopt just processes the
			 * option value and argv[1] is adjusted.
			 *
			 * At some later date we might want to
			 * do specification processing at this
			 * point.
			 */
			break;
		case '?':
			return (usage());
		}
	argc -= optind-1;
	argv += optind-1;

	if ((aflag && argc >= 2) || (!aflag && argc < 2))
		return (usage());
	if (aflag) {

#define	TabStop		8
#define	RightColumn	32
#define	DefPathName	"."

		/*
		 * sort the table by the "name" field
		 * so we print it in sorted order
		 */
	qsort(&sctab[0], (sizeof (sctab)/sizeof (struct sctab))-1,
	    sizeof (struct sctab), namecmp);

		/*
		 * print all the known symbols and their values
		 */
		for (scp = &sctab[0]; scp->name != NULL; ++scp) {
			int stat;

			/*
			 * create 2 columns:
			 *   variable name in the left column,
			 *   value in the right column.
			 * The right column starts at a tab stop.
			 */
			printf("%s:\t", scp->name);

			c = strlen(scp->name) + 1;
			c = (c+TabStop) / TabStop, c *= TabStop;
			for (; c < RightColumn; c += TabStop)
				putchar('\t');

			/*
			 * for pathconf() related variables that require
			 * a pathname, use "."
			 */
			stat = getconf(scp, scp->type == PATHCONF ? 3 : 2,
			    scp->name, DefPathName);

			exstat |= stat;

			/*
			 * if stat != 0, then an error message was already
			 * printed in getconf(),
			 * so don't need to print one here
			 */
		}
	} else {

		/*
		 * find the name of the specified variable (argv[1])
		 * and print its value.
		 */
		for (scp = &sctab[0]; scp->name != NULL; ++scp)
			if (strcmp(argv[1], scp->name) == 0) {
				exstat = getconf(scp, argc, argv[1], argv[2]);
				break;
			}

		/*
		 * if at last entry in table, then the user specified
		 * variable is invalid
		 */
		if (scp->name == NULL) {
			errno = EINVAL;
			fprintf(stderr, INVAL_ARG, progname, argv[1]);
			return (1);
		}
	}
	return (exstat);
}
