/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "parse.c	1.10	05/06/08 SMI" 	 SVr4.0 2.1		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)parse.c	1.5 (gritter) 6/22/05
 */
#include "mail.h"
/*
	Parse the command line.
	Return index of first non-option field (i.e. user)
*/
int
parse(int argc, char **argv)
{
	int	 	c;
	static char		pn[] UNUSED = "parse";

	/*
		"mail +" means to print in reverse order and is
		equivalent to "mail -r"
	*/
	if ((argc > 1) && (argv[1][0] == '+')) {
		if (ismail) {
			argv[1] = "-r";
		} else {
			goerr++;
		}
	}

	while ((c = getopt(argc, argv, "m:f:x:shrpPqeEdtT:w")) != EOF) {
		switch(c) {
		/*
			Set debugging level...
		*/
		case 'x':
			debug = atoi(optarg);
			orig_dbglvl = debug;
			if (debug < 0) {
				/* Keep trace file even if successful */
				keepdbgfile = -1;
				debug = -debug;
			}
			break;

		/*
			for backwards compatability with mailx...
		*/
		case 's':
			/* ignore this option */
			break;
                /*
		 * Deliver directly to a mailbox. Do Not go to sendmail
		 */
		case 'd':
			deliverflag = TRUE;
			break;

		/*
			do not print mail
 		*/
		case 'e':
			if (ismail) {
				flge = 1;
			} else {
				goerr++;
			}
			optcnt++;
			break;
		/*
			do not print mail
 		*/
		case 'E':
			if (ismail) {
				flgE = 1;
			} else {
				goerr++;
			}
			optcnt++;
			break;
		/*
		 *	use alternate file as mailfile, when reading mail
		 *      use this from user when sending mail.
		 */
		case 'f':
			flgf = 1;
			fromflag = TRUE;
			mailfile = optarg;
			strncpy(from_user, optarg, sizeof (from_user));
			from_user[sizeof (from_user) - 1] = '\0';
			optcnt++;
			break;

		/*
			Print headers first
		*/
		case 'h':
			if (ismail) {
				flgh = 1;
			} else {
				goerr++;
			}
			optcnt++;
			break;

		/*
			print without prompting
		*/
		case 'p':
			if (ismail) {
				flgp++;
			} else {
				goerr++;
			}
			optcnt++;
			break;

		/*
			override selective display default setting
			when reading mail...
		*/
		case 'P':
			if (ismail) {
				flgP++;
			}
			optcnt++;
			break;

		/*
			terminate on deletes
		*/
		case 'q':
			if (ismail) {
				delflg = 0;
			} else {
				goerr++;
			}
			optcnt++;
			break;

		/*
			print by first in, first out order
		*/
		case 'r':
			if (ismail) {
				flgr = 1;
			} else {
				goerr++;
			}
			optcnt++;
			break;

		/*
			add To: line to letters
		*/
		case 't':
			flgt = 1;
			optcnt++;
			break;

		/*
			don't wait on sends
		*/
		case 'w':
			flgw = 1;
			break;

		/*
			set message-type:
		*/
		case 'm':
			msgtype = optarg;
			if (msgtype[0] == '\0' || msgtype[0] == '-') {
				goerr++;
			} else {
				flgm = 1;
			}
			break;

		/*
			bad option
		*/
		case '?':
			goerr++;
			break;
		}
	}



	if (argc == optind) {

	    if (flgm) {
		errmsg(E_SYNTAX,
			"-m option used but no recipient(s) specified.");
		goerr++;
	    }
	    if (flgt) {
		errmsg(E_SYNTAX,
			"-t option used but no recipient(s) specified.");
		goerr++;
	    }
	    if (flgw) {
		errmsg(E_SYNTAX,
			"-w option used but no recipient(s) specified.");
		goerr++;
	    }
	    if (flgf) {
		    if (mailfile[0] == '-') {
			    errmsg(E_SYNTAX,
				   "Files names must not begin with '-'");
			    done(0);
		    }
		    if (!ismail)
			    goerr++;
	    }
	}

	if (ismail && (goerr > 0)) {
		errmsg(E_SYNTAX,"Usage: [-ehpPqr] [-f file] [-x debuglevel]");
		fprintf (stderr, "or\t[-tw] [-m message_type] [-T file] [-x debuglevel] persons\n");
		fprintf (stderr, "or\t[-x debuglevel]\n");
		done(0);
	}

	return (optind);
}
