/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "goback.c	1.8	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)goback.c	1.6 (gritter) 6/22/05
 */

/*
 * Copyright 2002 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include "mail.h"
/*
	This routine returns undeliverable mail as well as handles
	replying to letters
*/

#define	Return	free(buf); free(work); free(wuser); return

void
goback(int letnum)
{
	static char pn[] = "goback";
	int	i, w;
	char	*cp;
	char	*buf = NULL, *work = NULL, *wuser = NULL;
	size_t	bufsize = 0, worksize = 0, wusersize = 0;

	/*
	 * If dflag already == 1, then been here already and
	 * having a problem delivering failure notification.
	 * Reset dflag to 9 to avoid endless loop.....
	 */
	if (dflag == 1) {
		dflag = 9;
		Dout(pn, 0, "dflag = %d\n", dflag);
		if (!error) {
			error = E_REMOTE;
			Dout(pn, 0, "error = %d\n", error);
		}
	}

	if (dflag < 2) {
		cpy(&work, &worksize, "");
		cpy(&wuser, &wusersize, "");
		fclose(tmpf);
		tmpf = NULL;
		if (!replying) {
			dflag = 1;
			Dout(pn, 0, "dflag = 1\n");
			if ((debug > 0) && (keepdbgfile == 0)) {
				keepdbgfile++;
			}
			if (ckdlivopts(H_TCOPY, (int *)0) & IGNORE) {
				goto skipsend;
			}
		}
		tmpf = doopen(lettmp, "r+", E_TMP);
		if (replying) {
			fseek(tmpf, let[letnum].adr, 0);
		}
		for (fgetline(&line, &linesize, NULL, tmpf);
		strncmp(line, header[H_FROM].tag, strlen(header[H_FROM].tag))
		    == SAME ||
		    strncmp(line, header[H_FROM1].tag,
			strlen(header[H_FROM1].tag)) == SAME; ) {
			if ((i = substr(line, "remote from")) != -1) {
				for (i = 0, cp = strrchr(line, ' ') + 1;
					*cp != '\n';
					cp++) {
					nextb(&buf, &bufsize, i) = *cp;
				}
				nextb(&buf, &bufsize, i) = '!';
				nextb(&buf, &bufsize, i) = '\0';
				concat(&work, &worksize, buf);
				if (line[0] == '>') {
					i = 6;
				} else {
					i = 5;
				}
				for (w = i; line[w] != ' '; w++) {
					wuser[w-i] = line[w];
				}
				wuser[w-i] = '\0';
			} else if ((i = substr(line, "forwarded by")) == -1) {
				if (line[0] == '>') {
					break;
				} else {
					i = 5;
				}
				for (w = i; line[w] != ' '; w++) {
					wuser[w-i] = line[w];
				}
				wuser[w-i] = '\0';
			} else if ((i = substr(line, "forwarded by")) > -1) {
				break;
			}
			fgetline(&line, &linesize, NULL, tmpf);
		}
		concat(&work, &worksize, wuser);
		fclose(tmpf);
		tmpf = doopen(lettmp, "r+", E_TMP);
		if (work[0] != '\0') {
			reciplist list;
			if (replying) {
				if (m_sendto == NULL)
					cpy(&m_sendto, &m_sendtosize, "");
				buf = srealloc(buf, bufsize = strlen(m_sendto) +
						strlen(work) + 20);
				snprintf(buf, bufsize,
				    "mail %s %s", m_sendto, work);
				printf("%s\n", buf);
				systm(buf);
				Return;
			}
			if (interactive) {
				cpy(&work, &worksize, my_name);
			}
			fprintf(stderr, "%s: Return to %s\n", program, work);
			/* Put header info from message aside so it won't */
			/* get confused with the Delivery Notification info */
			Daffbytecnt = affbytecnt; affbytecnt = 0;
			Daffcnt = affcnt; affcnt = 0;
			Drcvbytecnt = rcvbytecnt; rcvbytecnt = 0;

			hdrlines[H_DAFWDFROM].head = hdrlines[H_AFWDFROM].head;
			hdrlines[H_DAFWDFROM].tail = hdrlines[H_AFWDFROM].tail;
			hdrlines[H_AFWDFROM].head = (struct hdrs *)NULL;
			hdrlines[H_AFWDFROM].tail = (struct hdrs *)NULL;
			hdrlines[H_DRECEIVED].head = hdrlines[H_RECEIVED].head;
			hdrlines[H_DRECEIVED].tail = hdrlines[H_RECEIVED].tail;
			hdrlines[H_RECEIVED].head = (struct hdrs *)NULL;
			hdrlines[H_RECEIVED].tail = (struct hdrs *)NULL;
			hdrlines[H_DTCOPY].head = hdrlines[H_TCOPY].head;
			hdrlines[H_DTCOPY].tail = hdrlines[H_TCOPY].tail;
			hdrlines[H_TCOPY].head = (struct hdrs *)NULL;
			hdrlines[H_TCOPY].tail = (struct hdrs *)NULL;

			pushlist(H_TCOPY, HEAD, work, FALSE);

			new_reciplist(&list);
			add_recip(&list, work, FALSE);
			sendlist(&list, 0, 0);
			del_reciplist(&list);
		}
	}

	skipsend:
	if (dflag == 9) {
		fprintf(stderr,
			"%s: Cannot return mail.\n",
			program);
		mkdead();
	}

	else if (dflag < 2) {
		if (!maxerr && (dflag != 1)) {
			maxerr = error;
			Dout(pn, 0, "maxerr = %d\n", maxerr);
		}
		dflag = 0;
		error = 0;
		Dout(pn, 0, "before return,  dflag = %d, error = %d\n",
			dflag, error);
	}
	Return;
}
