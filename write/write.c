/*
 * write.c - write to another user
 *
 * Copyright (C) 2024: Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 *
 *	from Unix 32V /usr/src/cmd/write.c
 *	November 6th, 1978.
 *
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * SPDX-Licence-Identifier: Caldera
 */

#pragma clang diagnostic ignored "-Wmain-return-type"
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strmenta.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utmp.h>
#include <utmpx.h>
#define MAXNAMLEN _POSIX_LOGIN_NAME_MAX

int	signum[] = {SIGHUP, SIGINT, SIGQUIT, 0};
FILE	*tf;

void	main(int argc, char *argv[]);
void	eof(void);
void	timout(void);
int	ex(char* bp);
void	sigs(void (*sig)(int));
void	usage(void);

void main(int argc, char *argv[]) {
	int i = 0,
	    c = 0,
	    tfd = 0,
	    logcnt = 0,
	    places = 0;
	char me[MAXNAMLEN] = "???",
		*mytty = NULL,
		*him = NULL,
		histty[(UT_LINESIZE + 1)] = "",
		*histtya = NULL,
		*otherplaces[20];
	struct passwd *passw;
	struct utmpx *u;
	struct stat stbuf;

	if (argc < 2)
		usage();
	him = argv[1];
	if (argc > 2)
		histtya = argv[2];
	passw = getpwuid(geteuid());
	if (passw) {
		for (i = 0; (c = passw->pw_name[i]); i++) me[i]=c;
		me[(i + 1)] = '\0'; /* sender initials */
	} else {
		fprintf(stderr, "Can't open password database file\n");
		exit(1);
	}
	mytty = ttyname(fileno(stderr));
	if (mytty == NULL) {
		fprintf(stderr, "Can't find your tty\n");
		exit(1);
	}
	/* Get everything after /dev. */
	for (i = 1; i < sizeof(mytty) && mytty[i] != '/'; i++);
	mytty += (i + 1);
	if (histtya[0] != '\0') {
		strncpy(histty, "/dev/", 5);
		strncat(histty, histtya, UT_LINESIZE);
	}
	setutxent();
	while ((u = getutxent()) != NULL) {
		if(him[0] != '-' || him[1] != 0)
		switch (strncmp(him, u->ut_user, MAXNAMLEN)) {
			case 0:
				if (logcnt == 0 && histty[0]=='\0') {
					strncpy(histty, "/dev/", 5);
					strncat(histty, u->ut_line,
							UT_LINESIZE);
				} else {
					otherplaces[places] = strdup(u->ut_line);
					places++;
				}
				break;
			default:
				continue; /* nomat */
		}
		logcnt++;
	}
	endutxent();
/* cont */
	if (logcnt == 0 && histty[0] == '\0') {
		fprintf(stderr, "%s not logged in.\n", him);
		exit(1);
	}
	if (histtya[0] == '\0' && logcnt > 1) {
		fprintf(stderr, "%s logged more than once\nwriting to %s\nother places where %s is at:\n",
					him, histty, him);
		for (i=0; i < places; i++)
			fprintf(stderr, "%s\n", otherplaces[i]);
	}
	if (histty[0] == '\0') {
		fprintf(stderr, "%s", him);
		if (logcnt)
			fprintf(stderr, " not on that tty\n");
		else
			fprintf(stderr, " not logged in\n");
		exit(1);
	}
	signal(SIGALRM, (void (*)(int))timout);
	alarm(5);
	if ((tf = fopen(histty, "w")) == NULL)
		goto perm;
	alarm(0);
	tfd = fileno(tf);
	if (isatty(tfd) < 0) {
		fprintf(stderr, "No such tty\n");
		exit(1);
	}
	if (fstat(tfd, &stbuf) < 0)
		goto perm;
	/*
	 * Check if the sender user's group (as per S_IWGRP)
	 * or everyone (as per S_IWOTH) can write to
	 * receiver's terminal.
	 */
	if ((stbuf.st_mode & (S_IWGRP|S_IWOTH)) == 0)
		goto perm;
	sigs((void (*)(int))eof);
	fprintf(tf, "Message from ");
#ifdef interdata
	fprintf(tf, "(Interdata) " );
#endif
	fprintf(tf, "%s %s...\n\7\7\7", me, mytty);
	fflush(tf);
	for(;;) {
		char *buf = "";
		char sbuf[128] = "";
		i = read(0, sbuf, 128);
		if (i <= 0)
			eof();

		/* Purified string. */
		buf = ssafe(sbuf);
		i = strlen(buf);

		switch (buf[0]) {
			case '!':
				buf[i] = 0;
				ex(buf);
				continue;
			default:
				break;
		}
		write(tfd, buf, i);
	}

perm:
	fprintf(stderr, "Permission denied\n");
	exit(1);
}

void timout(void) {
	fprintf(stderr, "Timeout opening his tty\n");
	exit(1);
}

void eof(void) {
	fprintf(tf, "EOF\n");
	exit(0);
}

int ex(char *bp) {
	int i = 0;

	sigs(SIG_IGN);
	i = fork();
	if (i < 0) {
		fprintf(stderr, "Try again\n");
		goto out;
	}
	if (i == 0) {
		sigs((void (*)(int))0);
		execl(SHELL, "sh", "-c", bp+1, (char *)0);
		exit(0);
	}
	while(wait((int *)NULL) != i)
		;
	printf("!\n");
out:
	sigs((void (*)(int))eof);
	return 0;
}

void sigs(void (*sig)(int)) {
	int i = 0;

	for(i=0; signum[i]; i++)
		signal(signum[i], sig);
}

void usage(void) {
	fprintf(stderr, "usage: write user [ttyname]\n");
	exit(1);
}
