/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	from OpenSolaris "sendmail.c	1.19	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)sendmail.c	1.10 (gritter) 5/14/06
 */

#include "mail.h"
/*
	 Send mail - High level sending routine
 */
void 
sendmail(int argc, char **argv)
{
	char		**args;
	char		*tp, *zp;
	char		*buf = NULL;
	size_t		bufsize = 0;
	char		last1c;
	FILE		*input;
	struct stat 	sbuf;
	int		aret;
	int		i, n;
	int		oldn = 1;	
	int		ttyf = 0;
	int		pushrest = 0;
	int		hdrtyp = 0;
	int		ctf = FALSE;
	int		binflg = 0;
	long		count = 0L;
	struct tm	*bp;
	struct hdrs	*hptr;
	static char	pn[] = "sendmail";
	reciplist	list;

	buf = smalloc(bufsize = 2048);
	Dout(pn, 0, "entered\n");
	new_reciplist(&list);
	for (i = 1; i < argc; ++i) {
	        if (argv[i][0] == '-') {
		        if (argv[i][1] == '\0') {
				errmsg(E_SYNTAX,"Hyphens MAY NOT be followed by spaces");
			}
		        if (i > 1) {
				errmsg(E_SYNTAX,"Options MUST PRECEDE persons");
			}
		        done(0);
	        }
		/*
			Ensure no NULL names in list
		*/
	        if (argv[i][0] == '\0' || argv[i][strlen(argv[i])-1] == '!') {
			errmsg(E_SYNTAX,"Null names are not allowed");
	  	       	done(0);
		}
		add_recip(&list, argv[i], FALSE); /* Don't check for duplication */
	}

	mktmp();
	/*
		Format time
	*/
	time(&iop);
	bp = localtime(&iop);
	tp = asctime(bp);
	zp = tzname[bp->tm_isdst];
	sprintf(datestring, "%.16s %.4s %.5s", tp, zp, tp+20);
	trimnl (datestring);
	/* asctime: Fri Sep 30 00:00:00 1986\n */
	/*          0123456789012345678901234  */
	/* RFCtime: Fri, 28 Jul 89 10:30 EDT   */
	sprintf(RFC822datestring, "%.3s, %.2s %.3s %.4s %.5s %.3s",
		tp, tp+8, tp+4, tp+20, tp+11, zp);

	/*
		Write out the from line header for the letter
	*/
	if (fromflag && deliverflag && from_user[0] != '\0') {
		snprintf(buf, bufsize, "%s%s %s\n", 
			header[H_FROM].tag, from_user, datestring);
	} else {
		snprintf(buf, bufsize, "%s%s %s\n", 
			header[H_FROM].tag, my_name, datestring);
	}
	if (!wtmpf(buf, strlen(buf))) {
		done(0);
	}
	savehdrs(buf, H_FROM);

	/*
		Copy to list in mail entry?
	*/
	if (flgt == 1 && argc > 1) {
		aret = argc;
		args = argv;
		while (--aret > 0) {
			snprintf(buf, bufsize, "%s %s\n", header[H_TO].tag, *++args);
			if (!wtmpf(buf, strlen(buf))) {
				done(0);
			}
			savehdrs(buf, H_TO);
		}
	}

	flgf = 1;	/* reset when first read of message body succeeds */
	/*
		Read mail message, allowing for lines of infinite 
		length. This is tricky, have to watch for newlines.
	*/
	saveint = setsig(SIGINT, savdead);
	last1c = ' ';	/* anything other than newline */
	ttyf = isatty (fileno(stdin));
	pushrest = 0;

	/*
	 * scan header & save relevant info.
	 */
	cpy(&fromU, &fromUsize, my_name);
	cpy(&fromS, &fromSsize, "");
	input = stdin;
	if (fstat(fileno(input), &sbuf) < 0) {
#ifdef EOVERFLOW
		if (errno == EOVERFLOW) {
			perror("stdin");
			exit(1);
		}
#endif
	}

	while ((n = getline (&line, &linesize, stdin)) > 0) {
		last1c = line[n-1];
		if (pushrest) {
			if (!wtmpf(line,n)) {
				done(0);
			}
			pushrest = (last1c != '\n');
			continue;
		}
		pushrest = (last1c != '\n');

		if ((hdrtyp = isheader (line, &ctf)) == FALSE) {
			break;
		}
		flgf = 0;
		switch (hdrtyp) {
		case H_RVERS:
			/* Are we dealing with a delivery report? */
			/* dflag = 9 ==> do not return on failure */
			dflag = 9;
			Dout(pn, 0, "dflag = 9\n");
			break;
		case H_FROM:
			if (!wtmpf(">", 1)) {
				done(0);
			}
			/* note dropthru */
			hdrtyp = H_FROM1;
		case H_FROM1:
			if (substr(line, "forwarded by") > -1) {
				break;
			}
			pickFrom (line);
			if (Rpath[0] != '\0')
				concat(&Rpath, &Rpathsize, "!");
			concat(&Rpath, &Rpathsize, fromS);
			n = 0; /* don't copy remote from's into mesg. */
			break;
		case H_CTYPE:
			/* suppress it: only generated if needed */
			n = 0; /* suppress */
			break;
		case H_TCOPY:
			/* Write out placeholder for later */
			snprintf(buf, bufsize, "%s \n", header[H_TCOPY].tag);
			if (!wtmpf(buf, strlen(buf))) {
				done(0);
			}
			n = 0; /* suppress */
			break;
		case H_MTYPE:
			if (flgm) {
				/* suppress if message-type argument */
				n = 0;
			}
			break;
		case H_CONT:
			if (oldn == 0) {
				/* suppress continuation line also */
				n = 0;
			}
			break;
		}
		oldn = n;	/* remember if this line was suppressed */
		if (n && !wtmpf(line,n)) {
			done(0);
		}
		if (!n) savehdrs(line, hdrtyp);
	}
	if (Rpath != NULL && Rpath[0] != '\0')
		concat(&Rpath, &Rpathsize, "!");
	concat(&Rpath, &Rpathsize, fromU);

	/* push out message type if so requested */
	if (flgm) {	/* message-type */
		snprintf(buf, bufsize, "%s%s\n", header[H_MTYPE].tag, msgtype);
		if (!wtmpf(buf, strlen(buf))) {
			done(0);
		}
	}

	if (n > bufsize) {
		free(buf);
		buf = smalloc(bufsize = n);
	}
	memcpy (buf, line, n);
	if (n == 0 || (ttyf && !strncmp (buf, ".\n", 2)) ) {
		if (flgf) {
			/* no input */
			return;
		} else {
			/*
			 * no content: put content-type
			 * only if explicitly present.
			 * Write out 'place-holders' only. (see below....)
			 */
			if ((hptr = hdrlines[H_CTYPE].head) !=
						    (struct hdrs *)NULL) {
				snprintf(line, linesize, "%s \n", header[H_CTYPE].tag);
				if (!wtmpf(line, strlen(line))) {
					done(0);
				}
			}
			goto wrapsend;
		}
	}

	if (n == 1 && last1c == '\n') {	/* blank line -- suppress */
		n = getline (&buf, &bufsize, stdin);
		if (n == 0 || (ttyf && !strncmp (buf, ".\n", 2)) ) {
			/*
			 * no content: put content-type
			 * only if explicitly present.
			 * Write out 'place-holder' only. (see below....)
			 */
			if ((hptr = hdrlines[H_CTYPE].head) !=
						    (struct hdrs *)NULL) {
				snprintf(line, linesize, "%s \n", header[H_CTYPE].tag);
				if (!wtmpf(line, strlen(line))) {
					done(0);
				}
			}
			goto wrapsend;
		}
	}

	if (debug > 0) {
		buf[n] = '\0';
		Dout(pn, 0, "header scan complete, readahead %d = \"%s\"\n", n, buf);
	}

	/* 
	 * Write out H_CTYPE line. This is used only as 
	 * placeholders in the tmp file. When the 'real' message is sent,
	 * the proper value will be put out by copylet().
	 */
	snprintf(line, linesize, "%s \n", header[H_CTYPE].tag);
	if (!wtmpf(line, strlen(line))) {
		done(0);
	}
	if (hdrlines[H_CTYPE].head == (struct hdrs *)NULL) {
		savehdrs(line,H_CTYPE);
	}
	/* and a blank line */
	if (!wtmpf("\n", 1)) {
		done(0);
	}
	Dout(pn, 0, "header out completed\n");

	pushrest = 0;
	count = 0L;
	/*
	 *	Are we returning mail from a delivery failure of an old-style
	 *	(SVR3.1 or SVR3.0) rmail? If so, we won't return THIS on failure
	 *	[This line should occur as the FIRST non-blank non-header line]
	 */
	if (!strncmp("***** UNDELIVERABLE MAIL sent to",buf,32)) {
	     dflag = 9; /* 9 says do not return on failure */
	     Dout(pn, 0, "found old-style UNDELIVERABLE line. dflag = 9\n");
	}

	/* scan body of message */
	while (n > 0) {
		if (ttyf && !strcmp (buf, ".\n"))
			break;
		if (!binflg) {
			binflg = !istext ((unsigned char *)buf, n);
		}

		if (!wtmpf(buf, n)) {
			done(0);
		}
		count += n;
		n = ttyf
			? getline (&buf, &bufsize, stdin)
			: fread (buf, 1, bufsize, stdin);
	}
	setsig(SIGINT, saveint);

wrapsend:
	/*
	 *	In order to use some of the subroutines that are used to
	 *	read mail, the let array must be set up
	 */
	nlet = 1;
	let[0].adr = 0;
	let[1].adr = ftell(tmpf);
	let[0].text = (binflg == 1 ? FALSE : TRUE);
	Dout(pn, 0, "body copy complete, count %ld\n", count);
	/*
	 * Modify value of H_CTYPE if necessary.
	 */
	if ((hptr = hdrlines[H_CTYPE].head) != (struct hdrs *)NULL) {
		if (strlen(hptr->value) == 0)
			strcpy(hptr->value, binflg ? "binary" : "text");
	}

	if (fclose(tmpf) == EOF) {
		tmpf = NULL;
		tmperr();
		done(0);
	}

	tmpf = doopen(lettmp,"r+",E_TMP);

	/* Do not send mail on SIGINT */
	if (dflag == 2) {
		done(0);
	}

	sendlist(&list, 0, 0);
	done(0);
}
