#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)cpio.sl	2.5 (gritter) 4/14/07";
/* SLIST */
/*
blast.c: * Sccsid @(#)blast.c	1.2 (gritter) 2/17/04
blast.h: * Sccsid @(#)blast.h	1.2 (gritter) 2/17/04
cpio.c: * Sccsid @(#)cpio.c	1.303 (gritter) 4/14/07
cpio.h:	Sccsid @(#)cpio.h	1.29 (gritter) 3/26/07	
crc32.c: * Sccsid @(#)crc32.c	1.2 (gritter) 5/29/03
expand.c:	Sccsid @(#)expand.c	1.6 (gritter) 12/15/03	
explode.c: * Sccsid @(#)explode.c	1.6 (gritter) 9/30/03
flags.c:	Sccsid @(#)flags.c	1.6 (gritter) 3/26/07	
inflate.c: * Sccsid @(#)inflate.c	1.6 (gritter) 10/13/04
nonpax.c:	Sccsid @(#)nonpax.c	1.1 (gritter) 2/24/04	
pax.c:static const char sccsid[] USED = "@(#)pax_su3.sl	1.26 (gritter) 6/26/05";
pax.c:static const char sccsid[] USED = "@(#)pax.sl	1.26 (gritter) 6/26/05";
pax.c:	Sccsid @(#)pax.c	1.26 (gritter) 6/26/05	
unshrink.c: * Sccsid @(#)unshrink.c	1.4 (gritter) 6/18/04
unzip.h: * Sccsid @(#)unzip.h	1.5 (gritter) 7/16/04
*/
