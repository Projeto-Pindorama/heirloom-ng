/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "doopen.c	1.8	05/06/08 SMI" 	 SVr4.0 2.		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)doopen.c	1.5 (gritter) 5/14/06
 */
#include "mail.h"
/*
	Generic open routine.
	Exits on error with passed error value.
	Returns file pointer on success.

	Note: This should be used only for critical files
	as it will terminate mail(1) on failure.
*/
FILE *
doopen(char *file, char *type, int errnum)
{
	static char pn[] = "doopen";
	FILE *fptr = NULL;
	struct stat st;

	if ((stat(file, &st) < 0
#ifdef EOVERFLOW
		&& errno == EOVERFLOW
#endif
		) ||
		(fptr = fopen(file, type)) == NULL) {
		fprintf(stderr,
			"%s: Can't open '%s' type: %s\n",program,file,type);
		error = errnum;
		sav_errno = errno;
		Dout(pn, 0, "can't open '%s' type: %s\n",program,file,type);
		Dout(pn, 0, "error set to %d\n", error);
		done(0);
	}
	return(fptr);
}
