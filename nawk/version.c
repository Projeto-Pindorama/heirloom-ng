#include "awk.h"
#if defined (SU3)
const char version[] = "@(#)awk_su3.sl  1.51 (gritter) 12/25/06";
int	posix = 1;
#elif defined (SUS)
const char version[] = "@(#)awk_sus.sl  1.51 (gritter) 12/25/06";
int	posix = 1;
#else
const char version[] = "@(#)nawk.sl  1.51 (gritter) 12/25/06";
int	posix = 0;
#endif
/* SLIST */
/*
awk.g.y:   Sccsid @(#)awk.g.y	1.9 (gritter) 5/14/06>
awk.h:   Sccsid @(#)awk.h	1.23 (gritter) 12/25/04>
awk.lx.l:   Sccsid @(#)awk.lx.l	1.13 (gritter) 11/22/05>
b.c:   Sccsid @(#)b.c	1.6 (gritter) 5/15/04>
lib.c:   Sccsid @(#)lib.c	1.27 (gritter) 12/25/06>
main.c:   Sccsid @(#)main.c	1.14 (gritter) 12/19/04>
maketab.c:   Sccsid @(#)maketab.c	1.11 (gritter) 12/4/04>
parse.c:   Sccsid @(#)parse.c	1.7 (gritter) 12/4/04>
run.c:   Sccsid @(#)run.c	1.33 (gritter) 12/25/06>
tran.c:   Sccsid @(#)tran.c	1.16 (gritter) 2/4/05>
rerule.sed:# Sccsid @(#)rerule.sed	1.1 (gritter) 2/6/05
*/
