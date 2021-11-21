/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 1995 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	from OpenSolaris "done.c	1.10	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)done.c	1.5 (gritter) 6/22/05
 */

#include "mail.h"

/*
 * remap the bin/mail exit code to sendmail recognizable
 * exit code when in deliver mode.
 */
static int 
maperrno(int err)
{
	int rc;

	switch (sav_errno) {
	case 0:
		rc = 0;		/* EX_OK */
		break;
	case EPERM:
	case EACCES:
	case ENOSPC:
        case EDQUOT:
		rc = 73;	/* EX_CANTCREAT */
		break;
	case EAGAIN:
		rc = 75;	/* EX_TEMPFAIL */
		break;
	case ENOENT:
	case EISDIR:
	case ENOTDIR:
		rc = 72;	/* EX_OSFILE */
		break;
	default:
		rc = 74;	/* EX_IOERR */
		break;
	}
	return(rc);
}

/* Fix for bug 1207994 */
void 
sig_done(int needtmp)
{
	static char pn[] = "sig_done";
	Dout(pn, 0, "got signal %d\n", needtmp);
	done(0);
}


void 
done(int needtmp)
{
	static char pn[] = "done";
	unlock();
	if (!maxerr) {
		maxerr = error;
		Dout(pn, 0, "maxerr set to %d\n", maxerr);
		if ((debug > 0) && (keepdbgfile == 0)) {
			unlink (dbgfname);
		}
	}
	if (maxerr && sending)
		mkdead();
	if (tmpf) {
		fclose(tmpf);
		tmpf = NULL;
	}
	if (!needtmp && lettmp) {
		Dout(pn, 0, "temp file removed\n");
		unlink(lettmp);
	}

	if (deliverflag) {
		/* bug fix for 1104684 */
		exit(maperrno(sav_errno));
	}
	exit(maxerr);
	/* NOTREACHED */
}
