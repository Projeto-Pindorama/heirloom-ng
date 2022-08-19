/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "errmsg.c	1.6	05/06/08 SMI" 	 SVr4.0 2.		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)errmsg.c	1.3 (gritter) 6/18/05
 */
#include "mail.h"
/*
    NAME
	errmsg - print error message

    SYNOPSIS
	void errmsg(int error_value, char *error_message)

    DESCRIPTION
	Errmsg() prints error messages. If error_message is supplied,
	that is taken as the text for the message, otherwise the
	text for the err_val message is gotten from the errlist[] array.
*/
void 
errmsg(int err_val, char *err_txt)
{
	static char pn[] = "errmsg";
	error = err_val;
	if (err_txt && *err_txt) {
		fprintf(stderr,"%s: %s\n",program,err_txt);
		Dout(pn, 0, "%s\n",err_txt);
	} else {
		fprintf(stderr,"%s: %s\n",program,errlist[err_val]);
		Dout(pn, 0,"%s\n",errlist[err_val]);
	}
	Dout(pn, 0,"error set to %d\n", error);
}
