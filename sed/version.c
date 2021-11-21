#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
#if defined (SU3)
static const char sccsid[] USED = "@(#)sed_su3.sl	2.34 (gritter) 6/26/05";
#elif defined (SUS)
static const char sccsid[] USED = "@(#)sed_sus.sl	2.34 (gritter) 6/26/05";
#elif defined (S42)
static const char sccsid[] USED = "@(#)sed_s42.sl	2.34 (gritter) 6/26/05";
#else	/* !SUS, !SU3, !S42 */
static const char sccsid[] USED = "@(#)sed.sl	2.34 (gritter) 6/26/05";
#endif	/* !SUS, !SU3, !S42 */
/* SLIST */
/*
sed.h:	Sccsid @(#)sed.h	1.32 (gritter) 2/6/05	
sed0.c:	Sccsid @(#)sed0.c	1.64 (gritter) 3/12/05>	
sed1.c:	Sccsid @(#)sed1.c	1.42 (gritter) 2/6/05>	
*/
