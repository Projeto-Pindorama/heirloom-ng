/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	from OpenSolaris "main.c	1.16	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)main.c	1.11 (gritter) 7/3/05
 */

#include "mail.h"
#include <locale.h>
/*
 *	mail [ -ehpPqrtw ] [-x debuglevel] [ -f file ] [ -F user(s) ]
 *	mail -T file persons
 *	mail [ -tw ] [ -m messagetype ] persons
 *	rmail [ -tw ] persons
 */
int
main(int argc, char **argv)
{
	register int i;
	char *cptr, *p;
	static char pn[] = "main";
	extern char **environ;
	int env_var_idx, next_slot_idx;
	int tmpfd = -1;

	(void)&argc;
	(void)&argv;
	setlocale(LC_CTYPE, "");
	mb_cur_max = MB_CUR_MAX;
	/* fix here for bug #1086130 - security hole	*/
	/* skip over the LD_* env variable		*/
	env_var_idx = 0; next_slot_idx = 0;
	while (environ[env_var_idx] != NULL) {
			environ[next_slot_idx] = environ[env_var_idx];
		if (strncmp(environ[env_var_idx], "LD_", 3)) {
			next_slot_idx++;
		}
		env_var_idx++;
	}
	environ[next_slot_idx] = NULL;

	line = smalloc(linesize = LSIZE);
	*line = '\0';

#ifdef SIGCONT
	{
	struct sigaction nsig;
	nsig.sa_handler = SIG_DFL;
	sigemptyset(&nsig.sa_mask);
	nsig.sa_flags = SA_RESTART;
	sigaction(SIGCONT, &nsig, (struct sigaction *)0);
	}
#endif

	/*
	 *	Strip off path name of this command for use in messages
	 */
	if ((program = strrchr(argv[0], '/')) != NULL) {
		program++;
	} else {
		program = argv[0];
	}

	/* Close all file descriptors except stdin, stdout & stderr */
	for (i = 3; close(i) == 0; i++);

	/*
	 *	Get group id for mail, exit if none exists
	 */
	if ((grpptr = getgrnam("mail")) == NULL) {
		errmsg(E_GROUP, "");
		exit(1);
	} else {
		mailgrp = grpptr->gr_gid;
	}

	/*
	 *	Save the *id for later use.
	 */
	my_uid = getuid();
	my_gid = getgid();
	my_euid = geteuid();
	my_egid = getegid();

	/*
	 *	What command (rmail or mail)?
	 */
	if (strcmp(program, "rmail") == SAME) {
		ismail = FALSE;
	}

	/*
	 *	Parse the command line and adjust argc and argv
	 *	to compensate for any options
	 */
	i = parse(argc, argv);
	argv += (i - 1);
	argc -= (i - 1);

	/* block a potential security hole */
	if (flgT && (my_euid != 0)) {
		setgid(my_gid);
		Tout(pn, "Setgid unset\n");
	}

	if (debug == 0) {
		/* If not set as an invocation option, check for system-wide */
		/* global flag */
		char *xp = xgetenv("DEBUG");
		if (xp != (char *)NULL) {
			debug = atoi(xp);
			if (debug < 0) {
				/* Keep trace file even if successful */
				keepdbgfile = -1;
				debug = -debug;
			}
		}
	}
	if (debug > 0) {
		strcpy(dbgfname, "/tmp/MLDBGXXXXXX");
		if ((tmpfd = mkstemp(dbgfname)) == -1) {
			fprintf(stderr, "%s: can't open debugging file '%s'\n",
				program, dbgfname);
			exit(13);
		}
		if ((dbgfp = fdopen(tmpfd, "w")) == (FILE *)NULL) {
			fprintf(stderr, "%s: can't open debugging file '%s'\n",
				program, dbgfname);
			close(tmpfd);
			exit(13);
		}
		setbuf(dbgfp, NULL);
		fprintf(dbgfp, "main(): debugging level == %d\n", debug);
		fprintf(dbgfp, "main(): trace file ='%s': kept %s\n", dbgfname,
			((keepdbgfile < 0) ?
				"on success or failure." : "only on failure."));
	}

	if (!ismail && (goerr > 0 || !i)) {
		Dout(pn, 11, "!ismail, goerr=%d, i=%d\n", goerr, i);
		if (goerr > 0) {
			errmsg(E_SYNTAX, "Usage: rmail [-wt] person(s)");
		}
		if (!i) {
			errmsg(E_SYNTAX, "At least one user must be specified");
		}
		Dout(pn, 11, "exiting!\n");
		done(0);
	}

	umsave = umask(7);
	uname(&utsn);
	if ((p = xgetenv("CLUSTER")) != (char *)NULL) {
		/*
		 * We are not who we appear...
		 */
		thissys = p;
	} else {
		thissys = utsn.nodename;
	}
	Dout(pn, 11, "thissys = '%s', uname = '%s'\n", thissys, utsn.nodename);

	failsafe = xgetenv("FAILSAFE");
	if (failsafe)
		Dout(pn, 11, "failsafe processing enabled to %s\n", failsafe);

	/*
	 *	Use environment variables
	 */
	home = getenv("HOME");
	if (!home || !*home) {
		home = ".";
	}

	pwd = getpwuid(my_uid);
	if (pwd)
		cpy(&my_name, &my_namesize, pwd->pw_name);
	else
		cpy(&my_name, &my_namesize, "");

	/* If root, use LOGNAME if set */
	if (my_uid == 0) {
		/* If root, use LOGNAME if set */
		if (((cptr = getenv("LOGNAME")) != NULL) &&
		    (strlen(cptr) != 0)) {
			cpy(&my_name, &my_namesize, cptr);
		}
	}
	Dout(pn, 11, "my_name = '%s'\n", my_name);

	/*
	 *	Catch signals for cleanup
	 */
	if (setjmp(sjbuf)) {
		done(0);
	}

	setsig(SIGINT, delete);
	setsig(SIGQUIT, delete);
	setsig(SIGTRAP, delete);
#ifdef	SIGIOT
	setsig(SIGIOT, delete);
#endif
#ifdef	SIGEMT
	setsig(SIGEMT, delete);
#endif
	setsig(SIGBUS, delete);
	setsig(SIGSEGV, delete);
	setsig(SIGPIPE, delete);
	setsig(SIGALRM, delete);

	setsig(SIGHUP, sig_done);
	setsig(SIGTERM, sig_done);

	cksaved(my_name);

	/*
	 *	Rmail is always invoked to send mail
	 */
	Dout(pn, 11, "ismail=%d, argc=%d\n", ismail, argc);
	if (ismail && (argc == 1)) {
		sending = FALSE;
		printmail();

	} else {
		sending = TRUE;
		sendmail(argc, argv);
	}
	done(0);
	/*NOTREACHED*/
	return 0;
}
