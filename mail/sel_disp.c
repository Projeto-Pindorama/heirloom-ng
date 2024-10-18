/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "sel_disp.c	1.7	05/06/08 SMI" 	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)sel_disp.c	1.3 (gritter) 6/18/05
 */

#include "mail.h"

/*
 * If in default display mode from printmail(), selectively output
 * header lines. Any recognized header lines will have flag stored in
 * header[] structure. Other header lines which should be displayed in
 * the default output mode will be listed in the seldisp[] array.
 * This can all be overridden via the 'P' command at the ? prompt.
 */
int
sel_disp(int type, int hdrtype, char *s)
{
	static char pn[] = "sel_disp";
	char	*p;
	static	int	sav_lastrc = 0;
	int		i, rc = 0;

	if (sending || Pflg || (type != TTY)) {
		return (0);
	}

	switch (hdrtype) {
	case H_CONT:
		rc = sav_lastrc;
		break;
	case H_NAMEVALUE:
		for (i=0,p=seldisp[i]; p; p=seldisp[++i]) {
			if (casncmp(s, p, strlen(p)) == 0) {
				break;
			}
		}
		if (p == (char *)NULL) {
			rc = -1;
		}
		break;
	default:
		if (header[hdrtype].default_display == FALSE) {
			rc = -1;
			break;
		}
	}

	Dout(pn, 2, "type = %d, hdrtype = %d/'%s', rc = %d\n",
		type, hdrtype, header[hdrtype].tag, rc);
	sav_lastrc = rc;	/* In case next one is H_CONT... */
	return (rc);
}
