#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)dc.sl	2.12 (gritter) 12/25/06";
/* SLIST */
/*
dc.c:	Sccsid @(#)dc.c	1.21 (gritter) 12/25/06>	
dc.h:	Sccsid @(#)dc.h	1.9 (gritter) 2/4/05>	
*/
