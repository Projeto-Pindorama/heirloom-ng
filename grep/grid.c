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

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)grep.sl	2.51 (gritter) 12/27/06";
/* SLIST */
/*
ac.c:static const char sccsid[] USED = "@(#)fgrep.sl	2.10 (gritter) 5/29/05";
alloc.c:	Sccsid @(#)alloc.c	1.3 (gritter) 4/17/03>
alloc.h:	Sccsid @(#)alloc.h	1.3 (gritter) 4/17/03>
egrep.y:static const char sccsid[] USED = "@(#)egrep.sl	2.22 (gritter) 5/29/05";
fgrep.c:	Sccsid @(#)fgrep.c	1.12 (gritter) 12/17/04>
ggrep.c:	Sccsid @(#)ggrep.c	1.26 (gritter) 1/4/05>
grep.c:	Sccsid @(#)grep.c	1.53 (gritter) 12/27/06>
grep.h:	Sccsid @(#)grep.h	1.23 (gritter) 1/4/05>
plist.c:	Sccsid @(#)plist.c	1.22 (gritter) 12/8/04>
rcomp.c:	Sccsid @(#)rcomp.c	1.27 (gritter) 2/6/05>
sus.c:	Sccsid @(#)sus.c	1.24 (gritter) 5/29/05>
svid3.c:	Sccsid @(#)svid3.c	1.7 (gritter) 4/17/03>
*/
